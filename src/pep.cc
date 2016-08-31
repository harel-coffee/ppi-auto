/************************************************************************/
/**     Name:Amanda Sabatini Dufek                                     **/
/**          Douglas Adriano Augusto            Date:01/06/2015        **/
/**     Parallel Ensemble Prediction (PEP)                             **/
/************************************************************************/

/** ****************************************************************** **/
/** *************************** OBJECTIVE **************************** **/
/** ****************************************************************** **/
/**                                                                    **/
/** ****************************************************************** **/

#include <stdio.h> 
#include <stdlib.h>
#include <cmath>    
#include <string>   
#include "util/CmdLineParser.h"
#include "interpreter/accelerator.h"
#include "interpreter/sequential.h"
#include "pep.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

namespace { static struct t_data { int nlin; Symbol* phenotype; float* ephemeral; int* size; float* vector; int prediction; int version; } data; };

/** ****************************************************************** **/
/** ************************* MAIN FUNCTIONS ************************* **/
/** ****************************************************************** **/

void pep_init( float** input, int nlin, int argc, char** argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.String.Add( "-sol", "--solution" );
   Opts.Bool.Add( "-acc" );
   Opts.Bool.Add( "-pred", "--prediction" );
   Opts.Int.Add( "-ncol", "--number_of_columns" );
   Opts.Process();

   std::istringstream iss;
   std::string solution = Opts.String.Get("-sol");
   iss.str (solution);

   data.prediction = Opts.Bool.Get("-pred");
   data.version = Opts.Bool.Get("-acc");

   data.size = new int[1];
   // Put the size of the solution (number of Symbols) into the variable data.size[0]
   iss >> data.size[0];

   data.phenotype = new Symbol[data.size[0]];
   data.ephemeral = new float[data.size[0]];

   int actual_size = 0; int tmp = -1;
   for( int i = 0; i < data.size[0]; ++i )
   {
      iss >> tmp;
      if (iss)
         ++actual_size;
      else
         break; // Stops if the actual number of Symbols is less than the informed

      data.phenotype[i] = (Symbol)tmp;
      if(
#ifndef NOT_USING_T_CONST
      data.phenotype[i] == T_CONST ||
#endif
      data.phenotype[i] == T_ATTRIBUTE )
      {
         iss >> data.ephemeral[i];
      }
   }

   while (iss >> tmp) ++actual_size; // Let's check if there are still more Symbols remaining
   if (actual_size != data.size[0])
      std::cerr << "WARNING: The given size (" << data.size[0] << ") and actual size (" << actual_size << ") differ: '" << solution << "'\n";

   data.nlin = nlin;

   if( data.version )
   {
      if( acc_interpret_init( argc, argv, data.size[0], -1, 1, input, nlin, 1, data.prediction ) )
      {
         fprintf(stderr,"Error in initialization phase.\n");
      }
   }
   else
   {
      seq_interpret_init( data.size[0], input, nlin, Opts.Int.Get("-ncol") );
   }
}

void pep_interpret()
{
   if( data.prediction )
   {
      data.vector = new float[data.nlin];
      if( data.version )
      {
         acc_interpret( data.phenotype, data.ephemeral, data.size, data.vector, 1, NULL, NULL, NULL, NULL, NULL, NULL, 1, 1, 0 );
      }
      else
      {
         seq_interpret( data.phenotype, data.ephemeral, data.size, data.vector, 1, NULL, NULL, 1, 1, 0 );
      }
   }
   else
   {
      data.vector = new float[1];
      if( data.version )
      {
         acc_interpret( data.phenotype, data.ephemeral, data.size, data.vector, 1, NULL, NULL, NULL, NULL, NULL, NULL, 1, 0, 0 );
      }
      else
      {
         seq_interpret( data.phenotype, data.ephemeral, data.size, data.vector, 1, NULL, NULL, 1, 0, 0 );
      }
   }
}

void pep_print( FILE* out )
{
   if( data.prediction )
   {
      for( int i = 0; i < data.nlin; ++i )
         fprintf( out, "%.12f\n", data.vector[i] );
   }
   else
      fprintf( out, "%.12f\n", data.vector[0] );
}

void pep_destroy() 
{
   delete[] data.phenotype;
   delete[] data.ephemeral;
   delete[] data.size;
   delete[] data.vector;

   if( !data.version ) {seq_interpret_destroy();}
}
