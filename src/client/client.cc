#include "client.h"

#include <sstream>

/******************************************************************************/
void Client::SndIndividual()
{
   if( Connect() )
   {
      try {
         //std::cerr << "SndIndividual: connected!\n";
         if (SndHeader( 'I', m_results.size() ))
            SndMessage( m_results.data(), m_results.size() );
         else std::cerr << "SndIndividual: error in SndHeader!\n";
      } catch (Poco::Exception& exc) {
         std::cerr << "> Error [SndIndividual()]: " << exc.displayText() << std::endl;
      }

      Disconnect();
   }
}

/******************************************************************************/
int Client::Connect()
{
   bool connect = false;
   try {
      //Thread::sleep(5000);
      m_ss.connect(SocketAddress( m_server ));

      connect = true;

   } catch (Poco::Exception& exc) {
      std::cerr << "> Warning [Connect() to " << m_server << "]: " << exc.displayText() << " (Is the remote island '" << m_server << "' running?)" << std::endl;
   } catch (...) {
      std::cerr << "> Error [Connect() to " << m_server << "]: Unknown error\n" ;
   }

   return( connect );
}

/******************************************************************************/
void Client::Disconnect()
{
   try {
      m_ss.close();
   } catch (Poco::Exception& exc) {
      std::stringstream error_msg;
      error_msg << "> [Disconnect() close() from " << m_server << "]: " << exc.displayText() << std::endl;
      poco_debug(m_logger, error_msg.str());
   } catch (...) {
      std::cerr << "Closing failed: unknown error" << std::endl;
   }
#if 0
   try {
      m_ss.shutdown();
   } catch (Poco::Exception& exc) {
      std::cerr << "> Error [Disconnect() shutdown() from " << m_server << "]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "Shutdown failed: connection already closed" << std::endl;
   }
#endif
}
