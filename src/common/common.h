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

typedef unsigned long long range_t;

struct TaskData {
   TaskData() {}
   TaskData( range_t begin, range_t end, range_t id ):
      begin(begin), end(end), id(id) {}

   range_t begin;
   range_t end;
   unsigned id;
};

/******************************************************************************/
#if 0
const char* Time();
#endif

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


#if 0
/** Buffer using the copy-and-swap idiom
    http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
**/
struct Buffer {
   Buffer( std::size_t s = 0 ): size( s + 1 ), buffer( new char[size] )
   {
      // NULL termination char
      buffer[size-1] = '\0';

      std::cerr << "<C>";
   }

   ~Buffer() { 
      std::cerr << "<~>";
      delete[] buffer; }

   // Copy constructor
   Buffer( const Buffer& o ): size( o.size ), buffer( new char[size] ) {
      std::cerr << "<CC>";
      std::copy( o.buffer, o.buffer + size, buffer );
   }

   // Copy assignment constructor
   Buffer& operator=( Buffer o ) {
      std::cerr << "<=>";
      swap( *this, o );

      return *this;
   }

public:
   // Actual storage space for content (excludes the char used for NULL
   // termination char)
   std::size_t capacity() const { return size - 1; }

   char* data() { return buffer; }
   const char* data() const { return buffer; }

   char& operator[]( std::size_t i ) { return buffer[i]; }
   const char& operator[]( std::size_t i ) const { return buffer[i]; }

   // Swap every class member
   friend void swap( Buffer& first, Buffer& second ) {
      using std::swap;

      swap( first.size, second.size );
      swap( first.buffer, second.buffer );
   }

public:
   std::size_t size;
   char* buffer;
};
#endif

/******************************************************************************/
#endif
