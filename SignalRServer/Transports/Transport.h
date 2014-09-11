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


#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <string>

using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class PersistentConnection;
class Request;

class Transport
{
public:
    Transport();
    virtual ~Transport() = default;

public:
    static Transport* createInstance(const char* name);
    void processRequest(PersistentConnection* conn, Request* request);
    virtual void doProcessRequest(PersistentConnection* conn, Request* request) = 0;

    string connectionId() const;
    void setConnectionId(const string &connectionId);

protected:
    virtual bool isAbortRequest(Request* request);
    virtual bool isConnectRequest(Request* request);
    virtual bool isReconnectRequest(Request* request);

    virtual void processAbortRequest(PersistentConnection* conn, Request* request);
    virtual void processConnectRequest(PersistentConnection* conn, Request* request);
    virtual void processReconnectRequest(PersistentConnection* conn, Request* request);


protected:
    string _connectionId;

};
}}}

#endif // TRANSPORT_H
