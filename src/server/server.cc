#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;
Server::m_writepos;


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int genome_size; int immigrants_size; int population_size; } data;


void server_init( int argc, char** argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-ps", "--population-size", 1024, 5, std::numeric_limits<int>::max() );
   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
   Opts.Process();
   data.genome_size = Opts.Int.Get("-nb");
   data.population_size = Opts.Int.Get("-ps");

   data.immigrants_size = 10;
}


/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do the following action:

   1) Receive the results from the worker
**/
void Server::run()
{
   char command; int msg_size;
   command = RcvHeader( msg_size );

   Population* mypop;

   switch( command ) {
      case 'I': {
                   while( m_writepos >= data.immigrants_size ) { Thread::sleep(1000); }

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      //m_writepos = 0 ... (data.immigrants_size-1)
                      if( ++m_writepos >= data.immigrants_size ) { return; } 

                      mypop = m_pop;
                   }
 
                   // Receive the message
                   char* buffer = RcvMessage( msg_size );
                   int offset;

                   //std::cerr << "Receiving Individual: " << std::endl;

                   // Checking if the m_pop was modified during the evolutionary process (pee_receice_individual)
                   if( mypop != m_pop ) { return; }
                   sscanf( buffer, "%f%n", m_pop->fitness[data.population_size - data.immigrants_size + m_writepos], &offset );
                   buffer += offset;
                   for( int i = 0; i < data.genome_size-1; i++ )
                   {
                      if( mypop != m_pop ) { return; }
                      sscanf( buffer, "%d%n", m_pop->genome[(data.population_size - data.immigrants_size + m_writepos) * data.genome_size + i], &offset );
                      buffer += offset;
                   }
                   if( mypop != m_pop ) { return; }
                   sscanf( buffer, "%d", m_pop->genome[(data.population_size - data.immigrants_size + m_writepos) * (data.genome_size - 1)]);

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      //data.immigrants_size = 1 ... data.immigrants_size
                      if( mypop == m_pop && m_immigrants < data.immigrants_size ) { m_immigrants++; } 
                   }

                }
                break;
      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }
}
