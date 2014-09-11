#include "PersistentConnection.h"
#include "Transports/Transport.h"
#include "Helper.h"
#include "Log.h"
#include "Messaging/ClientMessage.h"
#include "Hubs/Group.h"

#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>


namespace P3 { namespace SignalR { namespace Server {

// =================================== PERSISTENT CONNECTION CLASS ===============================

PersistentConnection::PersistentConnection()
{
    _connfd = 0;
    _server = NULL;
    _connectionId = "";

    // Connection settings
    _keepAliveTimeout = DEFAULT_KEEPALIVE_TIMEOUT;
    _disconnectTimeout = DEFAULT_DISCONNECT_TIMEOUT;
    _transportConnectTimeout = DEFAULT_TRANSPORT_CONNECTIONTIMEOUT;
    _longPollDelay = DEFAULT_LONGPOLLDELAY;
    _tryWebSockets = DEFAULT_TRYWEBSOCKETS;

    _innerConnection = NULL;
}


PersistentConnection::~PersistentConnection()
{
    if (_innerConnection)
        delete _innerConnection;
}


bool PersistentConnection::authorizeRequest(Request* )
{
    return true;
}


void PersistentConnection::onConnected(Request *, const char* )
{

}


void PersistentConnection::onReconnected(Request *, const char* )
{

}


string PersistentConnection::onReceived(Request *, const char* , const char* )
{
    return "bullshit";
}


void PersistentConnection::onDisconnected(Request *, const char* )
{

}

bool PersistentConnection::isStartRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/start");
}

bool PersistentConnection::isNegotiationRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/negotiate");
}

bool PersistentConnection::isPingRequest(Request* request)
{
    return Helper::endWith(request->uri(),"/ping");
}

string PersistentConnection::getUserIdentity(Request* request)
{
    string ret = "anonymous";
    if (request->user()!="")
        ret = request->user();
    return ret;
}

int PersistentConnection::resolveClientProtocol(const char* ver)
{
    char version[16];
    char* major;
    char* minor;
    int ret=0;

    strcpy(version,ver);
    major = version;

    minor = strstr(version,".");
    if (minor!=NULL)
    {
        *minor='\0'; // replace "." with EOL
        minor++;
        ret = atoi(major)*10 + atoi(minor);
    }
    return ret;
}

string PersistentConnection::assemblyVersion(int ver)
{
    string ret;
    char version[16];

    sprintf(version,"%d.%d",ver/10, ver%10);
    ret.assign(version);
    return ret;
}

void PersistentConnection::processStartRequest(Request* request)
{
    VariantMap start;
    start.insert(VARIANT_PAIR("Response", std::string("started")));

    Variant ret = Variant::fromValue(start);
    writeData(Json::stringify(ret).c_str());
}

void PersistentConnection::processNegotiationRequest(Request* request)
{
    // Request QueryString look like this:
    // e.g.
    // http://192.168.1.68:8080/signalr/signalr/negotiate?clientProtocol=1.4&connectionData=[%7B%22Name%22:%22Chat%22%7D]

    string connectionId = Helper::createGUID();
    string connectionToken = connectionId + ':' + getUserIdentity(request);
    string localPath = request->uri();
    string protocolVersion;

    // Resolve protocol
    int clientProtocol = resolveClientProtocol(request->getParameter("clientProtocol").c_str());
    if (clientProtocol > SIGNALR_VER_MAX)
        clientProtocol = SIGNALR_VER_MAX;
    else if (clientProtocol < SIGNALR_VER_MIN)
        clientProtocol = SIGNALR_VER_MIN;
    protocolVersion = assemblyVersion(clientProtocol);


    Helper::replace(localPath, "/negotiate", ""); // Cut the string

    VariantMap negot;
    negot.insert(VARIANT_PAIR("Url", std::string(localPath)));
    negot.insert(VARIANT_PAIR("ConnectionToken", std::string(connectionToken)));
    negot.insert(VARIANT_PAIR("ConnectionId", std::string(connectionId)));
    negot.insert(VARIANT_PAIR("KeepAliveTimeout", _keepAliveTimeout));
    negot.insert(VARIANT_PAIR("DisconnectTimeout", _disconnectTimeout));
    negot.insert(VARIANT_PAIR("TryWebSockets", _tryWebSockets));
    negot.insert(VARIANT_PAIR("ProtocolVersion", std::string(protocolVersion)));
    negot.insert(VARIANT_PAIR("TransportConnectTimeout", _transportConnectTimeout));
    negot.insert(VARIANT_PAIR("LongPollDelay", _longPollDelay));
    Variant ret = Variant::fromValue(negot);
    writeData(Json::stringify(ret).c_str());
}

void PersistentConnection::processPingRequest(Request* )
{
    VariantMap ping;
    ping.insert(VARIANT_PAIR("Response", std::string("pong")));

    Variant ret = Variant::fromValue(ping);
    writeData(Json::stringify(ret).c_str());
}

bool PersistentConnection::writeData(const char* buffer, int retcode)
{
    int buflen;
    char header[256];
    char val[16];

    buflen = strlen(buffer);

    strcpy(header, "HTTP/1.0");
    sprintf(val," %d ", retcode);
    strcat(header,val);
    switch (retcode)
    {
        case 200: strcat(header,"OK");
            break;
        case 408: strcat(header,"Request Time-out");
            break;
        case 401: strcat(header,"Unauthorized");
            break;
    }

    strcat(header,"\n");
    if (buflen>0)
    {
        strcat(header,"Content-Length: ");
        sprintf(val,"%d", buflen);
        strcat(header,val);
        strcat(header,"\n");
        strcat(header,"\n");
    }

    printf("<<");
    printf(header);
    printf(buffer);
    printf("\n\n\n\n");

    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    write(_connfd, header, strlen(header));
    write(_connfd, buffer, strlen(buffer));
    return true;
}

void PersistentConnection::failResponse(const char* errormsg, int retcode)
{
    writeData(errormsg, retcode);
}

std::string PersistentConnection::protectGroupsToken(const char* token)
{
    int len = strlen(token);
    return Helper::base64_encode((const unsigned char*)token,len);
}


std::string PersistentConnection::unprotectGroupsToken(const char* token)
{
    return Helper::base64_decode(token);
}


std::list<string> PersistentConnection::splitGroupsToken(const char* token)
{
    std::list<string> result;
    if (strlen(token)>0)
    {
        std::string json = Helper::getRightOfSeparator(token,':');
        Variant v = Json::parse(json);
        VariantList groups = v.toList();
        for(Variant group : groups)
        {
            result.push_back(group.toString());
        }
    }
    return result;
}


std::list<string> PersistentConnection::findRemovedGroups(std::list<std::string>* oldGroups, std::list<std::string>* newGroups)
{
    std::list<string> result;

    if (oldGroups && newGroups)
    {
        for (string oldGroup : *oldGroups)
        {
            bool found=false;
            for (string newGroup: *newGroups)
            {
                if (oldGroup==newGroup)
                {
                    found=true;
                    break;
                }
            }
            if (!found)
                result.push_back(oldGroup);
        }
    }
    return result;
}

std::list<string> PersistentConnection::findAddedGroups(std::list<std::string>* oldGroups, std::list<std::string>* newGroups)
{
    std::list<string> result;

    if (oldGroups && newGroups)
    {
        for (string newGroup: *newGroups)
        {
            bool found=false;
            for (string oldGroup : *oldGroups)
            {
                if (newGroup==oldGroup)
                {
                    found=true;
                    break;
                }
            }
            if (!found)
                result.push_back(newGroup);
        }
    }
    return result;
}


std::string PersistentConnection::encodeGroupsToken(std::list<std::string>* groups)
{
    std::string list="[";
    for (std::string g: *groups)
    {
        list += "\"" + g + "\",";
    }
    if (list[list.length()-1]==',') list = list.substr(0,list.length()-1); // Remove trailing comma
    list+="]";

    std::string sgroups = _connectionId + ":" + list;
    return protectGroupsToken(sgroups.c_str());
}

void PersistentConnection::writeResponse(Request* , bool bInitializing, bool bReconnect, std::list<std::string> *groups, int longPollDelay, list<ClientMessage*>* messages)
{
    char messageId[16];
    static int i=1;
    i++;

    sprintf(messageId,"%d", i);

    VariantMap resp;
    resp.insert(VARIANT_PAIR("C", std::string(messageId)));  // The messageid generated

    if (bInitializing)
    {
        resp.insert(VARIANT_PAIR("S", 1)); // True if the connection is in process of initializing
    }
    if (bReconnect)
    {
        resp.insert(VARIANT_PAIR("T", 1));  // This is set when the host is shutting down.
    }

    // Get the old groups from query string
    std::list<std::string> oldGroups = splitGroupsToken(_groupsToken.c_str());
    std::list<std::string> addedGroups = findAddedGroups(&oldGroups,groups);
    std::list<std::string> removedGroups = findRemovedGroups(&oldGroups,groups);

    if (addedGroups.size()>0)
    {
        resp.insert(VARIANT_PAIR("G", encodeGroupsToken(&addedGroups))); // Signed token representing the list of groups. Updates on change
    }

    if (removedGroups.size()>0)
    {
        resp.insert(VARIANT_PAIR("g", encodeGroupsToken(&removedGroups))); // Removed groups
    }

    if(longPollDelay > 0)
    {
        resp.insert(VARIANT_PAIR("L", longPollDelay)); // The time the long polling client should wait before reestablishing a connection if no data is received.
    }

    VariantList retMessages;

    if(messages != NULL)
    {
        for(ClientMessage *m : *messages)
        {
            VariantMap message = m->toMap();
            retMessages.push_back(Variant::fromValue(message));
        }
    }
    resp.insert(VARIANT_PAIR("M", retMessages));

    Variant ret = Variant::fromValue(resp);
    std::string json = Json::stringify(ret);
    writeData(json.c_str());
}

void PersistentConnection::handleConnected(Request* request, const char* connectionId)
{
    onConnected(request, connectionId);
}

void PersistentConnection::handleReconnected(Request* request, const char* connectionId)
{
    onReconnected(request, connectionId);
}

string PersistentConnection::handleReceived(Request* request, const char* connectionId, const char* data)
{
    return onReceived(request, connectionId, data);
}

void PersistentConnection::handleDisconnected(Request* request, const char* connectionId)
{
    onDisconnected(request, connectionId);
}

void PersistentConnection::processRequest(Request* request)
{
    string data="";
    string connectionToken;
    Transport* transport = NULL;

    printf(">>");
    printf(request->uri().c_str());
    printf("\n");
    printf(request->queryString().c_str());
    printf("\n");
    printf(Helper::decode(request->body().c_str()).c_str());
    printf("\n\n");

    if (authorizeRequest(request))
    {
        if (isNegotiationRequest(request))
        {
            processNegotiationRequest(request);
            return;
        }
        else if (isPingRequest(request))
        {
            processPingRequest(request);
            return;
        }

        connectionToken = request->getParameter("connectionToken");
        if (connectionToken.empty())
        {
            failResponse("Missing connection token");
            return;
        }

        // Extract the groups token
        _groupsToken = request->getParameter("groupsToken");
        if (_groupsToken!="")
        {
            // decode the groups token
            _groupsToken = unprotectGroupsToken(_groupsToken.c_str());
        }

        // Extract connectionid from token
        _connectionId = Helper::extractConnectionIdFromToken(connectionToken.c_str());

        if (isStartRequest(request))
        {
            processStartRequest(request);
            return;
        }

        // Create the transport instance
        transport = Transport::createInstance(request->getParameter("transport").c_str());
        if (transport==NULL)
        {
            failResponse("Unknow transport");
            return;
        }

        try
        {
            transport->setConnectionId(_connectionId);

            _innerConnection = newConnection(_connectionId.c_str());


            // This is transport specific
            transport->processRequest(this, request);

        }
        catch(...)
        {
            Log::GetInstance()->Write("An exception occured in transporting.");
        }

        delete transport;
    }
    else
    {
        failResponse("", 401);
        return;
    }
}

Connection* PersistentConnection::newConnection(const char* connectionId)
{
    return new Connection(connectionId);
}

// ============================================= CONNECTION ======================================

Connection::Connection(const char* connId)
{
    m_connectionId.assign(connId);
}

}}}
