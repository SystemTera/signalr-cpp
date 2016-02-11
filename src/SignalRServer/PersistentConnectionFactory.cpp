#include "PersistentConnectionFactory.h"

#include "stdio.h"
#include "Hubs/HubDispatcher.h"


namespace P3 { namespace SignalR { namespace Server {
// =============================== PERSISTENT CONNECTION FACTORY ============================

PersistentConnectionFactory::PersistentConnectionFactory()
{
    _delayMs = 0;
}


PersistentConnection* PersistentConnectionFactory::createInstance()
{
    return new PersistentConnection(_delayMs);
}

void PersistentConnectionFactory::setRepsonseDelay(int delayMs)
{
    _delayMs = delayMs;
}


// ============================== HUB DISPATCHER FACTORY ========================================

HubDispatcherFactory::HubDispatcherFactory()
{
}


PersistentConnection* HubDispatcherFactory::createInstance()
{
    return new HubDispatcher(_delayMs);
}

}}}
