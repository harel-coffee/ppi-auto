#include <stdlib.h>
#include <stdio.h> 
#include <cmath>    
#include <ctime>
#include "util/CmdLineParser.h"
#include "pee.h"
#include "pep.h"


/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

int read( const char* dataset, double**& input, double**& model, double*& obs, int ninput, int nmodel, int& nlin, int prediction )
{
   char c;
   FILE *arqentra;

   arqentra = fopen(dataset,"r");
   if(arqentra == NULL) {
     fprintf(stderr, "Could not open file for reading (%s).\n", dataset);
     return 1;
   }
 
   nlin = 0;
   while( (c = fgetc(arqentra)) != EOF )
       if ( c == '\n' ) {nlin++;}

   input   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new double[ninput];
   model   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new double[nmodel];
   obs   = new double[nlin]();

   rewind(arqentra);
   for( int i = 0; i < nlin; i++ )
   {
      for( int j = 0; j < ninput; j++ )
      {
         //fprintf(stdout,"%d ",fscanf(arqentra,"%lf,",&input[i][j]));
         //fprintf(stdout,"%lf\n",input[i][j]);
         if( fscanf(arqentra,"%lf,",&input[i][j]) != 1 || isnan(input[i][j]) || isinf(input[i][j]) )
         {
            fprintf(stderr, "Invalid input.\n");
            return 2;
         }
      }
      for( int j = 0; j < nmodel; j++ )
      {
         if( prediction && j == (nmodel-1) )
         {
            if( fscanf(arqentra,"%lf%*[^\n]",&model[i][j]) != 1 || isnan(model[i][j]) || isinf(model[i][j]) )
            {
               fprintf(stderr, "Invalid model.\n");
               return 2;
            }
         }
         else
         {
            if( fscanf(arqentra,"%lf,",&model[i][j]) != 1 || isnan(model[i][j]) || isinf(model[i][j]) )
            {
               fprintf(stderr, "Invalid model.\n");
               return 2;
            }
         }
      }
      if( !prediction )
      {
         if( fscanf(arqentra,"%lf",&obs[i]) != 1 || isnan(obs[i]) || isinf(obs[i]) )
            {
               fprintf(stderr, "Invalid observation.\n");
               return 2;
            }
      }
   }
   fclose (arqentra);
   return 0;
}

void destroy( double** input, double** model, double* obs, int nlin )
{
   delete[] obs;

   for( int i = 0; i < nlin; ++i )
     delete [] input[i];
   delete [] input;

   for( int i = 0; i < nlin; ++i )
     delete [] model[i];
   delete [] model;
}


int main(int argc, char **argv)
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Bool.Add( "-pred", "--prediction" );
   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.String.Add( "-d", "--dataset" );
   Opts.String.Add( "-run", "--program_file" );

   Opts.Process();

   int ninput = Opts.Int.Get("-ni");   
   int nmodel = Opts.Int.Get("-nm");   
   const char* dataset = Opts.String.Get("-d").c_str();

   
   int nlin;
   double** input;
   double** model;
   double* obs;

   if( read( dataset, input, model, obs, ninput, nmodel, nlin, Opts.Bool.Get("-pred") ) ) {return 0;}

   if( Opts.String.Found("-run") )
   {
      pep_init( input, model, obs, nlin, argc, argv );
      pep_interpret();
      pep_print( stdout );
      pep_destroy();
   }
   else
   {
      srand( time(NULL) );
      pee_init( input, model, obs, nlin, argc, argv );
      pee_evolve();
      pee_print_best( stdout, 1 );
      pee_destroy();
   }
  
   destroy(input, model, obs, nlin);

   return 0;
}
