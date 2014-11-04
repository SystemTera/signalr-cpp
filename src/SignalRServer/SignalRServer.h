/*
 * Copyright (c) 2014, p3root - Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at) and
 * Norbert Kleininger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef SIGNALRSERVER_H
#define SIGNALRSERVER_H

#include "PersistentConnectionFactory.h"
#include "Hubs/HubFactory.h"

#include <list>
#include <string>

#define SIGNALR_SOCKETS_BSD     0
#define SIGNALR_SOCKETS_UNIX    1

#define SIGNALR_SOCKET_TYPE     SIGNALR_SOCKETS_BSD

#define MAX_HUBNAME 128
#define MAX_SIGNALRSVR_TIMEOUT 5 // seconds

#define MAX_CONNECTION_IDLE_TIMEOUT         300 // seconds


using namespace std;
namespace P3 { namespace SignalR { namespace Server {

typedef struct tagConnOptions
{
    int _keepAliveTimeout;
    int _disconnectTimeout;
    int _transportConnectTimeout;
    int _longPollDelay;
    bool _tryWebSockets;

    int _connectionIdleTimeout;

} CONNECTION_OPTIONS;

// ================================= User Credentials Struct =====================================
class UserCredential
{
public:
    UserCredential(const char* username, const char* password) { _username = username; _password = password; }
    virtual ~UserCredential() { }

private:
    std::string _username;
    std::string _password;

public:
    void setUsername(const char *username) { _username = username; }
    void setPassword(const char *password) { _password = password; }
    std::string username() { return _username; }
    std::string password() { return _password; }
};


class PersistentConnectionInfo
{
private:
    list<PersistentConnection*> _pcs;
    string _connectionId;
    time_t _start;
    int _timeout;

    pthread_mutex_t _lock;
    pthread_mutexattr_t _attr;

public:
    PersistentConnectionInfo(const char* connectionId, int timeout=MAX_CONNECTION_IDLE_TIMEOUT)
    {
        _connectionId = connectionId;
        time(&_start);
        _timeout = timeout;

        pthread_mutexattr_init(&_attr);
        pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_lock, &_attr);
    }
    virtual ~PersistentConnectionInfo()
    {
        pthread_mutexattr_destroy(&_attr);
        pthread_mutex_destroy(&_lock);
    }

    string& connectionId() { return _connectionId; }
    void setConnectionId(const char* id) { _connectionId = id; }

    bool containsPersistentConnection(PersistentConnection* pc)
    {
        bool found=false;
        lock();
        for (PersistentConnection* c:_pcs)
        {
            if (c==pc) {
                found=true;
                break;
            }
        }
        unlock();
        return found;
    }

    void addPersistentConnection(PersistentConnection* pc)
    {
        if (!containsPersistentConnection(pc))
        {
            lock();
            _pcs.push_back(pc);
            unlock();
        }
    }

    void removePersistentConnection(PersistentConnection* pc)
    {
        if (containsPersistentConnection(pc))
        {
            lock();
            _pcs.remove(pc);
            unlock();
        }
    }

    time_t &start() { return _start; }
    time_t timeout() { return _start+_timeout; }
    void setStart(time_t t) { _start=t; }
    void reset() { time(&_start); }

    bool exceeded() { time_t cur; time(&cur); return (cur>timeout()); }
    int waittime() { int s; time_t cur; time(&cur); s = timeout()-cur; if (s<0) s=0; return s; }

    list<PersistentConnection*>& getConnections() { return _pcs; }

private:
    void lock() { pthread_mutex_lock(&_lock); }
    void unlock() { pthread_mutex_unlock(&_lock); }

};



// ================================= SIGNALR SERVER ===============================================
class SignalRServer
{
protected:
    bool _bRun;
    PersistentConnectionFactory* _connFactory;
    std::list<UserCredential*> _credentials;
    int _maxThreads;
    int _currThreads;

    pthread_t _watchDogThread;

    list<PersistentConnection*> _connections;
    pthread_mutex_t _lock_conn;
    pthread_mutexattr_t _attr_conn;

    list<PersistentConnectionInfo*> _infos;
    pthread_mutex_t _lock_info;
    pthread_mutexattr_t _attr_info;

    sem_t _sem_wd;
    sem_t _sem_quit;

    string _currentWaitConnId;

    bool createNewConnWorker(int connfd);
    void initOptions();
public:
    SignalRServer();
    SignalRServer(PersistentConnectionFactory* factory, int maxThreads=10);
    virtual ~SignalRServer();

    void inc() { _currThreads++; }
    void dec() { _currThreads--; }

    bool startWatchdog();
    void startConnectionInfo(const char* connectionId, PersistentConnection* pc);
    void stopConnectionInfo(const char* connectionId);
    void touchConnectionInfo(const char* connectionId, PersistentConnection* pc);
    void removeConnectionInfo(const char* connectionId, PersistentConnection* pc);
    PersistentConnectionInfo* findConnectionInfo(const char* connectionId);
    void invalidateConnection(const char* connectionId);
    static void *wdThreadFunc(void *data);

    void lock_info() { pthread_mutex_lock(&_lock_info); }
    void unlock_info() { pthread_mutex_unlock(&_lock_info); }

    void lock_conn() { pthread_mutex_lock(&_lock_conn); }
    void unlock_conn() { pthread_mutex_unlock(&_lock_conn); }

    void onWatchDog();
    PersistentConnectionInfo* popSmallestWaittime();
    void freePersistentConnections();
    void freeConnectionInfos();
    virtual void onConnectionTimeout(const char* connectionId);
    virtual void onIdle();



private:
    static void *requestThreadFunc(void *data);
    void writeRetCode(int connfd,int retcode, const char* hint);
    void run(int listenfd);
    bool internalStop(int timeout_ms=1000);


public:
    CONNECTION_OPTIONS _options;

    void startTcp(int port);
    void startUnix(const char *sock);

    virtual void onSetConnectionOption(int connfd);
    std::list<UserCredential*>& credentials() { return _credentials; }
    bool stop(int timeout_ms=1000);
    bool isRunning() { return _bRun; }
};


// ===================================SIGNALR HUB SERVER =============================================



class SignalRHubServer : public SignalRServer
{
public:
    SignalRHubServer(HubFactory* hubfactory, int maxThreads=10);
    virtual ~SignalRHubServer();

protected:
    HubFactory* _hubFactory;

public:
    Hub* createHub(const char* hubName, PersistentConnection* conn, Request* r);
};

}}}

#endif // SIGNALRSERVER_H
