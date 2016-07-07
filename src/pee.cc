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
#include <iostream> 
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

Pool::Pool( unsigned size ): ss( size, NULL), clients( size, NULL )
{
      /* Expands defaultPool() if the available number of threads is less than
       * the number of peers, otherwise the exception "No thread available" is
       * thrown. */
   if (threadpool.available() < size)
      threadpool.addCapacity(size - threadpool.available());

      isrunning = new int[size];
      for( unsigned i = 0; i < size; i++ ) isrunning[i] = 0;
}

struct Peer {
  Peer( const std::string& s, float f ):
      address( s ), frequency( f ) {}

  std::string address;
  float frequency;
};

namespace { struct t_data { Symbol initial_symbol; Population best_individual; int best_size; unsigned max_size_phenotype; int nlin; Symbol* phenotype; float* ephemeral; int* size; int verbose; int elitism; int population_size; int immigrants_size; int generations; int number_of_bits; int bits_per_gene; int bits_per_constant; int seed; int tournament_size; float mutation_rate; float crossover_rate; float interval[2]; int version; double time_total; double time_evaluate; double time_crossover; double time_mutation; double time_clone; double time_tournament; std::vector<Peer> peers; Pool* pool; unsigned long stagnation_tolerance; } data; };

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

#define swap(i, j) {Population t = *i; *i = *j; *j = t;}

//double random_number() {return (double)rand() / ((double)RAND_MAX + 1.0f);} // [0.0, 1.0)
double random_number() { return RNG::Real(); };


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

#include "pee_individual_print_function"

void pee_init( float** input, int nlin, int argc, char** argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Bool.Add( "-v", "--verbose" );

   Opts.Bool.Add( "-acc" );

   Opts.String.Add( "-error", "--function-difference", "fabs((X)-(Y))" );

   Opts.Int.Add( "-ncol", "--number-of-columns" );

   Opts.Int.Add( "-g", "--generations", 50, 1, std::numeric_limits<int>::max() );

   Opts.Int.Add( "-s", "--seed", 0, 0, std::numeric_limits<long>::max() );

   Opts.Int.Add( "-ps", "--population-size", 1024, 1, std::numeric_limits<int>::max() );
   Opts.Int.Add( "-is", "--immigrants-size", 5, 1 );
   Opts.Float.Add( "-cp", "--crossover-probability", 0.95, 0.0, 1.0 );
   Opts.Float.Add( "-mr", "--mutation-rate", 0.01, 0.0, 1.0 );
   Opts.Int.Add( "-ts", "--tournament-size", 3, 1, std::numeric_limits<int>::max() );

   Opts.Bool.Add( "-e", "--elitism" );

   Opts.Int.Add( "-nb", "--number-of-bits", 2000, 16 );
   Opts.Int.Add( "-bg", "--bits-per-gene", 8, 8 );
   Opts.Int.Add( "-bc", "--bits-per-constant", 16, 4 );

   Opts.Float.Add( "-min", "--min-constant", 0 );
   Opts.Float.Add( "-max", "--max-constant", 300 );

   Opts.String.Add( "-peers", "--address_of_island" );
 
   /* Maximum allowed number of generations without improvement [default = disabled] */
   Opts.Int.Add( "-st", "--stagnation-tolerance", numeric_limits<unsigned long>::max(), 0 );

   // processing the command-line
   Opts.Process();

   // getting the results!
   data.verbose = Opts.Bool.Get("-v");

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

   data.version = Opts.Bool.Get("-acc");
   if( data.version )
   {
      if( acc_interpret_init( argc, argv, data.max_size_phenotype, MAX_QUANT_SIMBOLOS_POR_REGRA, data.population_size, input, nlin, 0 ) )
      {
         fprintf(stderr,"Error in initialization phase.\n");
      }
   }
   else
   {
      seq_interpret_init( Opts.String.Get("-error"), data.max_size_phenotype, input, nlin, Opts.Int.Get("-ncol") );
   }

   data.stagnation_tolerance = Opts.Int.Get( "-st" );

}

void pee_clone( Population* original, int idx_original, Population* copy, int idx_copy )
{
   const int* const org = original->genome + idx_original*data.number_of_bits;
   int* cpy = copy->genome + idx_copy*data.number_of_bits;

   for( int i = 0; i < data.number_of_bits; ++i ) cpy[i] = org[i];

   copy->fitness[idx_copy] = original->fitness[idx_original];
}

//void pee_individual_print( const Population* individual, int idx, FILE* out, int print_mode )
//{
//   Symbol phenotype[data.max_size_phenotype];
//   float ephemeral[data.max_size_phenotype];
//
//   int allele = 0;
//   int size = decode( individual->genome + (idx * data.number_of_bits), &allele, phenotype, ephemeral, 0, data.initial_symbol );
//   if( !size ) { return; }
//  
//   if( print_mode )
//   {
//      fprintf( out, "{%d}\n", size );
//      for( int i = 0; i < size; ++i )
//         if( phenotype[i] == T_CONST || phenotype[i] == T_ATTRIBUTE )
//            fprintf( out, "%d %.12f ", phenotype[i], ephemeral[i] );
//         else
//            fprintf( out, "%d ", phenotype[i] );
//      fprintf( out, "\n" );
//   } 
//   else 
//      fprintf( out, "{%d} ", size );
//
//   for( int i = 0; i < size; ++i )
//      switch( phenotype[i] )
//      {
//            case T_IF_THEN_ELSE:
//               fprintf( out, "IF-THEN-ELSE " );
//               break;
//            case T_AND:
//               fprintf( out, "AND " );
//               break;
//            case T_OR:
//               fprintf( out, "OR " );
//               break;
//            case T_XOR:
//               fprintf( out, "XOR " );
//               break;
//            case T_NOT:
//               fprintf( out, "NOT " );
//               break;
//            case T_GREATER:
//               fprintf( out, "> " );
//               break;
//            case T_GREATEREQUAL:
//               fprintf( out, ">= " );
//               break;
//            case T_LESS:
//               fprintf( out, "< " );
//               break;
//            case T_LESSEQUAL:
//               fprintf( out, "<= " );
//               break;
//            case T_EQUAL:
//               fprintf( out, "= " );
//               break;
//            case T_NOTEQUAL:
//               fprintf( out, "!= " );
//               break;
//            case T_ADD:
//               fprintf( out, "+ " );
//               break;
//            case T_SUB:
//               fprintf( out, "- " );
//               break;
//            case T_MULT:
//               fprintf( out, "* " );
//               break;
//            case T_DIV:
//               fprintf( out, "/ " );
//               break;
//            case T_MEAN:
//               fprintf( out, "MEAN " );
//               break;
//            case T_MAX:
//               fprintf( out, "MAX " );
//               break;
//            case T_MIN:
//               fprintf( out, "MIN " );
//               break;
//            case T_MOD:
//               fprintf( out, "MOD " );
//               break;
//            case T_POW:
//               fprintf( out, "POW " );
//               break;
//            case T_ABS:
//               fprintf( out, "ABS " );
//               break;
//            case T_SQRT:
//               fprintf( out, "SQRT " );
//               break;
//            case T_POW2:
//               fprintf( out, "POW2 " );
//               break;
//            case T_POW3:
//               fprintf( out, "POW3 " );
//               break;
//            case T_POW4:
//               fprintf( out, "POW4 " );
//               break;
//            case T_POW5:
//               fprintf( out, "POW5 " );
//               break;
//            case T_NEG:
//               fprintf( out, "NEG " );
//               break;
//            case T_ROUND:
//               fprintf( out, "ROUND " );
//               break;
//            case T_0:
//               fprintf( out, "0 " );
//               break;
//            case T_1:
//               fprintf( out, "1 " );
//               break;
//            case T_2:
//               fprintf( out, "2 " );
//               break;
//            case T_3:
//               fprintf( out, "3 " );
//               break;
//            case T_4:
//               fprintf( out, "4 " );
//               break;
//            case T_5:
//               fprintf( out, "5 " );
//               break;
//            case T_6:
//               fprintf( out, "6 " );
//               break;
//            case T_7:
//               fprintf( out, "7 " );
//               break;
//            case T_8:
//               fprintf( out, "8 " );
//               break;
//            case T_9:
//               fprintf( out, "9 " );
//               break;
//            case T_ATTRIBUTE:
//               fprintf( out, "ATTR-%d ", (int)ephemeral[i] );
//               break;
//            case T_CONST:
//               fprintf( out, "%.12f ",  ephemeral[i] );
//               break;
//      } 
//   if( print_mode )
//   {
//      fprintf( out, "\n" );
//      fprintf( out, ":: %.12f\n", individual->fitness[idx] - ALPHA*size ); // Print the raw error, that is, without the penalization for complexity
//   }
//   else
//      fprintf( out, ":: %.12f\n", individual->fitness[idx] - ALPHA*size ); // Print the raw error, that is, without the penalization for complexity
//}

int pee_tournament( const float* fitness )
{
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

   return idx_winner;
}

void pee_send_individual( Population* population )
{
   static bool firstcall = true;
   if( firstcall )
   {
      firstcall = false;
      return;
   }

   //std::cerr << data.peers.size() << std::endl;
   for( int i = 0; i < data.peers.size(); i++ )
   { 
      if( random_number() < data.peers[i].frequency )
      {
         //Testa se o thread ainda está mandando o indivíduo, escolhido na geração anterior, para a ilha.
         //if( !(data.pool->threads[i] == NULL) && data.pool->threads[i]->isRunning() ) continue;
         if( data.pool->isrunning[i] ) continue;

         if (!data.pool->threadpool.available())
         {
            std::cerr << "No more thread available in threadpool\n";

            /* Force a collect */
            data.pool->threadpool.collect();

            if (!data.pool->threadpool.available())
            {
               /* Try to increase the capacity of defaultPool */
               try {
                  data.pool->threadpool.addCapacity(1);
               } catch (Poco::Exception& exc) {
                  std::cerr << "ThreadPool: " << exc.displayText() << std::endl;
               }
            }

            /* If we still don't have enough threads, just skip! */
            if (!data.pool->threadpool.available()) continue;
         }
         std::cerr << "Poco::ThreadPool: available " << data.pool->threadpool.available() << " | allocated: " << data.pool->threadpool.allocated() << "\n";

         const int idx = pee_tournament( population->fitness );

         std::stringstream results; //results.str(std::string());
         results <<  population->fitness[idx] << " ";
         for( int j = 0; j < data.number_of_bits; j++ )
            results <<  population->genome[idx * data.number_of_bits + j];

         //std::stringstream results; 
         //results <<  data.best_individual.fitness[0] << " ";
         //for( int j = 0; j < data.number_of_bits; j++ )
         //   results <<  data.best_individual.genome[j];

#if 0
   try {
      if (data.pool->ss[i]) data.pool->ss[i]->close();
   } catch (...) {
      std::cerr << "[send] Closing failed: connection already closed" << std::endl;
   }
   try {
      if (data.pool->ss[i]) data.pool->ss[i]->shutdown();
   } catch (...) {
      std::cerr << "[send] Shutdown failed: connection already closed" << std::endl;
   }
#endif
         delete data.pool->clients[i]; delete data.pool->ss[i];
         data.pool->ss[i] = new StreamSocket();
         data.pool->clients[i] = new Client( *(data.pool->ss[i]), data.peers[i].address.c_str(), results.str(), data.pool->isrunning[i] );
         data.pool->threadpool.start( *(data.pool->clients[i]) );

         //std::cerr << "\nSending Individual Thread[" << i << "] to " << data.peers[i].address << ": " << data.best_individual.fitness[0] << std::endl;
         std::cerr << "\nSending Individual Thread[" << i << "] to " << data.peers[i].address << ": " << population->fitness[idx] << std::endl;
         //std::cerr << results.str() << std::endl;
      }
   }
}

int pee_receive_individual( int* immigrants )
{
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
      std::cerr << "\nReceive::Receiving[slot=" << slot << "]: " << Server::m_fitness[slot] << std::endl;

      //if( Server::m_fitness[slot] < data.best_individual.fitness[0] )
      //{
      //   std::cerr << "\nReceiving[slot=" << slot << "]: " << Server::m_fitness[slot] << " " << data.best_individual.fitness[0] << std::endl; 
      //   sleep(5);
      //}

      //tmp += offset;
      //fprintf(stdout,"Receiving[slot=%d]: ",slot);
      //for( int i = 0; i < (data.number_of_bits - 1); i++ )
      //{
      //   sscanf( tmp, "%d%n", &immigrants[nImmigrants * data.number_of_bits + i], &offset );
      //   tmp += offset;
      //   fprintf(stdout,"%d ",immigrants[nImmigrants * data.number_of_bits + i]);
      //}
      //sscanf( tmp, "%d", &immigrants[nImmigrants * (data.number_of_bits - 1)] );
      //fprintf(stdout,"%d\n",immigrants[nImmigrants * (data.number_of_bits - 1)]);

      //fprintf(stdout,"Receiving[slot=%d]: ",slot);

      /* It is possible that the number of char bits in 'tmp' is smaller than
       * 'number_of_bits', for instance when the parameter '-nb' of an external
       * island is smaller than the '-nb' of the current island. The line below
       * handles this case (it also takes into account the offset). */
      int chars_to_convert = std::min((int) data.number_of_bits, (int) Server::m_immigrants[slot].size() - offset - 1);
      //std::cerr << "\n[" << offset << ", " << Server::m_immigrants[slot].size() << ", " << chars_to_convert << ", " << data.number_of_bits << "]\n";
      //fprintf(stdout,"Importante::slot[%d]:: ",slot);
      for( int i = 0; i < chars_to_convert; i++ )
      // TODO? for( int i = 0; i < chars_to_convert && tmp[i] != '\0'; i++ )
      {
         immigrants[nImmigrants * data.number_of_bits + i] = tmp[i] - '0';
         //fprintf(stdout,"%d",immigrants[nImmigrants * data.number_of_bits + i]);
      }
      nImmigrants++;
      //fprintf(stdout,"\n");

      {
         Poco::FastMutex::ScopedLock lock( Server::m_mutex );

         Server::m_freeslots.push(slot);
      }

   }
   return nImmigrants;
}

unsigned long pee_evaluate( Population* descendentes, Population* antecedentes, int* nImmigrants )
{
#pragma omp parallel for
   for( int i = 0; i < data.population_size; i++ )
   {
      int allele = 0;
      data.size[i] = decode( descendentes->genome + (i * data.number_of_bits), &allele, data.phenotype + (i * data.max_size_phenotype), data.ephemeral + (i * data.max_size_phenotype), 0, data.initial_symbol );
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

   
   int index[data.best_size];

   if( data.version )
   {
      acc_interpret( data.phenotype, data.ephemeral, data.size, descendentes->fitness, data.population_size, &pee_send_individual, &pee_receive_individual, antecedentes, nImmigrants, index, &data.best_size, 0, ALPHA );
   }
   else
   {
      seq_interpret( data.phenotype, data.ephemeral, data.size, descendentes->fitness, data.population_size, index, &data.best_size, 0, ALPHA );
      *nImmigrants = pee_receive_individual( antecedentes->genome );
   }
   //std::cout << data.best_size << std::endl;

   //bool flag = false;
   //for( int i = 0; i < data.population_size; i++ )
   //{
   //   if( antecedentes->fitness[i] < data.best_individual.fitness[0] )
   //   {
   //      cerr << antecedentes->fitness[i] << endl;
   //      flag = true;
   //   }
   //}
   
   //if( flag )
   //{
   //   printf("Método1:%d  ",flag);
   //   pee_individual_print( &data.best_individual, stdout, 0 );
   //   printf("Método2:%d  ",flag);
   //   pee_individual_print( &population[index], stdout, 0 );
   //}

   static unsigned long stagnation = 0;
   for( int i = 0; i < data.best_size; i++ )
   {
      if( descendentes->fitness[index[i]] < data.best_individual.fitness[i] )
      {
         stagnation = 0;
         pee_clone( descendentes, index[i], &data.best_individual, i );
         //printf( "IMPORTANTE::[%d]:: %.12f\n", index[i], data.best_individual.fitness[i] ); 
         //sleep(5);
      }
      else
      {
         ++stagnation;
      }
   }

   return stagnation;
}

void pee_generate_population( Population* antecedentes, Population* descendentes, int* nImmigrants )
{
   for( int i = 0; i < data.population_size; ++i)
   {
      for( int j = 0; j < data.number_of_bits; j++ )
      {
         antecedentes->genome[i * data.number_of_bits + j] = (random_number() < 0.5) ? 1 : 0;
      }
   }
   pee_evaluate( antecedentes, descendentes, nImmigrants );
}

void pee_crossover( const int* father, const int* mother, int* offspring1, int* offspring2 )
{
   if (RNG::Probability(2./3.))
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
}

void pee_mutation( int* genome )
{
   /* First the number of bit positions that will be mutated is chosen
    * (num_bits_mutated), based on 'mutation_rate', which defines the maximum
    * fraction of the vector of bits that can be mutated at once. */
   const int max_bits_mutated = ceil(data.mutation_rate * data.number_of_bits);
   int num_bits_mutated = (int) (random_number() * (max_bits_mutated+1));

   if (num_bits_mutated == 0) return; // lucky guy, no mutation for him...

   if (RNG::Probability(0.75))
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

      int number_of_bits_to_shrink = int((num_bits_mutated+(data.bits_per_gene-1))/data.bits_per_gene)*data.bits_per_gene;

      /* Now we randomly choose the starting gene (again, this operates only on
         multiple of bits_per_gene): */
      int start = ((int)(random_number() * data.number_of_bits))/data.bits_per_gene * data.bits_per_gene;

      /* What if 'number_of_bits + gene_position' is greater than the size of
         the genome? We need to handle this situation: */
      int end = std::min(int(start+number_of_bits_to_shrink), int(data.number_of_bits));

      // Using memmove (overlapping) instead of a for-loop for efficiency
      // FIXME: replace sizeof(int) by GENOME_TYPE (or something similar) when it is implemented!
      memmove(&genome[start], &genome[end], (data.number_of_bits-end)*sizeof(int));
   }
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
   //cerr << "time_evaluate: " << data.time_evaluate << ", time_crossover: " << data.time_crossover << ", time_mutation: " << data.time_mutation << ", time_clone: " << data.time_clone << ", time_tournament: " << data.time_tournament << ", time_total: " << data.time_total << "\n" << endl;
}

void pee_evolve()
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

   // -----
   util::Timer t_total;
   // -----

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

   int nImmigrants;

   // -----
   util::Timer t_generate;
   // -----
   // 1 e 2:
   //cerr << "\nGeneration[0]  ";
   pee_generate_population( &antecedentes, &descendentes, &nImmigrants );
   //cerr << ", time_generate: " <<  t_generate.elapsed() << endl;

   data.time_evaluate   = 0.0f;
   data.time_crossover  = 0.0f;
   data.time_mutation   = 0.0f;
   data.time_clone      = 0.0f;
   data.time_tournament = 0.0f;
   
   // 3:
   for( int geracao = 1; geracao <= data.generations; ++geracao )
   {
      // 4:
      if( data.elitism ) 
      {
         // -----
         util::Timer t_clone;
         // -----
         pee_clone( &data.best_individual, 0, &descendentes, nImmigrants );
         data.time_clone += t_clone.elapsed();
         nImmigrants++;
      }

      //std::cerr << "\nnImmigrants[generation: " << geracao << "]: " << nImmigrants << std::endl;

      // 5
      // TODO: Parallelize this loop!
      for( int i = nImmigrants; i < data.population_size; i += 2 )
      {
         // -----
         util::Timer t_tournament;
         // -----
         // 6:
         int idx_father = pee_tournament( antecedentes.fitness );
         int idx_mother = pee_tournament( antecedentes.fitness );
         data.time_tournament += t_tournament.elapsed();

         // 7:
         if( random_number() < data.crossover_rate )
         {
            // -----
            util::Timer t_crossover;
            // -----
            // 8 e 9:
            if( i < ( data.population_size - 1 ) )
            {
               pee_crossover( antecedentes.genome + (idx_father * data.number_of_bits), antecedentes.genome + (idx_mother * data.number_of_bits), descendentes.genome + (i * data.number_of_bits), descendentes.genome + ((i + 1) * data.number_of_bits));
            }
            else 
            {
               pee_crossover( antecedentes.genome + (idx_father * data.number_of_bits), antecedentes.genome + (idx_mother * data.number_of_bits), descendentes.genome + (i * data.number_of_bits), descendentes.genome + (i * data.number_of_bits));
            }
            data.time_crossover += t_crossover.elapsed();
         } // 10
         else 
         {
            // -----
            util::Timer t_clone;
            // -----
            // 9:
            pee_clone( &antecedentes, idx_father, &descendentes, i );
            if( i < ( data.population_size - 1 ) )
            {
               pee_clone( &antecedentes, idx_mother, &descendentes, i + 1 );
            }
            data.time_clone += t_clone.elapsed();
         } // 10

         // -----
         util::Timer t_mutation;
         // -----
         // 11, 12, 13, 14 e 15:
         pee_mutation( descendentes.genome + (i * data.number_of_bits) );
         if( i < ( data.population_size - 1 ) )
         {
            pee_mutation( descendentes.genome + ((i + 1) * data.number_of_bits) );
         }
         data.time_mutation += t_mutation.elapsed();
      } // 16

      // -----
      util::Timer t_evaluate;
      // -----
      // 17:
      //cerr << "\nGeneration[" << geracao << "]  ";
      if (pee_evaluate( &descendentes, &antecedentes, &nImmigrants ) > data.stagnation_tolerance) geracao = data.generations;
      double time = t_evaluate.elapsed(); 
      //cerr << ", time_evaluate: " << time << endl;
      data.time_evaluate += time;

      // 18:
      swap( &antecedentes, &descendentes );

      if( data.verbose ) 
      {
         printf("\n[%d] ", geracao);
         pee_individual_print( &data.best_individual, 0, stdout, 0 );
      }
   } // 19

   // join all threads
   // Clean up
   delete[] antecedentes.genome, antecedentes.fitness, descendentes.genome, descendentes.fitness; 

   data.time_total = t_total.elapsed();
}

void pee_destroy()
{
   delete[] data.best_individual.genome, data.best_individual.fitness, data.phenotype, data.ephemeral, data.size;
   delete[] Server::m_immigrants, Server::m_fitness;
   delete data.pool;
   if( !data.version ) {seq_interpret_destroy();}
}
