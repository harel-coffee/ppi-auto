// ------------------------------------------------------------------------
#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <cmath> 
#include <limits>
#include <string>   
#include <vector>
#include <utility>
#include <iostream> 
#include <fstream> 
#include "accelerator.h"
#include "../server/server.h"
#include "../util/CmdLineParser.h"
#include "../util/Util.h"
#include <Poco/Path.h>

/* Macros to stringify an expansion of a macro/definition */
#define xstr(a) str(a)
#define str(a) #a

using namespace std;

std::string getAbsoluteDirectory(std::string filepath) {
   Poco::Path p(filepath);
   string filename = p.getFileName();
   p.makeAbsolute();
   string absolute_path = p.toString();

   /* The absolute_path consists of: directory + filename; so, to get
      only the directory, it suffices to do: absolute_path - filename */
   return std::string(absolute_path, 0, absolute_path.length() - filename.length());
}

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

namespace { static struct t_data { int max_size; int max_arity; int nlin; int population_size; unsigned local_size1; unsigned global_size1; unsigned local_size2; unsigned global_size2; std::string strategy; cl::Device device; cl::Context context; cl::Kernel kernel1; cl::Kernel kernel2; cl::CommandQueue queue; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_size; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; cl::Buffer buffer_error; cl::Buffer buffer_pb; cl::Buffer buffer_pi; double gpops_gen_kernel; double gpops_gen_communication; double time_gen_kernel1; double time_gen_kernel2; double time_gen_communication_send; double time_gen_communication_receive; double time_total_kernel1; double time_total_kernel2; double time_communication_dataset; double time_total_communication_send; double time_total_communication_receive; double time_total_communication1; std::string executable_directory; bool verbose; } data; };

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTION *********************** **/
/** ****************************************************************** **/

// -----------------------------------------------------------------------------
int opencl_init( int platform_id, int device_id, cl_device_type type )
{
   vector<cl::Platform> platforms;

   /* Iterate over the available platforms and pick the list of compatible devices
      from the first platform that offers the device type we are querying. */
   cl::Platform::get( &platforms );

   vector<cl::Device> devices;

   /*

   Possible options:

   INV = CL_INVALID_DEVICE_TYPE

        type  device  platform

   1.   INV     X        X
   2.   INV    -1        X     --> device = 0
   3.   INV     X       -1     --> platform = 0
   4.    X     -1        X     
   5.    X     -1       -1 
   6.   INV    -1       -1     --> platform = last_platform; device = 0 
   7.    X      X        X     --> reset type = CL_INVALID_DEVICE_TYPE 

   */

   if( platform_id >= 0 && device_id >= 0 ) // option 7
   {
      type = CL_INVALID_DEVICE_TYPE; 
   }

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

      if( type != CL_INVALID_DEVICE_TYPE && device_id < 0 ) // options 4 e 5
      {
         for ( int n = 0; n < devices.size(); n++ )
         {
           /*
        
           Possible options:
        
           int   bits      type

            1    0001  --> default
            2    0010  --> CPU
            4    0100  --> GPU
            8    1000  --> ACC

            3    0011  --> CPU+default  --> 0010
                                                 | (or)
                                            0001
                                           ------
                                            0011

            5   0101  --> GPU+default  --> 0100
                                                 | (or)
                                           0001
                                          ------
                                           0101


            Examples:

            if ( 0011 & 0010 ) return 0011 (CPU+default)
                                           & (and)
                                      0010 (type = CPU)
                                     ------
                                      0010 --> CPU

            if ( 0011 & 0100 ) return 0011 (CPU+default)
                                           & (and)
                                      0100 (type = GPU)
                                     ------
                                      0000 
           */

            if ( devices[n].getInfo<CL_DEVICE_TYPE>() & type ) 
            {
               leave = true;
               data.device = devices[n];
               break;
            }
         }
         if( leave ) break;
      }
   }

   if( type != CL_INVALID_DEVICE_TYPE && !leave )
   {
      fprintf(stderr, "Not a single compatible type found.\n");
      return 1;
   }

   data.context = cl::Context( devices );

   data.queue = cl::CommandQueue( data.context, data.device
#ifdef PROFILING
   , CL_QUEUE_PROFILING_ENABLE 
#endif
   );


   return 0;
}

// -----------------------------------------------------------------------------
int build_kernel( int maxlocalsize, int pep_mode, int prediction_mode )
{
   unsigned max_stack_size;
   if( pep_mode && prediction_mode ) 
   {
      max_stack_size = data.max_size;
   }
   else
   {
      /*
         In practice we don't need 'max_stack_size' to be equal to 'data.max_size',
         we can lower this and then save memory on the device. The formula below gives us
         the upper bound for stack use:

         /                           |            MTS             | \
         stack size = max|  1, MTS - |  ------------------------  |  |
         |                           |       /               \    |  |
         |                           |   min| arity max, MTS  |   |  |
         \                           |_      \               /   _| /

         where MTS is data.max_size().
       */
      max_stack_size = std::max( 1U, (unsigned)( data.max_size -
               std::floor(data.max_size /
                  (float) std::min( data.max_arity, data.max_size ) ) ) );
   }

   /* Use a prefix (the given label) to minimize the likelihood of collisions
    * when two or more problems are built into the same build directory.
      The directory of the executable binary is prefixed here so that OpenCL
      will find the kernels regardless of user's current directory. */
   std::string opencl_file = data.executable_directory + std::string(std::string(xstr(LABEL)) + "-accelerator.cl");
   ifstream file(opencl_file.c_str());
   string kernel_str( istreambuf_iterator<char>(file), ( istreambuf_iterator<char>()) );

   string program_str = 
      "#define MAX_STACK_SIZE " + util::ToString( max_stack_size ) + "\n" +
      "#define MAX_PHENOTYPE_SIZE " + util::ToString( data.max_size ) + "\n" +
      kernel_str;
   //cerr << program_str << endl;

   cl::Program::Sources source( 1, make_pair( program_str.c_str(), program_str.size() ) );
   
   cl::Program program( data.context, source );

   vector<cl::Device> device; device.push_back( data.device );
   try {
      /* Pass the following definition to the OpenCL compiler:
            -I<executable_absolute_directory>/INCLUDE_RELATIVE_DIR
         where <executable_absolute_directory> is the current directory
         of the executable binary and INCLUDE_RELATIVE_DIR is a relative
         subdirectory where the assembled source files will be put by CMake. */
      std::string flags = std::string(" -I" + data.executable_directory + std::string(xstr(INCLUDE_RELATIVE_DIR)));
      program.build( device, flags.c_str() );
   }
   catch( cl::Error& e )
   {
      cerr << "Build Log:\t " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(data.device) << std::endl;
      throw;
   }

   unsigned max_cu = data.device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
   unsigned max_local_size;
   if( maxlocalsize > 0 )
   {
      max_local_size = maxlocalsize;
   }
   else 
   {
      max_local_size = fmin( data.device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(), data.device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0] );
   
      //It is necessary to respect the local memory size. 
      //Depending on the maximum local size, there will not be enough space to allocate the local variables. 
      //The local size depends on the maximum local size. 
      //The division by 8: 2 local vectors in best_individual kernel * 4 bytes
      max_local_size = fmin( max_local_size, data.device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 8 );
   }

   if( data.strategy == "PP" )  // Population-parallel
   {
      data.local_size1 = fmin( max_local_size, (unsigned) ceil( data.population_size/(float) max_cu ) );
      data.global_size1 = (unsigned) ( ceil( data.population_size/(float) data.local_size1 ) * data.local_size1 );
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
            //data.local_size1 = 128;
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

   if (data.verbose) {
      std::cout << "\nDevice: " << data.device.getInfo<CL_DEVICE_NAME>() << ", Compute units: " << max_cu << ", Max local size: " << max_local_size << std::endl;
      std::cout << "Local size: " << data.local_size1 << ", Global size: " << data.global_size1 << ", Work groups: " << data.global_size1/data.local_size1 << std::endl;
   }

   if( !pep_mode )
   {
      // Evenly distribute the workload among the compute units (but avoiding local size
      // being more than the maximum allowed).
      data.local_size2 = fmin( max_local_size, (unsigned) ceil( data.population_size/(float) max_cu ) );
      // It is better to have global size divisible by local size
      data.global_size2 = (unsigned) ( ceil( data.population_size/(float) data.local_size2 ) * data.local_size2 );
      data.kernel2 = cl::Kernel( program, "best_individual" );
      if (data.verbose) {
         std::cout << "Local size: " << data.local_size2 << ", Global size: " << data.global_size2 << ", Work groups: " << data.global_size2/data.local_size2 << std::endl;
      }
   }


   return 0;
}

// -----------------------------------------------------------------------------

void create_buffers( float** input, int ncol, int pep_mode, int prediction_mode )
{
#ifdef PROFILING
   std::vector<cl::Event> events(2); 
#endif

   // Buffer (memory on the device) of training points (input, model and obs)
   data.buffer_inputs = cl::Buffer( data.context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * ncol * sizeof( float ) );

   float* inputs = (float*) data.queue.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_WRITE, 0, data.nlin * ncol * sizeof( float ), NULL
#ifdef PROFILING
   , &events[0]
#endif
   );

   if( data.strategy == "PP" ) 
   {
      for( int i = 0; i < data.nlin; i++ )
      {
         for( int j = 0; j < ncol; j++ )
         {
            inputs[i * ncol + j] = input[i][j];
         }
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
            for( int j = 0; j < ncol; j++ )
            {
               inputs[j * data.nlin + i] = input[i][j];
            }
         }
      }
      else
      {
         fprintf(stderr, "Valid strategy: PP, FP and PPCU.\n");
      }
   }



   // Unmapping
   data.queue.enqueueUnmapMemObject( data.buffer_inputs, inputs, NULL
#ifdef PROFILING
   , &events[1]
#endif
   );

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

   if( pep_mode && prediction_mode ) // Buffer (memory on the device) of prediction (one por example)
   { 
      data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * sizeof( float ) );
   }
   else // Buffer (memory on the device) of prediction errors
   {
      if( data.strategy == "FP" ) 
      {
         data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, (data.global_size1/data.local_size1) * data.population_size * sizeof( float ) );

         data.buffer_error = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.population_size * sizeof( float ) );
         if( !pep_mode) {data.kernel2.setArg( 0, data.buffer_error );}
      }
      else
      {
         if( data.strategy == "PPCU" || data.strategy == "PP" ) // (one por program)
         {
            // The evaluate's kernels WRITE in the vector; while the best_individual's kernel READ the vector
            data.buffer_vector = cl::Buffer( data.context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, data.population_size * sizeof( float ) );

            if( !pep_mode) {data.kernel2.setArg( 0, data.buffer_vector );}
         }
      }
   }

   data.kernel1.setArg( 0, data.buffer_phenotype );
   data.kernel1.setArg( 1, data.buffer_ephemeral );
   data.kernel1.setArg( 2, data.buffer_size );
   data.kernel1.setArg( 3, data.buffer_inputs );
   data.kernel1.setArg( 4, data.buffer_vector );
   data.kernel1.setArg( 5, data.nlin );
   data.kernel1.setArg( 6, ncol );
   data.kernel1.setArg( 7, prediction_mode );
   if( data.strategy == "PP" ) 
   {
      data.kernel1.setArg( 8, data.population_size );
   }
   else 
   {
      data.kernel1.setArg( 8, sizeof( float ) * data.local_size1, NULL ); // FIXME: Por que é size(float)?
   }


   if ( !pep_mode )
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

#ifdef PROFILING
   {
      std::vector<cl::Event> e(1, events[1]); 
      cl::Event::waitForEvents(e);
   }

   cl_ulong start, end;
   events[0].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[1].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_communication_dataset = (end - start)/1.0E9;
#endif

}


/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

// -----------------------------------------------------------------------------
int acc_interpret_init( int argc, char** argv, const unsigned size, const unsigned max_arity, const unsigned population_size, float** input, int nlin, int ncol, int pep_mode, int prediction_mode )
{
   CmdLine::Parser Opts( argc, argv );

   // Get the executable directory so we can find the kernels later, regardless
   // of the current directory (user directory).
   data.executable_directory = getAbsoluteDirectory(argv[0]);

   Opts.Bool.Add( "-v", "--verbose" );
   Opts.Int.Add( "-cl-p", "--cl-platform-id", -1, 0 );
   Opts.Int.Add( "-cl-d", "--cl-device-id", -1, 0 );
   Opts.Int.Add( "-cl-mls", "--cl-max-local-size", -1 );
   Opts.String.Add( "-type" );
   Opts.String.Add( "-strategy", "", "PPCU", "FP", "PP", "PPCU", NULL );
   Opts.Process();
   data.verbose = Opts.Bool.Get("-v");
   data.strategy = Opts.String.Get("-strategy");
   data.max_size = size;
   data.max_arity = max_arity;
   data.nlin = nlin;
   data.population_size = population_size;
#ifdef PROFILING
   data.time_total_kernel1  = 0.0;
   data.time_total_kernel2  = 0.0;
   data.time_total_communication_send    = 0.0;
   data.time_total_communication_receive = 0.0;
   data.time_total_communication1        = 0.0;
#endif

   cl_device_type type = CL_INVALID_DEVICE_TYPE;
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

   if( ncol < 0 )
   {
      fprintf(stderr, "Missing number of columns of dataset.\n");
      return 1;
   }

   if ( opencl_init( Opts.Int.Get("-cl-p"), Opts.Int.Get("-cl-d"), type ) )
   {
      fprintf(stderr,"Error in OpenCL initialization phase.\n");
      return 1;
   }

   if ( build_kernel( Opts.Int.Get("-cl-mls"), pep_mode, prediction_mode ) )
   {
      fprintf(stderr,"Error in build the kernel.\n");
      return 1;
   }

   create_buffers( input, ncol, pep_mode, prediction_mode );

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
void acc_interpret( Symbol* phenotype, float* ephemeral, int* size,
#ifdef PROFILING
unsigned long sum_size_gen,
#endif
float* vector, int nInd, void (*send)(Population*), int (*receive)(GENOME_TYPE*), Population* migrants, int* nImmigrants, int* index, int* best_size, int pep_mode, int prediction_mode, float alpha )
{
#ifdef PROFILING
   std::vector<cl::Event> events(12); 

   data.time_gen_kernel1  = 0.0;
   data.time_gen_kernel2  = 0.0;
   data.time_gen_communication_send    = 0.0;
   data.time_gen_communication_receive = 0.0;
#endif

   data.queue.enqueueWriteBuffer( data.buffer_phenotype, CL_TRUE, 0, data.max_size * nInd * sizeof( Symbol ), phenotype, NULL
#ifdef PROFILING
   , &events[0]
#endif
   );

   data.queue.enqueueWriteBuffer( data.buffer_ephemeral, CL_TRUE, 0, data.max_size * nInd * sizeof( float ), ephemeral, NULL
#ifdef PROFILING
   , &events[1]
#endif
   );

   data.queue.enqueueWriteBuffer( data.buffer_size, CL_TRUE, 0, nInd * sizeof( int ), size, NULL
#ifdef PROFILING
   , &events[2]
#endif
   );

   if( data.strategy == "FP" ) 
   {
      data.kernel1.setArg( 9, nInd );
   }

   //std::cerr << "Global size: " << data.global_size1 << " Local size: " << data.local_size1 << " Work group: " << data.global_size1/data.local_size1 << std::endl;
   try {
      // ---------- begin kernel execution
      data.queue.enqueueNDRangeKernel( data.kernel1, cl::NDRange(), cl::NDRange( data.global_size1 ), cl::NDRange( data.local_size1 ), NULL
#ifdef PROFILING
      , &events[3]
#endif
      );
      // ---------- end kernel execution
   }
   catch( cl::Error& e )
   {
      cerr << "\nERROR(kernel1): " << e.what() << " ( " << e.err() << " )\n";
      throw;
   }

   // substitui os três enqueueMapBuffer
   // event4 começa depois que o evento0 terminar
   // tem que criar uma segunda fila
   // TODO: data.queuetransfer.enqueuReadBuffer( data.buffer_vector, CL_FALSE, 0, size_which_depends_on_the_strategy, tmp, event0, &event4);

   if ( !pep_mode )
   {
      if( data.strategy == "PPCU" || data.strategy == "PP" ) 
      {
         //std::cerr << "Global size: " << data.global_size2 << " Local size: " << data.local_size2 << " Work group: " << data.global_size2/data.local_size2 << std::endl;
         try 
         {
            // ---------- begin kernel execution
            data.queue.enqueueNDRangeKernel( data.kernel2, cl::NDRange(), cl::NDRange( data.global_size2 ), cl::NDRange( data.local_size2 ), NULL
#ifdef PROFILING
            , &events[4]
#endif
            );
            // ---------- end kernel execution
         }
         catch( cl::Error& e )
         {
            cerr << "\nERROR(kernel2): " << e.what() << " ( " << e.err() << " )\n";
            throw;
         }
      }
      data.queue.flush();
   }

   if( !pep_mode )
   {
      send( migrants );
      *nImmigrants = receive( migrants->genome );
   }

   // Wait until the kernel has finished
   data.queue.finish();


   // TODO: data.queuetransfer.finish();
   float *tmp;
   if ( pep_mode && prediction_mode )
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
         tmp = (float*) data.queue.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, num_work_groups * nInd * sizeof( float ), NULL
#ifdef PROFILING
         , &events[5]
#endif
         );

         // Reduction on host!
#ifdef PROFILING
         util::Timer t_time;
#endif
         for( int i = 0; i < nInd; i++)
         {
            float sum = 0.0f;
            for( int gr_id = 0; gr_id < num_work_groups; gr_id++ ) 
            {
               float error = tmp[i * num_work_groups + gr_id];

               // Avoid further calculations if the current one has overflown the float
               // (i.e., it is inf or NaN).
               if( isinf(error) || isnan(error) ) { sum = std::numeric_limits<float>::max(); break; }

#ifdef REDUCEMAX
               sum = (error*data.nlin > sum) ? error*data.nlin : sum;
#else
               sum += error;
#endif
            }

            if( isnan( sum ) || isinf( sum ) )
               vector[i] = std::numeric_limits<float>::max();
            else
               vector[i] = sum/data.nlin + alpha * size[i];
         }
#ifdef PROFILING
         data.time_gen_kernel1   += t_time.elapsed();
         data.time_total_kernel1 += t_time.elapsed();
#endif

         data.queue.enqueueWriteBuffer( data.buffer_error, CL_TRUE, 0, data.population_size * sizeof( float ), vector, NULL
#ifdef PROFILING
         , &events[6]
#endif
         );

         if( !pep_mode )
         {
            //std::cerr << "Global size: " << data.global_size2 << " Local size: " << data.local_size2 << " Work group: " << data.global_size2/data.local_size2 << std::endl;
            try
            {
               // ---------- begin kernel execution
               data.queue.enqueueNDRangeKernel( data.kernel2, cl::NDRange(), cl::NDRange( data.global_size2 ), cl::NDRange( data.local_size2 ), NULL
#ifdef PROFILING
               , &events[4]
#endif
               );
               // ---------- end kernel execution
            }
            catch( cl::Error& e )
            {
               cerr << "\nERROR(kernel2): " << e.what() << " ( " << e.err() << " )\n";
               throw;
            }
    
            // Wait until the kernel has finished
         }
         data.queue.finish();
      }
      else
      {
         if( data.strategy == "PPCU" || data.strategy == "PP" ) 
         {
            tmp = (float*) data.queue.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, nInd * sizeof( float ), NULL
#ifdef PROFILING
         , &events[5]
#endif
         );
            for( int i = 0; i < nInd; i++ ) { vector[i] = tmp[i] + alpha * size[i]; }

            //printf("%f\n", vector[0]);
            // substitui as duas linhas de cima
            //for( int i = 0; i < nInd; i++ ) { tmp[i] += alpha * size[i]; }
            //vector = tmp;
         }
      }

      if( !pep_mode )
      {
         const unsigned num_work_groups2 = data.global_size2 / data.local_size2;
   
         // The line below maps the contents of 'data_buffer_pb' and 'data_buffer_pi' into 'PB' and 'PI', respectively.
         float* PB = (float*) data.queue.enqueueMapBuffer( data.buffer_pb, CL_TRUE, CL_MAP_READ, 0, num_work_groups2 * sizeof( float ), NULL
#ifdef PROFILING
         , &events[7]
#endif
         );
         int* PI = (int*) data.queue.enqueueMapBuffer( data.buffer_pi, CL_TRUE, CL_MAP_READ, 0, num_work_groups2 * sizeof( int ), NULL
#ifdef PROFILING
         , &events[8]
#endif
         );
   
         if( *best_size > num_work_groups2 ) { *best_size = num_work_groups2; }
         
#ifdef PROFILING
         util::Timer t_time;
#endif
         /* Reduction on host of the per-group partial reductions performed by kernel2. */
         util::PickNBest(*best_size, index, num_work_groups2, PB, PI);
#ifdef PROFILING
         data.time_gen_kernel2   += t_time.elapsed();
         data.time_total_kernel2 += t_time.elapsed();
#endif

         data.queue.enqueueUnmapMemObject( data.buffer_pb, PB, NULL 
#ifdef PROFILING
         , &events[9]
#endif
         );
         data.queue.enqueueUnmapMemObject( data.buffer_pi, PI, NULL
#ifdef PROFILING
         , &events[10]
#endif
         );
      }
   }

   //essa linha some
   data.queue.enqueueUnmapMemObject( data.buffer_vector, tmp, NULL
#ifdef PROFILING
   , &events[11]
#endif
   );

#ifdef PROFILING
   cl_ulong start, end;
   events[0].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[0].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_send    = (end - start)/1.0E9;
   data.time_total_communication_send += (end - start)/1.0E9;

   events[1].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[1].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_send   += (end - start)/1.0E9; 
   data.time_total_communication_send += (end - start)/1.0E9; 

   events[2].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[2].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_send   += (end - start)/1.0E9; 
   data.time_total_communication_send += (end - start)/1.0E9; 
   
   events[3].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[3].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_kernel1   += (end - start)/1.0E9;
   data.time_total_kernel1 += (end - start)/1.0E9;

   if( !pep_mode )
   {
      events[4].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
      events[4].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
      data.time_gen_kernel2   += (end - start)/1.0E9;
      data.time_total_kernel2 += (end - start)/1.0E9;
   }

   {
      std::vector<cl::Event> e(1, events[11]); 
      cl::Event::waitForEvents(e);
   }
   events[5].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[11].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_receive   += (end - start)/1.0E9;
   data.time_total_communication_receive += (end - start)/1.0E9;

   data.time_total_communication1 += data.time_gen_communication_send + data.time_gen_communication_receive;

   data.gpops_gen_kernel = (sum_size_gen * data.nlin) / data.time_gen_kernel1;
   data.gpops_gen_communication = (sum_size_gen * data.nlin) / (data.time_gen_kernel1 + data.time_gen_communication_send + data.time_gen_communication_receive);

   if( !pep_mode && data.strategy == "FP" ) 
   {
      events[6].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
      events[6].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
      data.time_gen_communication_send   += (end - start)/1.0E9;
      data.time_total_communication_send += (end - start)/1.0E9;
   }

   {
      std::vector<cl::Event> e(1, events[9]); 
      cl::Event::waitForEvents(e);
   }
   events[7].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[9].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_receive   += (end - start)/1.0E9;
   data.time_total_communication_receive += (end - start)/1.0E9;

   {
      std::vector<cl::Event> e(1, events[10]); 
      cl::Event::waitForEvents(e);
   }
   events[8].getProfilingInfo( CL_PROFILING_COMMAND_START, &start );
   events[10].getProfilingInfo( CL_PROFILING_COMMAND_END, &end );
   data.time_gen_communication_receive   += (end - start)/1.0E9;
   data.time_total_communication_receive += (end - start)/1.0E9;
#endif
}

// -----------------------------------------------------------------------------
#ifdef PROFILING
void acc_print_time( bool total, unsigned long long sum_size )
{
   double time_kernel1 = total ? data.time_total_kernel1 : data.time_gen_kernel1;
   double time_kernel2 = total ? data.time_total_kernel2 : data.time_gen_kernel2;
   double time_communication_send = total ? data.time_total_communication_send : data.time_gen_communication_send;
   double time_communication_receive = total ? data.time_total_communication_receive : data.time_gen_communication_receive;
   double gpops_kernel = total ? (sum_size * data.nlin) / data.time_total_kernel1 : data.gpops_gen_kernel;
   double gpops_communication = total ? (sum_size * data.nlin) / (data.time_total_kernel1 + data.time_total_communication1) : data.gpops_gen_communication;

   printf(", time_kernel[1]: %lf, time_kernel[2]: %lf, time_communication_dataset: %lf, time_communication_send: %lf, time_communication_receive: %lf", time_kernel1, time_kernel2, data.time_communication_dataset, time_communication_send, time_communication_receive);
   printf("; gpops_kernel: %lf, gpops_kernel_communication: %lf", gpops_kernel, gpops_communication);
}
#endif
