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

#define MAX_HUBNAME 128
#define MAX_SIGNALRSVR_TIMEOUT 5 // seconds


using namespace std;
namespace P3 { namespace SignalR { namespace Server {

typedef struct tagConnOptions
{
    int _keepAliveTimeout;
    int _disconnectTimeout;
    int _transportConnectTimeout;
    int _longPollDelay;
    bool _tryWebSockets;

} CONNECTION_OPTIONS;

// ================================= SIGNALR SERVER ===============================================
class SignalRServer
{
protected:
    int _port;
    bool _bRun;
    PersistentConnectionFactory* _connFactory;


    void createNewConnWorker(int connfd);
    void initOptions();
public:
    SignalRServer();
    SignalRServer(int port, PersistentConnectionFactory* factory);
    virtual ~SignalRServer();

private:
    static void *requestThreadFunc(void *threadid);

public:
    CONNECTION_OPTIONS _options;

    void run();
    virtual void onSetConnectionOption(int connfd);
    void stop() { _bRun = false; }
};


// ===================================SIGNALR HUB SERVER =============================================



class SignalRHubServer : public SignalRServer
{
public:
    SignalRHubServer(int port, HubFactory* hubfactory);
    virtual ~SignalRHubServer();

protected:
    HubFactory* _hubFactory;

public:
    Hub* createHub(const char* hubName, PersistentConnection* conn, Request* r);
};

}}}

#endif // SIGNALRSERVER_H
