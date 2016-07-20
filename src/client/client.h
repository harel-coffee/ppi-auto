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
   Client( StreamSocket& s, const char* server, const std::string& results, bool &isrunning, Poco::FastMutex &mutex ):
      Common( s ), m_server( server ), m_results( results ), m_isrunning(isrunning), m_mutex(mutex) {}

   int  Connect();
   void Disconnect();
   void SndIndividual();

   virtual void run()
   {
      { // Lock begin
         // Ensures that the following read and write to m_isrunning is done synchronously
         Poco::FastMutex::ScopedLock lock(m_mutex);

         if (m_isrunning)
         {
            poco_debug( m_logger, "Ops, some other thread took this task before me, exiting... (thread is already running)");
            return;
         }

         // Tell to everybody that this task is now of my responsibility!
         m_isrunning = 1;
      } // Lock end

      try {
         SndIndividual();
      } catch (Poco::Exception& exc) {
         std::cerr << "SndIndividual(): " << exc.displayText() << std::endl;
      } catch (...) {
         std::cerr << "SndIndividual(): Unknown error!" << std::endl;
      }

      // Ensures that this thread will release m_isrunning whatever happens!
      {
         Poco::FastMutex::ScopedLock lock(m_mutex);
         m_isrunning = 0;
      }
   }

private:
   const char* m_server;
   const std::string m_results;
   bool &m_isrunning;
   Poco::FastMutex &m_mutex;
};
/******************************************************************************/
#endif
