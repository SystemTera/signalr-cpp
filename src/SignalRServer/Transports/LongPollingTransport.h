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


#ifndef LONGPOLLINGTRANSPORT_H
#define LONGPOLLINGTRANSPORT_H
#include <semaphore.h>

#include "Transport.h"
#include "Hubs/HubSubscriber.h"


using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class LongPollingTransport : public Transport
{
public:
    LongPollingTransport();
    ~LongPollingTransport();

public:
    void doProcessRequest(PersistentConnection* conn, Request* request) override;

protected:
    bool isPollRequest(Request* request);

    void processLongPoll(PersistentConnection* conn, Request* request);
    void processAbortRequest(PersistentConnection* conn, Request* request) override;
    void processConnectRequest(PersistentConnection* conn, Request* request) override;
    string stripHubName(string& json);
    int getLastMsgIdFromCursors(const char* cursor, Subscriber* sub);
    std::string makeCursorKey(const char* name);
};

}}}

#endif // LONGPOLLINGTRANSPORT_H
