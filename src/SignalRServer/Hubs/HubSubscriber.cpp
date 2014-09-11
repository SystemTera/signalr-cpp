#include "HubSubscriber.h"
#include "HubClientMessage.h"

namespace P3 { namespace SignalR { namespace Server {

HubSubscriber::HubSubscriber()
    : Subscriber("")
{
    _hubName = "";    
}


HubSubscriber::HubSubscriber(const char* hub, const char* connectionId)
    : Subscriber(connectionId)
{
    _hubName = hub;
}

HubSubscriber::~HubSubscriber()
{
    clearPendingMessages();
}

const string &HubSubscriber::hubName() const
{
    return _hubName;
}

const list<HubClientMessage *> &HubSubscriber::clientMessages() const
{
    return _clientMessages;
}


}}}
