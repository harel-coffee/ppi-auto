////////////////////////////////////////////////////////////////////////////////
//  Parallel Program Induction (PPI) is free software: you can redistribute it
//  and/or modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation, either version 3 of the License,
//  or (at your option) any later version.
//
//  PPI is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//  details.
//
//  You should have received a copy of the GNU General Public License along
//  with PPI.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

#include "client.h"
#include "../util/Util.h"
#include <sstream>

double Client::time = 0.;

/******************************************************************************/
void Client::SndIndividual()
{
#ifdef PROFILING
   util::Timer t_send;
#endif
   if( Connect() )
   {
      try {
         //std::cerr << "SndIndividual: connected!\n";
         if (SndHeader( 'I', m_results.size() ))
            SndMessage( m_results.data(), m_results.size() );
         else std::cerr << "SndIndividual: error in SndHeader!\n";
      } catch (Poco::Exception& exc) {
         std::cerr << "> Error [SndIndividual()]: " << exc.displayText() << std::endl;
      }

      Disconnect();
   }
#ifdef PROFILING
   Client::time += t_send.elapsed();
#endif
}

/******************************************************************************/
int Client::Connect()
{
   bool connect = false;
   try {
      //Thread::sleep(5000);
      m_ss.connect(SocketAddress( m_server ));

      connect = true;

   } catch (Poco::Exception& exc) {
#ifdef NDEBUG
      std::cerr << '!';
#else
      std::cerr << "> Warning [Connect() to " << m_server << "]: " << exc.displayText() << " (Is the remote island '" << m_server << "' running?)" << std::endl;
#endif
   } catch (...) {
      std::cerr << "> Error [Connect() to " << m_server << "]: Unknown error\n" ;
   }

   return( connect );
}

/******************************************************************************/
void Client::Disconnect()
{
   try {
      m_ss.close();
   } catch (Poco::Exception& exc) {
      std::stringstream error_msg;
      error_msg << "> [Disconnect() close() from " << m_server << "]: " << exc.displayText() << std::endl;
      poco_debug(m_logger, error_msg.str());
   } catch (...) {
      std::cerr << "Closing failed: unknown error" << std::endl;
   }
#if 0
   try {
      m_ss.shutdown();
   } catch (Poco::Exception& exc) {
      std::cerr << "> Error [Disconnect() shutdown() from " << m_server << "]: " << exc.displayText() << std::endl;
   } catch (...) {
      std::cerr << "Shutdown failed: connection already closed" << std::endl;
   }
#endif
}
