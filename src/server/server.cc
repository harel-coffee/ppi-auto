#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;
Population* Server::m_pop;
int Server::m_writepos;
int Server::m_immigrants;
int Server::m_genome_size;
int Server::m_immigrants_size;
int Server::m_population_size;


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
                   while( m_writepos >= m_immigrants_size ) { Thread::sleep(1000); }

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      //m_writepos = 0 ... (m_immigrants_size-1)
                      if( ++m_writepos >= m_immigrants_size ) { return; } 

                      mypop = m_pop;
                   }
 
                   // Receive the message
                   char* buffer = RcvMessage( msg_size );
                   int offset;

                   //std::cerr << "Receiving Individual: " << std::endl;

                   // Checking if the m_pop was modified during the evolutionary process (pee_receice_individual)
                   if( mypop != m_pop ) { return; }
                   sscanf( buffer, "%f%n", &m_pop->fitness[m_population_size + m_writepos], &offset );
                   buffer += offset;
                   for( int i = 0; i < m_genome_size-1; i++ )
                   {
                      if( mypop != m_pop ) { return; }
                      sscanf( buffer, "%d%n", &m_pop->genome[(m_population_size + m_writepos) * m_genome_size + i], &offset );
                      buffer += offset;
                   }
                   if( mypop != m_pop ) { return; }
                   sscanf( buffer, "%d", &m_pop->genome[(m_population_size + m_writepos) * (m_genome_size - 1)]);

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      //m_immigrants_size = 1 ... m_immigrants_size
                      if( mypop == m_pop && m_immigrants < m_immigrants_size ) { m_immigrants++; } 
                   }

                }
                break;
      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }
}
