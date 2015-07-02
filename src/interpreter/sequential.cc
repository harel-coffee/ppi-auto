#include <stdio.h> 
#include <stdlib.h>
#include <cmath>    
#include <limits>
#include "sequential.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { float** inputs; float* obs; unsigned size; int nlin; } data;


/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void seq_interpret_init( const unsigned size, float** input, float** model, float* obs, int nlin, int ninput, int nmodel ) 
{
   data.size = size;
   data.nlin = nlin;

   data.inputs = new float*[nlin];
   for( int i = 0; i < nlin; i++ )
     data.inputs[i] = new float[ninput + nmodel];
   data.obs = new float[nlin];

   for( int i = 0; i < nlin; i++ )
   {
     for( int j = 0; j < ninput; j++ )
     {
       data.inputs[i][j] = input[i][j];
     }
     for( int j = 0; j < nmodel; j++ )
     {
       data.inputs[i][j + ninput] = model[i][j];
     }
     data.obs[i] = obs[i];
   }

//   for( int i = 0; i < nlin; i++ )
//   {
//      if( i == 289 )
//      {
//         for( int j = 0; j < ninput; j++ )
//         {
//            fprintf(stdout,"%f ",data.inputs[i][j]);
//         }
//         for( int j = 0; j < nmodel; j++ )
//         {
//            fprintf(stdout,"%f ",data.inputs[i][j + ninput]);
//         }
//         fprintf(stdout,"%f\n",data.obs[i]);
//      }
//   }
}

void seq_interpret( Symbol* phenotype, float* ephemeral, int* size, float* vector, int nInd, int prediction_mode )
{
   float pilha[data.size]; 
   float sum; 
   int topo;

   for( int ind = 0; ind < nInd; ++ind )
   {
      if( size[ind] == 0 && !prediction_mode )
      {
         vector[ind] = std::numeric_limits<float>::max();
         continue;
      }

      sum = 0.0;
      for( int ponto = 0; ponto < data.nlin; ++ponto )
      {
         #include "core.h"
         if( prediction_mode ) {vector[ponto] = pilha[topo];}
         else {sum += fabs(pilha[topo] - data.obs[ponto]);}
      }
      if ( !prediction_mode )
      {
         if( isnan( sum ) || isinf( sum ) ) {vector[ind] = std::numeric_limits<float>::max();}
         else {vector[ind] = sum/data.nlin;}
      }
   }
}

void seq_interpret_destroy() 
{
   delete[] data.obs;

   for( int i = 0; i < data.nlin; ++i )
     delete [] data.inputs[i];
   delete [] data.inputs;
}
