// ------------------------------------------------------------------------
// Habilita disparar exceções C++
#define __CL_ENABLE_EXCEPTIONS

// Cabeçalho OpenCL para C++
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
#include "../util/CmdLineParser.h"

using namespace std;

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int max_size; int nlin; unsigned local_size; unsigned global_size; cl::Context context; cl::Kernel kernel; cl::CommandQueue fila; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_size; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; } data;

/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

int acc_interpret_init( int argc, char** argv, const unsigned size, const unsigned population_size, float** input, float** model, float* obs, int nlin, int prediction_mode )
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.Int.Add( "-platform-id", "", -1, 0 );
   Opts.Int.Add( "-device-id", "", -1, 0 );
   Opts.String.Add( "-type" );
   Opts.Process();
   int platform_id = Opts.Int.Get("-platform-id");
   int device_id = Opts.Int.Get("-device-id");
   int ninput = Opts.Int.Get("-ni");
   int nmodel = Opts.Int.Get("-nm");

   if( !Opts.Int.Found("-platform-id") && Opts.Int.Found("-device-id") )
   {
      device_id = Opts.Int.Get("-device-id");
      platform_id = 0;
   }

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

//   try
//   {


      // Descobre as plataformas instaladas no hospedeiro
      vector<cl::Platform> platforms;
      cl::Platform::get( &platforms );

      vector<cl::Device> device(1); 
      int device_type;

      bool leave = false;
      if( platform_id >= (int) platforms.size() )
      {
         fprintf(stderr, "Valid platform range: [0, %d].\n", (int) (platforms.size()-1));
         return 1;
      }
      int first_platform = platform_id >= 0 ? platform_id : 0;
      int last_platform  = platform_id >= 0 ? platform_id + 1 : platforms.size();
      for( int m = first_platform; m < last_platform; m++ )
      {
         vector<cl::Device> devices;

         platforms[m].getDevices( CL_DEVICE_TYPE_ALL, &devices );

         if( device_id >= (int) devices.size() )
         {
            fprintf(stderr, "Valid device range: [0, %d].\n", (int) (devices.size()-1));
            return 1;
         }
         int first_device = device_id >= 0 ? device_id : 0;
         device[0] = devices[first_device];
         device_type = device[0].getInfo<CL_DEVICE_TYPE>();

         if( type >= 0 && device_id < 0 )
         {
            for ( int n = 0; n < devices.size(); n++ )
            {
               if ( devices[n].getInfo<CL_DEVICE_TYPE>() == type ) 
               {
                  leave = true;
                  device[0] = devices[n];
                  device_type = device[0].getInfo<CL_DEVICE_TYPE>();
                  break;
               }
            }
            if( leave ) break;
         }
      }
      if( type >= 0 && !leave )
      {
         fprintf(stderr, "Not a single compatible device found.\n");
         return 1;
      }


      data.max_size = size;
      data.nlin = nlin;

      // Criar o contexto
      data.context = cl::Context( device );

      // Criar a fila de comandos para um dispositivo (aqui só o primeiro)
      data.fila = cl::CommandQueue( data.context, device[0], CL_QUEUE_PROFILING_ENABLE );

      int ncol = ninput + nmodel + 1;

      data.buffer_inputs = cl::Buffer( data.context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, nlin * ncol * sizeof( float ) );

      float* inputs = (float*) data.fila.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_WRITE, 0, nlin * ncol * sizeof( float ) );

//      if ( device_type == CL_DEVICE_TYPE_CPU ) 
//      {
//         fprintf(stdout,"Lendo CPU...\n");
//         for( int i = 0; i < nlin; i++ )
//         {
//            for( int j = 0; j < ninput; j++ )
//            {
//               inputs[i * ncol + j] = input[i][j];
//            }
//            for( int j = 0; j < nmodel; j++ )
//            {
//               inputs[i * ncol + (j + ninput)] = model[i][j];
//            }
//            inputs[i * ncol + (nmodel + ninput)] = obs[i];
//         }
//
//         for( int j = 0; j < (ninput+nmodel+1); j++ )
//         {
//            fprintf(stdout,"%f ",inputs[289*ncol+j]);
//         }
//         fprintf(stdout,"\n");

//      }
//      else
//      {
//         if ( device_type == CL_DEVICE_TYPE_GPU ) 
//         {
//            fprintf(stdout,"Lendo GPU...\n");
            for( int i = 0; i < nlin; i++ )
            {
               for( int j = 0; j < ninput; j++ )
               {
                  inputs[j * nlin + i] = input[i][j];
               }
               for( int j = 0; j < nmodel; j++ )
               {
                  inputs[(j + ninput) * nlin + i] = model[i][j];
               }
               inputs[(nmodel + ninput) * nlin + i] = obs[i];
            }

//            for( int j = 0; j < (ninput+nmodel+1); j++ )
//            {
//               fprintf(stdout,"%f ",inputs[j*nlin+500]);
//            }
//            fprintf(stdout,"\n");

//         }
//         else
//         {
//            fprintf(stderr,"Problem to get device type (%d).\n", device_type);
//         }
//      }

      // Unmapping
      data.fila.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

//      inputs = (float*) data.fila.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_READ, 0, nlin * ncol * sizeof( float ) );
//      for( int i = 0; i < nlin*ncol; i++ )
//      {
//         if( i % nlin == 0 ) fprintf(stdout,"\n");
//         fprintf(stdout,"%f ", inputs[i]);
//      }
//      // Unmapping
//      data.fila.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

      // Carregar o programa, compilá-lo e gerar o kernel
      ifstream file("accelerator.cl");
      string kernel_str( istreambuf_iterator<char>(file),
            ( istreambuf_iterator<char>()) );
      //cerr << "kernel: [" << kernel_str << "]";
      cl::Program::Sources fonte( 1, make_pair( kernel_str.c_str(), kernel_str.size() ) );

      cl::Program programa( data.context, fonte );

      // Compila para todos os dispositivos associados a 'programa' através do
      // 'contexto': vector<cl::Device>() é um vetor nulo
      char buildOptions[60];
      sprintf( buildOptions, "-DMAX_PHENOTYPE_SIZE=%u -I.", data.max_size );  
      try {
         programa.build( device, buildOptions );
      }
      catch( cl::Error& e )
      {
         cerr << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device[0]) << std::endl;

         throw;
      }

      unsigned max_cu = device[0].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
      unsigned max_local_size = fmin( device[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(), device[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0] );

      if( device_type == CL_DEVICE_TYPE_CPU ) 
      {
         data.local_size = 1;
         data.global_size = nlin;
      }
      else
      {
         if( device_type == CL_DEVICE_TYPE_GPU ) 
         {
            if( !strcmp(Opts.String.Get("-strategy").c_str(),"FP") ) 
            {
               data.local_size = fmin( max_local_size, (unsigned) ceil( nlin/(float) max_cu ) );
               data.global_size = (unsigned) ( ceil( nlin/(float) data.local_size ) * data.local_size );
               data.kernel = cl::Kernel( programa, "evaluate_fp" );
            }
            else
            {
               if( !strcmp(Opts.String.Get("-strategy").c_str(),"PPCU") ) 
               {
                  if( nlin < max_local_size )
                  {
                     data.local_size = nlin;
                  }
                  else
                  {
                     data.local_size = max_local_size;
                  }
                  data.global_size = population_size * data.local_size;
                  data.kernel = cl::Kernel( programa, "evaluate_ppcu" );
               }
               else
               {
                  fprintf(stderr, "Not a compatible strategy found.\n");
                  return 1;
               }
            }
         }
      }

      std::cout << "\nDevice: " << device[0].getInfo<CL_DEVICE_NAME>() << ", Compute units: " << max_cu << ", Max local size: " << max_local_size << std::endl;
      std::cout << "\nLocal size: " << data.local_size << ", Global size: " << data.global_size << ", Work groups: " << data.global_size/data.local_size << "\n" << std::endl;


      // 2) Preparação da memória dos dispositivos (leitura e escrita)
      data.buffer_phenotype = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * population_size * sizeof( Symbol ) );
      data.buffer_ephemeral = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * population_size * sizeof( float ) );
      data.buffer_size      = cl::Buffer( data.context, CL_MEM_READ_ONLY, population_size * sizeof( int ) );
      if( prediction_mode ) 
      { 
         data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * sizeof( float ) );
      }
      else 
      {
         data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, (data.global_size/data.local_size) * population_size * sizeof( float ) );
      } 


      // Execução do kernel: definição dos argumentos e trabalho/particionamento
      data.kernel.setArg( 0, data.buffer_phenotype );
      data.kernel.setArg( 1, data.buffer_ephemeral );
      data.kernel.setArg( 2, data.buffer_size );
      data.kernel.setArg( 3, data.buffer_inputs );
      data.kernel.setArg( 4, data.buffer_vector );
      data.kernel.setArg( 5, sizeof( float ) * data.local_size, NULL );
      data.kernel.setArg( 6, nlin );
      data.kernel.setArg( 7, ncol );
      data.kernel.setArg( 8, prediction_mode );
//   }
//   catch( cl::Error& e )
//   {
//      cerr << "ERROR: " << e.what() << " ( " << e.err() << " )\n";
//   }
   return 0;
}

void acc_interpret( Symbol* phenotype, float* ephemeral, int* size, float* vector, int nInd, int prediction_mode )
{
   // Transferência de dados para o dispositivo
   data.fila.enqueueWriteBuffer( data.buffer_phenotype, CL_TRUE, 0, data.max_size * nInd * sizeof( Symbol ), phenotype );
   data.fila.enqueueWriteBuffer( data.buffer_ephemeral, CL_TRUE, 0, data.max_size * nInd * sizeof( float ), ephemeral ); 
   data.fila.enqueueWriteBuffer( data.buffer_size, CL_TRUE, 0, nInd * sizeof( int ), size ); 

   data.kernel.setArg( 9, nInd );

   data.fila.enqueueNDRangeKernel( data.kernel, cl::NDRange(), cl::NDRange( data.global_size ), cl::NDRange( data.local_size ) );
  
   // Espera pela finalização da execução do kernel (forçar sincronia)
   data.fila.finish();

   // Transferência dos resultados para o hospedeiro 
   if ( prediction_mode )
   {
      float* tmp = (float*) data.fila.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, data.nlin * sizeof( float ) );
      for( int i = 0; i < data.nlin; i++ ) {vector[i] = tmp[i];}
   }
   else
   {
      float* tmp = (float*) data.fila.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, (data.global_size/data.local_size) * nInd * sizeof( float ) );
      float sum;
      for( int ind = 0; ind < nInd; ind++)
      {
         sum = 0.0;
         for( int i = 0; i < (data.global_size/data.local_size); i++ ) {sum += tmp[ind * (data.global_size/data.local_size) + i];}

         if( isnan( sum ) || isinf( sum ) ) {vector[ind] = std::numeric_limits<float>::max();}
         else {vector[ind] = sum/data.nlin;}
      }
      // Unmapping
      data.fila.enqueueUnmapMemObject( data.buffer_vector, tmp ); 
   }
}
