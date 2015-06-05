#include <stdlib.h>
#include <stdio.h> 
#include <ctime>
#include "util/CmdLineParser.h"
#include "pee.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct t_individual { int* genome; double fitness; };


int main(int argc, char **argv)
{
   CmdLine::Parser Opts( argc, argv );

   Opts.Int.Add( "-ni", "--number_of_inputs" );
   Opts.Int.Add( "-nm", "--number_of_models" );
   Opts.String.Add( "-tr", "--training_set" );
   Opts.String.Add( "-te", "--test_set" );

   Opts.Process();

   int ninput = Opts.Int.Get("-ni");   
   int nmodel = Opts.Int.Get("-nm");   
   const char* training = Opts.String.Get("-tr").c_str();
   const char* test = Opts.String.Get("-te").c_str();


/** ****************************************************************** **/
/** ************************** TRAINING DATA ************************* **/
/** ****************************************************************** **/

   FILE *arqentra;
   arqentra = fopen(training,"r");
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

   arqentra = fopen(training,"r");
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

   init( input, model, obs, nlin, argc, argv );
   Individual best_individual = evolve();

   printf("Conjunto Treinamento:\n");
   individual_print( &best_individual, stderr );

   delete[] obs;

   for( int i = 0; i < nlin; ++i )
     delete [] input[i];
   delete [] input;

   for( int i = 0; i < nlin; ++i )
     delete [] model[i];
   delete [] model;

/** ****************************************************************** **/
/** **************************** TEST DATA *************************** **/
/** ****************************************************************** **/

   arqentra = fopen(test,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }
   for( nlin = 0; fgets( linha,200,arqentra ) != NULL; nlin++ );
   fclose (arqentra);

   input  = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new double[ninput];
   model     = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new double[nmodel];
   obs    = new double[nlin];

   arqentra = fopen(test,"r");
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


   init( input, model, obs, nlin, argc, argv );
   evaluate_best( &best_individual );
   printf("Conjunto Teste:\n");
   individual_print( &best_individual, stderr );

   delete[] obs;

   for( int i = 0; i < nlin; ++i )
     delete [] input[i];
   delete [] input;

   for( int i = 0; i < nlin; ++i )
     delete [] model[i];
   delete [] model;

   return 0;
}
