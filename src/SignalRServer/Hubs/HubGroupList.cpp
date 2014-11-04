#include "HubGroupList.h"
#include "Hub.h"

namespace P3 { namespace SignalR { namespace Server {

HubGroupList::HubGroupList()
{
    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &_attr);
}

HubGroupList::~HubGroupList()
{
    pthread_mutexattr_destroy(&_attr);
    pthread_mutex_destroy(&_lock);
}

void HubGroupList::add(Hub* hub,const char* connectionId, const char* groupName)
{
    if (!exists(hub,connectionId,groupName))
    {
        lock();
        std::string prefixedGroup = hub->hubName() + "." + groupName;
        Group* group = new Group(connectionId,prefixedGroup.c_str());
        push_back(group);
        unlock();
    }
}


bool HubGroupList::exists(Hub* hub,const char* connectionId, const char* groupName)
{
    bool ret = false;
    lock();
    std::string prefixedGroup = hub->hubName() + "." + groupName;
    for (Group* g : *this)
    {
        if (g->connectionId()==connectionId && g->groupName()==prefixedGroup)
        {
            ret = true;
            break;
        }
    }
    unlock();
    return ret;
}

void HubGroupList::kill(Hub* hub,const char* connectionId, const char* groupName)
{    
    std::string prefixedGroup = hub->hubName() + "." + groupName;

    lock();
    for (Group* g : *this)
    {
        if (g->groupName() == prefixedGroup && g->connectionId() == connectionId)
        {            
            remove(g);
            delete g;
            break;
        }
    }
    unlock();
}

std::list<std::string> HubGroupList::getForClient(const char* connectionId)
{    
    std::list<std::string> result;

    lock();
    for (Group *g: *this)
    {
        if (g->connectionId()==connectionId)
        {
            result.push_back(g->removePrefix());
        }
    }
    unlock();
    return result;
}


void HubGroupList::killAll(const char* connectionId)
{
    lock();
    std::list<Group*>::iterator i = begin();
    while (i != end())
    {
        Group* g = *i;
        if (g->connectionId()==connectionId)
        {
            delete g;
            erase(i++);
        }
        else
        {
            ++i;
        }
    }
    unlock();
}

void HubGroupList::removeAll()
{
    lock();
    for (Group *g: *this)
        delete g;
    clear();
    unlock();
}

}}}
