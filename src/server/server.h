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

#include <queue>

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

class Server: public TCPServerConnection, public Common {
public:
   /* We cannot do Common(s) here because 's' is 'const'. TCPServerConnection(s)
      makes a copy of 's', which then can be accessed through 'socket()'. */
   Server( const StreamSocket& s ): TCPServerConnection(s), Common( socket() ) {}

   void run();

   //~Server()
   //{
   //   delete[] m_immigrants; delete[] m_fitness;
   //}

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
   static std::vector<char>* m_immigrants;
   static float* m_fitness;

   static std::queue<int> m_freeslots;
   static std::queue<int> m_ready;

   static unsigned long stagnation;
   static unsigned long immigrants_acceptance_threshold;
};

#endif
