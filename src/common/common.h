#ifndef __common_h
#define __common_h

#include "Poco/Net/StreamSocket.h"
#include "Poco/Logger.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/AutoPtr.h"

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <memory>
#include <vector>

#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"

using Poco::FormattingChannel;
using Poco::PatternFormatter;

using Poco::Net::StreamSocket;
using Poco::Logger;
using Poco::ConsoleChannel;
using Poco::AutoPtr;


/******************************************************************************/
class Common {
public:
   Common( StreamSocket& ss ): //, const char* logger = "" ): 
      m_ss( ss )//, m_logger( Logger::get( logger ) )
   {}

   static void SetupLogger( const char* level = "information", 
                     const char* pattern = "<%q> [%N %Y-%m-%d %H:%M:%S] %t" )
   {
      // Set the default logging level
      m_logger.setLevel( level );

      AutoPtr<PatternFormatter> pPF(new PatternFormatter);
      pPF->setProperty("pattern", pattern );

      // Set the formatted ConsoleChannel as the default channel
      m_logger.setChannel( AutoPtr<FormattingChannel>(new FormattingChannel(pPF,
                           AutoPtr<ConsoleChannel>(new ConsoleChannel))) );
   }

   void SndMessage( const void* buffer, int msg_size );
   char* RcvMessage( int msg_size );
   void SndHeader( char command, int msg_size = 0 );
   char RcvHeader( int& msg_size );

public:
   /* As each server connection thread constructs a new object then we
      do not need to declared m_ss as ThreadLocal because each thread
      will already have its own private copy. */
   StreamSocket& m_ss;
   static Logger& m_logger;

public:
   void ExpandBuffer( std::size_t size )
   {
      if( m_buffer.size() < size + 1 ) m_buffer.resize( size + 1 );

      // NULL termination char
      m_buffer[size] = '\0';
   }

   std::vector<char> m_buffer;
};
/******************************************************************************/
#endif
