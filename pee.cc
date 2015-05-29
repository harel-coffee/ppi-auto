/**********************************************************************/
/*     Nome:Amanda Sabatini Dufek (amandasd@lncc.br)                  */
/*     Professor: Douglas A. Augusto (douglas@lncc.br)                */
/*     Data:10/01/2012                                                */
/*     GB-500-202: Segundo Trabalho Prático                           */
/**********************************************************************/

/* ****************************************************************** */
/* *************************** OBJETIVO ***************************** */
/* ****************************************************************** */
/* Código em OpenCL para o cálculo paralelo do elemento               */
/* i, i=0,...,n-1, de um vetor 'y' como a média aritmética dos        */
/* elementos de um dado vetor 'x' compreendidos entre i-r e i+r,      */
/* excluindo-se i, onde 0<r<n representa o raio da vizinhança.        */
/* rodar ./trabalho tipo n r g                                        */
/* Exemplo: ./trabalho CPU 4194304 10 5                               */
/*          onde g=2^5                                                */
/* ****************************************************************** */


// ------------------------------------------------------------------------
// Habilita disparar exceções C++
#define __CL_ENABLE_EXCEPTIONS

// Cabeçalho OpenCL para C++
#include <cl.hpp>

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

// -----------------------------------------------------------------------------
// Parâmetros
int    numeroGeracoes          = 2000;  // número de iterações (critério de parada)
int    tamanhoPopulacao        = 1500;  // número de indivíduos na população
int    elitismo                = 1;     // 1 = habilita, 0 = desabilita
int    numeroBits              = 300;   // quanto maior, maior tende ser o tamanho do fenótipo ("programa")
int    tamanhoTorneio          = 2;     // 1 = busca aleatória, valores acima aumentam a pressão de seleção
double probabilidadeCruzamento = 0.95;  // 95%
double taxaMutacao             = 0.005; // 0.5% (por bit), geralmente 1/numeroBits 
double intervalo[]             = { -5.0, 5.0 }; // valores mínimo e máximo das constantes efêmeras

// Quantidade de bits para codificar as regras (cada gene)
#define BITS_POR_GENE 8
// Quantidade de bits para representar as constantes reais no intervalo definido
#define BITS_POR_CONSTANTE 32
// Quantidade máxima de símbolos por regra. 
#define MAX_QUANT_SIMBOLOS_POR_REGRA 4
// Ponto de separação entre os símbolos não-terminais e os terminais.
#define TERMINAL_MIN 10000

const unsigned max_tam_fenotipo = MAX_QUANT_SIMBOLOS_POR_REGRA * numeroBits/BITS_POR_GENE;

typedef enum { NT_IF_THEN_ELSE_INICIAL, NT_IF_THEN_ELSE, NT_LOGICO, NT_RELACIONAL, NT_NUMERICO, NT_ATRIB_NUMERICO, NT_CONST_NUMERICO, NT_CLASSE, T_IF_THEN_ELSE = TERMINAL_MIN, T_AND, T_OR, T_NEG, T_MAIOR, T_MAIOR_IGUAL, T_MENOR, T_MENOR_IGUAL, T_IGUAL, T_DIFERENTE, T_0, T_1, T_EFEMERO, T_VAR1, T_VAR2, T_VAR3, T_VAR4, T_VAR5, T_VAR6, T_VAR7, T_VAR8, T_VAR9, T_VAR10, T_VAR11, T_VAR12 } Simbolo; 

// -----------------------------------------------------------------------------
/**
   Estrutura de uma regra, formada pela quantidade de símbolos e os símbolos.
*/
typedef struct { unsigned quantidade; Simbolo simbolos[5]; } t_regra;

// -----------------------------------------------------------------------------
/**
   Estrutura do indivíduo, composto pelo vetor de bits (genoma) e sua aptidão.
*/
typedef struct { int* genoma; double aptidao; } t_individuo;

// ----------------------------------------------------------------------------
// <IF_THEN_ELSE> -> IF_THEN_ELSE <LOGICO> <IF_THEN_ELSE_OPTION> <IF_THEN_ELSE_OPTION> (0)
t_regra IF_THEN_ELSE_INICIAL_0 = { 4, {T_IF_THEN_ELSE, NT_LOGICO, NT_IF_THEN_ELSE, NT_IF_THEN_ELSE} };

// <IF_THEN_ELSE> -> <CLASSE>                                            (0)
// <IF_THEN_ELSE> -> IF_THEN_ELSE <LOGICO> <IF_THEN_ELSE> <IF_THEN_ELSE> (1)
t_regra IF_THEN_ELSE_0 = { 1, {NT_CLASSE}                                                   };
t_regra IF_THEN_ELSE_1 = { 4, {T_IF_THEN_ELSE, NT_LOGICO, NT_IF_THEN_ELSE, NT_IF_THEN_ELSE} };

// <LOGICO> -> AND <LOGICO> <LOGICO> (0)
// <LOGICO> -> OR  <LOGICO> <LOGICO> (1)
// <LOGICO> -> NEG <LOGICO>          (2)
// <LOGICO> -> <RELACIONAL>          (3)
t_regra LOGICO_0 = { 3, {T_AND, NT_LOGICO, NT_LOGICO} };
t_regra LOGICO_1 = { 3, {T_OR, NT_LOGICO, NT_LOGICO}  };
t_regra LOGICO_2 = { 2, {T_NEG, NT_LOGICO}            };
t_regra LOGICO_3 = { 1, {NT_RELACIONAL}               };

// <RELACIONAL> -> MAIOR       <NUMERICO> <NUMERICO> (0)
// <RELACIONAL> -> MAIOR-IGUAL <NUMERICO> <NUMERICO> (1)
// <RELACIONAL> -> MENOR       <NUMERICO> <NUMERICO> (2)
// <RELACIONAL> -> MENOR-IGUAL <NUMERICO> <NUMERICO> (3)
// <RELACIONAL> -> IGUAL       <NUMERICO> <NUMERICO> (4)
// <RELACIONAL> -> DIFERENTE   <NUMERICO> <NUMERICO> (5)
t_regra RELACIONAL_0 = { 3, {T_MAIOR, NT_NUMERICO, NT_NUMERICO}       };
t_regra RELACIONAL_1 = { 3, {T_MAIOR_IGUAL, NT_NUMERICO, NT_NUMERICO} };
t_regra RELACIONAL_2 = { 3, {T_MENOR, NT_NUMERICO, NT_NUMERICO}       };
t_regra RELACIONAL_3 = { 3, {T_MENOR_IGUAL, NT_NUMERICO, NT_NUMERICO} };
t_regra RELACIONAL_4 = { 3, {T_IGUAL, NT_NUMERICO, NT_NUMERICO}       };
t_regra RELACIONAL_5 = { 3, {T_DIFERENTE, NT_NUMERICO, NT_NUMERICO}   };

// <NUMERICO> -> <ATRIB_NUMERICO>                    (0)
// <NUMERICO> -> <CONST_NUMERICO>                    (1)
t_regra NUMERICO_0 = { 1, {NT_ATRIB_NUMERICO} };
t_regra NUMERICO_1 = { 1, {NT_CONST_NUMERICO} };

// <ATRIB_NUMERICO> -> VAR1 (0)
t_regra ATRIB_NUMERICO_0  = { 1, {T_VAR1}  };
t_regra ATRIB_NUMERICO_1  = { 1, {T_VAR2}  };
t_regra ATRIB_NUMERICO_2  = { 1, {T_VAR3}  };
t_regra ATRIB_NUMERICO_3  = { 1, {T_VAR4}  };
t_regra ATRIB_NUMERICO_4  = { 1, {T_VAR5}  };
t_regra ATRIB_NUMERICO_5  = { 1, {T_VAR6}  };
t_regra ATRIB_NUMERICO_6  = { 1, {T_VAR7}  };
t_regra ATRIB_NUMERICO_7  = { 1, {T_VAR8}  };
t_regra ATRIB_NUMERICO_8  = { 1, {T_VAR9}  };
t_regra ATRIB_NUMERICO_9  = { 1, {T_VAR10} };
t_regra ATRIB_NUMERICO_10 = { 1, {T_VAR11} };
t_regra ATRIB_NUMERICO_11 = { 1, {T_VAR12} };

// <CONST_NUMERICO> -> EFEMERO (0)
t_regra CONST_NUMERICO_0 = { 1, {T_EFEMERO} };

// <CLASSE> -> 0 (0)
// <CLASSE> -> 1 (1)
t_regra CLASSE_0 = { 1, {T_0} };
t_regra CLASSE_1 = { 1, {T_1} };

// Conjunto de regras
t_regra* regras_IF_THEN_ELSE_INICIAL[] = { &IF_THEN_ELSE_INICIAL_0 };
t_regra* regras_IF_THEN_ELSE[]         = { &IF_THEN_ELSE_0, &IF_THEN_ELSE_1 };
t_regra* regras_LOGICO[]               = { &LOGICO_0, &LOGICO_1, &LOGICO_2, &LOGICO_3 };
t_regra* regras_RELACIONAL[]           = { &RELACIONAL_0, &RELACIONAL_1, &RELACIONAL_2, &RELACIONAL_3, &RELACIONAL_4, &RELACIONAL_5 };
t_regra* regras_NUMERICO[]             = { &NUMERICO_0, &NUMERICO_1 };
t_regra* regras_CONST_NUMERICO[]       = { &CONST_NUMERICO_0 };
t_regra* regras_CLASSE[]               = { &CLASSE_0, &CLASSE_1 };
t_regra* regras_ATRIB_NUMERICO[]       = { &ATRIB_NUMERICO_0, &ATRIB_NUMERICO_1, &ATRIB_NUMERICO_2, &ATRIB_NUMERICO_3, &ATRIB_NUMERICO_4, &ATRIB_NUMERICO_5, &ATRIB_NUMERICO_6, &ATRIB_NUMERICO_7, &ATRIB_NUMERICO_8, &ATRIB_NUMERICO_9, &ATRIB_NUMERICO_10, &ATRIB_NUMERICO_11 };

// A gramática em si (conjunto de todas as regras)
t_regra** gramatica[] = { regras_IF_THEN_ELSE_INICIAL, regras_IF_THEN_ELSE, regras_LOGICO, regras_RELACIONAL, regras_NUMERICO, regras_ATRIB_NUMERICO, regras_CONST_NUMERICO, regras_CLASSE };
unsigned  tamanhos[] =  { sizeof(regras_IF_THEN_ELSE_INICIAL) / sizeof(t_regra*),
                          sizeof(regras_IF_THEN_ELSE)         / sizeof(t_regra*), 
                          sizeof(regras_LOGICO)               / sizeof(t_regra*),
                          sizeof(regras_RELACIONAL)           / sizeof(t_regra*),
                          sizeof(regras_NUMERICO)             / sizeof(t_regra*),
                          sizeof(regras_ATRIB_NUMERICO)       / sizeof(t_regra*),
                          sizeof(regras_CONST_NUMERICO)       / sizeof(t_regra*),
                          sizeof(regras_CLASSE)               / sizeof(t_regra*) };

// Simbolo inicial
Simbolo simboloInicial = NT_IF_THEN_ELSE_INICIAL;
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
/** 
   Variável global que guarda o melhor indivíduo até então. 
*/
t_individuo melhorIndividuo = { NULL, 0.0 };

// -----------------------------------------------------------------------------
/**
  Função auxiliar que faz a população recém criada se tornar a população atual
  e vice-versa (troca-se apenas os ponteiros, evitando-se cópias bit-a-bit).
*/
#define swap(i, j) {t_individuo* t = i; i = j; j = t;}

// -----------------------------------------------------------------------------
/**
  Retorna um valor real pseudo aleatório entre 0.0 e 1.0 (exclusive).
*/
double NumeroAleatorio()
{
   return rand() / (RAND_MAX + 1.0); // [0.0, 1.0)
}

// -----------------------------------------------------------------------------
/**
  Decodifica uma regra codificada em bits a partir de um dado alelo.
*/
t_regra* DecodificaRegra( const int* genoma, int* const alelo, Simbolo cabeca )
{
   // Verifica se é possível decodificar mais uma regra, caso contrário
   // retorna uma regra nula.
   if( *alelo + BITS_POR_GENE > numeroBits ) { return NULL; }

   // Número de regras encabeçada por "cabeça"
   unsigned num_regras = tamanhos[cabeca];

   // Converte BITS_POR_GENE bits em um valor integral
   unsigned valor_bruto = 0;
   for( int i = 0; i < BITS_POR_GENE; ++i )  
      if( genoma[(*alelo)++] ) valor_bruto += pow( 2, i );

   // Seleciona uma regra no intervalo [0, num_regras - 1]
   return gramatica[cabeca][valor_bruto % num_regras];
}

// -----------------------------------------------------------------------------
/**
  Decodifica uma sequência de bits em um valor real em um certo intervalo.
*/
double DecodificaReal( const int* genoma, int* const alelo )
{
   // Retorna a constante zero se não há número suficiente de bits
   if( *alelo + BITS_POR_CONSTANTE > numeroBits ) { return 0.; }

   // Converte BITS_POR_CONSTANTE bits em um valor real
   double valor_bruto = 0.;
   for( int i = 0; i < BITS_POR_CONSTANTE; ++i ) 
      if( genoma[(*alelo)++] ) valor_bruto += pow( 2.0, i );

   // Normalizar para o intervalo desejado: a + valor_bruto * (b - a)/(2^n - 1)
   return intervalo[0] + valor_bruto * (intervalo[1] - intervalo[0]) / 
          (pow( 2.0, BITS_POR_CONSTANTE ) - 1);
}

// -----------------------------------------------------------------------------
/**
  Decodifica o genoma, expresso por bits, em um programa (fenótipo).
*/
int Decodifica( const int* genoma, int* const alelo, Simbolo* fenotipo, float* efemeros, 
                int pos, Simbolo simbolo_inicial )
{
   t_regra* r = DecodificaRegra( genoma, alelo, simbolo_inicial ); 
   if( !r ) { return 0; }

   for( int i = 0; i < r->quantidade; ++i )
      if( r->simbolos[i] >= TERMINAL_MIN )
      {
         fenotipo[pos] = r->simbolos[i];

         // Tratamento especial para constantes efêmeras
         if( r->simbolos[i] == T_EFEMERO )
         {
           /* 
              Esta estratégia faz uso do próprio código genético do indivíduo 
              para extrair uma constante real. Extrai-se BITS_POR_CONSTANTE bits a 
              partir da posição atual e os decodifica como um valor real.
            */
            efemeros[pos] = DecodificaReal( genoma, alelo );
         }
         ++pos;
      }
      else
      {
         pos = Decodifica( genoma, alelo, fenotipo, efemeros, pos, r->simbolos[i] );
         if( !pos ) return 0;
      }

   return pos;
}

// -----------------------------------------------------------------------------
/**
  Copia genoma e aptidão do indivíduo 'original' para o indivíduo 'clone'.
*/
void Clona( const t_individuo* original, t_individuo* clone )
{
   for( int i = 0; i < numeroBits; ++i ) clone->genoma[i] = original->genoma[i];

   clone->aptidao = original->aptidao;
}

// -----------------------------------------------------------------------------
/**
  Seleciona um indivíduo com base em competições (aquele que tiver a maior 
  aptidão será o selecionado ("vencedor").
*/
const t_individuo* Torneio( const t_individuo* populacao )
{
   const t_individuo* vencedor = 
                        &populacao[(int)(NumeroAleatorio() * tamanhoPopulacao)];

   for( int t = 1; t < tamanhoTorneio; ++t )
   {
      const t_individuo* competidor = 
                        &populacao[(int)(NumeroAleatorio() * tamanhoPopulacao)];

      if( competidor->aptidao > vencedor->aptidao ) vencedor = competidor;
   }

   return vencedor;
}

// -----------------------------------------------------------------------------
/**
  Combina os genomas do pai e da mãe, gerando os genomas alocados em filho1
  e filho2.
*/
void Cruzamento( const int* pai, const int* mae, int* filho1, int* filho2 )
{
   // Cruzamento de um ponto
   int pontoDeCruzamento = (int)(NumeroAleatorio() * numeroBits);

   for( int i = 0; i < pontoDeCruzamento; ++i )
   {
      filho1[i] = pai[i];
      filho2[i] = mae[i];
   }
   for( int i = pontoDeCruzamento; i < numeroBits; ++i )
   {
      filho1[i] = mae[i];
      filho2[i] = pai[i];
   }
}

// -----------------------------------------------------------------------------
/**
  Percorre o genoma e com certa taxa troca aleatoriamente alguns bits.
*/
void Mutacao( int* genoma )
{
   for( int i = 0; i < numeroBits; ++i )
      if( NumeroAleatorio() < taxaMutacao ) genoma[i] = !genoma[i];
}

// -----------------------------------------------------------------------------
/**
  Imprime o fenótipo e aptidão do indivíduo.
*/
void Imprima( const t_individuo* individuo )
{
   const unsigned max_tam_fenotipo = MAX_QUANT_SIMBOLOS_POR_REGRA * numeroBits/BITS_POR_GENE;

   // -----------------------------------------------------------------------------
   // Decodificação
   Simbolo fenotipo[max_tam_fenotipo];
   float  efemeros[max_tam_fenotipo];

   int alelo = 0;
   int tam = Decodifica( individuo->genoma, &alelo, fenotipo, efemeros, 0, simboloInicial );
   if( !tam ) { return; }

   // -----------------------------------------------------------------------------
   printf( "{%d} ", tam );
   for( int i = 0; i < tam; ++i )
      switch( fenotipo[i] )
      {
            case T_IF_THEN_ELSE:
               printf( "IF-THEN-ELSE " );
               break;
            case T_AND:
               printf( "AND " );
               break;
            case T_OR:
               printf( "OR " );
               break;
            case T_NEG:
               printf( "NEG " );
               break;
            case T_MAIOR:
               printf( "> " );
               break;
            case T_MAIOR_IGUAL:
               printf( ">= " );
               break;
            case T_MENOR:
               printf( "< " );
               break;
            case T_MENOR_IGUAL:
               printf( "<= " );
               break;
            case T_IGUAL:
               printf( "= " );
               break;
            case T_DIFERENTE:
               printf( "!= " );
               break;
            case T_0:
               printf( "0 " );
               break;
            case T_1:
               printf( "1 " );
               break;
            case T_VAR1:
               printf( "VAR1 " );
               break;
            case T_VAR2:
               printf( "VAR2 " );
               break;
            case T_VAR3:
               printf( "VAR3 " );
               break;
            case T_VAR4:
               printf( "VAR4 " );
               break;
            case T_VAR5:
               printf( "VAR5 " );
               break;
            case T_VAR6:
               printf( "VAR6 " );
               break;
            case T_VAR7:
               printf( "VAR7 " );
               break;
            case T_VAR8:
               printf( "VAR8 " );
               break;
            case T_VAR9:
               printf( "VAR9 " );
               break;
            case T_VAR10:
               printf( "VAR10 " );
               break;
            case T_VAR11:
               printf( "VAR11 " );
               break;
            case T_VAR12:
               printf( "VAR12 " );
               break;
            case T_EFEMERO:
               printf( "%.12f ",  efemeros[i] );
               break;
      } 
   printf( " [Aptidao: %.12f]\n", individuo->aptidao );
}

// -----------------------------------------------------------------------------
/**
  Decodifica o genoma, expresso por bits, em um programa (fenótipo).
*/
//void Avaliacao( t_individuo* individuo, int nlinha , double time_kernel, double time_overhead )
//void Avaliacao( t_individuo* individuo, int nlinha )
void Avaliacao( t_individuo* individuo, int nlinha, int local_size, double* time_kernel, double* time_overhead, cl::Buffer bufferFenotipo, cl::Buffer bufferEfemeros, cl::Buffer bufferE, cl::Kernel kernel, cl::CommandQueue fila )
{
   Simbolo fenotipo[max_tam_fenotipo];
   float  efemeros[max_tam_fenotipo];

   //float *E = new float[nlinha / local_size];

   int alelo = 0;
   int size = Decodifica( individuo->genoma, &alelo, fenotipo, efemeros, 0, simboloInicial );
   if( !size ) { individuo->aptidao = 0.0; return; }

   vector<cl::Event> eventos(3); 

   // 3) Execução
   // Transferência de dados para o dispositivo
   fila.enqueueWriteBuffer( bufferFenotipo, CL_TRUE, 0, size * sizeof( Simbolo ), fenotipo, NULL, &eventos[1] );
   fila.enqueueWriteBuffer( bufferEfemeros, CL_TRUE, 0, size * sizeof( float ), efemeros, NULL, &eventos[2] );

   // Execução do kernel: definição dos argumentos e trabalho/particionamento
   kernel.setArg( 6, size );

   fila.enqueueNDRangeKernel( kernel, cl::NDRange(), cl::NDRange( nlinha ), cl::NDRange( local_size ), NULL, &eventos[0] );
  
   // Espera pela finalização da execução do kernel (forçar sincronia)
   fila.finish();

   cl_ulong inicio, final;
   eventos[0].getProfilingInfo( CL_PROFILING_COMMAND_START, &inicio );
   eventos[0].getProfilingInfo( CL_PROFILING_COMMAND_END, &final );
   //double time = (final - inicio)/1.0E9;
   *time_kernel += (final - inicio)/1.0E9;
   //printf("time_kernel = %.10lf time = %.10lf\n", *time_kernel,time);

   eventos[1].getProfilingInfo( CL_PROFILING_COMMAND_START, &inicio );
   eventos[1].getProfilingInfo( CL_PROFILING_COMMAND_END, &final );
   //double time1 = (final - inicio)/1.0E9;
   *time_overhead += (final - inicio)/1.0E9;
   //printf("time_overhead = %.10f time1 = %.10f\n", *time_overhead,time1);
   eventos[2].getProfilingInfo( CL_PROFILING_COMMAND_START, &inicio );
   eventos[2].getProfilingInfo( CL_PROFILING_COMMAND_END, &final );
   //double time2 = (final - inicio)/1.0E9;
   *time_overhead += (final - inicio)/1.0E9;
   //printf("time_overhead = %.10f time2 = %.10f\n", *time_overhead,time2);
 
   // Transferência dos resultados para o hospedeiro (e joga no Y)
   float *E = (float*) fila.enqueueMapBuffer( bufferE, CL_TRUE, CL_MAP_READ, 0, nlinha / local_size * sizeof( float ) );
   float sum = 0.;
   for( int k = 0; k < nlinha / local_size; k++ ) sum += E[k];
 
   // -----------------------------------------------------------------------------
   // Verifica se o fenótipo é matematicamente inválido, se for então atribuir
   // a pior aptidão ao indivíduo:
   if( isnan( sum ) || isinf( sum ) ) { individuo->aptidao = 0.; return; }

   // Transformação do acerto em aptidão (são inversamente proporcionais)
   individuo->aptidao = sum / (nlinha / local_size);

   // Verifica se a aptidão é a melhor até então
   if( individuo->aptidao > melhorIndividuo.aptidao )
   {
      Clona( individuo, &melhorIndividuo );
   }
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
int main( int argc, char* argv[] )
{

   clock_t ini,fim;
   ini = clock();

   // Teste para verificar se o número de elementos foi informado
   if( argc < 4 )
   {
      cerr << "ERRO: Especificar a arquitetura do dispositivo.\n";
      return 1;
   }

   int tipo;
   if( !strcmp(argv[1],"CPU") ) tipo = CL_DEVICE_TYPE_CPU;
   else tipo = CL_DEVICE_TYPE_GPU;

   const unsigned local_size = atoi( argv[2] );

   char *arquivo;
   arquivo = argv[3];
   //strcat(arquivo,"/treinamento.txt");
   //char arquivo[30] = "arquivo1/treinamento.txt";

   FILE *arqentra;
   arqentra = fopen(arquivo,"r");
   if(arqentra == NULL) 
   {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }

   char linha[100];
   int nlinha;
   for( nlinha = 0; fgets( linha,100,arqentra ) != NULL; nlinha++ );
   float *VAR   = new float[nlinha * 13];
   fclose (arqentra);

   arqentra = fopen(arquivo,"r");
   if(arqentra == NULL) 
   {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }

   for( int i = 0; i < nlinha; i++ )
   {
      for( int j = 0; j < 13; j++ )
      {
         fscanf(arqentra,"%f",&VAR[j * 13 + i]);
      }
   }
   fclose (arqentra);

   // Inicia o gerador de números aleatórios com uma semente "aleatória"
   srand( time(NULL) );

   // -----------------------
   // Alocação de memórias
   // -----------------------
   t_individuo* populacao_a = new t_individuo[tamanhoPopulacao];
   t_individuo* populacao_b = new t_individuo[tamanhoPopulacao];

   melhorIndividuo.genoma = new int[numeroBits];

   // Alocação dos indivíduos
   for( int i = 0; i < tamanhoPopulacao; ++i )
   {
      populacao_a[i].genoma = new int[numeroBits];
      populacao_b[i].genoma = new int[numeroBits];
   }

   t_individuo* antecedentes = populacao_a;
   t_individuo* descendentes = populacao_b;

   // 1) Inicialização
   // Descobrir e escolher as plataformas
   vector<cl::Platform> plataformas;

   // Descobre as plataformas instaladas no hospedeiro
   cl::Platform::get( &plataformas );

   vector<cl::Device> dispositivo(1);
   for( int m = 0; m < plataformas.size(); m++ )
   {
     // Descobrir os dispositivos
     vector<cl::Device> dispositivos;
     // Descobre os dispositivos da plataforma i
     plataformas[m].getDevices( CL_DEVICE_TYPE_ALL, &dispositivos );
     for ( int n = 0; n < dispositivos.size(); n++ )
       if ( dispositivos[n].getInfo<CL_DEVICE_TYPE>() == tipo ) 
         dispositivo[0] = dispositivos[n];
   }

   // Criar o contexto
   cl::Context contexto( dispositivo );

   // Criar a fila de comandos para um dispositivo (aqui só o primeiro)
   cl::CommandQueue fila( contexto, dispositivo[0], CL_QUEUE_PROFILING_ENABLE );

   // Carregar o programa, compilá-lo e gerar o kernel
   cl::Program::Sources fonte( 1, make_pair( kernel_str, strlen( kernel_str ) ) );
   cl::Program programa( contexto, fonte );

   // Compila para todos os dispositivos associados a 'programa' através do
   // 'contexto': vector<cl::Device>() é um vetor nulo
   char buildOptions[60];
   sprintf( buildOptions, "-DTAM_MAX=%u -DTERMINAL_MIN=%u", max_tam_fenotipo, TERMINAL_MIN );  
   try {
      programa.build( vector<cl::Device>(), buildOptions );
   } catch( cl::Error )
   {
     cout << "Build Log:\t " << programa.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dispositivo[0]) << std::endl;
   }

   // Cria a variável kernel que vai representar o kernel "avalia"
   cl::Kernel kernel( programa, "avalia" );

   cl::Event e_tempo;

   // 2) Preparação da memória dos dispositivos (leitura e escrita}
   cl::Buffer bufferFenotipo( contexto, CL_MEM_READ_ONLY, max_tam_fenotipo * sizeof( Simbolo ) );
   cl::Buffer bufferEfemeros( contexto, CL_MEM_READ_ONLY, max_tam_fenotipo * sizeof( float ) );
   cl::Buffer bufferVAR( contexto, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, nlinha * 13 * sizeof( float ), VAR, NULL, &e_tempo );
   cl::Buffer bufferE( contexto, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, nlinha / local_size * sizeof( float ) );

   // 3) Execução
   // Transferência de dados para o dispositivo
   //fila.enqueueWriteBuffer( bufferVAR, CL_TRUE, 0, nlinha * 13 * sizeof( float ), VAR, NULL, &e_tempo );

   cl_ulong inicio, final;
   e_tempo.getProfilingInfo( CL_PROFILING_COMMAND_START, &inicio );
   e_tempo.getProfilingInfo( CL_PROFILING_COMMAND_END, &final );
   double time_overhead = (final - inicio)/1.0E9;
   //printf("time_overhead = %f\n", time_overhead);

   // Execução do kernel: definição dos argumentos e trabalho/particionamento
   kernel.setArg( 0, bufferFenotipo );
   kernel.setArg( 1, bufferEfemeros );
   kernel.setArg( 2, bufferVAR );
   kernel.setArg( 3, bufferE );
   kernel.setArg( 4, sizeof( float ) * local_size, NULL );
   kernel.setArg( 5, nlinha );

   // Criação da população inicial (1ª geração)
   double time_kernel;
   InicializaPopulacao( antecedentes, nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );


   // Processo evolucionário
   for( int geracao = 1; geracao <= numeroGeracoes; ++geracao )
   {
      for( int i = 0; i < tamanhoPopulacao; i += 2 )
      {
         // Seleção de indivíduos adaptados para gerar descendentes
         const t_individuo* pai = Torneio( antecedentes );
         const t_individuo* mae = Torneio( antecedentes );

         // Cruzamento
         if( NumeroAleatorio() < probabilidadeCruzamento )
         {
            Cruzamento( pai->genoma, mae->genoma, 
                        descendentes[i].genoma, descendentes[i + 1].genoma );
         }
         else // Apenas clonagem
         {
            Clona( pai, &descendentes[i] );
            Clona( mae, &descendentes[i + 1] );
         }

         // Mutações
         Mutacao( descendentes[i].genoma );
         Mutacao( descendentes[i + 1].genoma );

         // Avaliação dos novos indivíduos
         Avaliacao( &descendentes[i], nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
         Avaliacao( &descendentes[i + 1], nlinha, local_size, &time_kernel, &time_overhead, bufferFenotipo, bufferEfemeros, bufferE, kernel, fila );
      }

      // Elitismo
      if( elitismo ) Clona( &melhorIndividuo, &descendentes[0] );

      // Faz população nova ser a atual, e vice-versa.
      swap( antecedentes, descendentes );

      // Imprime melhor indivíduo
      printf( "[Geracao %04d] ", geracao );
      Imprima( &melhorIndividuo );
   }

   // -----------------------
   // Liberação de memória
   // -----------------------
   // As variáveis específicas do OpenCL já são automaticamente destruídas
   for( int i = 0; i < tamanhoPopulacao; ++i )
   {
      delete[] populacao_a[i].genoma, populacao_b[i].genoma;
   }

   delete[] populacao_a, populacao_b, VAR; 

   delete[] melhorIndividuo.genoma;

   fim = clock();
   double time_total = ((double)(fim - ini))/((double)(CLOCKS_PER_SEC));
   printf("time_overhead = %lf time_kernel = %lf time_total = %lf\n", time_overhead,time_kernel,time_total);

   return 0;
}
