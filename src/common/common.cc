////////////////////////////////////////////////////////////////////////////////
//  Parallel Program Induction (PPI) is free software: you can redistribute it
//  and/or modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation, either version 3 of the License,
//  or (at your option) any later version.
//
//  PPI is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with PPI.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

#include "common.h"

#define MAX_MESSAGE_SIZE 65536

/******************************************************************************/
Logger& Common::m_logger( Logger::get( "logger" ) ); 


/******************************************************************************/
bool Common::SndMessage( const void* buffer, int msg_size )
{
   // FIXME: Implement a loop as in RcvMessage because it is not guaranteed
   // that the whole message will be sent at once.
   try {
      int n = m_ss.sendBytes( buffer, msg_size );
      //std::cerr << "SndMessage " << n << std::endl;
      if (n != msg_size) throw Poco::Exception("Could not send the whole message");
   }
   catch (Poco::Exception& exc) {
      std::cerr << "Connection: " << exc.displayText() << std::endl;

      return false;
   }

   return true;
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

   if( n != msg_size ) {
      std::cerr <<  "Error receiving message: only received " << n << " bytes out of " << msg_size << " bytes!\n";
      //poco_error( "Error receiving message!" );
   }

   return buffer.data();
}

char* Common::RcvMessage( int msg_size )
{
   return RcvMessage( msg_size, m_buffer );
}

/******************************************************************************/
bool Common::SndHeader( char command, int msg_size )
{
   try
   {
      char header[10];
      // Put the command:
      header[0] = command;

      // Add the message size:
      snprintf( header+1, 9, "%-8d", msg_size );

      // Send the header through the open stream:
      int n = m_ss.sendBytes( header, 10 );

      //std::cerr << "SndHeader " << n << std::endl;
      if (n != 10) throw Poco::Exception("Could not send the whole header");
   }
   catch (Poco::Exception& exc) {
      std::cerr << "Connection: " << exc.displayText() << std::endl;

      return false;
   }

   return true;
}

/******************************************************************************/
char Common::RcvHeader( int& msg_size )
{
   char command;

   try
   {
      char header[11]; header[10] = '\0';
      int n = m_ss.receiveBytes( header, 10 );

      if (n != 10) throw Poco::Exception("Could not read the whole header");

      //std::cerr << header << std::endl;

      // Check if the msg_size's first character is a digit
      if ( header[1] < '0' || header[1] > '9' ) throw Poco::Exception("Malformed header");

      sscanf( header, "%c%d", &command, &msg_size );
      if (!( header[1] == '0' || msg_size > 0 )) throw Poco::Exception("Invalid message size");

      if (msg_size > MAX_MESSAGE_SIZE) throw Poco::Exception("Invalid message size, too big");

      if (!( command == 'I' || command == 'T' || command == 'R' || command == 'S')) throw Poco::Exception("Invalid command");
   }
   catch (Poco::Exception& exc) {
      command = '\0';
      msg_size = 0;

      std::cerr << "Connection: " << exc.displayText() << std::endl;
   }

   return command;
}
