#include "client.h"

/******************************************************************************/
void Client::SndIndividual()
{
   if( Connect() )
   {
      SndHeader( 'I', m_results.size() );
      SndMessage( m_results.data(), m_results.size() );

      Disconnect();
   }
}

/******************************************************************************/
int Client::Connect()
{
   bool connect = false;
   try {
      poco_debug( m_logger, "Trying to connect..." );
      //std::cerr << "Trying to connect to " << m_server << " ...";
      //Thread::sleep(5000);
      //m_ss.connect( SocketAddress( m_server ) );
      m_ss.connect( SocketAddress( m_server ), 10000 );
      //std::cerr << " connected!";
      poco_debug( m_logger, "Connected!" );

      connect = true;

   } catch (...) {
      //std::cerr << "Connection failed! Is the server running?\n" ;
      poco_error( m_logger, "Connection failed! Is the server running?" );
   }
   return( connect );
}

/******************************************************************************/
void Client::Disconnect()
{
   m_ss.close();
}
