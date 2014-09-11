#include "HubGroupList.h"
#include "Hub.h"

namespace P3 { namespace SignalR { namespace Server {

HubGroupList::HubGroupList()
{
}

void HubGroupList::add(Hub* hub,const char* connectionId, const char* groupName)
{
    std::string prefixedGroup = hub->hubName() + "." + groupName;
    Group* group = new Group(connectionId,prefixedGroup.c_str());
    push_back(group);
}

void HubGroupList::kill(Hub* hub,const char* connectionId, const char* groupName)
{
    std::string prefixedGroup = hub->hubName() + "." + groupName;
    for (Group* g : *this)
    {
        if (g->groupName() == prefixedGroup && g->connectionId() == connectionId)
        {
            delete g;
            remove(g);
            break;
        }
    }
}

std::list<std::string> HubGroupList::getForClient(const char* connectionId)
{
    std::list<std::string> result;
    for (Group *g: *this)
    {
        if (g->connectionId()==connectionId)
        {
            result.push_back(g->removePrefix());
        }
    }
    return result;
}

}}}
