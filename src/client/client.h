#ifndef __client_h
#define __client_h

#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Thread.h"
#include "Poco/Timespan.h"

using Poco::Net::StreamSocket;
using Poco::Net::Socket;
using Poco::Net::SocketAddress;
using Poco::Thread;

#include "../common/common.h"

/******************************************************************************/
class Client: public Common, public Poco::Runnable {
public:
   Client( StreamSocket& s, const char* server, const std::string& results ):
      Common( s ), m_server( server ), m_results( results ) {}

   int  Connect();
   void Disconnect();
   void SndIndividual();

   void run()
   {
      SndIndividual();
   }

private:
   const char* m_server;
   const std::string m_results;
};
/******************************************************************************/
#endif
