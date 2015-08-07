#include "common.h"

/******************************************************************************/
#if 0
const char* Time()
{
   static char buffer[80];
   static std::time_t now;

   std::time( &now );
   strftime(buffer, 80, "%F %T", std::localtime(&now) );

   return buffer;
}
#endif 

Logger& Common::m_logger( Logger::get( "logger" ) ); 


/******************************************************************************/
void Common::SndMessage( const void* buffer, int msg_size )
{
   int n = m_ss.sendBytes( buffer, msg_size );
   assert( n == msg_size );
}

/******************************************************************************/
char* Common::RcvMessage( int msg_size )
{
   ExpandBuffer( msg_size );

   int n = m_ss.receiveBytes( m_buffer.data(), msg_size );

   /*
   if( n != msg_size ) {
      //poco_error( "Error receiving message!" );
      return 0;
   }
   */

   assert( n == msg_size );

   return m_buffer.data();
}

/******************************************************************************/
void Common::SndHeader( char command, int msg_size )
{
   // TODO: make errors not hard

   char header[10];
   // Put the command:
   header[0] = command;

   // Add the message size:
   snprintf( header+1, 9, "%-8d", msg_size );

   // Send the header through the open stream:
   int n = m_ss.sendBytes( header, 10 );

   assert( n == 10 );
}

/******************************************************************************/
char Common::RcvHeader( int& msg_size )
{
   // TODO: make errors not hard

   char command;

   try
   {
      char header[11]; header[10] = '\0';
      int n = m_ss.receiveBytes( header, 10 );
      assert( n == 10 );

      //std::cerr << header << std::endl;

      assert( header[1] >= '0' && header[1] <= '9' );

      sscanf( header, "%c%d", &command, &msg_size );
      assert( header[1] == '0' || msg_size > 0 );

      assert( command == 'I' || command == 'T' || command == 'R' || command == 'S' );
   }
   catch (Poco::Exception& exc) {
      command = '\0';
      msg_size = 0;

      std::cerr << "Connection: " << exc.displayText() << std::endl;
   }

   return command;
}
