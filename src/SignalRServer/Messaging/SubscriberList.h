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


#ifndef SUBSCRIBERLIST_H
#define SUBSCRIBERLIST_H

#include <list>
#include "Subscriber.h"
#include <Variant.h>

using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class ClientMessage;

class SubscriberList : public list<Subscriber*>
{
public:
    SubscriberList();
    virtual ~SubscriberList();

private:
    pthread_mutex_t _lock;
    pthread_mutexattr_t _attr;

public:
    Subscriber* getFirst(const char* connectionId);
    virtual void send(const char* func, VariantList& args);
    bool hasMessages(const char* connectionId);
    list<ClientMessage *> getMessages(const char* connectionId);
    void removeAllMessages(const char* connectionId);
    list<Subscriber*> getSubscriptions(const char* connectionId);
    void lock() { pthread_mutex_lock(&_lock); }
    void unlock() { pthread_mutex_unlock(&_lock); }
    void removeAll();
};


}}}
#endif // SUBSCRIBERLIST_H
