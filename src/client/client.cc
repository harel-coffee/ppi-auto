#include "client.h"

/******************************************************************************/
void Client::SndIndividual( const char* results )
{
   Connect();

   SndHeader( 'I', strlen( results ) );
   SndMessage( results, strlen( results ) );

   Disconnect();
}

/******************************************************************************/
void Client::Connect()
{
   while( true ) {
      try {
         //std::cout << "[" << Time() << "] Trying to connect... ";
         poco_debug( m_logger, "Trying to connect..." );
         m_ss.connect( SocketAddress( m_server ), 10000 );
         poco_debug( m_logger, "Connected!" );
         //std::cout << "connected!\n";

         break;
      } catch (...) {
         poco_error( m_logger, "Connection failed! Is the server running?" );
         Thread::sleep( 5000 );
      }
   }
}

/******************************************************************************/
void Client::Disconnect()
{
   m_ss.close();
}
