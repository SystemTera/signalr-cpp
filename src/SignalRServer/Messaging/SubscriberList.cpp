#include "SubscriberList.h"
#include "ClientMessage.h"
#include "Hubs/HubSubscriber.h"

namespace P3 { namespace SignalR { namespace Server {

SubscriberList::SubscriberList()
{
    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &_attr);
}


SubscriberList::~SubscriberList()
{
    pthread_mutexattr_destroy(&_attr);
    pthread_mutex_destroy(&_lock);
}

Subscriber* SubscriberList::getFirst(const char* connectionId)
{
    Subscriber* ret=NULL;
    lock();

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId()==connectionId)
        {
            ret = sub;
            break;
        }
    }

    unlock();
    return ret;
}


void SubscriberList::send(const char* func, VariantList& args)
{
    lock();

    for (Subscriber* sub : *this)
    {
        sub->postClientMessage(new ClientMessage(func, args));
    }

    unlock();
}


bool SubscriberList::hasMessages(const char* connectionId)
{
    bool ret=false;

    lock();

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId() == connectionId && sub->clientMessages().size() > 0)
        {
            ret = true;
            break;
        }
    }

    unlock();

    return ret;
}


list<ClientMessage*> SubscriberList::getMessages(const char* connectionId)
{
    list<ClientMessage*> list;

    lock();

    // Get all messages for the given connectionid for all hubs
    for (Subscriber* sub : *this)
    {
        if (sub->connectionId()==connectionId)
        {
            for (ClientMessage* msg : sub->clientMessages())
            {
                list.push_back(msg);
            }
        }
    }
    unlock();

    return list;
}


void SubscriberList::removeAllMessages(const char* connectionId)
{
    lock();

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId()==connectionId)
        {
            sub->clearPendingMessages();
        }
    }

    unlock();
}


list<Subscriber*> SubscriberList::getSubscriptions(const char* connectionId)
{
    list<Subscriber*> ret;
    lock();

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId()==connectionId)
        {
            ret.push_back(sub);
        }
    }
    unlock();

    return ret;
}


void SubscriberList::removeAll()
{
    lock();
    for (Subscriber *s: *this)
        delete s;
    clear();
    unlock();
}

}}}
