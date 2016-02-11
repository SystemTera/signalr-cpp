#-------------------------------------------------
#
# Project created by QtCreator 2014-07-03T11:28:35
#
#-------------------------------------------------

QT       -= core gui

TARGET = SignalRServer
TEMPLATE = lib
CONFIG += sharedlib

DEFINES += SIGNALRSERVER_LIBRARY

SOURCES += SignalRServer.cpp \
    PersistentConnection.cpp \
    Request.cpp \
    PersistentConnectionFactory.cpp \
    Helper.cpp \
    Log.cpp \
    Hubs/HubDispatcher.cpp \
    Hubs/Hub.cpp \
    Hubs/HubFactory.cpp \
    Messaging/ClientMessage.cpp \
    Hubs/HubSubscriber.cpp \
    Hubs/HubManager.cpp \
    Messaging/SubscriberList.cpp \
    Transports/Transport.cpp \
    Transports/LongPollingTransport.cpp \
    Messaging/Subscriber.cpp \
    Hubs/HubSubscriberList.cpp \
    Hubs/HubClientMessage.cpp \
    Hubs/HubGroupList.cpp \
    Hubs/Group.cpp \
    GarbageCollector.cpp \
    Transports/ServerSentEventsTransport.cpp

HUB_HEADERS += \
    Hubs/HubDispatcher.h \
    Hubs/Hub.h \
    Hubs/HubFactory.h \
    Hubs/HubSubscriberList.h \
    Hubs/HubClientMessage.h \
    Hubs/HubGroupList.h \
    Hubs/Group.h \

MESS_HEADERS += \
    Messaging/ClientMessage.h \
    Messaging/SubscriberList.h \
    Messaging/Subscriber.h \

PUBLIC_HEADERS += \
    SignalRServer.h \
    PersistentConnection.h \
    PersistentConnectionFactory.h \
    Log.h \
    Request.h \

HEADERS += \
    $$PUBLIC_HEADERS \
    $$HUB_HEADERS \
    Helper.h \
    Hubs/HubSubscriber.h \
    Hubs/HubManager.h \
    Transports/Transport.h \
    Transports/LongPollingTransport.h \
    GarbageCollector.h \
    Transports/ServerSentEventsTransport.h

unix {
    signalrserver.files = $$PUBLIC_HEADERS
    signalrserver.extra = mkdir $INSTALL_ROOT/lib/include/SystemTera/SignalRServer -p
    signalrserver.path = /lib/include/SystemTera/SignalRServer

    hubs.files = $$HUB_HEADERS
    hubs.extra = mkdir $INSTALL_ROOT/lib/include/SystemTera/SignalRServer/Hubs -p
    hubs.path = /lib/include/SystemTera/SignalRServer/Hubs

    msg.files = $$MESS_HEADERS
    msg.extra = mkdir $INSTALL_ROOT/lib/include/SystemTera/SignalRServer/Messaging -p
    msg.path = /lib/include/SystemTera/SignalRServer/Messaging

    INSTALL_HEADERS {
        INSTALLS += signalrserver hubs msg
    }
    DEV_MACHINE {
        QMAKE_POST_LINK += cd $$OUT_PWD && make INSTALL_ROOT=$$SYSTEMTERA_ROOT install
    }

    target.path = /lib
    INSTALLS += target
}

include(../TeraServer.pri)

LIBS += -luuid

unix:!macx: LIBS += -L$$OUT_PWD/../Utils/p3-json/ -lp3-json

INCLUDEPATH += $$PWD/../Utils/p3-json
DEPENDPATH += $$PWD/../Utils/p3-json

OTHER_FILES += \
    Hubs/MessageInfo.txt
