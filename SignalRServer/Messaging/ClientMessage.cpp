#include "ClientMessage.h"

namespace P3 { namespace SignalR { namespace Server {

ClientMessage::ClientMessage(const char* func, VariantList& args)
{
    _clientMethod = func;
    _arguments = args;
}


ClientMessage::~ClientMessage()
{
}

const string &ClientMessage::clientMethod() const
{
    return _clientMethod;
}

const VariantList &ClientMessage::arguments() const
{
    return _arguments;
}

VariantMap ClientMessage::toMap() const
{
    VariantMap message;
    message.insert(VARIANT_PAIR("M", std::string(clientMethod())));
    message.insert(VARIANT_PAIR("A", arguments()));
    return message;
}

}}}
