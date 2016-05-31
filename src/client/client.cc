#include "client.h"

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
         // FIXME: remove
         Disconnect();
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
      m_ss.connect( SocketAddress( m_server ), 10000 );
      //std::cerr << " connected!";
      poco_debug( m_logger, "Connected!" );

      connect = true;

   } catch (Poco::Exception& exc) {
      std::cerr << "> Error [Connect()]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "> Error [Connect()]: Unknown error\n" ;
      poco_error( m_logger, "Connection failed! Is the server running?" );
   }
   //std::cerr << "Connect: " << connect << std::endl;
   return( connect );
}

/******************************************************************************/
void Client::Disconnect()
{
   // FIXME: Poco with ThreadPool is not closing the socket at all! So,
   // eventually the client cannot connect anymore because of "too many open
   // sockets"!

   //m_ss.close();
   try {
      m_ss.close();
   } catch (Poco::Exception& exc) {
      std::cerr << "> Error [Disconnect() close()]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "Closing failed: connection already closed" << std::endl;
   }
   try {
      m_ss.shutdown();
   } catch (Poco::Exception& exc) {
      std::cerr << "> Error [Disconnect() shutdown()]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "Shutdown failed: connection already closed" << std::endl;
   }
}
