#include <sstream>
#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;
std::string Server::m_config;
//bool Server::m_stop = false;

WorkQueue* Server::m_workqueue = 0;


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

//static struct t_data { Individual* individuals; int next; genome_size; int size; } data;

//void server_init( int argc, char** argv ) 
//{
//   CmdLine::Parser Opts( argc, argv );
//
//   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
//   Opts.Process();
//   data.genome_size = Opts.Int.Get("-nb");
//
//   data.next = 0;
//   data.size = 10;
//   data.individuals = new Individual[data.size];
//   for( int i = 0; i < data.size; ++i )
//   {
//      data.individuals[i].genome = new int[data.genome_size];
//   }
//}


/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do one of the following actions:

   1) Send the initialization data and first task to the worker;
   2) Receive the results from the worker and send another task to it
**/
void Server::run()
{
   //poco_information_f1( m_logger, "Connection from %s", socket().peerAddress().toString() );

   std::string results;

   bool RET = false;
   TaskData  task_ret;
   //Individual individual;
   //individual.genome = new int[data.genome_size];

   char command; int msg_size;
   command = RcvHeader( msg_size );

   switch( command ) {
      case 'R': {
                   RET = true;

                   // Receive the message
                   char* buffer = RcvMessage( msg_size );

                   sscanf( buffer, "%d %llu %llu", &task_ret.id, &task_ret.begin, &task_ret.end );
                   //sscanf( buffer, "%f ", &individual.fitness );
                   //for( int i = 0; i < data.genome_size-1; i++ )
                   //{
                   //   sscanf( buffer, "%d ", &individual.genome[i] );
                   //}
                   //sscanf( buffer, "%d", &individual.genome[data.genome_size-1] );

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
            //poco_information_f1( m_logger, "Task completed: %u", task_ret.id );
         }
         //else
            //poco_information_f1( m_logger, "Task already completed: %u", task_ret.id );

         // Get results from worker
         //std::cerr << results << std::endl;

         //data.individuals[data.next].fitness = individual.fitness;
         //for( int i = 0; i < genome_size; ++i )
         //{
            //data.individuals[data.next].genome[i] = individual.genome[i];
         //}
         //data.next++;
         //if( data.next >= data.size ) 
         //{
         //   data.next = 0;
         //}
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

   //poco_information_f2( m_logger, "Sending task to %s: %s", socket().peerAddress().toString(), task_str.str() );
   // Send task
   SndHeader( 'T', task_str.str().size() );
   SndMessage( task_str.str().data(), task_str.str().size() );

   //std::cerr << "]";

   //delete[] individual.genome;
}
