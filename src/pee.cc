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
#include <ctime>
#include <string>   
#include "util/CmdLineParser.h"
#include "interpreter/accelerator.h"
#include "interpreter/sequential.h"
#include "pee.h"
#include "grammar"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct Individual { int* genome; float fitness; };

static struct t_data { Symbol initial_symbol; Individual best_individual; unsigned max_size_phenotype; int nlin; Symbol* phenotype; float* ephemeral; int* size; float* error; int verbose; int elitism; int population_size; int generations; int number_of_bits; int bits_per_gene; int bits_per_constant; int seed; int tournament_size; float mutation_rate; float crossover_rate; float interval[2]; int version; } data;

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

#define swap(i, j) {Individual* t = i; i = j; j = t;}

float random_number() {return rand() / (RAND_MAX + 1.0);} // [0.0, 1.0)

t_rule* decode_rule( const int* genome, int* const allele, Symbol cabeca )
{
   // Verifica se é possível decodificar mais uma regra, caso contrário
   // retorna uma regra nula.
   if( *allele + data.bits_per_gene > data.number_of_bits ) { return NULL; }

   // Número de regras encabeçada por "cabeça"
   unsigned num_regras = tamanhos[cabeca];

   // Converte data.bits_per_gene bits em um valor integral
   unsigned valor_bruto = 0;
   for( int i = 0; i < data.bits_per_gene; ++i )  
      if( genome[(*allele)++] ) valor_bruto += pow( 2, i );

   // Seleciona uma regra no intervalo [0, num_regras - 1]
   return gramatica[cabeca][valor_bruto % num_regras];
}

float decode_real( const int* genome, int* const allele )
{
   // Retorna a constante zero se não há número suficiente de bits
   if( *allele + data.bits_per_constant > data.number_of_bits ) { return 0.; }

   // Converte data.bits_per_constant bits em um valor real
   float valor_bruto = 0.;
   for( int i = 0; i < data.bits_per_constant; ++i ) 
      if( genome[(*allele)++] ) valor_bruto += pow( 2.0, i );

   // Normalizar para o intervalo desejado: a + valor_bruto * (b - a)/(2^n - 1)
   return data.interval[0] + valor_bruto * (data.interval[1] - data.interval[0]) / 
          (pow( 2.0, data.bits_per_constant ) - 1);
}

int decode( const int* genome, int* const allele, Symbol* phenotype, float* ephemeral, int pos, Symbol initial_symbol )
{
   t_rule* r = decode_rule( genome, allele, initial_symbol ); 
   if( !r ) { return 0; }

   for( int i = 0; i < r->quantity; ++i )
      if( r->symbols[i] >= TERMINAL_MIN )
      {
         phenotype[pos] = r->symbols[i];

         // Tratamento especial para constantes efêmeras
         if( r->symbols[i] == T_CONST )
         {
           /* 
              Esta estratégia faz uso do próprio código genético do indivíduo 
              para extrair uma constante real. Extrai-se data.bits_per_constant bits a 
              partir da posição atual e os decodifica como um valor real.
            */
            ephemeral[pos] = decode_real( genome, allele );
         }
         else
         {
            if( r->symbols[i] >= ATTRIBUTE_MIN )
            {
               phenotype[pos] = T_ATTRIBUTE;
               ephemeral[pos] = r->symbols[i] - ATTRIBUTE_MIN;
            } 
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

void pee_init( float** input, float** model, float* obs, int nlin, int argc, char** argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Bool.Add( "-acc" );
   Opts.Bool.Add( "-v" );
   Opts.Bool.Add( "-e" );
   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.Int.Add( "-p", "--population_size", 2000, 2 );
   Opts.Int.Add( "-g", "--generations", 50, 0 );
   Opts.Int.Add( "-ts", "--tournament_size", 3, 1 );
   Opts.Int.Add( "-nb", "--number_of_bits", 2000, 16 );
   Opts.Int.Add( "-bg", "--bits_per_gene", 8, 8 );
   Opts.Int.Add( "-bc", "--bits_per_constant", 16, 4 );
   Opts.Int.Add( "-s", "--seed", time(NULL) );
   Opts.Float.Add( "-m", "--mutation_rate", 0.0025, 0, 1 );
   Opts.Float.Add( "-c", "--crossover_rate", 0.95, 0, 1 );
   Opts.Float.Add( "-min", "--min_constant", 0 );
   Opts.Float.Add( "-max", "--max_constant", 300 );

   // processing the command-line
   Opts.Process();

   // getting the results!
   data.verbose = Opts.Bool.Get("-v");
   data.elitism = Opts.Bool.Get("-e");
   data.population_size = Opts.Int.Get("-p");
   data.generations = Opts.Int.Get("-g");
   data.tournament_size = Opts.Int.Get("-ts");
   data.number_of_bits = Opts.Int.Get("-nb");
   data.bits_per_gene = Opts.Int.Get("-bg");
   data.bits_per_constant = Opts.Int.Get("-bc");
   data.seed = Opts.Int.Get("-s");
   data.mutation_rate = Opts.Float.Get("-m");
   data.crossover_rate = Opts.Float.Get("-c");

   if( Opts.Float.Get("-min") > Opts.Float.Get("-max") )
   {
      data.interval[0] = Opts.Float.Get("-max");
      data.interval[1] = Opts.Float.Get("-min");
   }
   else
   {
      data.interval[0] = Opts.Float.Get("-min");
      data.interval[1] = Opts.Float.Get("-max");
   }

   data.initial_symbol = NT_IF_THEN_ELSE_INICIAL;
   data.best_individual.genome = NULL;
   data.best_individual.fitness = std::numeric_limits<float>::max();
   data.max_size_phenotype = MAX_QUANT_SIMBOLOS_POR_REGRA * data.number_of_bits/data.bits_per_gene;
   data.nlin = nlin;
   data.phenotype = new Symbol[data.population_size * data.max_size_phenotype];
   data.ephemeral = new float[data.population_size * data.max_size_phenotype];
   data.size = new int[data.population_size];
   data.error = new float[data.population_size];
   data.version = Opts.Bool.Get("-acc");

   if( data.version )
   {
      if( acc_interpret_init( argc, argv, data.max_size_phenotype, data.population_size, input, model, obs, nlin, 0 ) )
      {
         fprintf(stderr,"Error in initialization phase.\n");
      }
   }
   else
   {
      seq_interpret_init( data.max_size_phenotype, input, model, obs, nlin, Opts.Int.Get("-ni"), Opts.Int.Get("-nm") );
   }
}

void pee_clone( const Individual* original, Individual* copy )
{
   for( int i = 0; i < data.number_of_bits; ++i ) copy->genome[i] = original->genome[i];

   copy->fitness = original->fitness;
}

void pee_evaluate( Individual* individual, int nInd )
{
   int allele;
   for( int i = 0; i < nInd; i++ )
   {
      allele = 0;
      data.size[i] = decode( individual[i].genome, &allele, data.phenotype + (i * data.max_size_phenotype), data.ephemeral + (i * data.max_size_phenotype), 0, data.initial_symbol );
   }
   
//   for( int j = 0; j < nInd; j++ )
//   {
//      fprintf(stdout,"Ind[%d]=%d\n",j,data.size[j]);
//      for( int i = 0; i < data.size[j]; i++ )
//      {
//         fprintf(stdout,"%d ",data.phenotype[j * data.max_size_phenotype + i]);
//      }
//      fprintf(stdout,"\n");
//      for( int i = 0; i < data.size[j]; i++ )
//      {
//         fprintf(stdout,"%f ",data.ephemeral[j * data.max_size_phenotype + i]);
//      }
//      fprintf(stdout,"\n");
//   }


   if( data.version )
   {
      acc_interpret( data.phenotype, data.ephemeral, data.size, data.error, nInd, 0 );
   }
   else
   {
      seq_interpret( data.phenotype, data.ephemeral, data.size, data.error, nInd, 0 );
   }


   for( int i = 0; i < nInd; i++ )
   {
//      fprintf(stdout,"fitness[Ind=%d]=%f\n",i,data.error[i]);
      individual[i].fitness = data.error[i] + 0.00001*data.size[i]; 
      if( individual[i].fitness < data.best_individual.fitness )
      {
         pee_clone( &individual[i], &data.best_individual );
      }
   }
}

void pee_generate_population( Individual* population )
{
   for( int i = 0; i < data.population_size; ++i)
   {
      for( int j = 0; j < data.number_of_bits; j++ )
      {
         population[i].genome[j] = (random_number() < 0.5) ? 1 : 0;
      }
   }
   pee_evaluate( population, data.population_size );
}

void pee_crossover( const int* father, const int* mother, int* offspring1, int* offspring2 )
{
#ifdef TWO_POINT_CROSSOVER
   // Cruzamento de dois pontos
   int pontos[2];

   // Cortar apenas nas bordas dos genes
   pontos[0] = ((int)(random_number() * data.number_of_bits))/data.bits_per_gene * data.bits_per_gene;
   pontos[1] = ((int)(random_number() * data.number_of_bits))/data.bits_per_gene * data.bits_per_gene;

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
   for( int i = pontos[1]; i < data.number_of_bits; ++i )
   {
      offspring1[i] = father[i];
      offspring2[i] = mother[i];
   }
#else
   // Cruzamento de um ponto
   int pontoDeCruzamento = (int)(random_number() * data.number_of_bits);

   for( int i = 0; i < pontoDeCruzamento; ++i )
   {
      offspring1[i] = father[i];
      offspring2[i] = mother[i];
   }
   for( int i = pontoDeCruzamento; i < data.number_of_bits; ++i )
   {
      offspring1[i] = mother[i];
      offspring2[i] = father[i];
   }
#endif
}

void pee_mutation( int* genome )
{
   for( int i = 0; i < data.number_of_bits; ++i )
      if( random_number() < data.mutation_rate ) genome[i] = !genome[i];
}

const Individual* pee_tournament( const Individual* population )
{
   const Individual* vencedor = &population[(int)(random_number() * data.population_size)];

   for( int t = 1; t < data.tournament_size; ++t )
   {
      const Individual* competidor = &population[(int)(random_number() * data.population_size)];

      if( competidor->fitness < vencedor->fitness ) vencedor = competidor;
   }

   return vencedor;
}

void pee_individual_print( const Individual* individual, FILE* out, int print_mode )
{
   Symbol phenotype[data.max_size_phenotype];
   float ephemeral[data.max_size_phenotype];

   int allele = 0;
   int size = decode( individual->genome, &allele, phenotype, ephemeral, 0, data.initial_symbol );
   if( !size ) { return; }
  
   if( print_mode )
   {
      fprintf( out, "%d\n", size );
      for( int i = 0; i < size; ++i )
         if( phenotype[i] == T_CONST || phenotype[i] == T_ATTRIBUTE )
            fprintf( out, "%d %.12f ", phenotype[i], ephemeral[i] );
         else
            fprintf( out, "%d ", phenotype[i] );
      fprintf( out, "\n" );
   } 
   else 
      fprintf( out, "%d ", size );

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
            case T_GREATER:
               fprintf( out, "> " );
               break;
            case T_LESS:
               fprintf( out, "< " );
               break;
            case T_EQUAL:
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
            case T_ATTRIBUTE:
               fprintf( out, "ATTR-%d ", (int)ephemeral[i] );
               break;
            case T_1P:
               fprintf( out, "1 " );
               break;
            case T_2P:
               fprintf( out, "2 " );
               break;
            case T_3P:
               fprintf( out, "3 " );
               break;
            case T_4P:
               fprintf( out, "4 " );
               break;
            case T_CONST:
               fprintf( out, "%.12f ",  ephemeral[i] );
               break;
      } 
   if( print_mode )
   {
      fprintf( out, "\n" );
      fprintf( out, "%.12f\n", individual->fitness );
   }
   else
      fprintf( out, " %.12f\n", individual->fitness );
}

void pee_print_best( FILE* out, int print_mode ) 
{
   pee_individual_print( &data.best_individual, out, print_mode );
}


void pee_evolve()
{
   srand( data.seed );

   Individual* population_a = new Individual[data.population_size];
   Individual* population_b = new Individual[data.population_size];

   data.best_individual.genome = new int[data.number_of_bits];

   // Alocação dos indivíduos
   for( int i = 0; i < data.population_size; ++i )
   {
      population_a[i].genome = new int[data.number_of_bits];
      population_b[i].genome = new int[data.number_of_bits];
   }

   Individual* antecedentes = population_a;
   Individual* descendentes = population_b;

   // Criação da população inicial (1ª geração)
   pee_generate_population( antecedentes );
    
   // Processo evolucionário
   for( int geracao = 1; geracao <= data.generations && data.best_individual.fitness > 0.0005; ++geracao )
   {
      for( int i = 0; i < data.population_size; i += 2 )
      {
         // Seleção de indivíduos adaptados para gerar descendentes
         const Individual* father = pee_tournament( antecedentes );
         const Individual* mother = pee_tournament( antecedentes );

         // Cruzamento
         if( random_number() < data.crossover_rate )
         {
            pee_crossover( father->genome, mother->genome, 
                        descendentes[i].genome, descendentes[i + 1].genome );
         }
         else // Apenas clonagem
         {
            pee_clone( father, &descendentes[i] );
            pee_clone( mother, &descendentes[i + 1] );
         }

         // Mutações
         pee_mutation( descendentes[i].genome );
         pee_mutation( descendentes[i + 1].genome );
      }

      // Avaliação dos novos indivíduos
      pee_evaluate( descendentes, data.population_size );
      pee_evaluate( descendentes, data.population_size );

      // Elitismo
      if( data.elitism ) pee_clone( &data.best_individual, &descendentes[0] );

      // Faz população nova ser a atual, e vice-versa.
      swap( antecedentes, descendentes );

      if( data.verbose ) 
      {
         printf("[%d] ", geracao);
         pee_individual_print( &data.best_individual, stdout, 0 );
      }
   }

   for( int i = 0; i < data.population_size; ++i )
   {
      delete[] population_a[i].genome, population_b[i].genome;
   }
   delete[] population_a, population_b; 
}

void pee_destroy()
{
   delete[] data.best_individual.genome, data.phenotype, data.ephemeral, data.size, data.error;
   if( !data.version ) {seq_interpret_destroy();}
}
