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
class Client: public Common {
public:
   Client( StreamSocket& s, const char* server ):
      Common( s ), m_server( server ) {}

   void Connect();
   void Disconnect();
   void SndIndividual( const char* results );

private:
   const char* m_server;
};

/******************************************************************************/
#endif
