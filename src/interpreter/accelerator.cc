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

using namespace std;

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int max_size; int nlin; int local_size; int global_size; cl::Context context; cl::Kernel kernel; cl::CommandQueue fila; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_size; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; } data;

/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void acc_interpret_init( const unsigned size, const unsigned population_size, float** input, float** model, float* obs, int nlin, int ninput, int nmodel, int mode, const char* type )
{
   try
   {
      data.max_size = size;
      data.nlin = nlin;
      int ncol = ninput + nmodel + 1;
      data.local_size = 64;

      data.global_size = nlin;
      if( data.global_size % data.local_size != 0 )
      {
         data.global_size += data.local_size - (data.global_size % data.local_size); 
      }
      fprintf(stdout,"global_size=%d; local_size=%d\n",data.global_size,data.local_size);

      int acc;
      if( !strcmp(type,"CPU") ) {acc = CL_DEVICE_TYPE_CPU;}
      else {acc = CL_DEVICE_TYPE_GPU;}
      fprintf(stdout,"acc=%d\n",acc);

      // Descobre as plataformas instaladas no hospedeiro
      vector<cl::Platform> plataformas;
      cl::Platform::get( &plataformas );

      vector<cl::Device> dispositivo(1); 
      int device_type;

      bool leave = false;
      for( int m = 0; m < plataformas.size(); m++ )
      {
         vector<cl::Device> dispositivos;
         // Descobre os dispositivos da plataforma m
         plataformas[m].getDevices( CL_DEVICE_TYPE_ALL, &dispositivos );

         dispositivo[0] = dispositivos[0];
         device_type = dispositivo[0].getInfo<CL_DEVICE_TYPE>();
         fprintf(stdout,"device_type=%d\n",device_type);
         for ( int n = 0; n < dispositivos.size(); n++ )
         {
            if ( dispositivos[n].getInfo<CL_DEVICE_TYPE>() == acc ) 
            {
               leave = true;
               dispositivo[0] = dispositivos[n];
               device_type = dispositivo[0].getInfo<CL_DEVICE_TYPE>();
               fprintf(stdout,"break;device_type=%d\n",device_type);
               break;
            }
         }
         if( leave ) break;
      }

      // Criar o contexto
      data.context = cl::Context( dispositivo );

      // Criar a fila de comandos para um dispositivo (aqui só o primeiro)
      //data.fila = cl::CommandQueue( data.context, dispositivo[0], CL_QUEUE_PROFILING_ENABLE );
      data.fila = cl::CommandQueue( data.context, dispositivo[0], CL_QUEUE_PROFILING_ENABLE );

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
      sprintf( buildOptions, "-DTAM_MAX=%u -I.", data.max_size );  
      try {
         programa.build( dispositivo, buildOptions );
      }
      catch( cl::Error& e )
      {
         cerr << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivo[0]) << std::endl;

         throw;
      }

      // Cria a variável kernel que vai representar o kernel "avalia"
//      if ( device_type == CL_DEVICE_TYPE_CPU ) 
//      {
//         fprintf(stdout,"Rodando CPU...\n");
//         data.kernel = cl::Kernel( programa, "evaluate_cpu" );
//      }
//      else
//      {
//         fprintf(stdout,"Rodando GPU...\n");
         data.kernel = cl::Kernel( programa, "evaluate_gpu" );
//      }

      // 2) Preparação da memória dos dispositivos (leitura e escrita)
      data.buffer_phenotype = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * population_size * sizeof( Symbol ) );
      data.buffer_ephemeral = cl::Buffer( data.context, CL_MEM_READ_ONLY, data.max_size * population_size * sizeof( float ) );
      data.buffer_size      = cl::Buffer( data.context, CL_MEM_READ_ONLY, population_size * sizeof( int ) );
      if( mode ) 
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
      data.kernel.setArg( 8, mode );
   }
   catch( cl::Error& e )
   {
      cerr << "ERROR: " << e.what() << " ( " << e.err() << " )\n";
   }
}

void acc_interpret( Symbol* phenotype, float* ephemeral, int* size, float* vector, int nInd, int mode )
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
   if ( mode )
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
