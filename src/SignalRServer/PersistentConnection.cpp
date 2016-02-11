#include "SignalRServer.h"
#include "PersistentConnection.h"
#include "Transports/Transport.h"
#include "Helper.h"
#include "Log.h"
#include "Messaging/ClientMessage.h"
#include "Hubs/Group.h"
#include "Hubs/HubManager.h"

#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <csignal>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


namespace P3 { namespace SignalR { namespace Server {

// =================================== PERSISTENT CONNECTION CLASS ===============================


PersistentConnection::PersistentConnection(int delayResponseMs)
{
    _delayResponseMs = delayResponseMs;
    _connfd = 0;
    _server = NULL;
    _connectionId = "";
    _hasFinished = false;

    // Connection settings
    _keepAliveTimeout = DEFAULT_KEEPALIVE_TIMEOUT;
    _disconnectTimeout = DEFAULT_DISCONNECT_TIMEOUT;
    _transportConnectTimeout = DEFAULT_TRANSPORT_CONNECTIONTIMEOUT;
    _longPollDelay = DEFAULT_LONGPOLLDELAY;
    _tryWebSockets = DEFAULT_TRYWEBSOCKETS;
}


PersistentConnection::~PersistentConnection()
{

}


bool PersistentConnection::authorizeRequest(Request* r)
{
    server()->lock_credentials();
    bool retVal = false;
    for (UserCredential*  u : server()->credentials())
    {
        if (u->isAuthorized(r->user(), r->password()))
        {
            retVal = true;
            break;
        }
    }

    server()->unlock_credentials();
    return retVal;
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
    string ret = std::string("anonymous");
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

void PersistentConnection::processStartRequest(Request* /* request */)
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
    string connectionToken = connectionId + std::string(":") + getUserIdentity(request);
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

    strcpy(header, "HTTP/1.1");
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
        case 500: strcat(header, "Internal Server Error");
            break;
    }

    strcat(header,"\r\n");
    if (buflen>0)
    {
        strcat(header,"Content-Length: ");
        sprintf(val,"%d", buflen);
        strcat(header,val);
        strcat(header,"\r\n");
        strcat(header,"\r\n");
    }

    Log::GetInstance()->Write("<<", LOGLEVEL_TRACE);
    Log::GetInstance()->Write(header, LOGLEVEL_TRACE);
    Log::GetInstance()->Write(buffer, LOGLEVEL_TRACE);


    if(_delayResponseMs > 0)
        usleep(1000*_delayResponseMs);

    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    write(_connfd, header, strlen(header));
    write(_connfd, buffer, strlen(buffer));

    return true;
}

bool PersistentConnection::writeServerSentEventsInitialization()
{
    int writeResult;
    char response[256];

    strcpy(response, "HTTP/1.1 200 OK\r\n");
    strcat(response, "Cache-Control: no-cache\r\n");
    strcat(response, "Pragma: no-cache\r\n");
    strcat(response, "Transfer-Encoding: chunked\r\n");
    strcat(response, "Content-Type: text/event-stream\r\n");
    strcat(response, "Expires: -1\r\n");
    strcat(response, "\r\n");
    strcat(response, "13\r\n");
    strcat(response, "data: initialized");

    Log::GetInstance()->Write("<<", LOGLEVEL_TRACE);
    Log::GetInstance()->Write(response, LOGLEVEL_TRACE);

    strcat(response, "\r\n\r\n");

    if(_delayResponseMs > 0)
        usleep(1000*_delayResponseMs);

    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    writeResult = write(_connfd, response, strlen(response));
    if (!writeResult)
        return false;

    return true;
}

bool PersistentConnection::writeServerSentEventsChunk(const char* chunkData)
{
    if(_delayResponseMs > 0)
        usleep(1000*_delayResponseMs);

    // We have to split the chunkData on each occurence of \r\n
    // Then we have to add length informationen (in hex)
    // Example
    //  This is a two line\r\n
    //  example...\r\n
    // Example result:
    //  12\r\n
    //  This is a two line\r\n
    //  a\r\n
    //  example...\r\n

    Log::GetInstance()->Write("<<", LOGLEVEL_TRACE);

    bool writeResult;
    int len;
    char *line;
    const char *curPos;
    const char *newLine;
    line = 0;
    curPos = chunkData;
    newLine = strstr(chunkData, "\r\n");
    while (newLine)
    {
        len = newLine - curPos;
        line = strndup(curPos, len);
        writeResult = writeServerSentEventsChunkLine(len, line);
        if (line)
            free(line);

        if (!writeResult)
            return false;

        curPos = newLine + 2;
        newLine = strstr(curPos, "\r\n");
    }
    if (curPos < chunkData + strlen(chunkData))
    {
        len = chunkData + strlen(chunkData) - curPos;
        line = strndup(curPos, len);
        writeResult = writeServerSentEventsChunkLine(len, line);
        if (line)
            free(line);

        if (!writeResult)
            return false;
    }


    // Write end of chunk
    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    writeResult = write(_connfd, "\r\n", 2);
    if (!writeResult)
        return false;

    return true;
}

bool PersistentConnection::writeServerSentEventsChunkLine(int len, const char* line)
{
    int writeResult;

    char lenStr[18];
    snprintf(lenStr, sizeof(lenStr - 2), "%x", len + 2);
    Log::GetInstance()->Write(lenStr, LOGLEVEL_TRACE);
    strcat(lenStr, "\r\n");
    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    writeResult = write(_connfd, lenStr, strlen(lenStr));
    if (writeResult < 0)
        return false;

    Log::GetInstance()->Write(line, LOGLEVEL_TRACE);
    signal(SIGPIPE, SIG_IGN); // supress signal if other socket is already closed
    writeResult = write(_connfd, line, strlen(line));
    if (writeResult < 0)
        return false;
    writeResult = write(_connfd, "\r\n", 2);
    if (writeResult < 0)
        return false;

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
        std::string json = Helper::getRightOfSeparator(token,":");
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

std::string PersistentConnection::createResponse(Request* , bool bInitializing, bool bReconnect, std::list<std::string> *groups, int longPollDelay, list<ClientMessage*>* messages, const char* messageIds)
{
    VariantMap resp;
    std::string mid = Helper::NullToEmpty(messageIds);


    resp.insert(VARIANT_PAIR("C", mid));  // The messageid generated


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
    return json;
}

void PersistentConnection::handleConnected(Request* request, const char* connectionId)
{
    // start a Timer for this connection ID
    // if there is no other action (e.g. "send" or "poll") for this connection for a long
    // time, throw a timeout and close the connection (call handleDisconnect)
    _server->startConnectionInfo(connectionId, this);

    onConnected(request, connectionId);
}

void PersistentConnection::handleReconnected(Request* request, const char* connectionId)
{
    // restart the connection timer
    _server->touchConnectionInfo(connectionId, this);

    onReconnected(request, connectionId);
}

string PersistentConnection::handleReceived(Request* request, const char* connectionId, const char* data)
{
    // Reset the timer
    _server->touchConnectionInfo(connectionId, this);

    return onReceived(request, connectionId, data);
}

void PersistentConnection::handleDisconnected(Request* request, const char* connectionId)
{
    // stop the connection timer
    _server->stopConnectionInfo(connectionId);

    onDisconnected(request, connectionId);
}

void PersistentConnection::processRequest(Request* request)
{
    if(!_server->isRunning())
        return;

    string connectionToken;
    Transport* transport = NULL;

    Log::GetInstance()->Write(">>", LOGLEVEL_TRACE);
    Log::GetInstance()->Write(request->uri().c_str(), LOGLEVEL_TRACE);
    Log::GetInstance()->Write(request->queryString().c_str(), LOGLEVEL_TRACE);
    Log::GetInstance()->Write(Helper::decode(request->body().c_str()).c_str(), LOGLEVEL_TRACE);


    if (authorizeRequest(request))
    {
        if (isNegotiationRequest(request))
        {
            Log::GetInstance()->Write("Negotiation request.", LOGLEVEL_DEBUG);

            processNegotiationRequest(request);
            return;
        }
        else if (isPingRequest(request))
        {
            Log::GetInstance()->Write("Ping request.", LOGLEVEL_DEBUG);
            processPingRequest(request);
            return;
        }

        connectionToken = request->getParameter("connectionToken");
        if (connectionToken.empty())
        {
            Log::GetInstance()->Write("Missing connection token", LOGLEVEL_ERROR);
            failResponse("Missing connection token");
            return;
        }

        // Extract the groups token
        _groupsToken = request->getParameter("groupsToken");
        if (_groupsToken!="")
        {
            Log::GetInstance()->Write("Group token found", LOGLEVEL_DEBUG);

            // decode the groups token
            _groupsToken = unprotectGroupsToken(_groupsToken.c_str());
        }

        // Extract connectionid from token
        _connectionId = Helper::extractConnectionIdFromToken(connectionToken.c_str());

        if (isStartRequest(request))
        {
            Log::GetInstance()->Write("Start request.",LOGLEVEL_DEBUG);
            processStartRequest(request);
            return;
        }

        _messageId = request->getParameter("messageId");
        if (_messageId!="")
        {
            // For each client connection, the client’s progress in reading the message stream is tracked using a cursor.
            //(A cursor represents a position in the message stream.) If a client disconnects and then reconnects, it asks the bus
            //for any messages that arrived after the client’s cursor value. The same thing happens when a connection uses long polling.
            //After a long poll request completes, the client opens a new connection and asks for messages that arrived after the cursor.

            //The cursor mechanism works even if a client is routed to a different server on reconnect.
            //The backplane is aware of all the servers, and it doesn’t matter which server a client connects to.

            Log::GetInstance()->Write(_messageId.c_str(), LOGLEVEL_DEBUG);
        }

        // Create the transport instance
        transport = Transport::createInstance(request->getParameter("transport").c_str());
        if (transport==NULL)
        {
            Log::GetInstance()->Write("Unknown transport", LOGLEVEL_ERROR);
            failResponse("Unknow transport");
            return;
        }

        try
        {
            transport->setConnectionId(_connectionId);

            if(_server->isRunning())
                transport->processRequest(this, request);// This is transport specific

        }
        catch(...) {
            Log::GetInstance()->Write("An exception occured in transporting.", LOGLEVEL_ERROR);
        }

        delete transport;
    }
    else
    {
        Log::GetInstance()->Write("Not authorized request", LOGLEVEL_ERROR);
        failResponse("", 401);
        return;
    }
}

}}}
