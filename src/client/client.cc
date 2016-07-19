#include "client.h"

#define TIMEOUT 10000

/******************************************************************************/
void Client::SndIndividual()
{
   //std::cerr << "SndIndividual: trying to connect...\n";
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
   //else std::cerr << "SndIndividual: could not connect!\n";
}

/******************************************************************************/
int Client::Connect()
{
   bool connect = false;
   try {
      poco_debug( m_logger, "Trying to connect..." );
      //std::cerr << "Trying to connect to " << m_server << "...";
      //Thread::sleep(5000);
      //m_ss.connect( SocketAddress( m_server ) );
      m_ss.connect( SocketAddress( m_server ), TIMEOUT );
      //std::cerr << " connected!";
      poco_debug( m_logger, "Connected!" );

      connect = true;

   } catch (Poco::Exception& exc) {
      std::cerr << "> Warning [Connect() to " << m_server << "]: " << exc.displayText() << " (Is the remote island '" << m_server << "' running?)" << std::endl;
   } catch (...) {
      std::cerr << "> Error [Connect() to " << m_server << "]: Unknown error\n" ;
   }
   //std::cerr << "Connect: " << connect << std::endl;
   return( connect );
}

/******************************************************************************/
void Client::Disconnect()
{
   try {
      m_ss.close();
   } catch (Poco::Exception& exc) {
      std::cerr << "> Warning [Disconnect() close() from " << m_server << "]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "Closing failed: connection already closed" << std::endl;
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
