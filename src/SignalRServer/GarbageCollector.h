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
