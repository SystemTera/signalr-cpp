#include "SignalRServer.h"

#include <sys/socket.h>
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
#include "Helper.h"

#include "Hubs/Hub.h"

#define MAX_INBUFFER        1024
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
}

SignalRServer::SignalRServer()
{
    _port = 7788;
    _bRun = true;
    _connFactory = new PersistentConnectionFactory();
    initOptions();
}

SignalRServer::SignalRServer(int port, PersistentConnectionFactory* factory)
{
    _port = port;
    _bRun = true;
    _connFactory = factory;
    initOptions();
}

SignalRServer::~SignalRServer()
{
    if (_connFactory)
        delete _connFactory;
}


void SignalRServer::run()
{
    // Start a socket listener here

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    int ret;


    listenfd = socket(AF_INET, SOCK_STREAM, 0);


    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(_port);

    ret = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret!=0)
    {
        printf("bind not possible on port\n");
        return;
    }
    ret = listen(listenfd, 10);
    if (ret!=0)
    {
        printf("listen failed\n");
        return;
    }

    // Run forever
    while(_bRun)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        onSetConnectionOption(connfd);

        createNewConnWorker(connfd);

        sleep(1);
    }
}


void SignalRServer::onSetConnectionOption(int connfd)
{
    int nonblock = 1;
    ioctl(connfd, FIONBIO, &nonblock); // Set nonblocking option for the connection
}


void SignalRServer::createNewConnWorker(int connfd)
{
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

    // Start a new worker
    pthread_create(&th, NULL, requestThreadFunc, (void *)conn);
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
   char recvbuf[128];
   char req[MAX_INBUFFER];
   int contentlength = 0;
   int curlen=0;
   int headerlen=0;
   int bytesread;
   bool headerok=false;
   bool receive=true;
   Request* request = NULL;
   string querystring;
   string body;
   string version;
   string method;
   string uri;
   string user;
   string pwd;
   string auth;
   string path;
   time_t stimer, etimer;
   int waits;

   req[0]='\0';

   time(&stimer);

   // The reception loop
   while (receive)
   {
        bytesread = read(conn->_connfd, recvbuf, sizeof(recvbuf));
        if (bytesread>0)
        {
            strncat(req,recvbuf,bytesread); // append to buffer
            curlen+=bytesread;

            if (strstr(req,HTTP_ENDOFHEADER)) // End-Of-Header found ?
            {
                contentlength = atoi(Helper::getHttpParam(HTTP_CONTENT_LENGTH, req).c_str());
                method  = Helper::getStrByIndex(HTTP_IDX_METHOD,  req);
                uri     = Helper::getStrByIndex(HTTP_IDX_URI,     req);
                version = Helper::getStrByIndex(HTTP_IDX_VERSION, req);
                auth    = Helper::getHttpParam(HTTP_AUTH, req);
                user    = Helper::getBasicUser(auth.c_str());
                pwd     = Helper::getBasicPassword(auth.c_str());
                path    = Helper::getLeftOfSeparator(uri.c_str(), '?');
                querystring = Helper::getRightOfSeparator(uri.c_str(), '?');
                headerok = true;

                // Delete the header from buffer
                char* pos = strstr(req,HTTP_ENDOFHEADER);
                pos++; // Skip newline
                pos++; // Skip newline
                pos++; // Skip newline
                pos++; // Skip newline
                headerlen = pos-req;

                curlen -= headerlen;
                memmove (req, pos, curlen);
                req[curlen]='\0'; // null terminate the buffer
            }

            if (headerok && curlen>=contentlength) // A full body ?
            {
                if (contentlength>0)
                {
                    body.assign(req,contentlength);

                    // Delete the body from the buffer
                    curlen -= contentlength;
                    memmove(req,req+contentlength,curlen);
                    req[curlen]='\0';
                }

                // Create a fine request and handle it
                request = new Request(querystring, body, version, method, path, user, pwd);
                conn->processRequest(request);

                headerok=false;
                receive=false;
            }
        }

        time(&etimer);
        waits = difftime(etimer,stimer);

        if (waits>=MAX_SIGNALRSVR_TIMEOUT && receive)
        {
            receive=false; // timed out
            conn->writeData("", 408);
        }
   }

   // Close and delete connection
   if (conn!=NULL)
   {
       close(conn->_connfd);
       delete conn;
   }

   if (request!=NULL)
    delete request;

   // Exit our thread
   pthread_exit(NULL);
}



// =========================================== SIGNALR HUB SERVER =================================================


SignalRHubServer::SignalRHubServer(int port, HubFactory *hubfactory)
{
    _port = port;
    _connFactory = new HubDispatcherFactory();
    _hubFactory = hubfactory;
}

SignalRHubServer::~SignalRHubServer()
{
    if (_hubFactory)
        delete _hubFactory;
}

Hub* SignalRHubServer::createHub(const char* hubName, PersistentConnection* conn, Request* r)
{
    Hub* hub;
    hub = _hubFactory->createInstance(hubName);
    hub->_hubName = hubName;
    hub->_pConnection = conn;
    hub->_pRequest = r;
    return hub;
}

}}}
