#include "SubscriberList.h"
#include "ClientMessage.h"
#include "Hubs/HubSubscriber.h"

namespace P3 { namespace SignalR { namespace Server {

SubscriberList::SubscriberList()
{
    pthread_mutex_init(&_lock, NULL);
}


SubscriberList::~SubscriberList()
{
    pthread_mutex_destroy(&_lock);
}


void SubscriberList::subscribe(const char* hubName, const char* connectionId)
{
    pthread_mutex_lock(&_lock);

    HubSubscriber* sub = new HubSubscriber(hubName,connectionId);
    push_back(sub);

    pthread_mutex_unlock(&_lock);
}


Subscriber* SubscriberList::getFirst(const char* connectionId)
{
    SubscriberList::iterator it = begin();
    while (it != end())
    {
        Subscriber* sub = *it;
        if (sub->connectionId() == connectionId)
        {
            return sub;
        }
        it++;
    }
    return NULL;
}

void SubscriberList::unsubscribe(const char* connectionId)
{
    pthread_mutex_lock(&_lock);

    Subscriber* sub = getFirst(connectionId);
    while (sub)
    {
        remove(sub);
        delete sub;

        sub = getFirst(connectionId);
    }

    pthread_mutex_unlock(&_lock);
}


void SubscriberList::send(const char* func, VariantList& args)
{
    pthread_mutex_lock(&_lock);

    for (Subscriber* sub : *this)
    {
        sub->postClientMessage(new ClientMessage(func, args));
    }

    pthread_mutex_unlock(&_lock);
}


bool SubscriberList::hasMessages(const char* connectionId)
{
    bool ret=false;

    pthread_mutex_lock(&_lock);

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId() == connectionId && sub->clientMessages().size() > 0)
        {
            ret = true;
            break;
        }
    }
    pthread_mutex_unlock(&_lock);

    return ret;
}


list<ClientMessage*> SubscriberList::getMessages(const char* connectionId)
{
    list<ClientMessage*> list;

    pthread_mutex_lock(&_lock);

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
    pthread_mutex_unlock(&_lock);

    return list;
}

void SubscriberList::removeAllMessages(const char* connectionId)
{
    pthread_mutex_lock(&_lock);

    for (Subscriber* sub : *this)
    {
        if (sub->connectionId()==connectionId)
        {
            sub->clearPendingMessages();
        }
    }

    pthread_mutex_unlock(&_lock);
}



}}}
