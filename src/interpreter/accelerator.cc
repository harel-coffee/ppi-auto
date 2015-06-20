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
#include "accelerator.h"

using namespace std;

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int nlin; int local_size; int global_size; cl::Context context; cl::Kernel kernel; cl::CommandQueue fila; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; } data;

/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void acc_interpret_init( const unsigned size, float** input, float** model, float* obs, int nlin, int ninput, int nmodel, int mode, const char* type )
{
   data.nlin = nlin;
   int ncol = ninput + nmodel + 1;
   data.local_size = 64;

   data.global_size = nlin;
   if( data.global_size % data.local_size != 0 )
   {
      data.global_size += data.local_size - (data.global_size % data.local_size); 
   }

   int acc;
   if( !strcmp(type,"CPU") ) {acc = CL_DEVICE_TYPE_CPU;}
   else {acc = CL_DEVICE_TYPE_GPU;}

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
      for ( int n = 0; n < dispositivos.size(); n++ )
      {
         if ( dispositivos[n].getInfo<CL_DEVICE_TYPE>() == acc ) 
         {
            leave = true;
            dispositivo[0] = dispositivos[n];
            device_type = dispositivo[0].getInfo<CL_DEVICE_TYPE>();
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


//   if ( device_type == CL_DEVICE_TYPE_CPU ) 
//   {
//      for( int i = 0; i < nlin; i++ )
//      {
//         for( int j = 0; j < ninput; j++ )
//         {
//            inputs[i * ncol + j] = input[i][j];
//         }
//         for( int j = ninput; j < (nmodel + ninput); j++ )
//         {
//            inputs[i * ncol + j] = model[i][j];
//         }
//         inputs[i * ncol + (nmodel + ninput)] = obs[i];
//      }
//   }
//   else
//   {
      for( int i = 0; i < nlin; i++ )
      {
         for( int j = 0; j < ninput; j++ )
         {
            inputs[j * nlin + i] = input[i][j];
         }
         for( int j = ninput; j < (nmodel + ninput); j++ )
         {
            inputs[j * nlin + i] = model[i][j];
         }
         inputs[(nmodel + ninput) * nlin + i] = obs[i];
      }
//   }
 
   // Unmapping
   data.fila.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

//   inputs = (float*) data.fila.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_READ, 0, nlin * ncol * sizeof( float ) );
//   for( int i = 0; i < nlin*ncol; i++ )
//   {
//      if( i % nlin == 0 ) fprintf(stdout,"\n");
//      fprintf(stdout,"%f,", inputs[i]);
//   }
//   // Unmapping
//   data.fila.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

   // Carregar o programa, compilá-lo e gerar o kernel
   ifstream file("accelerator.cl");
   string kernel_str( istreambuf_iterator<char>(file),
                    ( istreambuf_iterator<char>()) );
   cl::Program::Sources fonte( 1, make_pair( kernel_str.c_str(), kernel_str.size() ) );

   cl::Program programa( data.context, fonte );

   // Compila para todos os dispositivos associados a 'programa' através do
   // 'contexto': vector<cl::Device>() é um vetor nulo
   char buildOptions[60];
   sprintf( buildOptions, "-DTAM_MAX=%u -DTERMINAL_MIN=%u", size, TERMINAL_MIN );  
   try {
      programa.build( dispositivo, buildOptions );
   }
   catch( cl::Error e )
   {
     cerr << "ERROR: " << e.what() << " ( " << e.err() << " )\n";
     cout << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivo[0]) << std::endl;
   }

   // Cria a variável kernel que vai representar o kernel "avalia"
   data.kernel = cl::Kernel( programa, "evaluate" );

   // 2) Preparação da memória dos dispositivos (leitura e escrita}
   data.buffer_phenotype = cl::Buffer( data.context, CL_MEM_READ_ONLY, size * sizeof( Symbol ) );
   data.buffer_ephemeral = cl::Buffer( data.context, CL_MEM_READ_ONLY, size * sizeof( float ) );
   if( mode ) 
   { 
      data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * sizeof( float ) );
   }
   else 
   {
      data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, (data.global_size/data.local_size) * sizeof( float ) );
   } 

   // Execução do kernel: definição dos argumentos e trabalho/particionamento
   data.kernel.setArg( 0, data.buffer_phenotype );
   data.kernel.setArg( 1, data.buffer_ephemeral );
   data.kernel.setArg( 2, data.buffer_inputs );
   data.kernel.setArg( 3, data.buffer_vector );
   data.kernel.setArg( 4, sizeof( float ) * data.local_size, NULL );
   data.kernel.setArg( 5, nlin );
   data.kernel.setArg( 6, ncol );
   data.kernel.setArg( 7, mode );
}

void acc_interpret( Symbol* phenotype, float* ephemeral, int size, float* vector, int mode )
{
   // Transferência de dados para o dispositivo
   data.fila.enqueueWriteBuffer( data.buffer_phenotype, CL_TRUE, 0, size * sizeof( Symbol ), phenotype );
   data.fila.enqueueWriteBuffer( data.buffer_ephemeral, CL_TRUE, 0, size * sizeof( float ), ephemeral ); 

   data.kernel.setArg( 8, size );

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
      float* tmp = (float*) data.fila.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, (data.global_size/data.local_size) * sizeof( float ) );
      float sum = 0.0;
      for( int i = 0; i < (data.global_size/data.local_size); i++ ) {sum += tmp[i];}

      // Unmapping
      data.fila.enqueueUnmapMemObject( data.buffer_vector, tmp ); 

      if( isnan( sum ) || isinf( sum ) ) {vector[0] = std::numeric_limits<float>::max();}
      else {vector[0] = sum/data.nlin;}
   }
}
