#include "PersistentConnectionFactory.h"

#include "stdio.h"
#include "Hubs/HubDispatcher.h"


namespace P3 { namespace SignalR { namespace Server {
// =============================== PERSISTENT CONNECTION FACTORY ============================

PersistentConnectionFactory::PersistentConnectionFactory()
{
}


PersistentConnection* PersistentConnectionFactory::createInstance()
{
    return new PersistentConnection();
}


// ============================== HUB DISPATCHER FACTORY ========================================

HubDispatcherFactory::HubDispatcherFactory()
{
}


PersistentConnection* HubDispatcherFactory::createInstance()
{
    return new HubDispatcher();
}

}}}
