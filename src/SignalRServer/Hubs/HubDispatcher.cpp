#include "HubDispatcher.h"
#include "../Request.h"
#include "../SignalRServer.h"
#include <Json.h>
#include "../Helper.h"
#include "Hub.h"
#include "HubManager.h"
#include "HubSubscriber.h"
#include "Log.h"

namespace P3 { namespace SignalR { namespace Server {

HubDispatcher::HubDispatcher(int responseDelyMs)
    : PersistentConnection(responseDelyMs)
{


}

HubDispatcher::~HubDispatcher()
{


}


void HubDispatcher::onConnected(Request *request, const char* connectionId)
{
    SignalRHubServer* hubServer =  (SignalRHubServer*)_server;
    // Get the HubName from the "connectionData" in request and send OnConnected to the desired hub

    string connectionData = request->getParameter("connectionData");
    Variant v = Json::parse(connectionData);

    VariantList hubs = v.toList();
    for(Variant hubV : hubs) {
        string hubName = hubV.toVariantMap()["Name"].toString();
        // Find the correct hub and forward message
        Hub* hub = hubServer->createHub(hubName.c_str(), this, request);
        if (hub)
        {
            Hub::getHubManager().getSubscribers().subscribe(hubName.c_str(),connectionId);

            hub->handleConnected();
        }        
    }
}

void HubDispatcher::onReconnected(Request *request, const char* connectionId)
{
    //TODO: check if connectionId is not faulted, if faulted send "D" : 1
    SignalRHubServer* hubServer =  (SignalRHubServer*)_server;
    // Get the HubName from the "connectionData" in request and send OnReconnected to the desired hub

    // connectionData be like "[%7B%22Name%22:%22Chat%22%7D]"
    string connectionData = request->getParameter("connectionData");
    Variant v = Json::parse(connectionData);

    VariantList hubs = v.toList();
    for(Variant hubV : hubs) {
        string hubName = hubV.toVariantMap()["Name"].toString();
        // Find the correct hub and forward message
        Hub* hub = hubServer->createHub(hubName.c_str(), this, request);
        if (hub)
        {
            Hub::getHubManager().getSubscribers().subscribe(hubName.c_str(),connectionId);

            hub->handleReconnected();
        }
    }
}

void HubDispatcher::onDisconnected(Request *request, const char* connectionId)
{
    SignalRHubServer* hubServer =  (SignalRHubServer*)_server;
    // Get the HubName from the "connectionData" in request and send OnDisconnected to the desired hub

    // connectionData be like "[%7B%22Name%22:%22Chat%22%7D]"
    string connectionData = request->getParameter("connectionData");
    Variant v = Json::parse(connectionData);

    VariantList hubs = v.toList();
    for(Variant hubV : hubs) {
        string hubName = hubV.toVariantMap()["Name"].toString();
        // Find the correct hub and forward message
        Hub* hub = hubServer->createHub(hubName.c_str(), this, request);
        if (hub)
        {
            hub->handleDisconnected();

        }
    }

    Hub::getHubManager().getSubscribers().unsubscribe(connectionId);
    Hub::getHubManager().getGroups().killAll(connectionId);
}

// "data=%7B%22I%22%3A%220%22%2C%22H%22%3A%22Chat%22%2C%22M%22%3A%22Send%22%2C%22A%22%3A%5B%22asdfasdf%22%5D%7D"
string HubDispatcher::onReceived(Request *request, const char* connectionId, const char* data)
{
    P3_UNUSED(connectionId);

    SignalRHubServer* hubServer =  (SignalRHubServer*)_server;
    string body = data;
    Variant ret = Variant::fromValue<VariantMap>(VariantMap());
    Variant state;
    Variant result;

    if (body=="")
        Log::GetInstance()->Write("\n################## ALERT NO BODY ###################\n", LOGLEVEL_ERROR);

    // Parse "data={"I":"0","H":"Chat","M":"Send","A":["asdfasdf"]}"

    string json = Helper::getQueryStringParam("data", body.c_str());
    Variant v = Json::parse(json);
    map<string, Variant> msg = v.toVariantMap();

    string hubName = msg["H"].toString();
    string index = msg["I"].toString();

    // Call the method
    Hub* hub = hubServer->createHub(hubName.c_str(), this, request);
    if (hub)
    {
        string func = msg["M"].toString();
        vector<Variant> params = msg["A"].toList();

        result = hub->handleMessage(func.c_str(), params);
    }
    else {

        writeData("", 500);
        return "";
    }

    // Return value
    ret.toVariantMap()["I"] = index;
    if (!state.isNull())
        ret.toVariantMap()["S"] = state;
    if (!result.isNull())
        ret.toVariantMap()["R"] = result;

    json = Json::stringify(ret);

    return json;
}

}}}
