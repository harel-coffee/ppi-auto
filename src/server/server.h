#ifndef __server_h
#define __server_h

#include "Poco/Net/TCPServer.h"
#include "Poco/Net/TCPServerConnection.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/TCPServerParams.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Thread.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cassert>

using Poco::Net::TCPServer;
using Poco::Net::TCPServerConnection;
using Poco::Net::TCPServerConnectionFactory;
using Poco::Net::TCPServerConnectionFactoryImpl;
using Poco::Net::TCPServerParams;
using Poco::Net::StreamSocket;
using Poco::Net::ServerSocket;
using Poco::Net::SocketAddress;
using Poco::Thread;

#include "../common/common.h"

struct Task {
   Task( range_t begin, range_t end, unsigned id = 0 ):
      next(0), prev(0), data(begin, end, id) {}

   Task* next;
   Task* prev;
   TaskData data;
};

struct WorkQueue {
   WorkQueue( range_t worksize, range_t tasksize ): first(0), last(0)
   {
      range_t i;
      for( i = 0; i < worksize - tasksize; i += tasksize ) {
         CreateTask( i, i + tasksize - 1 );
      }

      CreateTask( i, worksize - 1 );
   }

   ~WorkQueue() {
      // TODO
   }


   void CreateTask( range_t begin, range_t end )
   {
      Task* task = new Task( begin, end );

      if( last ) {
         last->next = task;
         task->prev = last;

         // Increment the task id
         task->data.id = last->data.id + 1;
      } else
         first = task; // First added task

      last = task;

      // Expand the vector of sent
      sent.push_back(0);

      std::cerr << "Created task: " << task->data.id << " " << task->data.begin << " " << task->data.end << std::endl;
   }

   void DeleteTask( Task* task )
   {
      // Corner cases
      if( task == first )
         first = task->next;
      if( task == last )
         last = task->prev;

      if( task->next )
         task->next->prev = task->prev;
      if( task->prev )
         task->prev->next = task->next;

      // Update sent vector (remove task from sent)
      sent[task->data.id] = 0;

      // Free task's memory
      delete task;
   }


   Task* PickFirst() {
      Task* task = first;

      if( task->next ) // Do we have more than one task?
      {
         // *** Remove from the first position ***
         // Make the second task the first
         first = task->next;

         // The first task doesn't have a prev
         first->prev = 0;

         // *** Replace at the end of the queue/list ***
         // The last task now has a neighbor (next task)
         last->next = task;

         // Conversely, the picked task now has a left neighbor (prev task)
         task->prev = last;

         // But it doesn't have a right neighbor (next task)
         task->next = 0;

         // Finally, make the picked task the last one
         last = task;
      }

      // Update the sent vector
      sent[task->data.id] = task;

      return task;
   }

   Task* first;
   Task* last;
   std::vector<Task*> sent;
};


/******************************************************************************/
//namespace xyz {
   class Server: public TCPServerConnection, public Common {
   public:
      /* We cannot do Common(s) here because 's' is 'const'. TCPServerConnection(s)
         makes a copy of 's', which then can be accessed through 'socket()'. */
      Server( const StreamSocket& s ): TCPServerConnection(s), Common( socket() ) {}

      void run();

   public:
      /* A 'static variable' will always be shared among all threads.

         A 'standard variable' will be shared among the threads that
         are run over the same object of this class.

         A 'ThreadLocal static variable' will not be shared even if the
         threads are run over the same object.

         Finally, all variables declared inside 'run()' are private for
         the current thread (executing the method run()). */
      static Poco::FastMutex m_mutex;
      static std::string m_config;
      //static bool m_stop;

   public:
      static WorkQueue* m_workqueue;

   };
//}

//using namespace xyz;

/******************************************************************************/
#endif
