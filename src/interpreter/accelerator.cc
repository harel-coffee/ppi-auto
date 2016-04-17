// ------------------------------------------------------------------------
#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <cmath> 
#include <limits>
#include <string>   
#include <vector>
#include <queue>
#include <utility>
#include <iostream> 
#include <fstream> 
#include "accelerator.h"
#include "../server/server.h"
#include "../util/CmdLineParser.h"
#include "../util/Util.h"

using namespace std;

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

namespace { static struct t_data { int max_size; int nlin; int population_size; unsigned local_size1; unsigned global_size1; unsigned local_size2; unsigned global_size2; std::string strategy; cl::Device device; cl::Context context; cl::Kernel kernel1; cl::Kernel kernel2; cl::CommandQueue queue; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_size; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; cl::Buffer buffer_error; cl::Buffer buffer_pb; cl::Buffer buffer_pi; double time_kernel; double time_overhead; } data; };

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTION *********************** **/
/** ****************************************************************** **/

// -----------------------------------------------------------------------------
int opencl_init( int platform_id, int device_id, int type )
{
   vector<cl::Platform> platforms;

   /* Iterate over the available platforms and pick the list of compatible devices
      from the first platform that offers the device type we are querying. */
   cl::Platform::get( &platforms );

   vector<cl::Device> devices;

   /*

   Possible options:

        type  device  platform

   1.   -1      X        X
   2.   -1     -1        X     --> device = 0
   3.   -1      X       -1     --> platform = 0
   4.    X     -1        X     
   5.    X     -1       -1 
   6.   -1     -1       -1     --> platform = last_platform; device = 0 

   */

   if( platform_id < 0 && device_id >= 0 ) // option 3
   {
      platform_id = 0; 
   }

   // Check if the user gave us a valid platform 
   if( platform_id >= (int) platforms.size() )
   {
      fprintf(stderr, "Valid platform range: [0, %d].\n", (int) (platforms.size()-1));
      return 1;
   }

   bool leave = false;

   int first_platform = platform_id >= 0 ? platform_id : 0;
   int last_platform  = platform_id >= 0 ? platform_id + 1 : platforms.size();
   for( int m = first_platform; m < last_platform; m++ )
   {
      platforms[m].getDevices( CL_DEVICE_TYPE_ALL, &devices );

      // Check if the user gave us a valid device 
      if( device_id >= (int) devices.size() ) 
      {
         fprintf(stderr, "Valid device range: [0, %d].\n", (int) (devices.size()-1));
         return 1;
      }

      int first_device = device_id >= 0 ? device_id : 0;
      data.device = devices[first_device];

      if( type >= 0 && device_id < 0 ) // options 4 e 5
      {
         for ( int n = 0; n < devices.size(); n++ )
         {
            if ( devices[n].getInfo<CL_DEVICE_TYPE>() == type ) 
            {
               leave = true;
               data.device = devices[n];
               break;
            }
         }
         if( leave ) break;
      }
   }

   if( type >= 0 && !leave )
   {
      fprintf(stderr, "Not a single compatible type found.\n");
      return 1;
   }

   data.context = cl::Context( devices );

   data.queue = cl::CommandQueue( data.context, data.device, CL_QUEUE_PROFILING_ENABLE );

   return 0;
}

// -----------------------------------------------------------------------------
int build_kernel( )
{
   ifstream file("accelerator.cl");
   string kernel_str( istreambuf_iterator<char>(file), ( istreambuf_iterator<char>()) );

   cl::Program::Sources source( 1, make_pair( kernel_str.c_str(), kernel_str.size() ) );

   cl::Program program( data.context, source );

   char buildOptions[60];
   sprintf( buildOptions, "-DMAX_PHENOTYPE_SIZE=%u -I.", data.max_size );  
   vector<cl::Device> device; device.push_back( data.device );
   try {
      program.build( device, buildOptions );
   }
   catch( cl::Error& e )
   {
      cerr << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(data.device) << std::endl;
      throw;
   }

   unsigned max_cu = data.device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
   unsigned max_local_size = fmin( data.device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(), data.device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0] );

   if( data.strategy == "PP" )  // Population-parallel
   {
      data.local_size1 = 1;
      data.global_size1 = data.population_size;
      data.kernel1 = cl::Kernel( program, "evaluate_pp" );
   }
   else
   {
      if( data.strategy == "FP" ) // Fitness-parallel
      {
         // Evenly distribute the workload among the compute units (but avoiding local size
         // being more than the maximum allowed).
         data.local_size1 = fmin( max_local_size, (unsigned) ceil( data.nlin/(float) max_cu ) );

         // It is better to have global size divisible by local size
         data.global_size1 = (unsigned) ( ceil( data.nlin/(float) data.local_size1 ) * data.local_size1 );
         data.kernel1 = cl::Kernel( program, "evaluate_fp" );
      }
      else
      {
         if( data.strategy == "PPCU" ) // Population-parallel computing unit
         {
            if( data.nlin < max_local_size )
            {
               data.local_size1 = data.nlin;
            }
            else
            {
               data.local_size1 = max_local_size;
            }
            // One individual per work-group
            data.global_size1 = data.population_size * data.local_size1;
            data.kernel1 = cl::Kernel( program, "evaluate_ppcu" );
         }
         else
         {
            fprintf(stderr, "Valid strategy: PP, FP and PPCU.\n");
            return 1;
         }
      }
   }
   // Evenly distribute the workload among the compute units (but avoiding local size
   // being more than the maximum allowed).
   data.local_size2 = fmin( max_local_size, (unsigned) ceil( data.population_size/(float) max_cu ) );
   // It is better to have global size divisible by local size
   data.global_size2 = (unsigned) ( ceil( data.population_size/(float) data.local_size2 ) * data.local_size2 );
   data.kernel2 = cl::Kernel( program, "best_individual" );


   //std::cout << "\nDevice: " << data.device.getInfo<CL_DEVICE_NAME>() << ", Compute units: " << max_cu << ", Max local size: " << max_local_size << std::endl;
   //std::cout << "\nLocal size: " << data.local_size1 << ", Global size: " << data.global_size1 << ", Work groups: " << data.global_size1/data.local_size1 << "\n" << std::endl;

   return 0;
}

// -----------------------------------------------------------------------------
void create_buffers( float** input, float** model, float* obs, int ninput, int nmodel, int prediction_mode )
{
   int ncol = ninput + nmodel + 1;

   cl::Event event; 

   // Buffer (memory on the device) of training points (input, model and obs)
   data.buffer_inputs = cl::Buffer( data.context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * ncol * sizeof( float ) );

   float* inputs = (float*) data.queue.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_WRITE, 0, data.nlin * ncol * sizeof( float ), NULL, &event );

   if( data.strategy == "PP" ) 
   {
      for( int i = 0; i < data.nlin; i++ )
      {
         for( int j = 0; j < ninput; j++ )
         {
            inputs[i * ncol + j] = input[i][j];
         }
         for( int j = 0; j < nmodel; j++ )
         {
            inputs[i * ncol + (j + ninput)] = model[i][j];
         }
         inputs[i * ncol + (nmodel + ninput)] = obs[i];
      }
   }
   else
   {
      // Transposed version for coalesced access on the GPU
      /*
      TRANSPOSITION (for coalesced access on the GPU)

                                 Transformed and linearized data points
                                 +----------++----------+   +----------+
                                 | 1 2     q|| 1 2     q|   | 1 2     q|
           +-------------------> |X X ... X ||X X ... X |...|X X ... X |
           |                     | 1 1     1|| 2 2     2|   | p p     p|
           |                     +----------++----------+   +----------+
           |                                ^             ^
           |    ____________________________|             |
           |   |       ___________________________________|
           |   |      |
         +--++--+   +--+
         | 1|| 1|   | 1|
         |X ||X |...|X |
         | 1|| 2|   | p|
         |  ||  |   |  |
         | 2|| 2|   | 2|
         |X ||X |...|X |
         | 1|| 2|   | p|
         |. ||. |   |. |
         |. ||. |   |. |
         |. ||. |   |. |
         | q|| q|   | q|
         |X ||X |...|X |
         | 1|| 2|   | p|
         +--++--+   +--+
      Original data points

      */
      if( data.strategy == "FP" || data.strategy == "PPCU" ) 
      {
         for( int i = 0; i < data.nlin; i++ )
         {
            for( int j = 0; j < ninput; j++ )
            {
               inputs[j * data.nlin + i] = input[i][j];
            }
            for( int j = 0; j < nmodel; j++ )
            {
               inputs[(j + ninput) * data.nlin + i] = model[i][j];
            }
            inputs[(nmodel + ninput) * data.nlin + i] = obs[i];
         }
      }
      else
      {
         fprintf(stderr, "Valid strategy: PP, FP and PPCU.\n");
      }
   }

   // Unmapping
   data.queue.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

   cl_ulong start, end;
   event.getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   event.getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_overhead += (end - start)/1.0E9;

   //inputs = (float*) data.queue.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_READ, 0, data.nlin * ncol * sizeof( float ) );
   //for( int i = 0; i < data.nlin * ncol; i++ )
   //{
   //   if( i % data.nlin == 0 ) fprintf(stdout,"\n");
   //   fprintf(stdout,"%f ", inputs[i]);
   //}
   //data.queue.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

   // Buffer (memory on the device) of the programs
   data.buffer_phenotype = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * data.population_size * sizeof( Symbol ) );
   data.buffer_ephemeral = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * data.population_size * sizeof( float ) );
   data.buffer_size      = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.population_size * sizeof( int ) );

   if( prediction_mode ) // Buffer (memory on the device) of prediction (one por example)
   { 
      data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * sizeof( float ) );
   }
   else // Buffer (memory on the device) of prediction errors
   {
      if( data.strategy == "FP" ) 
      {
         data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, (data.global_size1/data.local_size1) * data.population_size * sizeof( float ) );

         data.buffer_error = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.population_size * sizeof( float ) );
         data.kernel2.setArg( 0, data.buffer_error );
      }
      else
      {
         if( data.strategy == "PPCU" || data.strategy == "PP" ) // (one por program)
         {
            // The evaluate's kernels WRITE in the vector; while the best_individual's kernel READ the vector
            data.buffer_vector = cl::Buffer( data.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, data.population_size * sizeof( float ) );

            data.kernel2.setArg( 0, data.buffer_vector );
         }
      }
   }

   data.kernel1.setArg( 0, data.buffer_phenotype );
   data.kernel1.setArg( 1, data.buffer_ephemeral );
   data.kernel1.setArg( 2, data.buffer_size );
   data.kernel1.setArg( 3, data.buffer_inputs );
   data.kernel1.setArg( 4, data.buffer_vector );
   data.kernel1.setArg( 5, sizeof( float ) * data.local_size1, NULL ); // FIXME: Por que é size(float)?
   data.kernel1.setArg( 6, data.nlin );
   data.kernel1.setArg( 7, ncol );
   data.kernel1.setArg( 8, prediction_mode );

   if ( !prediction_mode )
   {
      const unsigned num_work_groups2 = data.global_size2 / data.local_size2;

      data.buffer_pb = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, num_work_groups2 * sizeof( float ) );
      data.buffer_pi = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, num_work_groups2 * sizeof( int ) );

      data.kernel2.setArg( 1, data.buffer_pb );
      data.kernel2.setArg( 2, data.buffer_pi );
      data.kernel2.setArg( 3, sizeof( float ) * data.local_size2, NULL ); // FIXME: Por que é size(float)?
      data.kernel2.setArg( 4, sizeof( int ) * data.local_size2, NULL );
      data.kernel2.setArg( 5, data.population_size );
   }
}


/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

// -----------------------------------------------------------------------------
int acc_interpret_init( int argc, char** argv, const unsigned size, const unsigned population_size, float** input, float** model, float* obs, int nlin, int prediction_mode )
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-ni", "--number-of-inputs" );
   Opts.Int.Add( "-nm", "--number-of-models" );
   Opts.Int.Add( "-cl-p", "--platform-id", -1, 0 );
   Opts.Int.Add( "-cl-d", "--device-id", -1, 0 );
   Opts.String.Add( "-type" );
   Opts.String.Add( "-strategy" );
   Opts.Process();
   data.strategy = Opts.String.Get("-strategy");
   data.max_size = size;
   data.nlin = nlin;
   data.population_size = population_size;
   data.time_kernel = 0.0f;
   data.time_overhead = 0.0f;

   int type = -1;
   if( Opts.String.Found("-type") )
   {
      if( !strcmp(Opts.String.Get("-type").c_str(),"CPU") ) 
      { 
         type = CL_DEVICE_TYPE_CPU; 
      }
      else 
      {
         if( !strcmp(Opts.String.Get("-type").c_str(),"GPU") ) 
         {
            type = CL_DEVICE_TYPE_GPU; 
         }
         else
         {
            fprintf(stderr, "Not a single compatible device found.\n");
            return 1;
         }
      }
   }

   if ( opencl_init( Opts.Int.Get("-cl-p"), Opts.Int.Get("-cl-d"), type ) )
   {
      fprintf(stderr,"Error in OpenCL initialization phase.\n");
      return 1;
   }

   if ( build_kernel() )
   {
      fprintf(stderr,"Error in build the kernel.\n");
      return 1;
   }

   create_buffers( input, model, obs, Opts.Int.Get("-ni"), Opts.Int.Get("-nm"), prediction_mode );

//   try
//   {
//   }
//   catch( cl::Error& e )
//   {
//      cerr << "ERROR: " << e.what() << " ( " << e.err() << " )\n";
//   }

   return 0;
}

// -----------------------------------------------------------------------------
void acc_interpret( Symbol* phenotype, float* ephemeral, int* size, float* vector, int nInd, void (*send)(Population*), int (*receive)(int*), Population* migrants, int* nImmigrants, int* index, int* best_size, int prediction_mode, int alpha )
{
   //vector<cl::Event> events(4); 
   cl::Event event0; 
   cl::Event event1; 
   cl::Event event2; 
   cl::Event event3; 

   data.queue.enqueueWriteBuffer( data.buffer_phenotype, CL_TRUE, 0, data.max_size * nInd * sizeof( Symbol ), phenotype, NULL, &event1 );
   data.queue.enqueueWriteBuffer( data.buffer_ephemeral, CL_TRUE, 0, data.max_size * nInd * sizeof( float ), ephemeral, NULL, &event2 ); 
   data.queue.enqueueWriteBuffer( data.buffer_size, CL_TRUE, 0, nInd * sizeof( int ), size, NULL, &event3 ); 

   if( data.strategy == "FP" ) 
   {
      data.kernel1.setArg( 9, nInd );
   }

   // ---------- begin kernel execution
   data.queue.enqueueNDRangeKernel( data.kernel1, cl::NDRange(), cl::NDRange( data.global_size1 ), cl::NDRange( data.local_size1 ), NULL, &event0 );
   // ---------- end kernel execution

   // substitui os três enqueueMapBuffer
   // event4 começa depois que o evento0 terminar
   // tem que criar uma segunda fila
   // TODO: data.queuetransfer.enqueuReadBuffer( data.buffer_vector, CL_FALSE, 0, size_which_depends_on_the_strategy, tmp, event0, &event4);

   if ( !prediction_mode )
   {
      if( data.strategy == "PPCU" || data.strategy == "PP" ) 
      {
         // ---------- begin kernel execution
         data.queue.enqueueNDRangeKernel( data.kernel2, cl::NDRange(), cl::NDRange( data.global_size2 ), cl::NDRange( data.local_size2 ) );
         // ---------- end kernel execution
      }
   }

   data.queue.flush();
   util::Timer t_kernel;


   send( migrants );
   *nImmigrants = receive( migrants->genome );

   // Wait until the kernel has finished
   //t_kernel.reset();
   data.queue.finish();
   cerr << t_kernel.elapsed() << endl;



   cl_ulong start, end;
   event0.getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   event0.getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_kernel += (end - start)/1.0E9;

   event1.getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   event1.getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_overhead += (end - start)/1.0E9;
   event2.getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   event2.getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_overhead += (end - start)/1.0E9;
   event3.getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   event3.getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_overhead += (end - start)/1.0E9;


   // TODO: data.queuetransfer.finish();
   float *tmp;
   if ( prediction_mode )
   {
      // TODO: vector = tmp (?) substituiu as duas linhas de baixo
      tmp = (float*) data.queue.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, data.nlin * sizeof( float ) );
      for( int i = 0; i < data.nlin; i++ ) {vector[i] = tmp[i];}
   }
   else
   {
      if( data.strategy == "FP" ) 
      {
         // -----------------------------------------------------------------------
         /*
            Each kernel execution will put in data.buffer_vector the partial errors of each individual:

            |  0 |  0 |     |  0    ||   1 |  1 |     |  1   |     |  ind-1 |  ind-1 |     |  ind-1 |
            | E  | E  | ... | E     ||  E  | E  | ... | E    | ... | E      | E      | ... | E      |        
            |  0 |  1 |     |  n-1  ||   0 |  1 |     |  n-1 |     |  0     |  1     |     |  n-1   |    

            where 'ind-1' is the index of the last individual, and 'n-1' is the index of the
            last 'partial error', that is, 'n-1' is the index of the last work-group (gr_id).
          */

         const unsigned num_work_groups = data.global_size1 / data.local_size1;

         // The line below maps the contents of 'data_buffer_vector' into 'tmp'.
         // essa linha some
         tmp = (float*) data.queue.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, num_work_groups * nInd * sizeof( float ) );
         
         float sum;

         // Reduction on host!
         for( int i = 0; i < nInd; i++)
         {
            sum = 0.0;
            for( int gr_id = 0; gr_id < num_work_groups; gr_id++ ) 
               sum += tmp[i * num_work_groups + gr_id];

            if( isnan( sum ) || isinf( sum ) ) 
               vector[i] = std::numeric_limits<float>::max();
            else 
               vector[i] = sum/data.nlin + alpha * size[i];
         }

         data.queue.enqueueWriteBuffer( data.buffer_error, CL_TRUE, 0, data.population_size * sizeof( float ), vector );

         // ---------- begin kernel execution
         data.queue.enqueueNDRangeKernel( data.kernel2, cl::NDRange(), cl::NDRange( data.global_size2 ), cl::NDRange( data.local_size2 ) );
         // ---------- end kernel execution

         // Wait until the kernel has finished
         data.queue.finish();
      }
      else
      {
         if( data.strategy == "PPCU" || data.strategy == "PP" ) 
         {
            tmp = (float*) data.queue.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, nInd * sizeof( float ) );
            for( int i = 0; i < nInd; i++ ) { vector[i] = tmp[i] + alpha * size[i]; }
            // substitui as duas linhas de cima
            //for( int i = 0; i < nInd; i++ ) { tmp[i] += alpha * size[i]; }
            //vector = tmp;
         }
      }

      const unsigned num_work_groups2 = data.global_size2 / data.local_size2;

      // The line below maps the contents of 'data_buffer_pb' and 'data_buffer_pi' into 'PB' and 'PI', respectively.
      float* PB = (float*) data.queue.enqueueMapBuffer( data.buffer_pb, CL_TRUE, CL_MAP_READ, 0, num_work_groups2 * sizeof( float ) );
      int* PI = (int*) data.queue.enqueueMapBuffer( data.buffer_pi, CL_TRUE, CL_MAP_READ, 0, num_work_groups2 * sizeof( int ) );

      if( *best_size > num_work_groups2 ) { *best_size = num_work_groups2; }
      
      std::priority_queue<std::pair<float, int> > q;
      for( int i = 0; i < num_work_groups2; ++i ) 
      {
         //fprintf(stdout,"%f ", PB[i]);
         q.push( std::pair<float, int>(PB[i]*(-1), i) );
      }
      //fprintf(stdout,"\n============================\n");
      for( int i = 0; i < *best_size; ++i ) 
      {
         int idx = q.top().second;
         //fprintf(stdout,"%f ", PB[idx]);
         index[i] = PI[idx];
         q.pop();
      }
      //fprintf(stdout,"\n============================\n");

      data.queue.enqueueUnmapMemObject( data.buffer_pb, PB ); 
      data.queue.enqueueUnmapMemObject( data.buffer_pi, PI );
   }
   //essa linha some
   data.queue.enqueueUnmapMemObject( data.buffer_vector, tmp ); 
}

// -----------------------------------------------------------------------------
void acc_print_time()
{
   printf("time_overhead: %lf, time_kernel: %lf, ", data.time_overhead, data.time_kernel);
}
