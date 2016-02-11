#include "Subscriber.h"
#include "ClientMessage.h"
#include "Helper.h"
#include "Hubs/Hub.h"
#include "Hubs/HubManager.h"
#include "Log.h"

#include <unistd.h>

namespace P3 { namespace SignalR { namespace Server {

Subscriber::Subscriber(const char* connectionId)
{    
    _connectionId = connectionId;

    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &_attr);

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


void Subscriber::signalSemaphores()
{
    lock();
    for (sem_t* semaphore: _semaphores)
        sem_post(semaphore);
    unlock();
}

void Subscriber::attachToSemaphore(sem_t* semaphore)
{
    lock();
    _semaphores.push_back(semaphore);
    unlock();
}

void Subscriber::detachFromSemaphore(sem_t* semaphore)
{
    lock();
    _semaphores.remove(semaphore);
    unlock();
}


void Subscriber::postClientMessage(ClientMessage* msg)
{
    lock();
    Log::GetInstance()->Write(("_clientMessages.push_back '" + msg->clientMethod() + "' to connection " + connectionId()).c_str(), LOGLEVEL_DEBUG);
    _clientMessages.push_back(msg);
    signalSemaphores();
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

list<ClientMessage *> Subscriber::clientMessages()
{
    lock();
    list<ClientMessage *> result = _clientMessages;
    unlock();
    return result;
}


ClientMessage* Subscriber::getNextMessage(int lastMessageId)
{
    ClientMessage* ret=NULL;

    lock();

    // Get the 1st message for the given connectionid for all hubs
    if (lastMessageId != -1)
    {
        for (ClientMessage* msg : _clientMessages)
        {
            if (lastMessageId == msg->messageId())
            {
                ret = msg;
                break;
            }
        }
    }

    // If we did not find the desired next message, we take the first from the queue
    if (ret==NULL && _clientMessages.size()>0)
    {
        ret = *(_clientMessages.begin());
    }

    unlock();

    return ret;
}


void Subscriber::removeMessage(ClientMessage* msg)
{
    lock();
    _clientMessages.remove(msg);
    unlock();
}

}}}
