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
#include <limits>
#include "util/CmdLineParser.h"
#include "interpreter/cpu.h"
#include "pep.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int nlin; Symbol* phenotype; double* ephemeral; int size; double value; } data;


/** ****************************************************************** **/
/** ************************* MAIN FUNCTIONS ************************* **/
/** ****************************************************************** **/

void pep_init( double** input, double** model, double* obs, int nlin, int argc, char **argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.String.Add( "-f", "--file" );
   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.Process();
   const char* file = Opts.String.Get("-f").c_str();

   FILE *arqentra;
   arqentra = fopen(file,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }

   fscanf(arqentra,"%d",&data.size);

   data.phenotype = new Symbol[data.size];
   data.ephemeral = new double[data.size];

   for( int i = 0, j = i; i < data.size; ++i, ++j )
   {
      fscanf(arqentra,"%d ",&data.phenotype[j]);
      if( data.phenotype[j] == T_EFEMERO )
      {
         fscanf(arqentra,"%lf ",&data.ephemeral[j]); j++;
      }
   }
   fclose (arqentra);

   data.nlin = nlin;

   interpret_init( data.size, input, model, obs, nlin, Opts.Int.Get("-ni"), Opts.Int.Get("-nm") );
}

void pep_interpret()
{
   data.value = interpret( data.phenotype, data.ephemeral, data.size );

   if( isnan( data.value ) || isinf( data.value ) ) {data.value = std::numeric_limits<float>::max();} 
   data.value = data.value/data.nlin + 0.00001*data.size; 
}

void pep_destroy() {delete[] data.phenotype, data.ephemeral;}

void pep_print( FILE* out )
{
   fprintf( out, "%.12f\n", data.value );
   pep_destroy();
}
