// ------------------------------------------------------------------------
// Habilita disparar exceções C++
#define __CL_ENABLE_EXCEPTIONS

// Cabeçalho OpenCL para C++
#include "CL/cl.hpp"

#include <stdlib.h>
#include <stdio.h>  // usar a função 'printf'
#include <iostream> // para jogar na tela as saídas
#include <vector>   // utiliza o vetor com alocação dinâmica do C++
#include <utility>  // para usar a função make_pair: "junta" duas variáveis
#include <cstdlib>  // usar a função 'atoi': converte uma string em um valor inteiro
#include <cmath>    // usar a função 'pow': calcula potencia
#include <ctime>    // usar a função 'clock': mede o tempo de execução
#include <string>   // usar a função 'strcmp': compara duas strings

using namespace std;


void interpret ()
//( t_individuo* individuo, int nlinha, int local_size, double* time_kernel, double* time_overhead, cl::Buffer bufferFenotipo, cl::Buffer bufferEfemeros, cl::Buffer bufferE, cl::Kernel kernel, cl::CommandQueue fila )
{
   // Transferência de dados para o dispositivo
   fila.enqueueWriteBuffer( bufferFenotipo, CL_TRUE, 0, size * sizeof( Simbolo ), fenotipo, NULL );
   fila.enqueueWriteBuffer( bufferEfemeros, CL_TRUE, 0, size * sizeof( float ), efemeros, NULL );

   // Execução do kernel: definição dos argumentos e trabalho/particionamento
   kernel.setArg( 6, size );

   fila.enqueueNDRangeKernel( kernel, cl::NDRange(), cl::NDRange( nlinha ), cl::NDRange( local_size ), NULL );
  
   // Espera pela finalização da execução do kernel (forçar sincronia)
   fila.finish();

   // Transferência dos resultados para o hospedeiro (e joga no Y)
   float *E = (float*) fila.enqueueMapBuffer( bufferE, CL_TRUE, CL_MAP_READ, 0, nlinha / local_size * sizeof( float ) );
   float sum = 0.;
   for( int k = 0; k < nlinha / local_size; k++ ) sum += E[k];
 
   // Transformação do acerto em aptidão (são inversamente proporcionais)
   //individuo->aptidao = sum / (nlinha / local_size);
}

// -----------------------------------------------------------------------------
/**
  Preenche com zeros e uns (aleatoriamente) os indivíduos da população.
*/
//void InicializaPopulacao( t_individuo* populacao, int nlinha )
void InicializaPopulacao( t_individuo* populacao, int nlinha, int local_size, double* time_kernel, double* time_overhead, cl::Buffer bufferFenotipo, cl::Buffer bufferEfemeros, cl::Buffer bufferE, cl::Kernel kernel, cl::CommandQueue fila )
{
   printf( "População inicial:\n" );

   for( int i = 0; i < tamanhoPopulacao; ++i)
   {
      for( int j = 0; j < numeroBits; j++ )
      {
         populacao[i].genoma[j] = (NumeroAleatorio() < 0.5) ? 1 : 0;
      }
      
//      Avaliacao( &populacao[i], nlinha );
      Avaliacao( &populacao[i], nlinha, local_size, time_kernel, time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
      Imprima( &populacao[i] );
   }

   printf( "\n" );
}

// ------------------------------------------------------------------------
const char * kernel_str  = 
   "typedef enum { NT_IF_THEN_ELSE_INICIAL, NT_IF_THEN_ELSE, NT_LOGICO, NT_RELACIONAL, NT_NUMERICO, NT_ATRIB_NUMERICO, NT_CONST_NUMERICO, NT_CLASSE,       T_IF_THEN_ELSE = TERMINAL_MIN, T_AND, T_OR, T_NEG, T_MAIOR, T_MAIOR_IGUAL, T_MENOR, T_MENOR_IGUAL, T_IGUAL, T_DIFERENTE, T_0, T_1, T_EFEMERO, T_VAR1, T_VAR2, T_VAR3, T_VAR4, T_VAR5, T_VAR6, T_VAR7, T_VAR8, T_VAR9, T_VAR10, T_VAR11, T_VAR12 } Simbolo; "
   "__kernel void "
   "avalia( __global const Simbolo * fenotipo, __global const float * efemeros, __global const float * VAR, __global float * E, __local float * EP, int nlinhas, int size ) "
   "{ "
   "   float pilha[TAM_MAX]; "
   "   int lo_id = get_local_id(0); "
   "   int gr_id = get_group_id(0); "
   "   EP[lo_id] = 0.; "
   "   int topo = -1; "
   "   __local Simbolo fenLocal; "
   "   __local float efemLocal; "
   "   for( int i = size - 1; i >= 0; --i ) "
   "   { "
   "      if( lo_id == 0 ) { fenLocal = fenotipo[i]; efemLocal = efemeros[i]; } "
   "      barrier( CLK_LOCAL_MEM_FENCE ); "
   "      switch( fenLocal ) "
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
   "         case T_NEG: "
   "            pilha[topo] = -pilha[topo]; "
   "            break; "
   "         case T_MAIOR: "
   "            pilha[topo - 1] = ( (pilha[topo] > pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_MAIOR_IGUAL: "
   "            pilha[topo - 1] = ( (pilha[topo] >= pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_MENOR: "
   "            pilha[topo - 1] = ( (pilha[topo] < pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_MENOR_IGUAL: "
   "            pilha[topo - 1] = ( (pilha[topo] <= pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_IGUAL: "
   "            pilha[topo - 1] = ( (pilha[topo] == pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_DIFERENTE: "
   "            pilha[topo - 1] = ( (pilha[topo] != pilha[topo - 1]) ? 1.0 : 0.0 ); "
   "            --topo; "
   "            break; "
   "         case T_0: "
   "            pilha[++topo] = 0.0; "
   "            break; "
   "         case T_1: "
   "            pilha[++topo] = 1.0; "
   "            break; "
   "         case T_VAR1: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) ]; "
   "            break; "
   "         case T_VAR2: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 1]; "
   "            break; "
   "         case T_VAR3: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 2]; "
   "            break; "
   "         case T_VAR4: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 3]; "
   "            break; "
   "         case T_VAR5: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 4]; "
   "            break; "
   "         case T_VAR6: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 5]; "
   "            break; "
   "         case T_VAR7: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 6]; "
   "            break; "
   "         case T_VAR8: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 7]; "
   "            break; "
   "         case T_VAR9: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 8]; "
   "            break; "
   "         case T_VAR10: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 9]; "
   "            break; "
   "         case T_VAR11: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 10]; "
   "            break; "
   "         case T_VAR12: "
   "            pilha[++topo] = VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 11]; "
   "            break; "
   "         case T_EFEMERO: "
   "            pilha[++topo] = efemLocal; "
   "            break; "
   "      } "
   "   } "
   "   if( pilha[topo] == VAR[(gr_id * get_local_size(0) + lo_id) + nlinhas * 12] ) { EP[lo_id] = 1.; } "
   "   for( int s = get_local_size(0)/2; s > 0; s/=2 ) "
   "   { "
   "     barrier(CLK_LOCAL_MEM_FENCE); "
   "     if( lo_id < s) { EP[lo_id] += EP[lo_id + s]; } "
   "   } "
   "   if( lo_id == 0) { E[gr_id] = EP[0]; } "
   "} "; 

// ------------------------------------------------------------------------
//int main( int argc, char* argv[] )
//{
//
//   clock_t ini,fim;
//   ini = clock();
//
//   // Teste para verificar se o número de elementos foi informado
//   if( argc < 4 )
//   {
//      cerr << "ERRO: Especificar a arquitetura do dispositivo.\n";
//      return 1;
//   }
//
//   int tipo;
//   if( !strcmp(argv[1],"CPU") ) tipo = CL_DEVICE_TYPE_CPU;
//   else tipo = CL_DEVICE_TYPE_GPU;
//
//   const unsigned local_size = atoi( argv[2] );
//
//   char *arquivo;
//   arquivo = argv[3];
//   //strcat(arquivo,"/treinamento.txt");
//   //char arquivo[30] = "arquivo1/treinamento.txt";
//
//   FILE *arqentra;
//   arqentra = fopen(arquivo,"r");
//   if(arqentra == NULL) 
//   {
//     printf("Nao foi possivel abrir o arquivo de entrada.\n");
//     exit(-1);
//   }
//
//   char linha[100];
//   int nlinha;
//   for( nlinha = 0; fgets( linha,100,arqentra ) != NULL; nlinha++ );
//   float *VAR   = new float[nlinha * 13];
//   fclose (arqentra);
//
//   arqentra = fopen(arquivo,"r");
//   if(arqentra == NULL) 
//   {
//     printf("Nao foi possivel abrir o arquivo de entrada.\n");
//     exit(-1);
//   }
//
//   for( int i = 0; i < nlinha; i++ )
//   {
//      for( int j = 0; j < 13; j++ )
//      {
//         fscanf(arqentra,"%f",&VAR[j * 13 + i]);
//      }
//   }
//   fclose (arqentra);
//
//   // Inicia o gerador de números aleatórios com uma semente "aleatória"
//   srand( time(NULL) );
//
//   // -----------------------
//   // Alocação de memórias
//   // -----------------------
//   t_individuo* populacao_a = new t_individuo[tamanhoPopulacao];
//   t_individuo* populacao_b = new t_individuo[tamanhoPopulacao];
//
//   melhorIndividuo.genoma = new int[numeroBits];
//
//   // Alocação dos indivíduos
//   for( int i = 0; i < tamanhoPopulacao; ++i )
//   {
//      populacao_a[i].genoma = new int[numeroBits];
//      populacao_b[i].genoma = new int[numeroBits];
//   }
//
//   t_individuo* antecedentes = populacao_a;
//   t_individuo* descendentes = populacao_b;
//
//   // 1) Inicialização
//   // Descobrir e escolher as plataformas
//   vector<cl::Platform> plataformas;
//
//   // Descobre as plataformas instaladas no hospedeiro
//   cl::Platform::get( &plataformas );
//
//   vector<cl::Device> dispositivo(1);
//   for( int m = 0; m < plataformas.size(); m++ )
//   {
//     // Descobrir os dispositivos
//     vector<cl::Device> dispositivos;
//     // Descobre os dispositivos da plataforma i
//     plataformas[m].getDevices( CL_DEVICE_TYPE_ALL, &dispositivos );
//     for ( int n = 0; n < dispositivos.size(); n++ )
//       if ( dispositivos[n].getInfo<CL_DEVICE_TYPE>() == tipo ) 
//         dispositivo[0] = dispositivos[n];
//   }
//
//   // Criar o contexto
//   cl::Context contexto( dispositivo );
//
//   // Criar a fila de comandos para um dispositivo (aqui só o primeiro)
//   cl::CommandQueue fila( contexto, dispositivo[0], CL_QUEUE_PROFILING_ENABLE );
//
//   // Carregar o programa, compilá-lo e gerar o kernel
//   cl::Program::Sources fonte( 1, make_pair( kernel_str, strlen( kernel_str ) ) );
//   cl::Program programa( contexto, fonte );
//
//   // Compila para todos os dispositivos associados a 'programa' através do
//   // 'contexto': vector<cl::Device>() é um vetor nulo
//   char buildOptions[60];
//   sprintf( buildOptions, "-DTAM_MAX=%u -DTERMINAL_MIN=%u", max_tam_fenotipo, TERMINAL_MIN );  
//   try {
//      programa.build( vector<cl::Device>(), buildOptions );
//   } catch( cl::Error )
//   {
//     cout << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivo[0]) << std::endl;
//   }
//
//   // Cria a variável kernel que vai representar o kernel "avalia"
//   cl::Kernel kernel( programa, "avalia" );
//
//   cl::Event e_tempo;
//
//   // 2) Preparação da memória dos dispositivos (leitura e escrita}
//   cl::Buffer bufferFenotipo( contexto, CL_MEM_READ_ONLY, max_tam_fenotipo * sizeof( Simbolo ) );
//   cl::Buffer bufferEfemeros( contexto, CL_MEM_READ_ONLY, max_tam_fenotipo * sizeof( float ) );
//   cl::Buffer bufferVAR( contexto, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, nlinha * 13 * sizeof( float ), VAR, NULL, &e_tempo );
//   cl::Buffer bufferE( contexto, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, nlinha / local_size * sizeof( float ) );
//
//   // 3) Execução
//   // Transferência de dados para o dispositivo
//   //fila.enqueueWriteBuffer( bufferVAR, CL_TRUE, 0, nlinha * 13 * sizeof( float ), VAR, NULL, &e_tempo );
//
//   cl_ulong inicio, final;
//   e_tempo.getProfilingInfo( CL_PROFILING_COMMAND_START, &inicio );
//   e_tempo.getProfilingInfo( CL_PROFILING_COMMAND_END, &final );
//   double time_overhead = (final - inicio)/1.0E9;
//   //printf("time_overhead = %f\n", time_overhead);
//
//   // Execução do kernel: definição dos argumentos e trabalho/particionamento
//   kernel.setArg( 0, bufferFenotipo );
//   kernel.setArg( 1, bufferEfemeros );
//   kernel.setArg( 2, bufferVAR );
//   kernel.setArg( 3, bufferE );
//   kernel.setArg( 4, sizeof( float ) * local_size, NULL );
//   kernel.setArg( 5, nlinha );
//
//   // Criação da população inicial (1ª geração)
//   double time_kernel;
//   InicializaPopulacao( antecedentes, nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
//
//
//   // Processo evolucionário
//   for( int geracao = 1; geracao <= numeroGeracoes; ++geracao )
//   {
//      for( int i = 0; i < tamanhoPopulacao; i += 2 )
//      {
//         // Seleção de indivíduos adaptados para gerar descendentes
//         const t_individuo* pai = Torneio( antecedentes );
//         const t_individuo* mae = Torneio( antecedentes );
//
//         // Cruzamento
//         if( NumeroAleatorio() < probabilidadeCruzamento )
//         {
//            Cruzamento( pai->genoma, mae->genoma, 
//                        descendentes[i].genoma, descendentes[i + 1].genoma );
//         }
//         else // Apenas clonagem
//         {
//            Clona( pai, &descendentes[i] );
//            Clona( mae, &descendentes[i + 1] );
//         }
//
//         // Mutações
//         Mutacao( descendentes[i].genoma );
//         Mutacao( descendentes[i + 1].genoma );
//
//         // Avaliação dos novos indivíduos
//         Avaliacao( &descendentes[i], nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
//         Avaliacao( &descendentes[i + 1], nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
//      }
//
//      // Elitismo
//      if( elitismo ) Clona( &melhorIndividuo, &descendentes[0] );
//
//      // Faz população nova ser a atual, e vice-versa.
//      swap( antecedentes, descendentes );
//
//      // Imprime melhor indivíduo
//      printf( "[Geracao %04d] ", geracao );
//      Imprima( &melhorIndividuo );
//   }
//
//   // -----------------------
//   // Liberação de memória
//   // -----------------------
//   // As variáveis específicas do OpenCL já são automaticamente destruídas
//   for( int i = 0; i < tamanhoPopulacao; ++i )
//   {
//      delete[] populacao_a[i].genoma, populacao_b[i].genoma;
//   }
//
//   delete[] populacao_a, populacao_b, VAR; 
//
//   delete[] melhorIndividuo.genoma;
//
//   fim = clock();
//   double time_total = ((double)(fim - ini))/((double)(CLOCKS_PER_SEC));
//   printf("time_overhead = %lf time_kernel = %lf time_total = %lf\n", time_overhead,time_kernel,time_total);
//
//   return 0;
//}
