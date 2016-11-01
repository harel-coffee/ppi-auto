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

/* The probability of applying two-point crossover instead of one-point */
#define TWOPOINT_CROSSOVER_PROBABILITY 0.66

/* Probability of applying the standard mutation (bit flip) instead of shrink
 * mutation */
#define BITFLIP_MUTATION_PROBABILITY 0.75

/* When applying the shrink mutation, the variable
 * AGGRESSIVE_SHRINK_MUTATION_PROBABILITY defines the ratio in which the
 * aggressive version will be applied instead. The aggressive version ignores
 * the mutation rate and might even shrink the entire genome. */
#define AGGRESSIVE_SHRINK_MUTATION_PROBABILITY 0.10

#include <stdio.h> 
#include <stdlib.h>
#include <cmath>    
#include <limits>
#include <ctime>
#include <string>   
#include <sstream>
#include <iostream> 
#include "common/common.h"
#include "util/CmdLineParser.h"
#include "interpreter/accelerator.h"
#include "interpreter/sequential.h"
#include "pee.h"
#include "server/server.h"
#include "client/client.h"
#include "individual"
#include "grammar"
#include "util/Util.h"
#include "util/Random.h"
#include "Poco/Logger.h"

// Definition of the Random Number Generator to be used (see util/Random.h)
#define RNG XorShift128Plus
//#define RNG Random

/*
 * The parameter ALPHA is the complexity penalization factor. Each individual
 * will have its error augmented by ALPHA*size, where size is the number of
 * operators and operands. ALPHA is usually very small, just enough to favor
 * those that have similar (same) error but are less complex than another.
 */
#define ALPHA 0.000001

using namespace std;

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct Peer {
  Peer( const std::string& s, float f ):
      address( s ), frequency( f ) {}

  std::string address;
  float frequency;
};

namespace { struct t_data { Symbol initial_symbol; Population best_individual; int best_size; unsigned max_size_phenotype; int nlin; Symbol* phenotype; float* ephemeral; int* size; unsigned long long sum_size; int verbose; int machine; int elitism; int population_size; int immigrants_size; int generations; int number_of_bits; int bits_per_gene; int bits_per_constant; int seed; int tournament_size; float mutation_rate; float crossover_rate; float interval[2]; int parallel_version; double time_total_evolve; double time_gen_evolve; double time_generate; double time_total_evaluate; double time_gen_evaluate; double gpops_gen_evaluate; double time_total_crossover; double time_gen_crossover; double time_total_mutation; double time_gen_mutation; double time_total_clone; double time_gen_clone; double time_total_tournament; double time_gen_tournament; double time_total_send; double time_gen_send; double time_total_receive; double time_gen_receive; std::vector<Peer> peers; Pool* pool; unsigned long stagnation_tolerance; int argc; char ** argv;  } data; };

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

#define swap(i, j) {Population t = *i; *i = *j; *j = t;}

//double random_number() {return (double)rand() / ((double)RAND_MAX + 1.0f);} // [0.0, 1.0)
double random_number() { return RNG::Real(); };


t_rule* decode_rule( const GENOME_TYPE* genome, int* const allele, Symbol cabeca )
{
   // Verifica se é possível decodificar mais uma regra, caso contrário
   // retorna uma regra nula.
   if( *allele + data.bits_per_gene > data.number_of_bits ) { return NULL; }

   // Número de regras encabeçada por "cabeça"
   unsigned num_regras = tamanhos[cabeca];

   // Converte data.bits_per_gene bits em um valor integral
   unsigned valor_bruto = 0;
   for( int i = 0; i < data.bits_per_gene; ++i )
      if( genome[(*allele)++] ) valor_bruto += (1 << i); // (1<<i) is a more efficient way of performing 2^i

   // Seleciona uma regra no intervalo [0, num_regras - 1]
   return gramatica[cabeca][valor_bruto % num_regras];
}

float decode_real( const GENOME_TYPE* genome, int* const allele )
{
   // Retorna a constante zero se não há número suficiente de bits
   if( *allele + data.bits_per_constant > data.number_of_bits ) { return 0.; }

   // Converte data.bits_per_constant bits em um valor real
   // NB: This works backwards in the sense that the most significant bit is the last bit (from left to right).
   unsigned long valor_bruto = 0;
   for( int i = 0; i < data.bits_per_constant; ++i )
      if( genome[(*allele)++] ) valor_bruto += (1UL << i);

   // Normalizar para o intervalo desejado: a + valor_bruto * (b - a)/(2^n - 1)
   return data.interval[0] + float(valor_bruto) * (data.interval[1] - data.interval[0]) / ((1UL << data.bits_per_constant) - 1.0);
}

int decode( const GENOME_TYPE* genome, int* const allele, Symbol* phenotype, float* ephemeral, int pos, Symbol initial_symbol )
{
   t_rule* r = decode_rule( genome, allele, initial_symbol ); 
   if( !r ) { return 0; }

   for( int i = 0; i < r->quantity; ++i )
      if( r->symbols[i] >= TERMINAL_MIN )
      {
         phenotype[pos] = r->symbols[i];

#ifndef NOT_USING_T_CONST
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
#endif
         {
            if( r->symbols[i] >= ATTRIBUTE_MIN )
            {
               phenotype[pos] = T_ATTRIBUTE;
               ephemeral[pos] = r->symbols[i] - ATTRIBUTE_MIN;
            } 
         }
         ++pos;
      }
      else // It's a non-terminal, so calling recursively decode again...
      {
         pos = decode( genome, allele, phenotype, ephemeral, pos, r->symbols[i] );
         if( !pos ) return 0;
      }

   return pos;
}


/** ****************************************************************** **/
/** ************************* MAIN FUNCTIONS ************************* **/
/** ****************************************************************** **/

#include <interpreter_core_print>

void pee_init( float** input, int nlin, int ncol, int argc, char** argv ) 
{
   data.argc = argc; data.argv = argv;
   CmdLine::Parser Opts( argc, argv );

   Opts.Bool.Add( "-v", "--verbose" );

   Opts.Bool.Add( "-machine" );

   Opts.Bool.Add( "-acc" );

   Opts.Int.Add( "-g", "--generations", 1000, 1, std::numeric_limits<int>::max() );

   Opts.Int.Add( "-s", "--seed", 0, 0, std::numeric_limits<long>::max() );

   Opts.Int.Add( "-ps", "--population-size", 1024, 1, std::numeric_limits<int>::max() );
   Opts.Int.Add( "-is", "--immigrants-size", 5, 1 );
   Opts.Float.Add( "-cp", "--crossover-probability", 0.90, 0.0, 1.0 );
   Opts.Float.Add( "-mr", "--mutation-rate", 0.01, 0.0, 1.0 );
   Opts.Int.Add( "-ts", "--tournament-size", 3, 1, std::numeric_limits<int>::max() );

   Opts.Bool.Add( "-e", "--elitism" );

   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
   Opts.Int.Add( "-bg", "--bits-per-gene", 8, 8, 31 );
   Opts.Int.Add( "-bc", "--bits-per-constant", 16, 4, 63 );

   Opts.Float.Add( "-min", "--min-constant", -10 );
   Opts.Float.Add( "-max", "--max-constant", 10 );

   Opts.String.Add( "-peers", "--address_of_island" );
 
   /* Maximum allowed number of generations without improvement [default = disabled] */
   Opts.Int.Add( "-st", "--stagnation-tolerance", numeric_limits<unsigned long>::max(), 0 );

   Opts.Float.Add( "-iat", "--immigrants-acceptance-threshold", 0.0, 0.0 );

   // processing the command-line
   Opts.Process();

   // getting the results!
   data.verbose = Opts.Bool.Get("-v");

   data.machine = Opts.Bool.Get("-machine");

   data.generations = Opts.Int.Get("-g");

   data.seed = Opts.Int.Get("-s") == 0 ? time( NULL ) : Opts.Int.Get("-s");

   data.population_size = Opts.Int.Get("-ps");
   data.immigrants_size = Opts.Int.Get("-is");
   data.crossover_rate = Opts.Float.Get("-cp");
   data.mutation_rate = Opts.Float.Get("-mr");
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

   data.initial_symbol = NT_INICIAL;

   data.best_size = 1;
   data.best_individual.genome = NULL;
   data.best_individual.fitness = NULL;
   
   data.max_size_phenotype = MAX_QUANT_SIMBOLOS_POR_REGRA * data.number_of_bits/data.bits_per_gene;

   data.nlin = nlin;

   data.phenotype = new Symbol[data.population_size * data.max_size_phenotype];
   data.ephemeral = new float[data.population_size * data.max_size_phenotype];
   data.size = new int[data.population_size];
   data.sum_size = 0;

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
   
   //A thread pool used to manage the sending of individuals to the islands.
   data.pool = new Pool( data.peers.size() );

   {
      Poco::FastMutex::ScopedLock lock( Server::m_mutex );

      Server::m_immigrants = new std::vector<char>[data.immigrants_size];
      Server::m_fitness = new float[data.immigrants_size];
      for( int i = 0; i < data.immigrants_size; i++ )
      {
         Server::m_freeslots.push(i);
      }
   }

   data.parallel_version = Opts.Bool.Get("-acc");
   if( data.parallel_version )
   {
      if( acc_interpret_init( argc, argv, data.max_size_phenotype, MAX_QUANT_SIMBOLOS_POR_REGRA, data.population_size, input, nlin, ncol, 0, 0 ) )
      {
         fprintf(stderr,"Error in initialization phase.\n");
      }
   }
   else
   {
      seq_interpret_init( data.max_size_phenotype, input, nlin, ncol );
   }

   data.stagnation_tolerance = Opts.Int.Get( "-st" );
   double iat = Opts.Float.Get( "-iat" );
   if (iat < 1.0) // If in [0.0,1.0), then it is expressed as a percentage of '-st'
      Server::immigrants_acceptance_threshold = iat * data.stagnation_tolerance;
   else // if '>= 1.0', it is an absolute value
      Server::immigrants_acceptance_threshold = static_cast<long int>(iat);
}

void pee_clone( Population* original, int idx_original, Population* copy, int idx_copy )
{
#ifdef PROFILING
   util::Timer t_clone;
#endif

   const GENOME_TYPE* const org = original->genome + idx_original*data.number_of_bits;
   GENOME_TYPE* cpy = copy->genome + idx_copy*data.number_of_bits;

   for( int i = 0; i < data.number_of_bits; ++i ) cpy[i] = org[i];

   copy->fitness[idx_copy] = original->fitness[idx_original];

#ifdef PROFILING
   data.time_gen_clone   += t_clone.elapsed();
   data.time_total_clone += t_clone.elapsed();
#endif
}

int pee_tournament( const float* fitness )
{
#ifdef PROFILING
   util::Timer t_tournament;
#endif

   int idx_winner = (int)(random_number() * data.population_size);
   float fitness_winner = fitness[idx_winner];

   for( int t = 1; t < data.tournament_size; ++t )
   {
      int idx_competitor = (int)(random_number() * data.population_size);
      const float fitness_competitor = fitness[idx_competitor];

      if( fitness_competitor < fitness_winner ) 
      {
         fitness_winner = fitness_competitor;
         idx_winner = idx_competitor;
      }
   }

#ifdef PROFILING
   data.time_gen_tournament   += t_tournament.elapsed();
   data.time_total_tournament += t_tournament.elapsed();
#endif

   return idx_winner;
}

void pee_send_individual( Population* population )
{
#ifdef PROFILING
   util::Timer t_send;
#endif

   static bool firstcall = true;
   if( firstcall ) {
      firstcall = false;
      return;
   }

   //std::cerr << data.peers.size() << std::endl;
   for( int i = 0; i < data.peers.size(); i++ )
   { 
      if( random_number() < data.peers[i].frequency )
      {
         //Testa se o thread ainda está mandando o indivíduo, escolhido na geração anterior, para a ilha.
         if( data.pool->isrunning[i] ) continue;

         if (!Poco::ThreadPool::defaultPool().available())
         {
            std::cerr << "No more thread available in defaultPool()\n";

            /* Force a collect */
            Poco::ThreadPool::defaultPool().collect();

            if (!Poco::ThreadPool::defaultPool().available())
            {
               /* Try to increase the capacity of defaultPool */
               try {
                  Poco::ThreadPool::defaultPool().addCapacity(1);
               } catch (Poco::Exception& exc) {
                  std::cerr << "ThreadPool: " << exc.displayText() << std::endl;
               }
            }

            /* If we still don't have enough threads, just skip! */
            if (!Poco::ThreadPool::defaultPool().available())
            {
               poco_warning( Common::m_logger, "pee_send_individual(): Still no available threads, skipping..." );
               continue;
            }
         }
         //std::cerr << "Poco::ThreadPool: available " << Poco::ThreadPool::defaultPool().available() << " | allocated: " << Poco::ThreadPool::defaultPool().allocated() << "\n";

         const int idx = pee_tournament( population->fitness );

         std::stringstream results;
         results <<  population->fitness[idx] << " ";
         for( int j = 0; j < data.number_of_bits; j++ )
            results <<  static_cast<int>(population->genome[idx * data.number_of_bits + j]); /* NB: A cast to 'int' is necessary here otherwise it will send characters instead of digits when GENOME_TYPE == char */

         delete data.pool->clients[i]; delete data.pool->ss[i];

         data.pool->ss[i] = new StreamSocket();
         data.pool->clients[i] = new Client( *(data.pool->ss[i]), data.peers[i].address.c_str(), results.str(), data.pool->isrunning[i], data.pool->mutexes[i] );
         Poco::ThreadPool::defaultPool().start( *(data.pool->clients[i]) );

         if (data.verbose)
         {
#ifdef NDEBUG
            std::cerr << "^";
#else
            std::cerr << "\nSending Individual Thread[" << i << "] to " << data.peers[i].address << ": " << population->fitness[idx] << std::endl;
#endif
         }
      }
   }

#ifdef PROFILING
   data.time_gen_send    = t_send.elapsed();
   data.time_total_send += t_send.elapsed();
#endif
}

int pee_receive_individual( GENOME_TYPE* immigrants )
{
#ifdef PROFILING
   util::Timer t_receive;
#endif

   int slot; int nImmigrants = 0;
   while( !Server::m_ready.empty() && nImmigrants < data.immigrants_size )
   {
      {
         Poco::FastMutex::ScopedLock lock( Server::m_mutex );

         slot = Server::m_ready.front();
         Server::m_ready.pop();
      }

      int offset;
      char *tmp = Server::m_immigrants[slot].data(); 

      sscanf( tmp, "%f%n", &Server::m_fitness[slot], &offset );
      tmp += offset + 1; 

      //std::cerr << "\nReceive::Receiving[slot=" << slot << "]: " << Server::m_immigrants[slot].data() << std::endl;
      if (data.verbose)
      {
#ifdef NDEBUG
         std::cerr << 'v';
#else
         std::cerr << "\nReceive::Receiving[slot=" << slot << "]: " << Server::m_fitness[slot] << std::endl;
#endif
      }

      /* It is possible that the number of char bits in 'tmp' is smaller than
       * 'number_of_bits', for instance when the parameter '-nb' of an external
       * island is smaller than the '-nb' of the current island. The line below
       * handles this case (it also takes into account the offset). */
      int chars_to_convert = std::min((int) data.number_of_bits, (int) Server::m_immigrants[slot].size() - offset - 1);
      //std::cerr << "\n[" << offset << ", " << Server::m_immigrants[slot].size() << ", " << chars_to_convert << ", " << data.number_of_bits << "]\n";
      for( int i = 0; i < chars_to_convert && tmp[i] != '\0'; i++ )
      {
         assert(tmp[i]-'0'==1 || tmp[i]-'0'==0); // In debug mode, assert that each value is either '0' or '1'
         immigrants[nImmigrants * data.number_of_bits + i] = static_cast<bool>(tmp[i] - '0'); /* Ensures that the allele will be binary (0 or 1) regardless of the received value--this ensures it would work even if a communication error occurs (or a malicious message is sent). */
      }
      nImmigrants++;

      {
         Poco::FastMutex::ScopedLock lock( Server::m_mutex );

         Server::m_freeslots.push(slot);
      }

   }

#ifdef PROFILING
   data.time_gen_receive    = t_receive.elapsed();
   data.time_total_receive += t_receive.elapsed();
#endif

   return nImmigrants;
}

unsigned long pee_evaluate( Population* descendentes, Population* antecedentes, int* nImmigrants )
{
#ifdef PROFILING
   util::Timer t_evaluate;
   unsigned long sum_size_gen = 0;
   //int max_size = 0;
#endif

#ifdef PROFILING
#pragma omp parallel for reduction(+:sum_size_gen)
#else
#pragma omp parallel for
#endif
   for( int i = 0; i < data.population_size; i++ )
   {
      int allele = 0;
      data.size[i] = decode( descendentes->genome + (i * data.number_of_bits), &allele, data.phenotype + (i * data.max_size_phenotype), data.ephemeral + (i * data.max_size_phenotype), 0, data.initial_symbol );
#ifdef PROFILING
      sum_size_gen += data.size[i];
      //if( max_size < data.size[i] ) max_size = data.size[i];
#endif
   }
#ifdef PROFILING
   //std::cout << sum_size_gen/(double)data.population_size << " " << max_size << std::endl;
   data.sum_size += sum_size_gen;
#endif

   int index[data.best_size];

   if( data.parallel_version )
   {
      acc_interpret( data.phenotype, data.ephemeral, data.size, 
#ifdef PROFILING
      sum_size_gen, 
#endif
      descendentes->fitness, data.population_size, &pee_send_individual, &pee_receive_individual, antecedentes, nImmigrants, index, &data.best_size, 0, 0, ALPHA );
   }
   else
   {
      seq_interpret( data.phenotype, data.ephemeral, data.size, 
#ifdef PROFILING
      sum_size_gen, 
#endif
      descendentes->fitness, data.population_size, index, &data.best_size, 0, 0, ALPHA );

      *nImmigrants = pee_receive_individual( antecedentes->genome );
   }

   for( int i = 0; i < data.best_size; i++ )
   {
      if( descendentes->fitness[index[i]] < data.best_individual.fitness[i] )
      {
         Server::stagnation = 0;
         pee_clone( descendentes, index[i], &data.best_individual, i );
      }
      else
      {
         ++Server::stagnation;
      }
   }

#ifdef PROFILING
   data.time_gen_evaluate     = t_evaluate.elapsed();
   data.time_total_evaluate  += t_evaluate.elapsed();
   data.gpops_gen_evaluate    = (sum_size_gen * data.nlin) / t_evaluate.elapsed();
#endif

   return Server::stagnation;
}

void pee_generate_population( Population* antecedentes, Population* descendentes, int* nImmigrants )
{
#ifdef PROFILING
   util::Timer t_generate;
#endif

   for( int i = 0; i < data.population_size; ++i)
   {
      for( int j = 0; j < data.number_of_bits; j++ )
      {
         antecedentes->genome[i * data.number_of_bits + j] = (random_number() < 0.5) ? 1 : 0;
      }
   }
   pee_evaluate( antecedentes, descendentes, nImmigrants );

#ifdef PROFILING
   data.time_generate = t_generate.elapsed();
#endif
}

void pee_crossover( const GENOME_TYPE* father, const GENOME_TYPE* mother, GENOME_TYPE* offspring1, GENOME_TYPE* offspring2 )
{
#ifdef PROFILING
   util::Timer t_crossover;
#endif

   if (RNG::Probability(TWOPOINT_CROSSOVER_PROBABILITY))
   {
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
   } else {
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
   }
#ifdef PROFILING
   data.time_gen_crossover   += t_crossover.elapsed();
   data.time_total_crossover += t_crossover.elapsed();
#endif
}

void pee_mutation( GENOME_TYPE* genome )
{
#ifdef PROFILING
   util::Timer t_mutation;
#endif
   /* First the number of bit positions that will be mutated is chosen
    * (num_bits_mutated), based on 'mutation_rate', which defines the maximum
    * fraction of the vector of bits that can be mutated at once. */
   const int max_bits_mutated = ceil(data.mutation_rate * data.number_of_bits);
   int num_bits_mutated = (int) (random_number() * (max_bits_mutated+1));

   if (num_bits_mutated == 0) return; // lucky guy, no mutation for him...

   if (RNG::Probability(BITFLIP_MUTATION_PROBABILITY))
   {
      //////////////////////////////////////////////////////////////////////////
      // Bit (allele) mutation
      //////////////////////////////////////////////////////////////////////////

      /* Then, each position is selected at random and its value is swapped. */
      while( num_bits_mutated-- > 0 )
      {
         int bit = (int)(random_number() * data.number_of_bits);
         genome[bit] = !genome[bit];
      }
   } else {
      //////////////////////////////////////////////////////////////////////////
      // Shrink mutation
      //////////////////////////////////////////////////////////////////////////

      /* An illustrative example of how this mutation works (it is very useful
         for simplifying solutions):

            Original genome:

                               ,> end
               [AAA|AAA|###|###|BBB|BBB|BBB]
                       L start


            Mutated:

                               ,> end
               [AAA|AAA|BBB|BBB|BBB|   |   ]
                       L start
       */

      /* This mutation works on entire genes, not single bits. Because of that,
         first we need to convert num_bits_mutated to a multiple of
         bits_per_gene (the next multiple of bits_per_gene). This equation
         below does exactly this. Note that the maximum number of bits to
         shrink is limited proportionally by 'mutation_rate'.
      */

      if (RNG::Probability(AGGRESSIVE_SHRINK_MUTATION_PROBABILITY))
         num_bits_mutated = (int) (random_number() * (data.number_of_bits));

      int number_of_bits_to_shrink = int((num_bits_mutated+(data.bits_per_gene-1))/data.bits_per_gene)*data.bits_per_gene;

      /* Now we randomly choose the starting gene (again, this operates only on
         multiple of bits_per_gene): */
      int start = ((int)(random_number() * data.number_of_bits))/data.bits_per_gene * data.bits_per_gene;

      /* What if 'number_of_bits + gene_position' is greater than the size of
         the genome? We need to handle this situation: */
      int end = std::min(int(start+number_of_bits_to_shrink), int(data.number_of_bits));

      // Using memmove (overlapping) instead of a for-loop for efficiency
      memmove(&genome[start], &genome[end], (data.number_of_bits-end)*sizeof(GENOME_TYPE));
   }
#ifdef PROFILING
   data.time_gen_mutation   += t_mutation.elapsed();
   data.time_total_mutation += t_mutation.elapsed();
#endif
}

void pee_print_best( FILE* out, int generation, int print_mode ) 
{
   pee_individual_print( &data.best_individual, 0, out, generation, data.argc, data.argv, print_mode );
}

#ifdef PROFILING
void pee_print_time( bool total ) 
{

   double time_evolve     = total ? data.time_total_evolve : data.time_gen_evolve;
   double time_evaluate   = total ? data.time_total_evaluate : data.time_gen_evaluate;
   double time_crossover  = total ? data.time_total_crossover : data.time_gen_crossover;
   double time_mutation   = total ? data.time_total_mutation : data.time_gen_mutation;
   double time_clone      = total ? data.time_total_clone : data.time_gen_clone;
   double time_tournament = total ? data.time_total_tournament : data.time_gen_tournament;
   double time_send       = total ? data.time_total_send : data.time_gen_send;
   double time_receive    = total ? data.time_total_receive : data.time_gen_receive;
   double gpops_evaluate  = total ? (data.sum_size * data.nlin) / data.time_total_evaluate : data.gpops_gen_evaluate;

   printf(", time_generate: %lf, time_evolve: %lf, time_evaluate: %lf, time_crossover: %lf, time_mutation: %lf, time_clone: %lf, time_tournament: %lf, time_send: %lf, time_receive: %lf", data.time_generate, time_evolve, time_evaluate, time_crossover, time_mutation, time_clone, time_tournament, time_send, time_receive);
   if( data.parallel_version )
   {
      acc_print_time(total, data.sum_size);
   }
   else 
   {
      seq_print_time(total, data.sum_size);
   }
   printf(", gpops_evaluate: %lf\n", gpops_evaluate);
}
#endif

int pee_evolve()
{
   /* Initialize the RNG seed */
   RNG::Seed(data.seed);

   /*
      Pseudo-code for evolve:

   1: Create (randomly) the initial population P
   2: Evaluate all individuals (programs) of P
   3: for generation ← 1 to NG do
      4: Copy the best (elitism) individuals of P to the temporary population Ptmp
      5: while |Ptmp| < |P| do
         6: Select and copy from P two fit individuals, p1 and p2
         7: if [probabilistically] crossover then
            8: Recombine p1 and p2, creating the children p1' and p2'
            9: p1 ← p1' ; p2 ← p2'
         10: end if
         11: if [probabilistically] mutation then
            12: Apply mutation operators in p1 and p2, generating p1' and p2'
            13: p1 ← p1' ; p2 ← p2'
         14: end if
         15: Insert p1 and p2 into Ptmp
      16: end while
      17: Evaluate all individuals (programs) of Ptmp
      18: P ← Ptmp; then discard Ptmp
   19: end for
   20: return the best individual so far
   */

#ifdef PROFILING
   data.time_total_evaluate    = 0.0;
   data.time_total_crossover   = 0.0;
   data.time_total_mutation    = 0.0;
   data.time_total_clone       = 0.0;
   data.time_total_tournament  = 0.0;
   data.time_total_send        = 0.0;
   data.time_total_receive     = 0.0;
#endif
   
   srand( data.seed );

   Population antecedentes, descendentes;

   antecedentes.fitness = new float[data.population_size];
   descendentes.fitness = new float[data.population_size];

   antecedentes.genome = new GENOME_TYPE[data.population_size * data.number_of_bits];
   descendentes.genome = new GENOME_TYPE[data.population_size * data.number_of_bits];

   data.best_individual.fitness = new float[data.best_size];
   data.best_individual.genome = new GENOME_TYPE[data.best_size * data.number_of_bits];
   for( int i = 0; i < data.best_size; i++ )
   {
      data.best_individual.fitness[i] = std::numeric_limits<float>::max();
   }

   int nImmigrants;

#ifdef PROFILING
   util::Timer t_gen_evolve;
#endif

   // 1 e 2:
   //cerr << "\nGeneration[0]  ";
   pee_generate_population( &antecedentes, &descendentes, &nImmigrants );

#ifdef PROFILING
      data.time_total_evolve = t_gen_evolve.elapsed();
#endif

   // 3:
   int geracao;
   for( geracao = 1; geracao <= data.generations; ++geracao )
   {
#ifdef PROFILING
      t_gen_evolve.restart();

      data.time_gen_crossover  = 0.0;
      data.time_gen_mutation   = 0.0;
      data.time_gen_clone      = 0.0;
      data.time_gen_tournament = 0.0;
#endif

      // 4:
      if( data.elitism ) 
      {
         pee_clone( &data.best_individual, 0, &descendentes, nImmigrants );
         nImmigrants++;
      }

      //std::cerr << "\nnImmigrants[generation: " << geracao << "]: " << nImmigrants << std::endl;

      // 5
      // TODO: Parallelize this loop!
      for( int i = nImmigrants; i < data.population_size; i += 2 )
      {
         // 6:
         int idx_father = pee_tournament( antecedentes.fitness );
         int idx_mother = pee_tournament( antecedentes.fitness );

         // 7:
         if( random_number() < data.crossover_rate )
         {
            // 8 e 9:
            if( i < ( data.population_size - 1 ) )
            {
               pee_crossover( antecedentes.genome + (idx_father * data.number_of_bits), antecedentes.genome + (idx_mother * data.number_of_bits), descendentes.genome + (i * data.number_of_bits), descendentes.genome + ((i + 1) * data.number_of_bits));
            }
            else 
            {
               pee_crossover( antecedentes.genome + (idx_father * data.number_of_bits), antecedentes.genome + (idx_mother * data.number_of_bits), descendentes.genome + (i * data.number_of_bits), descendentes.genome + (i * data.number_of_bits));
            }
         } // 10
         else 
         {
            // 9:
            pee_clone( &antecedentes, idx_father, &descendentes, i );
            if( i < ( data.population_size - 1 ) )
            {
               pee_clone( &antecedentes, idx_mother, &descendentes, i + 1 );
            }
         } // 10

         // 11, 12, 13, 14 e 15:
         pee_mutation( descendentes.genome + (i * data.number_of_bits) );
         if( i < ( data.population_size - 1 ) )
         {
            pee_mutation( descendentes.genome + ((i + 1) * data.number_of_bits) );
         }
      } // 16

      // 17:
      if (pee_evaluate( &descendentes, &antecedentes, &nImmigrants ) > data.stagnation_tolerance) geracao = data.generations;

      // 18:
      swap( &antecedentes, &descendentes );

#ifdef PROFILING
      data.time_gen_evolve    = t_gen_evolve.elapsed();
      data.time_total_evolve += t_gen_evolve.elapsed();
#endif

      if (data.verbose)
      {
         if (Server::stagnation == 0 || geracao < 2) {
            if (data.machine) { // Output meant to be consumed by scripts, not humans
               pee_print_best(stdout, geracao, 1);
#ifdef PROFILING
               pee_print_time(false);
#endif
            } else
               pee_print_best(stdout, geracao, 0);

         }
         else std::cerr << '.';
      }
   } // 19


   // Clean up
   delete[] antecedentes.genome;
   delete[] antecedentes.fitness;
   delete[] descendentes.genome;
   delete[] descendentes.fitness;

   return geracao;
}

void pee_destroy()
{
   // Wait for awhile then kill the non-finished threads. This ensures a
   // graceful termination and prevents segmentation faults at the end
   Poco::ThreadPool::defaultPool().stopAll();

   delete[] data.best_individual.genome;
   delete[] data.best_individual.fitness;
   delete[] data.phenotype;
   delete[] data.ephemeral;
   delete[] data.size;
   delete[] Server::m_immigrants;
   delete[] Server::m_fitness;

   delete data.pool;

   if( !data.parallel_version ) {seq_interpret_destroy();}
}
