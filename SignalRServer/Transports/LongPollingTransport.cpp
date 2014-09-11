#include "LongPollingTransport.h"
#include "../PersistentConnection.h"
#include "../Request.h"
#include "../Helper.h"
#include "Hubs/Hub.h"
#include "Hubs/HubManager.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

namespace P3 { namespace SignalR { namespace Server {

LongPollingTransport::LongPollingTransport()
{

}

bool LongPollingTransport::isSendRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/send");
}

bool LongPollingTransport::isPollRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/poll");
}

void LongPollingTransport::processConnectRequest(PersistentConnection* conn, Request* request)
{
    conn->writeResponse(request, true); // Initialized=true!
}


void LongPollingTransport::processAbortRequest(PersistentConnection* conn, Request* request)
{
    string connectionToken = request->getParameter("connectionToken");

    // graceful disconnect
    string connectionId    = Helper::extractConnectionIdFromToken(connectionToken.c_str());
    conn->handleDisconnected(request,connectionId.c_str());

    conn->writeData();
}

// Call of remote procedure on server
void LongPollingTransport::processSendRequest(PersistentConnection* conn, Request* request)
{
    string data = Helper::decode(request->body().c_str());

    string ret = conn->handleReceived(request,_connectionId.c_str(),data.c_str());
    conn->writeData(ret.c_str());
}

string LongPollingTransport::stripHubName(string& json)
{
    Variant v = Json::parse(json);
    vector<Variant> msg = v.toList();

    // [{"Name":"Chat"}]
    string hubName = msg.at(0).toVariantMap()["Name"].toString();
    return hubName;
}

// Wait for a special event from server
// e.g. http://192.168.1.68:8080/signalr/signalr/poll?transport=longPolling&clientProtocol=1.4&connectionToken=6b8b4567-5641-fca5-aea6-00000000ffff%3Aanonymous&messageId=2&connectionData=[{"Name":"Chat"}]
void LongPollingTransport::processLongPoll(PersistentConnection* conn, Request* request)
{
    time_t stimer, etimer;
    int waits;
    string connectionToken;
    string messageId;
    string connectionId;

    connectionToken = request->getParameter("connectionToken");
    messageId       = request->getParameter("messageId");
    connectionId    = Helper::extractConnectionIdFromToken(connectionToken.c_str());

    time(&stimer);

    while (true) // forever or timeout
    {
        // Check if message has arrived to be sent to client:
        if (Hub::getHubManager().getSubscribers().hasMessages(connectionId.c_str()))
        {
            break; // Quit polling loop when message in queue
        }

        time(&etimer);
        waits = difftime(etimer,stimer);

        // The time the long polling client should wait before reestablishing a connection if no data is received.
        if (waits>=conn->_longPollDelay)
        {
             break;
        }
        usleep(1000);
    }

    // Get all groups where the client belongs to
    std::list<std::string> groups;
    groups = Hub::getHubManager().getGroups().getForClient(connectionId.c_str());

    // Send response
    list<ClientMessage*> messages = Hub::getHubManager().getSubscribers().getMessages(connectionId.c_str());
    conn->writeResponse(request,false,false,&groups, 0, &messages);

    // Clear all messages after send
    Hub::getHubManager().getSubscribers().removeAllMessages(connectionId.c_str());
}


void LongPollingTransport::doProcessRequest(PersistentConnection* conn, Request* request)
{
    if (isSendRequest(request))
    {
        processSendRequest(conn, request);
    }
    else if (isPollRequest(request))
    {
        processLongPoll(conn, request);
    }
}

}}}
