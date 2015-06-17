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

static struct t_data { int nlin; int local_size; cl::Context context; cl::Kernel kernel; cl::CommandQueue fila; cl::Buffer buffer_phenotype; cl::Buffer buffer_ephemeral; cl::Buffer buffer_inputs; cl::Buffer buffer_vector; } data;


/** ****************************************************************** **/
/** **************************** KERNELS ***************************** **/
/** ****************************************************************** **/

const char* kernel_str  = 
   "enum Symbol { NT_IF_THEN_ELSE_INICIAL, NT_LOGICO, NT_LOG_OP, NT_RELACIONAL, NT_REL_OP, NT_ENSEMBLE, NT_BIN_OP, NT_UN_OP, NT_ATRIBUTO, NT_INDICE, NT_PAD, NT_CONST_NUMERICO, NT_MODELO, T_IF_THEN_ELSE = TERMINAL_MIN, T_AND, T_OR, T_NOT, T_MAIOR, T_MENOR, T_IGUAL, T_ADD, T_SUB, T_MULT, T_DIV, T_MEAN, T_MAX, T_MIN, T_ABS, T_SQRT, T_POW2, T_POW3, T_NEG, T_EFEMERO, T_PAD, T_1, T_2, T_3, T_4, T_BMA, T_CHUVA_ONTEM, T_CHUVA_ANTEONTEM, T_MEAN_MODELO, T_MAX_MODELO, T_MIN_MODELO, T_STD_MODELO, T_CHUVA_LAG1P, T_CHUVA_LAG1N, T_CHUVA_LAG2P, T_CHUVA_LAG2N, T_CHUVA_LAG3P, T_CHUVA_LAG3N, T_CHUVA_PADRAO, T_CHUVA_HISTORICA, T_K, T_TT, T_SWEAT, T_CHOVE, T_PADRAO_MUDA, T_PERTINENCIA, T_MOD1, T_MOD2, T_MOD3, T_MOD4, T_MOD5, T_MOD6, T_MOD7, T_MOD8 }; "
   "__kernel void "
   "evaluate( __global const Symbol* phenotype, __global const double* ephemeral, __global const double* inputs, __global double* vector, __local double* EP, int nlin, int ncol, int mode, int size ) "
   "{ "
   "   double pilha[TAM_MAX]; "
   "   int lo_id = get_local_id(0); "
   "   int gr_id = get_group_id(0); "
   "   int topo = -1; "
   "   __local Symbol phenotypeLocal; "
   "   __local double ephemeralLocal; "
   "   for( int i = size - 1; i >= 0; --i ) "
   "   { "
   "      if( lo_id == 0 ) { phenotypeLocal = phenotype[i]; ephemeralLocal = ephemeral[i]; } "
   "      barrier( CLK_LOCAL_MEM_FENCE ); "
   "      switch( phenotypeLocal ) "
   "      { "
   "         case T_IF_THEN_ELSE: "
   "            topo = ( (pilha[topo] == 1.0) ? topo - 1 : topo - 2 ); "
   "            break; "
   "         case T_AND: "
   "            pilha[topo - 1] = ( (pilha[topo] == 1.0 && pilha[topo - 1] == 1.0) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_OR: "
   "            pilha[topo - 1] = ( (pilha[topo] == 1.0 || pilha[topo - 1] == 1.0) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_NOT: "
   "            pilha[topo] = !pilha[topo]; "
   "            break; "
   "         case T_MAIOR: "
   "            pilha[topo - 1] = ( (pilha[topo] > pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_MENOR: "
   "            pilha[topo - 1] = ( (pilha[topo] < pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_IGUAL: "
   "            pilha[topo - 1] = ( (pilha[topo] == pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_ADD: "
   "            pilha[topo - 1] = pilha[topo] + pilha[topo - 1]; "
   "           --topo; "
   "            break; "
   "         case T_SUB: "
   "            pilha[topo - 1] = pilha[topo] - pilha[topo - 1]; "
   "            --topo; "
   "            break; "
   "         case T_MULT: "
   "            pilha[topo - 1] = pilha[topo] * pilha[topo - 1]; "
   "            --topo; "
   "            break; "
   "         case T_DIV: "
   "            pilha[topo - 1] = pilha[topo] / pilha[topo - 1]; "
   "            --topo; "
   "            break; "
   "         case T_MEAN: "
   "            pilha[topo - 1] = (pilha[topo] + pilha[topo - 1]) / 2; "
   "            --topo; "
   "            break; "
   "         case T_MAX: "
   "            pilha[topo - 1] = fmax(pilha[topo], pilha[topo - 1]); "
   "            --topo; "
   "            break; "
   "         case T_MIN: "
   "            pilha[topo - 1] = fmin(pilha[topo], pilha[topo - 1]); "
   "            --topo; "
   "            break; "
   "         case T_ABS: "
   "            pilha[topo] = fabs(pilha[topo]); "
   "            break; "
   "         case T_SQRT: "
   "            pilha[topo] = sqrt(pilha[topo]); "
   "            break; "
   "         case T_POW2: "
   "            pilha[topo] = pow(pilha[topo], 2); "
   "            break; "
   "         case T_POW3: "
   "            pilha[topo] = pow(pilha[topo], 3); "
   "            break; "
   "         case T_NEG: "
   "            pilha[topo] = -pilha[topo]; "
   "            break; "
   "         case T_BMA: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id)]; "
   "            break; "
   "          case T_CHUVA_ONTEM: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 1]; "
   "            break; "
   "          case T_CHUVA_ANTEONTEM: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 2]; "
   "            break; "
   "         case T_MEAN_MODELO: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 3]; "
   "            break; "
   "         case T_MAX_MODELO: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 4]; "
   "            break; "
   "         case T_MIN_MODELO: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 5]; "
   "            break; "
   "         case T_STD_MODELO: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 6]; "
   "            break; "
   "         case T_CHOVE: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 7]; "
   "            break; "
   "         case T_CHUVA_LAG1P: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 8]; "
   "            break; "
   "         case T_CHUVA_LAG1N: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 9]; "
   "            break; "
   "         case T_CHUVA_LAG2P: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 10]; "
   "            break; "
   "         case T_CHUVA_LAG2N: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 11]; "
   "            break; "
   "         case T_CHUVA_LAG3P: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 12]; "
   "            break; "
   "         case T_CHUVA_LAG3N: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 13]; "
   "            break; "
   "         case T_PADRAO_MUDA: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 14]; "
   "            break; "
   "         case T_PERTINENCIA: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 15]; "
   "            break; "
   "         case T_CHUVA_PADRAO: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 16]; "
   "            break; "
   "         case T_CHUVA_HISTORICA: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 17]; "
   "            break; "
   "         case T_K: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 18]; "
   "            break; "
   "         case T_TT: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 19]; "
   "            break; "
   "         case T_SWEAT: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 20]; "
   "            break; "
   "         case T_PAD: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 21]; "
   "            break; "
   "         case T_MOD1: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 22]; "
   "            break; "
   "         case T_MOD2: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 23]; "
   "            break; "
   "         case T_MOD3: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 24]; "
   "            break; "
   "         case T_MOD4: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 25]; "
   "            break; "
   "         case T_MOD5: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 26]; "
   "            break; "
   "         case T_MOD6: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 27]; "
   "            break; "
   "         case T_MOD7: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 28]; "
   "            break; "
   "         case T_MOD8: "
   "            pilha[++topo] = inputs[(gr_id * get_local_size(0) + lo_id) + nlin * 29]; "
   "            break; "
   "         case T_1: "
   "            pilha[++topo] = 1; "
   "            break; "
   "         case T_2: "
   "            pilha[++topo] = 2; "
   "            break; "
   "         case T_3: "
   "            pilha[++topo] = 3; "
   "            break; "
   "         case T_4: "
   "            pilha[++topo] = 4; "
   "            break; "
   "         case T_EFEMERO: "
   "            pilha[++topo] = ephemeralLocal; "
   "            break; "
   "      } "
   "   } "
   "   if( !mode ) " 
   "   { "
   "      EP[lo_id] = fabs( pilha[topo] - inputs[(gr_id * get_local_size(0) + lo_id) + nlin * (ncol - 1)] ); "
   "      for( int s = get_local_size(0)/2; s > 0; s/=2 ) "
   "      { "
   "        barrier(CLK_LOCAL_MEM_FENCE); "
   "        if( lo_id < s) { EP[lo_id] += EP[lo_id + s]; } "
   "      } "
   "      if( lo_id == 0) { vector[gr_id] = EP[0]; } "
   "   } "
   "} "; 

//   "   else "
//   "   { "
//   "     vector[lo_id] = pilha[topo]; "  
//   "   } "


/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void acc_interpret_init( const unsigned size, double** input, double** model, double* obs, int nlin, int ninput, int nmodel, int mode, const char* type )
{
   data.nlin = nlin;
   int ncol = ninput + nmodel + 1;
   fprintf(stdout,"%d\n",TERMINAL_MIN);

   data.local_size = 128;

   int acc;
   if( !strcmp(type,"CPU") ) {acc = CL_DEVICE_TYPE_CPU;}
   else {acc = CL_DEVICE_TYPE_GPU;}

   // Descobrir e escolher as plataformas
   vector<cl::Platform> plataformas;
   // Descobrir os dispositivos
   vector<cl::Device> dispositivos;

   // Descobre as plataformas instaladas no hospedeiro
   cl::Platform::get( &plataformas );

   plataformas[0].getDevices( CL_DEVICE_TYPE_ALL, &dispositivos );

//   vector<cl::Device> dispositivo(1);
//   for( int m = 0; m < plataformas.size(); m++ )
//   {
//     // Descobrir os dispositivos
//     vector<cl::Device> dispositivos;
//     // Descobre os dispositivos da plataforma m
//     plataformas[m].getDevices( CL_DEVICE_TYPE_ALL, &dispositivos );
//     for ( int n = 0; n < dispositivos.size(); n++ )
//     {
//        if ( dispositivos[n].getInfo<CL_DEVICE_TYPE>() == acc ) 
//         dispositivo[0] = dispositivos[n];
//     }
//   }

   // Criar o contexto
   //data.context = cl::Context( dispositivo );
   data.context = cl::Context( dispositivos );

   // Criar a fila de comandos para um dispositivo (aqui só o primeiro)
   //data.fila = cl::CommandQueue( data.context, dispositivo[0], CL_QUEUE_PROFILING_ENABLE );
   data.fila = cl::CommandQueue( data.context, dispositivos[0], CL_QUEUE_PROFILING_ENABLE );

   data.buffer_inputs = cl::Buffer( data.context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, nlin * ncol * sizeof( double ) );

   double* inputs = (double*) data.fila.enqueueMapBuffer( data.buffer_inputs, CL_TRUE, CL_MAP_WRITE, 0, nlin * ncol * sizeof( double ) );

   for( int i = 0; i < nlin; i++ )
   {
      for( int j = 0; j < ninput; j++ )
      {
         inputs[j * ncol + i] = input[i][j];
      }
      for( int j = ninput; j < (nmodel + ninput); j++ )
      {
         inputs[j * ncol + i] = model[i][j];
      }
      inputs[(nmodel + ninput) * ncol + i] = obs[i];
   }

   // Unmapping
   data.fila.enqueueUnmapMemObject( data.buffer_inputs, inputs ); 

   // Carregar o programa, compilá-lo e gerar o kernel
   cl::Program::Sources fonte( 1, make_pair( kernel_str, strlen( kernel_str ) ) );
   cl::Program programa( data.context, fonte );

   // Compila para todos os dispositivos associados a 'programa' através do
   // 'contexto': vector<cl::Device>() é um vetor nulo
   char buildOptions[60];
   sprintf( buildOptions, "-DTAM_MAX=%u -DTERMINAL_MIN=%u", size, TERMINAL_MIN );  
   try {
      programa.build( vector<cl::Device>(), buildOptions );
   } 
    catch (cl::Error err)
    {
        std::cerr
            << "ERROR: "
            << err.what()
            << "("
            << err.err()
            << ")"
            << std::endl;
    }


//   catch( cl::Error )
//   {
//     //cout << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivo[0]) << std::endl;
//     cout << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivos[0]) << std::endl;
//   }

   fprintf(stdout,"Chegou!\n");
   // Cria a variável kernel que vai representar o kernel "avalia"
   data.kernel = cl::Kernel( programa, "evaluate" );

   // 2) Preparação da memória dos dispositivos (leitura e escrita}
   data.buffer_phenotype = cl::Buffer( data.context, CL_MEM_READ_ONLY, size * sizeof( Symbol ) );
   data.buffer_ephemeral = cl::Buffer( data.context, CL_MEM_READ_ONLY, size * sizeof( double ) );
   if( mode ) {data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, data.nlin * sizeof( double ) );}
   else {data.buffer_vector = cl::Buffer( data.context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, (data.nlin/data.local_size) * sizeof( double ) );} // TODO: check the division (potential rounding error there) and operator precedence

   // Execução do kernel: definição dos argumentos e trabalho/particionamento
   data.kernel.setArg( 0, data.buffer_phenotype );
   data.kernel.setArg( 1, data.buffer_ephemeral );
   data.kernel.setArg( 2, data.buffer_inputs );
   data.kernel.setArg( 3, data.buffer_vector );
   data.kernel.setArg( 4, sizeof( double ) * data.local_size, NULL );
   data.kernel.setArg( 5, nlin );
   data.kernel.setArg( 6, ncol );
   data.kernel.setArg( 7, mode );
}

void acc_interpret( Symbol* phenotype, double* ephemeral, int size, double* vector, int mode )
{
   // Transferência de dados para o dispositivo
   data.fila.enqueueWriteBuffer( data.buffer_phenotype, CL_TRUE, 0, size * sizeof( Symbol ), phenotype, NULL );
   data.fila.enqueueWriteBuffer( data.buffer_ephemeral, CL_TRUE, 0, size * sizeof( double ), ephemeral, NULL ); 

   data.kernel.setArg( 8, size );

   data.fila.enqueueNDRangeKernel( data.kernel, cl::NDRange(), cl::NDRange( data.nlin ), cl::NDRange( data.local_size ), NULL );
  
   // Espera pela finalização da execução do kernel (forçar sincronia)
   data.fila.finish();

   // Transferência dos resultados para o hospedeiro 
   if ( !mode )
   {
      double* tmp = (double*) data.fila.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, data.nlin/data.local_size * sizeof( double ) );
      double sum = 0.0;
      for( int i = 0; i < (data.nlin/data.local_size); i++ ) {sum += tmp[i];}

      // Unmapping
      data.fila.enqueueUnmapMemObject( data.buffer_vector, tmp ); 

      if( isnan( sum ) || isinf( sum ) ) {vector[0] = std::numeric_limits<float>::max();}
      else {vector[0] = sum/(data.nlin/data.local_size);}
   }
//   else
//   {
//      vector = (double*) data.fila.enqueueMapBuffer( data.buffer_vector, CL_TRUE, CL_MAP_READ, 0, data.nlin * sizeof( double ) );
//   }
}
