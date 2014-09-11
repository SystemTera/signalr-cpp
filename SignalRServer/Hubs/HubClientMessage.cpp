#include "HubClientMessage.h"

namespace P3 { namespace SignalR { namespace Server {

HubClientMessage::HubClientMessage(const char *hub, const char *func, VariantList &args)
    : ClientMessage(func, args)
{
    _hubName = hub;
}
string HubClientMessage::hubName() const
{
    return _hubName;
}

VariantMap HubClientMessage::toMap() const
{
    VariantMap message = ClientMessage::toMap();
    message.insert(VARIANT_PAIR("H", std::string(hubName())));
    return message;
}

}}}
