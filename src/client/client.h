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
   Client( StreamSocket& s, const char* server, const std::string& results, int *isrunning ):
      Common( s ), m_server( server ), m_results( results ), m_isrunning(isrunning) {}

   int  Connect();
   void Disconnect();
   void SndIndividual();

   virtual void run()
   {
      if (*m_isrunning)
      {
         std::cerr << "Thread is already running!\n";
         return;
      }

      *m_isrunning = 1;
      //std::cerr << "\n[*m_isrunning = 1: (" << *m_isrunning << ")]\n";
      try {
         SndIndividual();
      } catch (Poco::Exception& exc) {
         *m_isrunning = 0;
         std::cerr << "SndIndividual(): " << exc.displayText() << std::endl;
      }
      *m_isrunning = 0;
      //std::cerr << "\n[*m_isrunning = 0: (" << *m_isrunning << ")]\n";
   }

private:
   const char* m_server;
   const std::string m_results;
   int *m_isrunning;
};
/******************************************************************************/
#endif
