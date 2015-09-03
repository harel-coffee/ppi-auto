#ifndef __server_h
#define __server_h

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/TCPServerParams.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Thread.h"

using Poco::Net::TCPServer;
using Poco::Net::TCPServerConnection;
using Poco::Net::TCPServerConnectionFactory;
using Poco::Net::TCPServerConnectionFactoryImpl;
using Poco::Net::TCPServerParams;
using Poco::Net::StreamSocket;
using Poco::Net::ServerSocket;
using Poco::Net::SocketAddress;
using Poco::Thread;

#include "../common/common.h"
#include "../individual"

void server_init( int argc, char** argv );


class Server: public TCPServerConnection, public Common {
public:
   /* We cannot do Common(s) here because 's' is 'const'. TCPServerConnection(s)
      makes a copy of 's', which then can be accessed through 'socket()'. */
   Server( const StreamSocket& s ): TCPServerConnection(s), Common( socket() ) {}

   void run();

public:
   /* A 'static variable' will always be shared among all threads.

      A 'standard variable' will be shared among the threads that
      are run over the same object of this class.

      A 'ThreadLocal static variable' will not be shared even if the
      threads are run over the same object.

      Finally, all variables declared inside 'run()' are private for
      the current thread (executing the method run()). */
   static Poco::FastMutex m_mutex;

public:
   static Population* m_pop;
   static int m_writepos;
   static int m_immigrants;
};

#endif
