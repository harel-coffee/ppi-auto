#include <stdlib.h>
#include <stdio.h> 
#include <ctime>
#include "util/CmdLineParser.h"
#include "pee.h"
#include "pep.h"


int main(int argc, char **argv)
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.String.Add( "-d", "--dataset" );
   Opts.Bool.Add( "-f", "--file" );

   Opts.Process();

   int ninput = Opts.Int.Get("-ni");   
   int nmodel = Opts.Int.Get("-nm");   
   const char* dataset = Opts.String.Get("-d").c_str();


   FILE *arqentra;
   arqentra = fopen(dataset,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }
   int nlin;
   char linha[200];
   for( nlin = 0; fgets( linha,200,arqentra ) != NULL; nlin++ );
   fclose (arqentra);

   double** input   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new double[ninput];
   double** model   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new double[nmodel];
   double* obs   = new double[nlin];

   arqentra = fopen(dataset,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }

   for( int i = 0; i < nlin; i++ )
   {
     for( int j = 0; j < ninput; j++ )
     {
       fscanf(arqentra,"%lf,",&input[i][j]);
     }
     for( int j = 0; j < nmodel; j++ )
     {
       fscanf(arqentra,"%lf,",&model[i][j]);
     }
     fscanf(arqentra,"%lf",&obs[i]);
   }
   fclose (arqentra);

   srand( time(NULL) );

   if( Opts.Bool.Get("-f") )
   {
      pep_init( input, model, obs, nlin, argc, argv );
      pep_interpret();
      pep_print( stderr );
   }
   else
   {
      pee_init( input, model, obs, nlin, argc, argv );
      pee_evolve();
      pee_print_best( stderr, 1 );
   }
  

   delete[] obs;

   for( int i = 0; i < nlin; ++i )
     delete [] input[i];
   delete [] input;

   for( int i = 0; i < nlin; ++i )
     delete [] model[i];
   delete [] model;

   return 0;
}
