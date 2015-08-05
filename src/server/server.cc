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
   will check the request and then do one of the following actions:

   1) Send the initialization data and first task to the worker;
   2) Receive the results from the worker and send another task to it
**/
void Server::run()
{
   bool RET = false;

   Individual individual;
   individual.genome = new int[data.genome_size];

   char command; int msg_size;
   command = RcvHeader( msg_size );
   //printf("command: %c msg_size:%d\n", command,msg_size);

   switch( command ) {
      case 'I': {
                   RET = true;

                   // Receive the message
                   char* buffer = RcvMessage( msg_size );
                   int offset;

                   sscanf( buffer, "%f%n", &individual.fitness, &offset );
                   buffer += offset;
                   //printf("individual.fitness: %f genome_size:%d\n", individual.fitness, data.genome_size);
                   for( int i = 0; i < data.genome_size-1; i++ )
                   {
                      sscanf( buffer, "%d%n", &individual.genome[i], &offset );
                      buffer += offset;
                   }
                   sscanf( buffer, "%d", &individual.genome[data.genome_size-1] );

                   //for( int i = 0; i < data.genome_size; i++ )
                   //   printf("%d ", individual.genome[i]);
                   //printf("\n");
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
         if( m_individuals.size() > data.size ) 
         {
             m_individuals.pop();
         }
         m_individuals.push( individual );
         Thread::sleep(30000);
      }
   } // The mutex will be released (unlocked) at this point

   delete[] individual.genome;
}
