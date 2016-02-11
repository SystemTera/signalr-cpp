#include "ServerSentEventsTransport.h"
#include "../PersistentConnection.h"
#include "../Request.h"
#include "../Helper.h"
#include "Hubs/Hub.h"
#include "Hubs/HubManager.h"
#include "Messaging/ClientMessage.h"
#include "SignalRServer.h"
#include "Log.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

namespace P3 { namespace SignalR { namespace Server {

ServerSentEventsTransport::ServerSentEventsTransport()
{

}

ServerSentEventsTransport::~ServerSentEventsTransport()
{

}

void ServerSentEventsTransport::doProcessRequest(PersistentConnection* conn, Request* request)
{
    if (isSendRequest(request))
    {
        // client sends data to server
        processSendRequest(conn, request);
    }
    else if (isConnectRequest(request) || isReconnectRequest(request))
    {
        // First: server has to respond with "data: initialized"
        // Then : server downstreams datat to client
        conn->server()->touchConnectionInfo(conn->_connectionId.c_str(), conn);
        if (conn->writeServerSentEventsInitialization())
            processServerSentEvents(conn, request);
    }
    else
    {
        Log::GetInstance()->Write(("ServerSentEventsTransport::doProcessRequest unknown request: " + request->uri()).c_str(), LOGLEVEL_DEBUG);
    }
}

void ServerSentEventsTransport::processServerSentEvents(PersistentConnection* conn, Request* request)
{
    string connectionToken;
    string connectionId;
    bool bInitializing;
    bool bRunning;

    connectionToken = request->getParameter("connectionToken");
    connectionId    = Helper::extractConnectionIdFromToken(connectionToken.c_str());
    bInitializing   = true;
    bRunning = true;

    while (bRunning && conn->server()->isRunning())
    {
        // Get all groups where the client belongs to
        std::list<std::string> groups;
        groups = Hub::getHubManager().getGroups().getForClient(connectionId.c_str());

        // Wait until timeout or condition
        list<Subscriber*> subs = Hub::getHubManager().getSubscribers().getSubscriptions(connectionId.c_str());
        if (subs.size()>0)
        {
            list<ClientMessage*> messages;

            // Wait until next messages are available or CONNECT_INIT_TIMEOUT / KEEP_ALIVE_TIMEOUT occurs
            // If there are new messages, these will be sent to the client
            // If there are no new message, an empty message is sent to the client that keeps the connection alive
            int timeout = bInitializing ? CONNECT_INIT_TIMEOUT : KEEP_ALIVE_TIMEOUT;
            waitAnySubscriberMessagesOrTimeout(subs, timeout);

            // Split the messageid with all cursors for the different hubs
            for (Subscriber* sub: subs)
            {
                ClientMessage* m = sub->getNextMessage(-1);
                while (m)
                {
                    messages.push_back(m);
                    sub->removeMessage(m);
                    m = sub->getNextMessage(-1);
                }
            }

            bool bReconnect = false;
            if (!conn->server()->isRunning())
                bReconnect = true;

            // Write the response
            conn->server()->touchConnectionInfo(conn->_connectionId.c_str(), conn);
            std::string response = conn->createResponse(request, bInitializing, bReconnect, &groups, 0, &messages);
            response.insert(0, "data: ");

            bRunning = conn->writeServerSentEventsChunk(response.c_str());

            // Call destructors
            for (ClientMessage* m: messages)
                delete m;

        }
        else
        {
            bool bReconnect = false;
            if (!conn->server()->isRunning())
                bReconnect = true;

            // There are no subscriptions => Try to send an empty response
            conn->server()->touchConnectionInfo(conn->_connectionId.c_str(), conn);
            std::string response = conn->createResponse(request, bInitializing, bReconnect, &groups, 0);
            response.insert(0, "data: ");

            bRunning = conn->writeServerSentEventsChunk(response.c_str());

            // If there are no subscriptions and we could send a response, wait 1000ms and give it another try
            if (bRunning)
                usleep(1000);
        }

        bInitializing = false;
    }
}

}}}

