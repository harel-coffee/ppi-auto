#include <stdlib.h>
#include <stdio.h> 
#include <cmath>    
#include "util/CmdLineParser.h"
#include "server/server.h"
#include "pee.h"
#include "pep.h"

//TODO fazer comentários; passar para inglês

/** ****************************************************************** **/
/** *********************** AUXILIARY FUNCTIONS ********************** **/
/** ****************************************************************** **/

int read( const char* dataset, float**& input, float**& model, float*& obs, int ninput, int nmodel, int& nlin, int prediction )
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

   input   = new float*[nlin];
   for( int i = 0; i < nlin; i++ )
     input[i] = new float[ninput];
   model   = new float*[nlin];
   for( int i = 0; i < nlin; i++ )
     model[i] = new float[nmodel];
   obs   = new float[nlin]();

   rewind(arqentra);
   for( int i = 0; i < nlin; i++ )
   {
      for( int j = 0; j < ninput; j++ )
      {
         //fprintf(stdout,"%d ",fscanf(arqentra,"%f,",&input[i][j]));
         //fprintf(stdout,"%f\n",input[i][j]);
         if( fscanf(arqentra,"%f,",&input[i][j]) != 1 || isnan(input[i][j]) || isinf(input[i][j]) )
         {
            fprintf(stderr, "Invalid input at line %d and column %d.\n", i, j);
            return 2;
         }
      }
      for( int j = 0; j < nmodel; j++ )
      {
         if( prediction && j == (nmodel-1) )
         {
            if( fscanf(arqentra,"%f%*[^\n]",&model[i][j]) != 1 || isnan(model[i][j]) || isinf(model[i][j]) )
            {
               fprintf(stderr, "Invalid model at line %d and column %d.\n", i, j);
               return 2;
            }
         }
         else
         {
            if( fscanf(arqentra,"%f,",&model[i][j]) != 1 || isnan(model[i][j]) || isinf(model[i][j]) )
            {
               fprintf(stderr, "Invalid model at line %d and column %d.\n", i, j);
               return 2;
            }
         }
      }
      if( !prediction )
      {
         if( fscanf(arqentra,"%f",&obs[i]) != 1 || isnan(obs[i]) || isinf(obs[i]) )
            {
               fprintf(stderr, "Invalid observation at line %d.\n", i);
               return 2;
            }
      }
   }
   fclose (arqentra);
   return 0;
}

void destroy( float** input, float** model, float* obs, int nlin )
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
   Opts.Int.Add( "-port", "--number_of_port" );
   Opts.String.Add( "-d", "--dataset" );
   Opts.String.Add( "-run", "--program_file" );

   Opts.Process();

   int ninput = Opts.Int.Get("-ni");   
   int nmodel = Opts.Int.Get("-nm");   
   int port = Opts.Int.Get("-port");
   const char* dataset = Opts.String.Get("-d").c_str();

   
   int nlin;
   float** input;
   float** model;
   float* obs;

   int error = read( dataset, input, model, obs, ninput, nmodel, nlin, Opts.Bool.Get("-pred") );
   if ( error ) {return error;}

   if( Opts.String.Found("-run") )
   {
      pep_init( input, model, obs, nlin, argc, argv );
      pep_interpret();
      pep_print( stdout );
      pep_destroy();
   }
   else
   {
      ServerSocket svs(SocketAddress( "0.0.0.0", port ) );
      TCPServerParams* pParams = new TCPServerParams;
      //pParams->setMaxThreads(10);
      //pParams->setMaxQueued(10);
      //pParams->setThreadIdleTime(100000000000);
      TCPServer srv(new TCPServerConnectionFactoryImpl<Server>(), svs, pParams);
      srv.start();

      pee_init( input, model, obs, nlin, argc, argv );
      pee_evolve();
      pee_print_best( stdout, 1 );
      pee_print_time();
      pee_destroy();
   }
  
   destroy(input, model, obs, nlin);

   return 0;
}
