#include "LongPollingTransport.h"
#include "../PersistentConnection.h"
#include "../Request.h"
#include "../Helper.h"
#include "Hubs/Hub.h"
#include "Hubs/HubManager.h"
#include "Messaging/ClientMessage.h"
#include "SignalRServer.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

namespace P3 { namespace SignalR { namespace Server {

LongPollingTransport::LongPollingTransport()
{

}

LongPollingTransport::~LongPollingTransport()
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


void LongPollingTransport::waitAnySubscriberMessagesOrTimeout(list<Subscriber*>& subs, int timeout)
{
    struct timespec ts;
    int count=0;
    sem_t sem;

    // Are there any messages from old cycles?
    for (Subscriber* s : subs)
    {
        count += s->clientMessages().size();
    }

    if (count==0)
    {
        // set the timeout
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout;

        sem_init(&sem, 0, 0);

        for (Subscriber* s : subs)
            s->attachToSemaphore(&sem);

        sem_timedwait(&sem, &ts);

        for (Subscriber* s : subs)
            s->detachFromSemaphore();

        sem_destroy(&sem);
    }
}


std::string LongPollingTransport::makeCursorKey(const char* name)
{
    std::string ret="";
    ret = name;

    if (ret=="")
        return "";

    return ret.substr(0,1);
}


int LongPollingTransport::getLastMsgIdFromCursors(const char* cursor, Subscriber* sub)
{    
    // Split by pipes
    list<string> cursors = Helper::split(cursor,"|");

    for (string c : cursors)
    {
        // get cursor name and value
        string key = Helper::getLeftOfSeparator(c.c_str(),",");
        string val = Helper::getRightOfSeparator(c.c_str(),",");
        if (key==sub->key())
            return strtol(val.c_str(),NULL,16);
    }
    return 0;
}


// Wait for a special event from server
// e.g. http://192.168.1.68:8080/signalr/signalr/poll?transport=longPolling&clientProtocol=1.4&connectionToken=6b8b4567-5641-fca5-aea6-00000000ffff%3Aanonymous&messageId=2&connectionData=[{"Name":"Chat"}]
void LongPollingTransport::processLongPoll(PersistentConnection* conn, Request* request)
{
    string connectionToken;
    string messageId;
    string connectionId;
    std::list<std::string> groups;
    std::string nextMessageIds;
    list<ClientMessage*> messages;
    char s[10];

    connectionToken = request->getParameter("connectionToken");
    messageId       = request->getParameter("messageId");
    connectionId    = Helper::extractConnectionIdFromToken(connectionToken.c_str());
    nextMessageIds   = "";

    // Get all groups where the client belongs to
    groups = Hub::getHubManager().getGroups().getForClient(connectionId.c_str());

    // Wait until timeout or condition
    list<Subscriber*> subs = Hub::getHubManager().getSubscribers().getSubscriptions(connectionId.c_str());
    if (subs.size()>0)
    {
        waitAnySubscriberMessagesOrTimeout(subs, conn->_longPollDelay);

        // Split the messageid with all cursors for the different hubs
        nextMessageIds = "";
        for (Subscriber* sub: subs)
        {
            int messageid = getLastMsgIdFromCursors(messageId.c_str(),sub);

            ClientMessage* m = sub->getNextMessage(messageid);
            if (m)
            {
                messages.push_back(m);

                // Join the next messageids into a cursor
                if (nextMessageIds!="")
                    nextMessageIds+="|";

                sprintf(s,"%X",messageid+1);
                nextMessageIds += sub->key() + "," +std::string(s);

                // Remove message from queue
                sub->removeMessage(m);
                break;
            }
        }
    }
    else {
        conn->writeResponse(request,false,true,&groups, 0, &messages, nextMessageIds.c_str());
        return;
    }

    // Was server stopped meanwhile?
    bool bReconnect = false;
    if (!conn->server()->isRunning())
        bReconnect = true;

    // Write the response
    conn->writeResponse(request,false,bReconnect,&groups, 0, &messages, nextMessageIds.c_str());

    // Call constructors
    for (ClientMessage* m: messages)
        delete m;
}


void LongPollingTransport::doProcessRequest(PersistentConnection* conn, Request* request)
{
    if (isSendRequest(request))
    {
        processSendRequest(conn, request);
    }
    else if (isPollRequest(request))
    {
        conn->server()->touchConnectionInfo(conn->_connectionId.c_str(), conn);
        processLongPoll(conn, request);        
    }
}

}}}
