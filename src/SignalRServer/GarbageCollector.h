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

#ifndef GARBAGECOLLECTOR_H
#define GARBAGECOLLECTOR_H

#include <list>
#include <time.h>

#include "Messaging/Subscriber.h"


using namespace std;
namespace P3 { namespace SignalR { namespace Server {

#define GARBAGE_EXPIRY_TIME_S 10

struct sSubscriberGarbage
{
    Subscriber* _ptr;
    struct timespec _tsdeleted;
};


class SubscriberGarbage
{
public:
    SubscriberGarbage();
    virtual ~SubscriberGarbage();

private:
    std::list<sSubscriberGarbage> _garbage;
    static SubscriberGarbage _instance;

public:
    static SubscriberGarbage& getInstance() { return _instance; }

    void add(Subscriber* ptr);
    void collect();
    std::list<sSubscriberGarbage>& garbage() { return _garbage; }


};

}}}

#endif // GARBAGECOLLECTOR_H
