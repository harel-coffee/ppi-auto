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
   float stack[data.size]; 
   float sum; 
   int stack_top;

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
         stack_top = -1;
         for( int i = size[ind] - 1; i >= 0; --i )
         {
            switch( phenotype[ind * data.size + i] )
            {
               #include "core"
               case T_ATTRIBUTE:
                  stack[++stack_top] = data.inputs[ponto][(int)ephemeral[ind * data.size + i]];
                  break;
               case T_CONST:
                  stack[++stack_top] = ephemeral[ind * data.size + i];
                  break;
               default:
                  break;
            }
         }
         if( prediction_mode ) {vector[ponto] = stack[stack_top];}
         else {sum += fabs(stack[stack_top] - data.obs[ponto]);}
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
