#include "common.h"

/******************************************************************************/
Logger& Common::m_logger( Logger::get( "logger" ) ); 


/******************************************************************************/
void Common::SndMessage( const void* buffer, int msg_size )
{
   int n = m_ss.sendBytes( buffer, msg_size );
   //std::cerr << "SndMessage " << n << std::endl;
   assert( n == msg_size );
}

/******************************************************************************/
char* Common::RcvMessage( int msg_size, std::vector<char>& buffer )
{
   ExpandBuffer( msg_size, buffer );

   /* The function receiveBytes only receives a dataframe at a time. Because of
that, a sequence of calls to it must be made until the complete message is
fully delivered. */
   int n = 0, bytes = 0;
   do {
      bytes = m_ss.receiveBytes( buffer.data()+n, msg_size-n );
      n += bytes;
   } while (n<msg_size && bytes != 0);

   //int n = m_ss.receiveBytes( buffer.data(), msg_size );

   ///*
   //if( n != msg_size ) {
   //   //poco_error( "Error receiving message!" );
   //   return 0;
   //}
   //*/

   //std::cerr << "RcvMessage " << n << std::endl;
   assert( n == msg_size );

   return buffer.data();
}

char* Common::RcvMessage( int msg_size )
{
   return RcvMessage( msg_size, m_buffer );
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

   //std::cerr << "SndHeader " << n << std::endl;
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

      //std::cerr << "RcvHeader " << n << std::endl;
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
