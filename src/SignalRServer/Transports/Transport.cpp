#include "Transport.h"
#include "../PersistentConnection.h"
#include "../Request.h"
#include <string.h>
#include "LongPollingTransport.h"
#include "ServerSentEventsTransport.h"
#include "../Helper.h"
#include "Log.h"

namespace P3 { namespace SignalR { namespace Server {

Transport::Transport()
{
    _connectionId = "";
}


Transport* Transport::createInstance(const char* name)
{
    if (strcmp(name, "foreverFrame")==0)
    {
        return NULL;
    }
    else if (strcmp(name, "serverSentEvents")==0)
    {
        return new ServerSentEventsTransport();
    }
    else if (strcmp(name, "longPolling")==0)
    {
        return new LongPollingTransport();
    }
    else if (strcmp(name, "webSockets")==0)
    {
        return NULL;
    }
    return NULL;
}

void Transport::processRequest(PersistentConnection *conn , Request *request)
{
    if (isAbortRequest(request))
    {
        Log::GetInstance()->Write("Abort request", LOGLEVEL_DEBUG);
        processAbortRequest(conn, request);
        conn->handleDisconnected(request, _connectionId.c_str());
    }
    else if (isConnectRequest(request))
    {
        Log::GetInstance()->Write("Connection request", LOGLEVEL_DEBUG);
        processConnectRequest(conn,request);
        conn->handleConnected(request, _connectionId.c_str());
    }
    else if (isReconnectRequest(request))
    {
        Log::GetInstance()->Write("Reconnect request", LOGLEVEL_DEBUG);
        conn->handleReconnected(request, _connectionId.c_str());
        std::string response = conn->createResponse(request);
        conn->writeData(response.c_str());
    }
    doProcessRequest(conn, request);
}

bool Transport::isAbortRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/abort");
}

bool Transport::isConnectRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/connect");
}

bool Transport::isReconnectRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/reconnect");
}

bool Transport::isSendRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/send");
}

void Transport::processAbortRequest(PersistentConnection *conn, Request *request)
{
    P3_UNUSED(conn);
    P3_UNUSED(request);
    //do nothing in base class
}
void Transport::processConnectRequest(PersistentConnection *conn, Request *request)
{
    P3_UNUSED(conn);
    P3_UNUSED(request);
    //do nothing in base class
}
void Transport::processReconnectRequest(PersistentConnection *conn, Request *request)
{
    P3_UNUSED(conn);
    P3_UNUSED(request);
    //do nothing in base class
}

// Call of remote procedure on server
void Transport::processSendRequest(PersistentConnection* conn, Request* request)
{
    string data = Helper::decode(request->body().c_str());

    string ret = conn->handleReceived(request,_connectionId.c_str(),data.c_str());
    conn->writeData(ret.c_str());
}

void Transport::waitAnySubscriberMessagesOrTimeout(list<Subscriber*>& subs, int timeout)
{
    struct timespec ts;
    int count=0;
    sem_t sem;

    // Are there any messages from old cycles?
    for (Subscriber* s : subs)
    {
        count += s->clientMessages().size();
    }

    Log::GetInstance()->Write(("waitAnySubscriberMessagesOrTimeout available messages from old cycles: " + std::to_string(count)).c_str(), LOGLEVEL_DEBUG);

    if (count!=0)
    {
        return;
    }

    // set the timeout
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout;

    sem_init(&sem, 0, 0);
    for (Subscriber* s : subs)
        s->attachToSemaphore(&sem);

    sem_timedwait(&sem, &ts);

    for (Subscriber* s : subs)
        s->detachFromSemaphore(&sem);

    sem_destroy(&sem);

#ifndef NDEBUG
    // Are there any messages from new cycle?
    for (Subscriber* s : subs)
    {
        count += s->clientMessages().size();
    }

    Log::GetInstance()->Write(("waitAnySubscriberMessagesOrTimeout available messages: " + std::to_string(count)).c_str(), LOGLEVEL_DEBUG);
#endif
}

string Transport::connectionId() const
{
    return _connectionId;
}

void Transport::setConnectionId(const string &connectionId)
{
    _connectionId = connectionId;
}


}}}
