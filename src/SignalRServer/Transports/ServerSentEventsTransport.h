#ifndef SERVERSENTEVENTSTRANSPORT_H
#define SERVERSENTEVENTSTRANSPORT_H
#include <semaphore.h>

#include "Transport.h"
#include "Hubs/HubSubscriber.h"

#define CONNECT_INIT_TIMEOUT 0 // seconds
#define KEEP_ALIVE_TIMEOUT 10  // seconds

using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class ServerSentEventsTransport : public Transport
{
public:
    ServerSentEventsTransport();
    ~ServerSentEventsTransport();

public:
    void doProcessRequest(PersistentConnection* conn, Request* request) override;

protected:
    void processServerSentEvents(PersistentConnection* conn, Request* request);

};

}}}

#endif // SERVERSENTEVENTSTRANSPORT_H
