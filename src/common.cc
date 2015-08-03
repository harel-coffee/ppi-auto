// ---------------------------------------------------------------------------
// Copyright (C) 2013 Douglas Adriano Augusto
//
// This file is part of MaPLA.
//
// MaPLA is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------------

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
