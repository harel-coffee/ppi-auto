/************************************************************************/
/**     Name:Amanda Sabatini Dufek                                     **/
/**          Douglas Adriano Augusto            Date:01/06/2015        **/
/**     Parallel Ensemble Evolution (PEE)                              **/
/************************************************************************/

/** ****************************************************************** **/
/** *************************** OBJECTIVE **************************** **/
/** ****************************************************************** **/
/**                                                                    **/
/** ****************************************************************** **/

#include <stdio.h> 
#include <stdlib.h>
#include <cmath>    
#include <limits>
#include "interpret.h"
#include "pee.h"
#include "grammar"
#include "parametros"

#define BITS_BY_GENE 8
#define BITS_BY_CONSTANT 16


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct t_individual { int* genome; double fitness; };

static struct t_data { Symbol initial_symbol; Individual best_individual; unsigned max_size_phenotype; int nlin; } data;


/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

#define swap(i, j) {Individual* t = i; i = j; j = t;}

double random_number() {return rand() / (RAND_MAX + 1.0);} // [0.0, 1.0)

t_rule* decode_rule( const int* genome, int* const allele, Symbol cabeca )
{
   // Verifica se é possível decodificar mais uma regra, caso contrário
   // retorna uma regra nula.
   if( *allele + BITS_BY_GENE > bits_number ) { return NULL; }

   // Número de regras encabeçada por "cabeça"
   unsigned num_regras = tamanhos[cabeca];

   // Converte BITS_BY_GENE bits em um valor integral
   unsigned valor_bruto = 0;
   for( int i = 0; i < BITS_BY_GENE; ++i )  
      if( genome[(*allele)++] ) valor_bruto += pow( 2, i );

   // Seleciona uma regra no intervalo [0, num_regras - 1]
   return gramatica[cabeca][valor_bruto % num_regras];
}

double decode_real( const int* genome, int* const allele )
{
   // Retorna a constante zero se não há número suficiente de bits
   if( *allele + BITS_BY_CONSTANT > bits_number ) { return 0.; }

   // Converte BITS_BY_CONSTANT bits em um valor real
   double valor_bruto = 0.;
   for( int i = 0; i < BITS_BY_CONSTANT; ++i ) 
      if( genome[(*allele)++] ) valor_bruto += pow( 2.0, i );

   // Normalizar para o intervalo desejado: a + valor_bruto * (b - a)/(2^n - 1)
   return interval[0] + valor_bruto * (interval[1] - interval[0]) / 
          (pow( 2.0, BITS_BY_CONSTANT ) - 1);
}

int decode( const int* genome, int* const allele, Symbol* phenotype, double* ephemeral, int pos, Symbol initial_symbol )
{
   t_rule* r = decode_rule( genome, allele, initial_symbol ); 
   if( !r ) { return 0; }

   for( int i = 0; i < r->quantity; ++i )
      if( r->symbols[i] >= TERMINAL_MIN )
      {
         phenotype[pos] = r->symbols[i];

         // Tratamento especial para constantes efêmeras
         if( r->symbols[i] == T_EFEMERO )
         {
           /* 
              Esta estratégia faz uso do próprio código genético do indivíduo 
              para extrair uma constante real. Extrai-se BITS_BY_CONSTANT bits a 
              partir da posição atual e os decodifica como um valor real.
            */
            ephemeral[pos] = decode_real( genome, allele );
         }
         ++pos;
      }
      else
      {
         pos = decode( genome, allele, phenotype, ephemeral, pos, r->symbols[i] );
         if( !pos ) return 0;
      }

   return pos;
}


/** ****************************************************************** **/
/** ************************* MAIN FUNCTIONS ************************* **/
/** ****************************************************************** **/

void init( double** input, double** model, double* obs, int nlin, int ninput, int nmodel ) 
{
   data.initial_symbol = NT_IF_THEN_ELSE_INICIAL;
   data.best_individual.genome = NULL;
   data.best_individual.fitness = std::numeric_limits<float>::max();
   data.max_size_phenotype = MAX_QUANT_SIMBOLOS_POR_REGRA * bits_number/BITS_BY_GENE;
   data.nlin = nlin;

   interpret_init( data.max_size_phenotype, input, model, obs, nlin, ninput, nmodel );
}

void evaluate( Individual* individual )
{
   Symbol phenotype[data.max_size_phenotype];
   double  ephemeral[data.max_size_phenotype];

   int allele = 0;
   int size = decode( individual->genome, &allele, phenotype, ephemeral, 0, data.initial_symbol );
   if( !size ) { individual->fitness = std::numeric_limits<float>::max(); return; }

   double erro = interpret(phenotype, ephemeral, size );

   if( isnan( erro ) || isinf( erro ) ) { individual->fitness = std::numeric_limits<float>::max(); return; } 

   individual->fitness = erro/data.nlin + 0.00001*size; 

   if( individual->fitness < data.best_individual.fitness )
   {
      clone( individual, &data.best_individual );
   }
}

void individual_print( const Individual* individual, FILE* out )
{
   Symbol phenotype[data.max_size_phenotype];
   double ephemeral[data.max_size_phenotype];

   int allele = 0;
   int size = decode( individual->genome, &allele, phenotype, ephemeral, 0, data.initial_symbol );
   if( !size ) { return; }

   fprintf( out, "{%d} ", size );
   for( int i = 0; i < size; ++i )
      switch( phenotype[i] )
      {
            case T_IF_THEN_ELSE:
               fprintf( out, "IF-THEN-ELSE " );
               break;
            case T_AND:
               fprintf( out, "AND " );
               break;
            case T_OR:
               fprintf( out, "OR " );
               break;
            case T_NOT:
               fprintf( out, "NOT " );
               break;
            case T_MAIOR:
               fprintf( out, "> " );
               break;
            case T_MENOR:
               fprintf( out, "< " );
               break;
            case T_IGUAL:
               fprintf( out, "= " );
               break;
            case T_ADD:
               fprintf( out, "+ " );
               break;
            case T_SUB:
               fprintf( out, "- " );
               break;
            case T_MULT:
               fprintf( out, "* " );
               break;
            case T_DIV:
               fprintf( out, "/ " );
               break;
            case T_MEAN:
               fprintf( out, "MEAN " );
               break;
            case T_MAX:
               fprintf( out, "MAX " );
               break;
            case T_MIN:
               fprintf( out, "MIN " );
               break;
            case T_ABS:
               fprintf( out, "ABS " );
               break;
            case T_SQRT:
               fprintf( out, "SQRT " );
               break;
            case T_POW2:
               fprintf( out, "POW2 " );
               break;
            case T_POW3:
               fprintf( out, "POW3 " );
               break;
            case T_NEG:
               fprintf( out, "NEG " );
               break;
            case T_BMA:
               fprintf( out, "BMA " );
               break;
            case T_CHUVA_ONTEM:
               fprintf( out, "CHUVA_ONTEM " );
               break;
            case T_CHUVA_ANTEONTEM:
               fprintf( out, "CHUVA_ANTEONTEM " );
               break;
            case T_MEAN_MODELO:
               fprintf( out, "MEAN_MODELO " );
               break;
            case T_MAX_MODELO:
               fprintf( out, "MAX_MODELO " );
               break;
            case T_MIN_MODELO:
               fprintf( out, "MIN_MODELO " );
               break;
            case T_STD_MODELO:
               fprintf( out, "STD_MODELO " );
               break;
            case T_CHOVE:
               fprintf( out, "CHOVE " );
               break;
            case T_CHUVA_LAG1P:
               fprintf( out, "CHUVA_LAG1P " );
               break;
            case T_CHUVA_LAG1N:
               fprintf( out, "CHUVA_LAG1N " );
               break;
            case T_CHUVA_LAG2P:
               fprintf( out, "CHUVA_LAG2P " );
               break;
            case T_CHUVA_LAG2N:
               fprintf( out, "CHUVA_LAG2N " );
               break;
            case T_CHUVA_LAG3P:
               fprintf( out, "CHUVA_LAG3P " );
               break;
            case T_CHUVA_LAG3N:
               fprintf( out, "CHUVA_LAG3N " );
               break;
            case T_PADRAO_MUDA:
               fprintf( out, "PADRAO_MUDA " );
               break;
            case T_PERTINENCIA:
               fprintf( out, "PERTINENCIA " );
               break;
            case T_CHUVA_PADRAO:
               fprintf( out, "CHUVA_PADRAO " );
               break;
            case T_CHUVA_HISTORICA:
               fprintf( out, "CHUVA_HISTORICA " );
               break;
            case T_K:
               fprintf( out, "K " );
               break;
            case T_TT:
               fprintf( out, "TT " );
               break;
            case T_SWEAT:
               fprintf( out, "SWEAT " );
               break;
            case T_PAD:
               fprintf( out, "PAD " );
               break;
            case T_MOD1:
               fprintf( out, "MOD " );
               break;
            case T_MOD2:
               fprintf( out, "MOD2 " );
               break;
            case T_MOD3:
               fprintf( out, "MOD3 " );
               break;
            case T_MOD4:
               fprintf( out, "MOD4 " );
               break;
            case T_MOD5:
               fprintf( out, "MOD5 " );
               break;
            case T_MOD6:
               fprintf( out, "MOD6 " );
               break;
            case T_MOD7:
               fprintf( out, "MOD7 " );
               break;
            case T_MOD8:
               fprintf( out, "MOD8 " );
               break;
            case T_1:
               fprintf( out, "1 " );
               break;
            case T_2:
               fprintf( out, "2 " );
               break;
            case T_3:
               fprintf( out, "3 " );
               break;
            case T_4:
               fprintf( out, "4 " );
               break;
            case T_EFEMERO:
               fprintf( out, "%.12f ",  ephemeral[i] );
               break;
      } 
   fprintf( out, " [Aptidao: %.12f]\n", individual->fitness );
}

void generate_population( Individual* population )
{
   for( int i = 0; i < population_size; ++i)
   {
      for( int j = 0; j < bits_number; j++ )
      {
         population[i].genome[j] = (random_number() < 0.5) ? 1 : 0;
      }
      
      evaluate( &population[i] );
   }
}

void crossover( const int* father, const int* mother, int* offspring1, int* offspring2 )
{
#ifdef TWO_POINT_CROSSOVER
   // Cruzamento de dois pontos
   int pontos[2];

   // Cortar apenas nas bordas dos genes
   pontos[0] = ((int)(random_number() * bits_number))/BITS_BY_GENE * BITS_BY_GENE;
   pontos[1] = ((int)(random_number() * bits_number))/BITS_BY_GENE * BITS_BY_GENE;

   int tmp;
   if( pontos[0] > pontos[1] ) { tmp = pontos[0]; pontos[0] = pontos[1]; pontos[1] = tmp; }

   for( int i = 0; i < pontos[0]; ++i )
   {
      offspring1[i] = father[i];
      offspring2[i] = mother[i];
   }
   for( int i = pontos[0]; i < pontos[1]; ++i )
   {
      offspring1[i] = mother[i];
      offspring2[i] = father[i];
   }
   for( int i = pontos[1]; i < bits_number; ++i )
   {
      offspring1[i] = father[i];
      offspring2[i] = mother[i];
   }
#else
   // Cruzamento de um ponto
   int pontoDeCruzamento = (int)(random_number() * bits_number);

   for( int i = 0; i < pontoDeCruzamento; ++i )
   {
      offspring1[i] = father[i];
      offspring2[i] = mother[i];
   }
   for( int i = pontoDeCruzamento; i < bits_number; ++i )
   {
      offspring1[i] = mother[i];
      offspring2[i] = father[i];
   }
#endif
}

void mutation( int* genome )
{
   for( int i = 0; i < bits_number; ++i )
      if( random_number() < mutation_rate ) genome[i] = !genome[i];
}

void clone( const Individual* original, Individual* copy )
{
   for( int i = 0; i < bits_number; ++i ) copy->genome[i] = original->genome[i];

   copy->fitness = original->fitness;
}

const Individual* tournament( const Individual* population )
{
   const Individual* vencedor = &population[(int)(random_number() * population_size)];

   for( int t = 1; t < tournament_size; ++t )
   {
      const Individual* competidor = &population[(int)(random_number() * population_size)];

      if( competidor->fitness < vencedor->fitness ) vencedor = competidor;
   }

   return vencedor;
}

Individual evolve()
{
   Individual* population_a = new Individual[population_size];
   Individual* population_b = new Individual[population_size];

   data.best_individual.genome = new int[bits_number];

   // Alocação dos indivíduos
   for( int i = 0; i < population_size; ++i )
   {
      population_a[i].genome = new int[bits_number];
      population_b[i].genome = new int[bits_number];
   }

   Individual* antecedentes = population_a;
   Individual* descendentes = population_b;

   // Criação da população inicial (1ª geração)
   generate_population( antecedentes );
    
   // Processo evolucionário
   for( int geracao = 1; geracao <= generation_number && data.best_individual.fitness > 0.0005; ++geracao )
   {
      for( int i = 0; i < population_size; i += 2 )
      {
         // Seleção de indivíduos adaptados para gerar descendentes
         const Individual* father = tournament( antecedentes );
         const Individual* mother = tournament( antecedentes );

         // Cruzamento
         if( random_number() < crossover_rate )
         {
            crossover( father->genome, mother->genome, 
                        descendentes[i].genome, descendentes[i + 1].genome );
         }
         else // Apenas clonagem
         {
            clone( father, &descendentes[i] );
            clone( mother, &descendentes[i + 1] );
         }

         // Mutações
         mutation( descendentes[i].genome );
         mutation( descendentes[i + 1].genome );

         // Avaliação dos novos indivíduos
          evaluate( &descendentes[i] );
          evaluate( &descendentes[i + 1] );
      }

      // Elitismo
      if( elitism ) clone( &data.best_individual, &descendentes[0] );

      // Faz população nova ser a atual, e vice-versa.
      swap( antecedentes, descendentes );

   }

   for( int i = 0; i < population_size; ++i )
   {
      delete[] population_a[i].genome, population_b[i].genome;
   }
   delete[] population_a, population_b; 

   return data.best_individual;

   delete[] data.best_individual.genome;
}
