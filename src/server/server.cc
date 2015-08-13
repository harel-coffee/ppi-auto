#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;
std::queue<Individual> Server::m_individuals;


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int genome_size; int size; } data;


void server_init( int argc, char** argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
   Opts.Process();
   data.genome_size = Opts.Int.Get("-nb");

   data.size = 10;
}


/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do the following action:

   1) Receive the results from the worker
**/
void Server::run()
{
   bool RET = false;

   Individual individual;
   individual.genome = new int[data.genome_size];

   char command; int msg_size;
   command = RcvHeader( msg_size );

   switch( command ) {
      case 'I': {
                   RET = true;

                   // Receive the message
                   char* buffer = RcvMessage( msg_size );
                   int offset;

                   sscanf( buffer, "%f%n", &individual.fitness, &offset );
                   buffer += offset;
                   for( int i = 0; i < data.genome_size-1; i++ )
                   {
                      sscanf( buffer, "%d%n", &individual.genome[i], &offset );
                      buffer += offset;
                   }
                   sscanf( buffer, "%d", &individual.genome[data.genome_size-1] );
               }
                break;

      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }

   {
      Poco::FastMutex::ScopedLock lock( m_mutex );
      if( RET )
      {
         while( m_individuals.size() > data.size ) 
         {
            Individual out = m_individuals.front();
            delete[] out.genome;
            m_individuals.pop();
         }
         m_individuals.push( individual );

         //printf("Individual:\n");
         //printf("%f ", individual.fitness);
         //for( int i = 0; i < data.genome_size; i++ )
         //   printf("%d ", individual.genome[i]);
         //printf("\n");

         //printf("Queue:\n");
         //printf("%f ", m_individuals.front().fitness);
         //for( int i = 0; i < data.genome_size; i++ )
         //   printf("%d ", m_individuals.front().genome[i]);
         //printf("\n");
 
         //Thread::sleep(10000);
      }
   } // The mutex will be released (unlocked) at this point
}
