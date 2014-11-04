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


#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <string>
#include <list>
#include <semaphore.h>

using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class ClientMessage;

class Subscriber
{
public:
    Subscriber(const char* connectionId);
    virtual ~Subscriber();

    const string &connectionId() const;

    void postClientMessage(ClientMessage* msg);
    void clearPendingMessages();

    list<ClientMessage *> &clientMessages();

    ClientMessage* getNextMessage(int lastMessageId);
    void removeMessage(ClientMessage* msg);

    void lock() { pthread_mutex_lock(&_lock); }
    void unlock() { pthread_mutex_unlock(&_lock); }

    void signalSemaphore();
    void attachToSemaphore(sem_t* s);
    void detachFromSemaphore();

    std::string key() { return _key; }

private:
    sem_t* _sem;
    pthread_mutex_t _lock;
    pthread_mutexattr_t _attr;

    std::string _key;

protected:
    string _connectionId;
    list<ClientMessage *> _clientMessages;
};

}}}
#endif // SUBSCRIBER_H
