#include "../util/CmdLineParser.h"
#include "server.h"

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;

std::vector<char>* Server::m_immigrants = NULL;
float* Server::m_fitness = NULL;

std::queue<int> Server::m_freeslots;
std::queue<int> Server::m_ready;


/******************************************************************************/
/** This function is called whenever a worker connects to master. The master
   will check the request and then do the following action:

   1) Receive the results from the worker
**/
void Server::run()
{
   char command; int msg_size;
   command = RcvHeader( msg_size );

   int slot;

   switch( command ) {
      case 'I': {
                   while( m_freeslots.empty() ) { Thread::sleep(1000); }

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      if( m_freeslots.empty() ) { return; }
                      slot = m_freeslots.front();
                      m_freeslots.pop();
                   }

                   /* Receive the message ("individual"). Note that if a
                    * problem occurs while receiving the message, the array
                    * m_immigrants[slot] will contain a truncated message
                    * ("junk"). This is not a serious problem because it will
                    * act as if the individual had undergone a mutation. */
                   RcvMessage( msg_size, m_immigrants[slot] );

                   {
                      Poco::FastMutex::ScopedLock lock( m_mutex );

                      m_ready.push(slot);
                   }

                   //std::cerr << "Server::Receiving[slot=" << slot << "]: " << m_immigrants[slot].data() << std::endl;
                }
                break;
      default:
                poco_fatal( m_logger, "Unrecognized command!" );
                return;
   }
}
