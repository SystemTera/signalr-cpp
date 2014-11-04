#include "GarbageCollector.h"

namespace P3 { namespace SignalR { namespace Server {

SubscriberGarbage SubscriberGarbage::_instance;

SubscriberGarbage::SubscriberGarbage()
{
    _garbage.clear();
}


SubscriberGarbage::~SubscriberGarbage()
{
    collect();
}


void SubscriberGarbage::add(Subscriber* ptr)
{
    sSubscriberGarbage g;
    g._ptr = ptr;
    clock_gettime(CLOCK_REALTIME, &g._tsdeleted);
    _garbage.push_back(g);
}


void SubscriberGarbage::collect()
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    std::list<sSubscriberGarbage>::iterator i = _garbage.begin();
    while (i != _garbage.end())
    {
        sSubscriberGarbage g = *i;

        time_t diff = now.tv_sec - g._tsdeleted.tv_sec;

        if (diff > GARBAGE_EXPIRY_TIME_S)
        {
            delete g._ptr;
            g._ptr = NULL;
            _garbage.erase(i++);
        }
        else
        {
            ++i;
        }
    }

}


}}}
