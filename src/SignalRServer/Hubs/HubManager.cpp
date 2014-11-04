#include "HubManager.h"

namespace P3 { namespace SignalR { namespace Server {

HubManager::HubManager()
{


}

HubManager::~HubManager()
{
    // Finally remove all remaining subscribers and groups
    _hubSub.removeAll();
    _groups.removeAll();
}


}}}
