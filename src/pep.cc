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
#include "util/CmdLineParser.h"
#include "interpreter/cpu.h"
#include "pep.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { int nlin; Symbol* phenotype; double* ephemeral; unsigned size; double* vector; int prediction; } data;


/** ****************************************************************** **/
/** ************************* MAIN FUNCTIONS ************************* **/
/** ****************************************************************** **/

void pep_init( double** input, double** model, double* obs, int nlin, int argc, char **argv ) 
{
   CmdLine::Parser Opts( argc, argv );

   Opts.String.Add( "-run", "--program_file" );
   Opts.Bool.Add( "-pred", "--prediction" );
   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.Process();
   const char* file = Opts.String.Get("-run").c_str();

   FILE *arqentra;
   arqentra = fopen(file,"r");
   if(arqentra == NULL) 
   {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }
   
   data.prediction = Opts.Bool.Get("-pred");

   fscanf(arqentra,"%d",&data.size);
   //fprintf(stdout,"%d\n",data.size);

   data.phenotype = new Symbol[data.size];
   data.ephemeral = new double[data.size];

   for( int tmp, i = 0; i < data.size; ++i )
   {
      fscanf(arqentra,"%d ",&tmp); data.phenotype[i] = (Symbol)tmp;
      //fprintf(stdout,"%d ",data.phenotype[i]);
      if( data.phenotype[i] == T_EFEMERO )
      {
         fscanf(arqentra,"%lf ",&data.ephemeral[i]);
         //fprintf(stdout,"%.12f ",data.ephemeral[i]);
      }
   }
   //fprintf(stdout,"\n");
   fclose (arqentra);

   data.nlin = nlin;

   interpret_init( data.size, input, model, obs, nlin, Opts.Int.Get("-ni"), Opts.Int.Get("-nm") );
}

void pep_interpret()
{
   if( data.prediction )
   {
      data.vector = new double[data.nlin];
      interpret( data.phenotype, data.ephemeral, data.size, data.vector, 1 );
   }
   else
   {
      data.vector = new double[1];
      interpret( data.phenotype, data.ephemeral, data.size, data.vector, 0 );
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
   delete[] data.phenotype, data.ephemeral, data.vector;
   interpret_destroy();
}
