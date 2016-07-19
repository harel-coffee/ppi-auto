#include "../util/CmdLineParser.h"
#include "server.h"

// Is it better to cancel immediately the connection or wait until a slot
// become available? Setting DROP_WHEN_THERE_ARE_NO_SLOTS causes the connection
// to be dropped immediately if there is not a single slot available
#define DROP_WHEN_THERE_ARE_NO_SLOTS

/******************************************************************************/
/** Definition of the static variables **/
Poco::FastMutex Server::m_mutex;

std::vector<char>* Server::m_immigrants = NULL;
float* Server::m_fitness = NULL;

unsigned long Server::stagnation = 0;
unsigned long Server::immigrants_acceptance_threshold = 0;

std::queue<int> Server::m_freeslots;
std::queue<int> Server::m_ready;


/******************************************************************************/
/** This function is called whenever an island connects to a server island. The
    server will check the request and then do the following action:
**/
void Server::run()
{
   char command; int msg_size;
   command = RcvHeader( msg_size );

   int slot;

   switch( command ) {
      case 'I': {
                   /* Only accepts an immigrant if we are stagnated (above a
                    * certain threshold)! This promotes niches and therefore
                    * diversification among islands. */
                   if (stagnation <= immigrants_acceptance_threshold) {
                      poco_debug(m_logger, "Rejecting individual because we still didn't stagnate!");
                      return;
                   }

#ifdef DROP_WHEN_THERE_ARE_NO_SLOTS
                   // Return immediately when not a single slot is available
                   if ( m_freeslots.empty() ) {
                      poco_debug( m_logger, "Dropping connection because there are no free slots!" );
                      return;
                   }
#else
                   // Wait until a slot become available
                   while( m_freeslots.empty() ) { Thread::sleep(1000); }
#endif


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
