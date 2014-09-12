#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([signalr-cpp], [0.1], [patrik.pfaffenbauer@p3.co.at])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/SignalRServer/Helper.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Log.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/PersistentConnection.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/PersistentConnectionFactory.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Request.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/SignalRServer.cpp])

AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/Group.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/Hub.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubClientMessage.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubDispatcher.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubFactory.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubGroupList.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubManager.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubSubscriber.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Hubs/HubSubscriberList.cpp])

AC_CONFIG_SRCDIR([src/SignalRServer/Messaging/ClientMessage.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Messaging/Subscriber.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Messaging/SubscriberList.cpp])

AC_CONFIG_SRCDIR([src/SignalRServer/Transports/Transport.cpp])
AC_CONFIG_SRCDIR([src/SignalRServer/Transports/LongPollingTransport.cpp])


AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11
AC_PROG_LIBTOOL
AC_C_INLINE
AM_PROG_CC_C_O
AC_PROG_CXX
AC_DEFINE([_GNU_SOURCE], 1, [Use GNU extensions])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h libusb.h map string vector stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero memset])

# Examples build
AC_ARG_ENABLE([examples-build], [AS_HELP_STRING([--enable-examples-build],
        [build example applications (default n)])],
        [build_examples=$enableval],
        [build_examples='no'])
AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$build_examples" != "xno"])


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/SignalRServer/Makefile])
AC_CONFIG_FILES([src/ThirdParty/Makefile])
AC_CONFIG_FILES([src/ThirdParty/p3-json/Makefile])
#AC_CONFIG_FILES([examples/Makefile])
AC_OUTPUT
