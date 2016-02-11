#include "SignalRServer.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include <string>
#include "PersistentConnection.h"
#include "Hubs/HubSubscriberList.h"
#include "Hubs/HubManager.h"
#include "Helper.h"
#include "Log.h"

#include "Hubs/Hub.h"
#include "GarbageCollector.h"

#define MAX_INBUFFER        2048
#define MAX_READBUFFER      512
#define HTTP_ENDOFHEADER    "\r\n\r\n"

#define HTTP_IDX_METHOD     0
#define HTTP_IDX_URI        1
#define HTTP_IDX_VERSION    2

#define HTTP_CONTENT_LENGTH "Content-Length: "
#define HTTP_CONTENT_TYPE   "Content-Type: "
#define HTTP_AUTH           "Authorization: "


// ========================================= SIGNALR SERVER ========================================

/*
This module is running with apache2.
Make following entry in the config file to work:

/etc/apache2/apache2.conf

# Proxy pass for signalR
<Location /signalr/>
    ProxyPass http://127.0.0.1:7788/
</Location>

All data streams are routed to our module, that should be listening on the above port. e.g. 7788
Streams are pure HTTP, SignalR protocol.

*/


namespace P3 { namespace SignalR { namespace Server {

void SignalRServer::initOptions()
{
    _options._keepAliveTimeout = DEFAULT_KEEPALIVE_TIMEOUT;
    _options._disconnectTimeout = DEFAULT_DISCONNECT_TIMEOUT;
    _options._transportConnectTimeout = DEFAULT_TRANSPORT_CONNECTIONTIMEOUT;
    _options._longPollDelay = DEFAULT_LONGPOLLDELAY;
    _options._tryWebSockets = DEFAULT_TRYWEBSOCKETS;
    _options._connectionIdleTimeout = MAX_CONNECTION_IDLE_TIMEOUT;

    pthread_mutexattr_init(&_attr_info);
    pthread_mutexattr_settype(&_attr_info, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock_info, &_attr_info);

    pthread_mutexattr_init(&_attr_conn);
    pthread_mutexattr_settype(&_attr_conn, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock_conn, &_attr_conn);

    pthread_mutexattr_init(&_creadentials_attr);
    pthread_mutexattr_settype(&_creadentials_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_credentials_lock, &_creadentials_attr);

    pthread_mutexattr_init(&brun_info);
    pthread_mutexattr_settype(&brun_info, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock_brun, &brun_info);

    sem_init(&_sem_wd, 0, 0);
    sem_init(&_sem_quit, 0, 0);

    _currentWaitConnId = "";
}

SignalRServer::SignalRServer()
{
    _bRun = true;
    _maxThreads = 100;
    _currThreads = 0;
    _connFactory = new PersistentConnectionFactory();
    initOptions();
}

SignalRServer::SignalRServer(PersistentConnectionFactory* factory, int maxThreads)
{
    _bRun = true;
    _maxThreads = maxThreads;
    _currThreads = 0;
    _connFactory = factory;
    initOptions();
}


SignalRServer::~SignalRServer()
{
    delete _connFactory;

    clearCredentials();

    pthread_mutexattr_destroy(&_attr_info);
    pthread_mutex_destroy(&_lock_info);    

    pthread_mutexattr_destroy(&_attr_conn);
    pthread_mutex_destroy(&_lock_conn);

    sem_destroy(&_sem_wd);
    sem_destroy(&_sem_quit);
}


void SignalRServer::run(int listenfd)
{
    int ret = 0;
    int connfd;

    _bRun = true;

    Log::GetInstance()->Write("Start listener", LOGLEVEL_INFO);
    ret = listen(listenfd, 10);
    if (ret!=0)
    {
        Log::GetInstance()->Write("Listen failed.", LOGLEVEL_ERROR);
        return;
    }

    // Start watchdog to have a look at all connections
    if (!startWatchdog())
        return;

    int nonblock = 1;
    ioctl(listenfd, FIONBIO, &nonblock); // Set nonblocking option for the listenhandle


    // Run the server main loop
    while(isRunning())
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if(!_bRun)
            break;

        if (connfd==-1)
        {
            if (errno==EWOULDBLOCK)
            {
                usleep(10 * 1000);
                if(!_bRun)
                    break;
                onIdle();
                continue;
            }
            else
            {
                Log::GetInstance()->Write("Error while waiting for connection.", LOGLEVEL_WARN);
                break;
            }
        }
        else
        {
            if (getCurrThreads() < _maxThreads)
            {
                Log::GetInstance()->Write("Accepting new connection.", LOGLEVEL_DEBUG);
                onSetConnectionOption(connfd);

                if (!createNewConnWorker(connfd))
                {
                    // Refuse this connection
                    Log::GetInstance()->Write("Could not create thread.", LOGLEVEL_ERROR);
                    writeRetCode(connfd, 429, "Could not create threads");
                    close(connfd);
                }
            }
            else
            {
                // Refuse this connection
                Log::GetInstance()->Write(("Refused connection. There are already that much threads: " + std::to_string(_maxThreads)).c_str(), LOGLEVEL_ERROR);
                writeRetCode(connfd, 429, "Too many threads");
                close(connfd);
            }
        }
    }

    sem_post(&_sem_quit); // Signal END
}


void SignalRServer::writeRetCode(int connfd,int retcode, const char* hint)
{
    char header[256];

    sprintf(header,"HTTP/1.0 %d %s\r\n", retcode, hint);

    write(connfd, header, strlen(header));
}

void SignalRServer::startTcp(int port)
{
    int listenfd;
    int ret = 0;
    struct sockaddr_in serv_addr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    Log::GetInstance()->Write("Binding socket..", LOGLEVEL_DEBUG);
    ret = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret!=0)
    {
        Log::GetInstance()->Write("Bind not possible on port.", LOGLEVEL_ERROR);
        return;
    }

    run(listenfd);

}

void SignalRServer::startUnix(const char *sock)
{
    int listenfd;

    string cmd = "/bin/rm -f ";
    cmd +=  sock;
    system(cmd.c_str());
    struct sockaddr_un local;
    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, sock);
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(listenfd, (struct sockaddr *)&local, len) == -1)
    {
        Log::GetInstance()->Write("Bind not possible on address.", LOGLEVEL_ERROR);
        return;
    }

    string cmd2 = "chmod 777 ";
    cmd2 += sock;
    system(cmd2.c_str());


    run(listenfd);
}



void SignalRServer::onSetConnectionOption(int connfd)
{
    int nonblock = 1;
    ioctl(connfd, FIONBIO, &nonblock); // Set nonblocking option for the connection
}

void SignalRServer::clearCredentials()
{
    pthread_mutex_lock(&_credentials_lock);
    for(UserCredential *u : _credentials)
        delete u;
    _credentials.clear();
    pthread_mutex_unlock(&_credentials_lock);
}


bool SignalRServer::createNewConnWorker(int connfd)
{
    pthread_mutex_lock(&_lock_brun);

    if(!_bRun) {
        pthread_mutex_unlock(&_lock_brun);
        return false;
    }
    pthread_mutex_unlock(&_lock_brun);

    PersistentConnection* conn;
    pthread_t th;

    conn = _connFactory->createInstance();
    conn->_connfd = connfd;
    conn->_server = this;

    // Set the options from server's global options
    conn->_keepAliveTimeout        = _options._keepAliveTimeout;
    conn->_longPollDelay           = _options._longPollDelay;
    conn->_transportConnectTimeout = _options._transportConnectTimeout;
    conn->_tryWebSockets           = _options._tryWebSockets;
    conn->_disconnectTimeout       = _options._disconnectTimeout;

    Log::GetInstance()->Write("Creating new worker..", LOGLEVEL_DEBUG);

    // Start a new worker
    int rc = pthread_create(&th, NULL, requestThreadFunc, (void *)conn);
    Log::GetInstance()->Write(("Creating new worker: " + std::to_string(rc)).c_str(), LOGLEVEL_DEBUG);

    std::string str("SigRWrkTh" + std::to_string(connfd));
    pthread_setname_np(th, str.c_str());

    if (rc!=0)
    {
        Log::GetInstance()->Write("Could not create thread.\n", LOGLEVEL_ERROR);
        return false;
    }
    conn->_threadid= th;
    lock_conn();
    _connections.push_back(conn);
    unlock_conn();
    return true;
}


// Receive HTTP message here like:
/*
POST /signalr/poll?transport=longPolling&clientProtocol=1.4&connectionToken=6b8b4567-5641-fca5-aea6-00000000ffff%3Aanonymous&messageId=2&connectionData=[%7B%22Name%22:%22Chat%22%7D] HTTP/1.1\r\n
Host: 127.0.0.1:7788\r\n
User-Agent: SignalR.Client.NET45/2.2.0.0 (Microsoft Windows NT 6.1.7601 Service Pack 1)\r\n
Content-Type: text/plain; charset=utf-8\r\n
X-Forwarded-For: 192.168.1.72\r\n
X-Forwarded-Host: 192.168.1.68\r\n
X-Forwarded-Server: tera-dev.beka-consulting.local\r\n
Connection: Keep-Alive\r\nContent-Length: 0\r\n
\r\n
ldsfklfjkklgfjlkjlkfjglkfjdgkjfdlgkjfdlgkfdgdg
gfdskgfdsgkjfdgljdfgdfgkdkslfgjdlfkgjdkfgjdklfgjklfdgjklfdg
fdgfdgfdgklfdjgksdfgjklfdsjglfdgfdkgfdklgjfdklgjfdklgj
*/


void *SignalRServer::requestThreadFunc(void *data)
{
    PersistentConnection* conn =  (PersistentConnection*)data;

    if(!conn->server()->isRunning()) {
        pthread_exit(NULL);
        return 0;
    }

    char recvbuf[MAX_READBUFFER];
    string req;
    size_t contentlength = 0;
    int bytesread;
    bool headerok=false;
    bool receive=true;
    string querystring;
    string body;
    string version;
    string method;
    string uri;
    string user;
    string pwd;
    string auth;
    string path;
    string httphead;
    string s;
    time_t stimer, etimer;
    int waits;

    conn->_server->inc(); // Increase the running thread counter

    time(&stimer);

    // The reception loop
    while (receive)
    {
        if(!conn->server()->isRunning()) {
            pthread_exit(NULL);
            return 0;
        }

        bytesread = read(conn->_connfd, recvbuf, sizeof(recvbuf));

        if (bytesread>0)
        {
            s.assign(recvbuf,bytesread);
            req+=s;

            if (req.find(HTTP_ENDOFHEADER) != std::string::npos) // End-Of-Header found ?
            {
                contentlength = atoi(Helper::getHttpParam(HTTP_CONTENT_LENGTH, req.c_str()).c_str());
                httphead= Helper::getLine(req.c_str());
                method  = Helper::getStrByIndex(HTTP_IDX_METHOD,  httphead.c_str());
                uri     = Helper::getStrByIndex(HTTP_IDX_URI,     httphead.c_str());
                version = Helper::getStrByIndex(HTTP_IDX_VERSION, httphead.c_str());
                auth    = Helper::getHttpParam(HTTP_AUTH, req.c_str());
                user    = Helper::getBasicUser(auth.c_str());
                pwd     = Helper::getBasicPassword(auth.c_str());
                path    = Helper::getLeftOfSeparator(uri.c_str(), "?");
                querystring = Helper::getRightOfSeparator(uri.c_str(), "?");
                headerok = true;

                // Delete the header from buffer
                size_t pos = req.find(HTTP_ENDOFHEADER)+4; // add 4 CRLFCRLF
                req.erase(0, pos);

                Log::GetInstance()->Write(recvbuf, LOGLEVEL_TRACE);
            }

            if (headerok && req.length()>=contentlength) // A full body ?
            {
                if (contentlength>0)
                {
                    body.assign(req.c_str(),contentlength);
                    req.erase(0, contentlength);
                }

                Log::GetInstance()->Write("New request received for dispatching.", LOGLEVEL_DEBUG);

                if(!conn->server()->isRunning()) {
                    pthread_exit(NULL);
                    return 0;
                }
                // Create a fine request and handle it
                Request request(querystring, body, version, method, path, user, pwd);

                conn->processRequest(&request);

                headerok=false;
                receive=false;
            }
        }
        else
        {
            // Sleep a while
            usleep(1000); // 1ms
        }

        time(&etimer);
        waits = difftime(etimer,stimer);

        if (waits>=MAX_SIGNALRSVR_TIMEOUT && receive)
        {
            receive=false; // timed out
            conn->writeData("", 408);
        }
    }

    // Close the connection and mark as finished
    conn->_server->removeConnectionInfo(conn->_connectionId.c_str(), conn);
    close(conn->_connfd);
    conn->_server->dec(); // Decrease the running thread counter

    Log::GetInstance()->Write("Deleted connection..", LOGLEVEL_DEBUG);

    conn->setFinished(); // Tell, that we are ready

    // Exit our thread
    pthread_exit(NULL);
}


bool SignalRServer::startWatchdog()
{
    // Start a new worker
    int rc = pthread_create(&_watchDogThread, NULL, wdThreadFunc, (void *)this);
    if (rc!=0)
    {
        Log::GetInstance()->Write("Could not create watchdog thread.\n", LOGLEVEL_ERROR);
        return false;
    }
    pthread_setname_np(_watchDogThread, "SigRWatchdog");
    return true;
}


PersistentConnectionInfo* SignalRServer::popSmallestWaittime()
{
    PersistentConnectionInfo* ret=NULL;
    int wt;
    int smallest_wt;

    lock_info();

    // Init with any waittime
    if (_infos.size()>0)
    {
        PersistentConnectionInfo* info =  _infos.front();
        smallest_wt = info->waittime();
        ret = info;
    }

    // Get smallest wait time
    for (PersistentConnectionInfo* info:_infos)
    {
        wt = info->waittime();
        if (wt<smallest_wt)
        {
            smallest_wt = wt;
            ret = info;
        }
    }

    unlock_info();
    return ret;
}


void SignalRServer::onWatchDog()
{
    int hasConnections=false;
    int wt;
    PersistentConnectionInfo* info;
    string id;
    int i=0;

    while (isRunning())
    {
        lock_info();
        hasConnections = (_infos.size()>0);
        unlock_info();

        if (hasConnections)
        {
            // Pop the oldes connection (smallest waittime) from the queue and wait
            lock_info();
            wt = 0;
            info = popSmallestWaittime();
            if (info)
            {
                wt = info->waittime();
                id = info->connectionId();
            }            
            unlock_info();


            // Sleep until next connection timeout
            if (wt>0)
            {
                _currentWaitConnId = id;

                timespec tim;
                clock_gettime(CLOCK_REALTIME, &tim);
                tim.tv_sec += wt;
                sem_timedwait(&_sem_wd, &tim);

                _currentWaitConnId = "";
            }


            // Check if exceeded..
            lock_info();
            info = popSmallestWaittime();
            if (info)
            {
                if (info->exceeded())
                {
                    onConnectionTimeout(info->connectionId().c_str());
                    _infos.remove(info);
                    delete info;
                }
            }
            unlock_info();

            i++;
        }
        else
        {
            sem_wait(&_sem_wd);
        }
    }
}


void SignalRServer::onConnectionTimeout(const char* connectionId)
{
    // Remove old subscriptions and groups
    Hub::getHubManager().getSubscribers().unsubscribe(connectionId);
    Hub::getHubManager().getGroups().killAll(connectionId);
}


void SignalRServer::onIdle()
{
    // Remove old subscriptions:
    SubscriberGarbage::getInstance().collect();

    // Remove old persistent connection objects
    freePersistentConnections();
}


void *SignalRServer::wdThreadFunc(void *data)
{
    SignalRServer* server = (SignalRServer*)data;
    server->onWatchDog();

    // Exit our thread
    pthread_exit(NULL);
}

void SignalRServer::unlock_credentials()
{
    pthread_mutex_unlock(&_credentials_lock);
}

void SignalRServer::lock_credentials()
{
    pthread_mutex_lock(&_credentials_lock);
}

PersistentConnectionInfo* SignalRServer::findConnectionInfo(const char* connectionId)
{
    PersistentConnectionInfo* ret=NULL;
    lock_info();
    for (PersistentConnectionInfo* info:_infos)
    {
        if (info->connectionId()==connectionId)
        {
            ret=info;
            break;
        }
    }
    unlock_info();

    return ret;
}


void SignalRServer::startConnectionInfo(const char* connectionId, PersistentConnection* pc)
{
    PersistentConnectionInfo* info = new PersistentConnectionInfo(connectionId, _options._connectionIdleTimeout);
    info->addPersistentConnection(pc);

    // Add timer to the end of the queue
    lock_info();
    _infos.push_back(info);
    unlock_info();
    sem_post(&_sem_wd);
}


void SignalRServer::stopConnectionInfo(const char* connectionId)
{
    lock_info();
    PersistentConnectionInfo* info = findConnectionInfo(connectionId);
    if (info)
    {
        _infos.remove(info); // remove the info
        delete info;
        sem_post(&_sem_wd);
    }
    unlock_info();
}


void SignalRServer::touchConnectionInfo(const char* connectionId, PersistentConnection* pc)
{
    lock_info();
    PersistentConnectionInfo* info = findConnectionInfo(connectionId);
    if (info)
    {
        info->addPersistentConnection(pc);
        info->reset();
    }
    unlock_info();

    if (_currentWaitConnId==connectionId)
        sem_post(&_sem_wd);
}


void SignalRServer::removeConnectionInfo(const char* connectionId, PersistentConnection* pc)
{
    lock_info();
    PersistentConnectionInfo* info = findConnectionInfo(connectionId);
    if (info)
    {
        info->removePersistentConnection(pc);
    }
    unlock_info();
}


void SignalRServer::freeConnectionInfos()
{
    lock_info();
    for (PersistentConnectionInfo* info: _infos)
    {
        delete info;
    }
    _infos.clear();
    unlock_info();
}


void SignalRServer::freePersistentConnections()
{
    lock_conn();
    list<PersistentConnection*>::iterator i = _connections.begin();
    while (i != _connections.end())
    {
        PersistentConnection* c = *i;

        if (c->isOutdated())
        {
            pthread_join(c->_threadid,NULL);
            delete c;

            _connections.erase(i++);
        }
        else
        {
            ++i;
        }
    }
    unlock_conn();
}


bool SignalRServer::internalStop(int timeout_ms)
{
    timespec timeout;
    pthread_mutex_lock(&_lock_brun);
    _bRun = false; // Stop the main loop
    pthread_mutex_unlock(&_lock_brun);

    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 1000 * 1000 * timeout_ms; // wait max n ms before killing threads
    while (timeout.tv_nsec >= 1000000000) {
        timeout.tv_sec += 1;
        timeout.tv_nsec -= 1000000000;
    }

    int rc = sem_timedwait(&_sem_quit, &timeout); // Wait for end of main loop
    if (rc!=0)
        return false;
    return true;
}


bool SignalRServer::stop(int timeout_ms)
{
    int rc;
    timespec timeout;

    if (!internalStop(timeout_ms))
    {
           Log::GetInstance()->Write("Main loop cannot be stopped...",LOGLEVEL_ERROR);
           return false;
    }

    // Loop thru all subscribers and signal to top
    Log::GetInstance()->Write("Terminate subscribers...",LOGLEVEL_DEBUG);
    for (Subscriber * sub : Hub::getHubManager().getSubscribers())
    {
        sub->signalSemaphores();
    }

    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_nsec += 1000*timeout_ms; // wait max n ms before killing threads

    // Stop the watchdog thread and wait for destruction
    Log::GetInstance()->Write("Terminate watchdog thread...",LOGLEVEL_DEBUG);

    sem_post(&_sem_wd);
    rc = pthread_timedjoin_np(_watchDogThread,NULL,&timeout);
    if (rc!=0)
    {
        pthread_cancel(_watchDogThread);
    }

    // Wait for the remaining connection threads
    Log::GetInstance()->Write("Terminate worker threads...",LOGLEVEL_DEBUG);

    lock_conn();
    for(PersistentConnection* c : _connections)
    {
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_nsec += 1000*timeout_ms; // wait max n ms before killing threads

        rc = pthread_timedjoin_np(c->_threadid,NULL,&timeout);
        if (rc!=0)
        {
            pthread_cancel(c->_threadid);
        }
        delete c;
    }
    _connections.clear();
    unlock_conn();

    // Purge lists in hub manager
    Hub::getHubManager().getSubscribers().removeAll();
    Hub::getHubManager().getGroups().removeAll();

    // Remove the connection infos
    freeConnectionInfos();


    Log::GetInstance()->Write("SignalR Server successfully stopped!",LOGLEVEL_INFO);
    return true;
}

// =========================================== SIGNALR HUB SERVER =================================================


SignalRHubServer::SignalRHubServer(HubFactory *hubfactory, int maxThreads)
    : SignalRServer(new HubDispatcherFactory(), maxThreads)
{
    _hubFactory = hubfactory;
}


SignalRHubServer::~SignalRHubServer()
{
    if (_hubFactory)
        delete _hubFactory;
}

void SignalRHubServer::setResponseDelayTimeout(int delayMs)
{
   _connFactory->setRepsonseDelay(delayMs);
}

Hub* SignalRHubServer::createHub(const char* hubName, PersistentConnection* conn, Request* r)
{
    Hub* hub;
    hub = _hubFactory->createInstance(hubName);
    if(!hub)
        return NULL;
    hub->_hubName = hubName;
    hub->_pConnection = conn;
    hub->_pRequest = r;
    return hub;
}

}}}
