#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;

std::vector<char>* Server::m_buffer_immigrants = NULL;
int* Server::m_immigrants = NULL;
float* Server::m_fitness = NULL;

std::queue<int> Server::m_freeslots;
std::queue<int> Server::m_ready;

int Server::m_genome_size;


/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do the following action:

   1) Receive the results from the worker
**/
void Server::run()
{
   char command; int msg_size;
   command = RcvHeader( msg_size );

   //bool flag = false;
   //char* buffer; int offset;
   int slot;

   switch( command ) {
      case 'I': {
                   while( m_freeslots.empty() ) { Thread::sleep(1000); }

                   //if( m_freeslots.empty() )
                   //{
                   //   // Receive the message
                   //   buffer = RcvMessage( msg_size );
                   //   flag = true;
                   //   while( m_freeslots.empty() ) { Thread::sleep(1000); }
                   //}

                   //if( !flag ) 
                   //{
                   //   // Receive the message
                   //   buffer = RcvMessage( msg_size );
                   //   flag = true;
                   //}

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      if( m_freeslots.empty() ) { return; }
                      slot = m_freeslots.front();
                      m_freeslots.pop();
                   }

                   // Receive the message
                   RcvMessage( msg_size, m_buffer_immigrants[slot] );
                   
                   //sscanf( buffer, "%f%n", &m_fitness[slot], &offset );
                   //buffer += offset;
                   //for( int i = 0; i < m_genome_size-1; i++ )
                   //{
                   //   sscanf( buffer, "%d%n", &m_immigrants[slot * m_genome_size + i], &offset );
                   //   buffer += offset;
                   //}
                   //sscanf( buffer, "%d", &m_immigrants[slot * (m_genome_size - 1)]);

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      m_ready.push(slot);
                   }

                   std::cerr << "Receiving[slot=" << slot << "]: " << m_buffer_immigrants[slot].data() << std::endl;
                }
                break;
      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }
}
