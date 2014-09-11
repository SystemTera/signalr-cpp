#include "Transport.h"
#include "../PersistentConnection.h"
#include "../Request.h"
#include <string.h>
#include "LongPollingTransport.h"
#include "../Helper.h"

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
        return NULL;
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
        processAbortRequest(conn, request);
        conn->handleDisconnected(request, _connectionId.c_str());
    }
    else if (isConnectRequest(request))
    {
        processConnectRequest(conn,request);
        conn->handleConnected(request, _connectionId.c_str());
    }
    else if (isReconnectRequest(request))
    {
        conn->handleReconnected(request, _connectionId.c_str());
        conn->writeResponse(request);
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
string Transport::connectionId() const
{
    return _connectionId;
}

void Transport::setConnectionId(const string &connectionId)
{
    _connectionId = connectionId;
}


}}}
