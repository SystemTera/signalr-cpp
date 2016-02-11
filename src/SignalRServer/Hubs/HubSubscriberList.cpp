#include "HubSubscriberList.h"
#include "HubSubscriber.h"
#include "HubClientMessage.h"
#include "Hub.h"
#include "HubManager.h"
#include "Log.h"

#include "GarbageCollector.h"

namespace P3 { namespace SignalR { namespace Server {

HubSubscriberList HubSubscriberList::byHub(const char *hubName)
{
    HubSubscriberList ret;

    lock();

    for (Subscriber *sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        if (hsub->hubName() == hubName) // Same hub name
            ret.push_back(sub);
    }
    unlock();

    return ret;
}

void HubSubscriberList::send(const char *hub, const char *func, VariantList &args)
{
    lock();
    for (Subscriber* sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        ClientMessage* m = new HubClientMessage(hub, func, args);
        m->setMessageId(hsub->generateMessageId());
        hsub->postClientMessage(m);
    }
    unlock();
}

void HubSubscriberList::send(Hub *h, const char *func, VariantList &args)
{
    lock();
    for (Subscriber* sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        Log::GetInstance()->Write(("HubSubscriberList::send to " + hsub->hubName()).c_str(), LOGLEVEL_DEBUG);
        ClientMessage* m = new HubClientMessage(h->hubName().c_str(), func, args);
        m->setMessageId(hsub->generateMessageId());
        hsub->postClientMessage(m);
    }
    unlock();
}

HubSubscriberList HubSubscriberList::allExcept(const char* connectionId)
{
    HubSubscriberList result;
    lock();
    for (Subscriber* s : *this)
    {
        if (s->connectionId()!=connectionId)
        {
            result.push_back(s);
        }
    }
    unlock();
    return result;
}

HubSubscriberList HubSubscriberList::group(const char* group)
{    
    std::string prefixedGroup;
    HubSubscriberList result;
    lock();
    for (Subscriber* s : *this)
    {
        HubSubscriber* sub = (HubSubscriber*)s;
        prefixedGroup = sub->hubName() + "." + group;

        HubGroupList& groups = Hub::getHubManager().getGroups();
        groups.lock();
        for (Group* g : groups)
        {
            if (g->groupName() == prefixedGroup && sub->connectionId() == g->connectionId())
            {
                result.push_back(sub);
            }
        }
        groups.unlock();
    }
    unlock();
    return result;
}


HubSubscriberList HubSubscriberList::groups(std::vector<std::string>& groups)
{
    HubSubscriberList result;
    lock();
    for(std::string g : groups)
    {
        HubSubscriberList subs = group(g.c_str());
        for (Subscriber *s:subs)
        {
            if (!result.contains(s))
                result.push_back(s);
        }
    }
    unlock();
    return result;
}

HubSubscriberList HubSubscriberList::othersInGroup(Hub* hub,const char* g)
{
    HubSubscriberList result;
    lock();
    HubSubscriberList subs = group(g);
    for (Subscriber *s:subs)
    {
        if (s->connectionId()!=hub->connectionId())
            result.push_back(s);
    }
    unlock();
    return result;
}

HubSubscriberList HubSubscriberList::othersInGroups(Hub* hub,std::vector<std::string>& gr)
{
    HubSubscriberList result;
    lock();
    HubSubscriberList subs = groups(gr);
    for (Subscriber *s:subs)
    {
        if (s->connectionId()!=hub->connectionId())
            result.push_back(s);
    }
    unlock();
    return result;
}

HubSubscriberList HubSubscriberList::clients(std::vector<std::string>& connectionIds)
{
    HubSubscriberList result;
    lock();
    for (std::string connId:connectionIds)
    {
        HubSubscriberList subs = client(connId.c_str());
        for (Subscriber *s:subs)
        {
                result.push_back(s);
        }
    }
    unlock();
    return result;
}

HubSubscriberList HubSubscriberList::client(const char* connectionId)
{
    HubSubscriberList result;
    lock();
    for (Subscriber* s :*this)
    {
        if (s->connectionId()==connectionId)
        {
            result.push_back(s);
        }
    }
    unlock();
    return result;
}

bool HubSubscriberList::contains(Subscriber *s)
{
    bool ret=false;
    lock();
    for (Subscriber* sub : *this)
    {
        if (s==sub)
        {
            ret = true;
            break;
        }
    }
    unlock();
    return ret;
}

bool HubSubscriberList::exists(const char* hubName, const char* connectionId)
{
    bool ret=false;
    lock();
    for (Subscriber* sub : *this)
    {
        HubSubscriber* hs = (HubSubscriber*)sub;
        if (hs->hubName()==hubName && hs->connectionId()==connectionId)
        {
            ret = true;
            break;
        }
    }
    unlock();
    return ret;
}


void HubSubscriberList::subscribe(const char* hubName, const char* connectionId)
{   
    lock();
    if (!exists(hubName,connectionId))
    {
        HubSubscriber* sub = new HubSubscriber(hubName,connectionId);
        push_back(sub);
    }
    unlock();
}

void HubSubscriberList::unsubscribe(const char* connectionId)
{   
    lock();
    SubscriberList::iterator i = begin();
    while (i != end())
    {
        Subscriber* sub = *i;

        if (sub->connectionId()==connectionId)
        {
            SubscriberGarbage::getInstance().add(sub);
            sub->signalSemaphores(); // Signal to stop any waiting subscriptions

            erase(i++);
        }
        else
        {
            ++i;
        }
    }
    unlock();
}


std::string HubSubscriberList::generateKey()
{
    static int i=0;
    char s[10];
    i++;
    sprintf(s,"S%d",i);
    return std::string(s);
}


}}}
