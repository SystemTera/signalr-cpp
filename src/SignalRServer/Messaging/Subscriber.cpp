#include "Subscriber.h"
#include "ClientMessage.h"

namespace P3 { namespace SignalR { namespace Server {

Subscriber::Subscriber(const char* connectionId)
{    
    _connectionId = connectionId;
    pthread_mutex_init(&_lock, NULL);
}

Subscriber::~Subscriber()
{
    pthread_mutex_destroy(&_lock);
}

const string &Subscriber::connectionId() const
{
    return _connectionId;
}

void Subscriber::postClientMessage(ClientMessage* msg)
{
    pthread_mutex_lock(&_lock);
    _clientMessages.push_back(msg);
    pthread_mutex_unlock(&_lock);
}

void Subscriber::clearPendingMessages()
{
    pthread_mutex_lock(&_lock);

    for (ClientMessage* m : _clientMessages)
    {
        delete m;
    }
    _clientMessages.clear();

    pthread_mutex_unlock(&_lock);
}
const list<ClientMessage *> &Subscriber::clientMessages() const
{
    return _clientMessages;
}

}}}
