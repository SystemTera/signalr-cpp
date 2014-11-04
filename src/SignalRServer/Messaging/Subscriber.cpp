#include "Subscriber.h"
#include "ClientMessage.h"
#include "Helper.h"
#include "Hubs/Hub.h"
#include "Hubs/HubManager.h"

#include <unistd.h>

namespace P3 { namespace SignalR { namespace Server {

Subscriber::Subscriber(const char* connectionId)
{    
    _connectionId = connectionId;

    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &_attr);

    _sem = NULL;
    _key = Hub::getHubManager().getSubscribers().generateKey();
}

Subscriber::~Subscriber()
{
    clearPendingMessages();

    pthread_mutexattr_destroy(&_attr);
    pthread_mutex_destroy(&_lock);
}

const string &Subscriber::connectionId() const
{
    return _connectionId;
}


void Subscriber::signalSemaphore()
{
    if (_sem)
        sem_post(_sem);
}

void Subscriber::attachToSemaphore(sem_t* s)
{
    _sem = s;
}

void Subscriber::detachFromSemaphore()
{
    _sem = NULL;
}


void Subscriber::postClientMessage(ClientMessage* msg)
{
    lock();
    _clientMessages.push_back(msg);
    signalSemaphore();
    unlock();
}

void Subscriber::clearPendingMessages()
{
    lock();

    for (ClientMessage* m : _clientMessages)
    {
        delete m;
    }
    _clientMessages.clear();

    unlock();
}

list<ClientMessage *> &Subscriber::clientMessages()
{
    return _clientMessages;
}


ClientMessage* Subscriber::getNextMessage(int lastMessageId)
{
    ClientMessage* ret=NULL;

    lock();

    // Get the 1st message for the given connectionid for all hubs
    for (ClientMessage* msg : clientMessages())
    {
        if (lastMessageId == msg->messageId())
        {
            ret = msg;
            break;
        }
    }

    unlock();

    // If we did not find the desired next message, we take the first from the queue
    if (ret==NULL && clientMessages().size()>0)
    {
        ret = *(clientMessages().begin());
    }

    return ret;
}


void Subscriber::removeMessage(ClientMessage* msg)
{
    lock();
    clientMessages().remove(msg);
    unlock();
}

}}}
