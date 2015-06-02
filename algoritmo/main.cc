#include <stdlib.h>
#include <stdio.h> 
#include <ctime>
#include <limits>
#include "pee.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

struct t_individual { int* genome; double fitness; };


int main(int argc, char **argv)
{

   char *treinamento, *teste;
   treinamento = argv[1];
   teste = argv[2];
   int ninput = atoi(argv[3]);   
   int nmodel = atoi(argv[4]);   

/** ****************************************************************** **/
/** ************************** TRAINING DATA ************************* **/
/** ****************************************************************** **/

   FILE *arqentra;
   arqentra = fopen(treinamento,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }
   int nlin;
   char linha[200];
   for( nlin = 0; fgets( linha,200,arqentra ) != NULL; nlin++ );
   fclose (arqentra);
   int start = 0, end = nlin-1;

   double** input   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new double[ninput];
   double** model   = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new double[nmodel];
   double* obs   = new double[nlin];

   arqentra = fopen(treinamento,"r");
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

   Individual best_individual = evolve( input, model, obs, start, end );

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

   arqentra = fopen(teste,"r");
   if(arqentra == NULL) {
     printf("Nao foi possivel abrir o arquivo de entrada.\n");
     exit(-1);
   }
   for( nlin = 0; fgets( linha,200,arqentra ) != NULL; nlin++ );
   fclose (arqentra);
   start = 0, end = nlin-1;

   input  = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new double[ninput];
   model     = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new double[nmodel];
   obs    = new double[nlin];

   arqentra = fopen(teste,"r");
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

   evaluate( &best_individual, input, model, obs, start, end );
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
