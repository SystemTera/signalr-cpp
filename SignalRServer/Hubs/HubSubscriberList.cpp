#include "HubSubscriberList.h"
#include "HubSubscriber.h"
#include "HubClientMessage.h"
#include "Hub.h"
#include "HubManager.h"

namespace P3 { namespace SignalR { namespace Server {

HubSubscriberList HubSubscriberList::byHub(const char *hubName)
{
    HubSubscriberList ret;

    for (Subscriber *sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        if (hsub->hubName() == hubName) // Same hub name
            ret.push_back(sub);
    }
    return ret;
}

void HubSubscriberList::send(const char *hub, const char *func, VariantList &args)
{
    for (Subscriber* sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        hsub->postClientMessage(new HubClientMessage(hub, func, args));
    }
}

void HubSubscriberList::send(Hub *h, const char *func, VariantList &args) const
{
    for (Subscriber* sub : *this)
    {
        HubSubscriber *hsub = (HubSubscriber*)sub;
        hsub->postClientMessage(new HubClientMessage(h->hubName().c_str(), func, args));
    }
}

HubSubscriberList HubSubscriberList::allExcept(const char* connectionId)
{
    HubSubscriberList result;
    for (HubSubscriberList::iterator it = begin(); it!=end();++it)
    {
        Subscriber* s = *it;
        if (s->connectionId()!=connectionId)
        {
            result.push_back(s);
        }
    }
    return result;
}

HubSubscriberList HubSubscriberList::group(const char* group)
{    
    std::string prefixedGroup;
    HubSubscriberList result;
    for (HubSubscriberList::iterator it = begin(); it!=end();++it)
    {
        HubSubscriber* s = (HubSubscriber*)*it;
        prefixedGroup = s->hubName() + "." + group;

        HubGroupList& groups = Hub::getHubManager().getGroups();
        for (Group* g : groups)
        {
            if (g->groupName() == prefixedGroup && s->connectionId() == g->connectionId())
            {
                result.push_back(s);
            }
        }
    }
    return result;
}


HubSubscriberList HubSubscriberList::groups(std::vector<std::string>& groups)
{
    HubSubscriberList result;
    for(std::string g : groups)
    {
        HubSubscriberList subs = group(g.c_str());
        for (Subscriber *s:subs)
        {
            if (!result.contains(s))
                result.push_back(s);
        }
    }
    return result;
}

HubSubscriberList HubSubscriberList::othersInGroup(Hub* hub,const char* g)
{
    HubSubscriberList result;
    HubSubscriberList subs = group(g);
    for (Subscriber *s:subs)
    {
        if (s->connectionId()!=hub->connectionId())
            result.push_back(s);
    }
    return result;
}

HubSubscriberList HubSubscriberList::othersInGroups(Hub* hub,std::vector<std::string>& gr)
{
    HubSubscriberList result;
    HubSubscriberList subs = groups(gr);
    for (Subscriber *s:subs)
    {
        if (s->connectionId()!=hub->connectionId())
            result.push_back(s);
    }
    return result;
}

HubSubscriberList HubSubscriberList::clients(std::vector<std::string>& connectionIds)
{
    HubSubscriberList result;
    for (std::string connId:connectionIds)
    {
        HubSubscriberList subs = client(connId.c_str());
        for (Subscriber *s:subs)
        {
                result.push_back(s);
        }
    }
    return result;
}

HubSubscriberList HubSubscriberList::client(const char* connectionId)
{
    HubSubscriberList result;
    for (HubSubscriberList::iterator it = begin(); it!=end();++it)
    {
        Subscriber* s = *it;
        if (s->connectionId()==connectionId)
        {
            result.push_back(s);
        }
    }
    return result;
}

bool HubSubscriberList::contains(Subscriber *s)
{
    for (Subscriber* sub : *this)
    {
        if (s==sub)
            return true;
    }
    return false;
}




}}}
