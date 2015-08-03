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

#include "master.h"

#include <sstream>

// TODO: Use exceptions (try-catch blocks, throws, etc.) throughout the code

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Master::m_mutex;
std::string Master::m_config;
//bool Master::m_stop = false;

WorkQueue* Master::m_workqueue = 0;

/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do one of the following actions:

   1) Send the initialization data and first task to the worker;
   2) Receive the results from the worker and send another task to it
**/
void Master::run()
{
   //poco_information_f1( m_logger, "Connection from %s", socket().peerAddress().toString() );

   std::string results;

   bool RET = false;
   TaskData  task_ret;

   char command; int msg_size;
   command = RcvHeader( msg_size );

   switch( command ) {
      case 'I': {
                   // Send config file
                   SndHeader( 'I', m_config.size() );
                   SndMessage( m_config.data(), m_config.size() );
                }
                break;

      case 'R': {
                   RET = true;

                   // Receive the message
                   char* buffer = RcvMessage( msg_size );

                   sscanf( buffer, "%d %llu %llu", &task_ret.id, &task_ret.begin, &task_ret.end );
                   //if( !buffer )
                      //poco_error( "Error receiving message!" );

                   // TODO: Extract all results and put them into a private temporary space
                   //results = std::string( buffer );
                }
                break;

      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }

   Task* task;
   {
      Poco::FastMutex::ScopedLock lock( m_mutex );
      //std::cerr << "[";
      if( RET )
      {
         // Check whether task is already done
         if( m_workqueue->sent[task_ret.id] != 0 )
         {
            m_workqueue->DeleteTask( m_workqueue->sent[task_ret.id] );
            poco_information_f1( m_logger, "Task completed: %u", task_ret.id );
         }
         else
            poco_information_f1( m_logger, "Task already completed: %u", task_ret.id );

         // Get results from worker
         //std::cerr << results << std::endl;

      }

      // Check if there still exists tasks to be processed
      if( m_workqueue->first == 0 )
      {
         SndHeader( 'S' );
         return;
      }

      task = m_workqueue->PickFirst();
   } // The mutex will be released (unlocked) at this point

   std::stringstream task_str;
   task_str << task->data.id << " " << task->data.begin << " " << task->data.end << "\n";

   poco_information_f2( m_logger, "Sending task to %s: %s", socket().peerAddress().toString(), task_str.str() );
   // Send task
   SndHeader( 'T', task_str.str().size() );
   SndMessage( task_str.str().data(), task_str.str().size() );

   //std::cerr << "]";
}

/******************************************************************************/
int main( int argc, char** argv )
{
   // Set up the configuration data (read kernel from file)
   std::ifstream file( (argc>1) ? argv[1] : "kernel.cl" );
   Master::m_config = std::string( std::istreambuf_iterator<char>(file),
                                 ( std::istreambuf_iterator<char>()) );

   Common::SetupLogger();

   // Workqueue
   WorkQueue wq( 10000000, 1000 );
   Master::m_workqueue = &wq;

   ServerSocket svs(SocketAddress( "0.0.0.0", 32768 ) );
   TCPServerParams* pParams = new TCPServerParams;
   pParams->setMaxThreads(4);
   pParams->setMaxQueued(4);
   pParams->setThreadIdleTime(100);
   TCPServer srv(new TCPServerConnectionFactoryImpl<Master>(), svs, pParams);
   srv.start();

   // Always alive so workers can receive the stop header (S)
   for( ;; ) Thread::sleep( 1000 );

   return 0;
}

/******************************************************************************/
