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
#include <sstream>
#include "util/CmdLineParser.h"
#include "interpreter/accelerator.h"
#include "interpreter/sequential.h"
#include "pee.h"
#include "server/server.h"
#include "client/client.h"
#include "individual"
#include "grammar"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct Peer {
  Peer( const std::string& s, float f ):
      address( s ), frequency( f ) {}

  std::string address;
  float frequency;
};

namespace { struct t_data { Symbol initial_symbol; Population best_individual; int best_size; unsigned max_size_phenotype; int nlin; Symbol* phenotype; float* ephemeral; int* size; int verbose; int elitism; int population_size; int generations; int number_of_bits; int bits_per_gene; int bits_per_constant; int seed; int tournament_size; float mutation_rate; float crossover_rate; float interval[2]; int version; double time_total; std::vector<Peer> peers; Pool* pool; } data; };

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

#define swap(i, j) {Population t = i; i = j; j = t;}
//#define swap(i, j) {Population t = *i; *i = *j; *j = t;}

double random_number() {return (double)rand() / ((double)RAND_MAX + 1.0f);} // [0.0, 1.0)

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

   Opts.Bool.Add( "-v", "--verbose" );

   Opts.Bool.Add( "-acc" );

   Opts.Int.Add( "-ni", "--number-of-inputs" );
   Opts.Int.Add( "-nm", "--number-of-models" );

   Opts.Int.Add( "-g", "--generations", 50, 1, std::numeric_limits<int>::max() );

   Opts.Int.Add( "-s", "--seed", 0, 0, std::numeric_limits<long>::max() );

   Opts.Int.Add( "-ps", "--population-size", 1024, 5, std::numeric_limits<int>::max() );
   Opts.Float.Add( "-cp", "--crossover-probability", 0.95, 0.0, 1.0 );
   Opts.Float.Add( "-mp", "--mutation-probability", 0.0025, 0.0, 1.0 );
   Opts.Int.Add( "-ts", "--tournament-size", 3, 1, std::numeric_limits<int>::max() );

   Opts.Bool.Add( "-e", "--elitism" );

   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
   Opts.Int.Add( "-bg", "--bits-per-gene", 8, 8 );
   Opts.Int.Add( "-bc", "--bits-per-constant", 16, 4 );

   Opts.Float.Add( "-min", "--min-constant", 0 );
   Opts.Float.Add( "-max", "--max-constant", 300 );

   Opts.String.Add( "-peers", "--address_of_island" );

   // processing the command-line
   Opts.Process();

   // getting the results!
   data.verbose = Opts.Bool.Get("-v");

   data.generations = Opts.Int.Get("-g");

   data.seed = Opts.Int.Get("-s") == 0 ? time( NULL ) : Opts.Int.Get("-s");

   data.population_size = Opts.Int.Get("-ps");
   data.crossover_rate = Opts.Float.Get("-cp");
   data.mutation_rate = Opts.Float.Get("-mp");
   data.tournament_size = Opts.Int.Get("-ts");

   data.elitism = Opts.Bool.Get("-e");

   data.number_of_bits = Opts.Int.Get("-nb");
   data.bits_per_gene = Opts.Int.Get("-bg");
   data.bits_per_constant = Opts.Int.Get("-bc");

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

   data.best_size = 10;
   data.best_individual.genome = NULL;
   data.best_individual.fitness = NULL;

   data.max_size_phenotype = MAX_QUANT_SIMBOLOS_POR_REGRA * data.number_of_bits/data.bits_per_gene;
   data.nlin = nlin;

   data.phenotype = new Symbol[data.population_size * data.max_size_phenotype];
   data.ephemeral = new float[data.population_size * data.max_size_phenotype];
   data.size = new int[data.population_size];

   std::string str = Opts.String.Get("-peers");
   //std::cout << str << std::endl;

   size_t pos;
   std::string s;
   float f;

   std::string delimiter = ",";
   while ( ( pos = str.find( delimiter ) ) != std::string::npos ) 
   {
      pos = str.find(delimiter);
      s = str.substr(0, pos);
      str.erase(0, pos + delimiter.length());
      //std::cout << s << std::endl;

      delimiter = ";";

      pos = str.find(delimiter);
      f = std::atof(str.substr(0, pos).c_str());
      str.erase(0, pos + delimiter.length());
      //std::cout << f << std::endl;

      data.peers.push_back( Peer( s, f ) );

      delimiter = ",";
   }

   data.pool = new Pool( data.peers.size() );

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

void pee_clone( Population* original, int idx_original, Population* copy, int idx_copy )
{
   for( int i = 0; i < data.number_of_bits; ++i ) copy->genome[idx_copy * data.number_of_bits + i] = original->genome[idx_original * data.number_of_bits + i];

   copy->fitness[idx_copy] = original->fitness[idx_original];
}

void pee_individual_print( const Population* individual, int idx, FILE* out, int print_mode )
{
   Symbol phenotype[data.max_size_phenotype];
   float ephemeral[data.max_size_phenotype];

   int allele = 0;
   int size = decode( individual->genome + (idx * data.number_of_bits), &allele, phenotype, ephemeral, 0, data.initial_symbol );
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
      fprintf( out, "%.12f\n", individual->fitness[idx] );
   }
   else
      fprintf( out, " %.12f\n", individual->fitness[idx] );
}


void pee_evaluate( Population* population, int nInd )
{
   int allele;
   for( int i = 0; i < nInd; i++ )
   {
      allele = 0;
      data.size[i] = decode( population->genome + (i * data.number_of_bits), &allele, data.phenotype + (i * data.max_size_phenotype), data.ephemeral + (i * data.max_size_phenotype), 0, data.initial_symbol );
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

   //std::cout << data.best_size << std::endl;
   int index[data.best_size];
   if( data.version )
   {
      acc_interpret( data.phenotype, data.ephemeral, data.size, population->fitness, nInd, index, &data.best_size, 0, 0.00001 );
   }
   else
   {
      seq_interpret( data.phenotype, data.ephemeral, data.size, population->fitness, nInd, index, &data.best_size, 0, 0.00001 );
   }
   //std::cout << data.best_size << std::endl;

   //bool flag = false;
   //for( int i = 0; i < nInd; i++ )
   //{
   //   //if( population[i].fitness < data.best_individual.fitness )
   //   //{
   //   //   flag = true;
   //   //   pee_clone( &population[i], &data.best_individual );
   //   //}
   //}
   
   //if( flag )
   //{
   //   printf("Método1:%d  ",flag);
   //   pee_individual_print( &data.best_individual, stdout, 0 );
   //   printf("Método2:%d  ",flag);
   //   pee_individual_print( &population[index], stdout, 0 );
   //}

   for( int i = 0; i < data.best_size; i++ )
   {
      if( population->fitness[index[i]] < data.best_individual.fitness[i] )
      {
         pee_clone( population, index[i], &data.best_individual, i );
      }
   }
}

void pee_generate_population( Population* population )
{
   for( int i = 0; i < data.population_size; ++i)
   {
      for( int j = 0; j < data.number_of_bits; j++ )
      {
         population->genome[i * data.number_of_bits + j] = (random_number() < 0.5) ? 1 : 0;
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

int pee_tournament( const Population* population )
{
   int idx_winner = (int)(random_number() * data.population_size);
   float fitness_winner = population->fitness[idx_winner];

   for( int t = 1; t < data.tournament_size; ++t )
   {
      int idx_competitor = (int)(random_number() * data.population_size);
      const float fitness_competitor = population->fitness[idx_competitor];

      if( fitness_competitor < fitness_winner ) 
      {
         fitness_winner = fitness_competitor;
         idx_winner = idx_competitor;
      }
   }

   return idx_winner;
}

int pee_reverse_tournament( const Population* population )
{
   int idx_winner = (int)(random_number() * data.population_size);
   float fitness_winner = population->fitness[idx_winner];

   for( int t = 1; t < data.tournament_size; ++t )
   {
      int idx_competitor = (int)(random_number() * data.population_size);
      const float fitness_competitor = population->fitness[idx_competitor];

      if( fitness_competitor > fitness_winner ) 
      {
         fitness_winner = fitness_competitor;
         idx_winner = idx_competitor;
      }
   }

   return idx_winner;
}

void pee_print_best( FILE* out, int print_mode ) 
{
   pee_individual_print( &data.best_individual, 0, out, print_mode );
}

void pee_print_time() 
{
   if( data.version )
   {
      acc_print_time();
   }
   printf("time_total: %lf\n", data.time_total);
}

void pee_receive_individual( Population* population )
{

   if( !Server::m_individuals.empty() && Server::m_mutex.tryLock() )
   {
      while( !Server::m_individuals.empty() )
      {
         int idx = pee_reverse_tournament( population );
         //delete[] population->genome[idx];
         //population[idx] = Server::m_individuals.front();
         Individual individual = Server::m_individuals.front();
         population->fitness[idx] = individual.fitness;
         for( int i = 0; i < data.number_of_bits; i++ )
            population->genome[idx * data.number_of_bits + i] = individual.genome[i];
         Server::m_individuals.pop();

         std::cerr << "Receiving Individual: " << population->fitness[idx] << std::endl;
         //printf("%f ", population->fitness[idx]);
         //for( int i = 0; i < data.number_of_bits; i++ )
         //   printf("%d ", population->genome[idx * data.number_of_bits + i]);
         //printf("\n");
      } 
      Server::m_mutex.unlock();
   }
}

void pee_send_individual( Population* population )
{
   //TODO:colocar no intervalo do kernel
   for( int i = 0; i < data.peers.size(); i++ )
   { 
      if( random_number() < data.peers[i].frequency )
      {
         //Testa se o thread ainda está mandando o indivíduo, escolhido na geração anterior, para a ilha.
         //if( !(data.pool->threads[i] == NULL) && data.pool->threads[i]->isRunning() ) continue;
         //std::cerr << "Thread[0]: " << data.pool->threads[0]->isRunning() << std::endl;
         if( data.pool->threads[i]->isRunning() ) continue;

         const int idx = pee_tournament( population );

         std::stringstream results; //results.str(std::string());
         results <<  population->fitness[idx] << " ";
         for( int j = 0; j < data.number_of_bits-1; j++ )
            results <<  population->genome[idx * data.number_of_bits + j] << " ";
         results <<  population->genome[idx * data.number_of_bits + data.number_of_bits];

         delete data.pool->clients[i], data.pool->ss[i];
         data.pool->ss[i] = new StreamSocket();
         data.pool->clients[i] = new Client( *(data.pool->ss[i]), data.peers[i].address.c_str(), results.str() );
         data.pool->threads[i]->start( *( data.pool->clients[i] ) );

         std::cerr << "Sending Individual Thread[" << i << "] to " << data.peers[i].address << ": " << population->fitness[idx] << std::endl;
         //std::cerr << results.str() << std::endl;
      }
   }
}

void pee_evolve()
{
   /*
      Pseudo-code for evolve:

   1: Create (randomly) the initial population P
   2: Evaluate all individuals (programs) of P
   3: for generation ← 1 to NG do
      4: while |Ptmp| < |P| do
         5: Select and copy from P two fit individuals, p1 and p2
         6: if [probabilistically] crossover then
            7: Recombine p1 and p2, creating the children p1' and p2'
            8: p1 ← p1' ; p2 ← p2'
         9: end if
         10: if [probabilistically] mutation then
            11: Apply mutation operators in p1 and p2, generating p1' and p2'
            12: p1 ← p1' ; p2 ← p2'
         13: end if
         14: Insert p1 and p2 into Ptmp
      15: end while
      16: Evaluate all individuals (programs) of Ptmp
      17: Copy the best (elitism) individuals of P to the temporary population Ptmp
      18: P ← Ptmp; then discard Ptmp
   19: end for
   20: return the best individual so far
   */

   clock_t start, end;
   start = clock();

   srand( data.seed );

   Population antecedentes, descendentes;

   antecedentes.fitness = new float[data.population_size];
   descendentes.fitness = new float[data.population_size];

   antecedentes.genome = new int[data.population_size * data.number_of_bits];
   descendentes.genome = new int[data.population_size * data.number_of_bits];

   data.best_individual.fitness = new float[data.best_size];
   data.best_individual.genome = new int[data.best_size * data.number_of_bits];
   for( int i = 0; i < data.best_size; i++ )
   {
      data.best_individual.fitness[i] = std::numeric_limits<float>::max();
   }


   // 1 e 2:
   pee_generate_population( &antecedentes );
    
   // 3:
   for( int geracao = 1; geracao <= data.generations; ++geracao )
   {
      // 4
      for( int i = 0; i < data.population_size; i += 2 )
      {
         // 5:
         int idx_father = pee_tournament( &antecedentes );
         int idx_mother = pee_tournament( &antecedentes );

         // 6:
         if( random_number() < data.crossover_rate )
         {
            // 7 e 8:
            pee_crossover( antecedentes.genome + (idx_father * data.number_of_bits), antecedentes.genome + (idx_mother * data.number_of_bits), descendentes.genome + (i * data.number_of_bits), descendentes.genome + ((i + 1) * data.number_of_bits));
         } // 9
         else 
         {
            // 8:
            pee_clone( &antecedentes, idx_father, &descendentes, i );
            pee_clone( &antecedentes, idx_mother, &descendentes, i + 1 );
         } // 9

         // 10, 11, 12, 13 e 14:
         pee_mutation( descendentes.genome + (i * data.number_of_bits) );
         pee_mutation( descendentes.genome + ((i + 1) * data.number_of_bits) );
      } // 15

      // 16:
      pee_evaluate( &descendentes, data.population_size );

      // 17:
      if( data.elitism ) pee_clone( &data.best_individual, 0, &descendentes, 0 );
      // 18:
      swap( antecedentes, descendentes );
      //swap( &antecedentes, &descendentes );

      std::cerr << "Geracao: " << geracao << std::endl;
      pee_receive_individual( &antecedentes );
      pee_send_individual( &antecedentes );
      //std::cerr << "Thread[0]: " << data.pool->threads[0]->isRunning() << std::endl;

      if( data.verbose ) 
      {
         printf("[%d] ", geracao);
         pee_individual_print( &data.best_individual, 0, stdout, 0 );
      }
   } // 19

   // join all threads
   // Clean up
   delete[] antecedentes.genome, antecedentes.fitness, descendentes.genome, descendentes.fitness; 

   end = clock();
   data.time_total = ((double)(end - start))/((double)(CLOCKS_PER_SEC));
}

void pee_destroy()
{
   delete[] data.best_individual.genome, data.best_individual.fitness, data.phenotype, data.ephemeral, data.size;
   delete data.pool;
   if( !data.version ) {seq_interpret_destroy();}
}
