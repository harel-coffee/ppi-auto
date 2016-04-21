#include <stdio.h> 
#include <stdlib.h>
#include <iostream>
#include <cmath>    
#include <string>   
#include <limits>
#include <queue>
#include "sequential.h"


/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { std::string error; unsigned size; float** inputs; int nlin; int ncol; } data;

//#define ERROR(X,Y) data.error
//#define ERROR(X,Y) fabs((X)-(Y))

/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void seq_interpret_init( std::string error, const unsigned size, float** input, int nlin, int ncol ) 
{
   data.error = error;
   data.size = size;
   data.nlin = nlin;
   data.ncol = ncol;

   data.inputs = new float*[nlin];
   for( int i = 0; i < nlin; i++ )
     data.inputs[i] = new float[ncol];

   for( int i = 0; i < nlin; i++ )
   {
     for( int j = 0; j < ncol; j++ )
     {
       data.inputs[i][j] = input[i][j];
     }
   }

//   for( int i = 0; i < nlin; i++ )
//   {
//      if( i == 289 )
//      {
//         for( int j = 0; j < ncol; j++ )
//         {
//            fprintf(stdout,"%f ",data.inputs[i][j]);
//         }
//      }
//   }
}

void seq_interpret( Symbol* phenotype, float* ephemeral, int* size, float* vector, int nInd, int* index, int* best_size, int prediction_mode, float alpha )
{
   //std::cerr << data.error << " " << ERROR(10,5) << std::endl;

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
                  //if ( ponto == 1 ) {printf( "T_ATTRIBUTE: %d %f \n", stack_top, stack[stack_top]);}
                  break;
               case T_CONST:
                  stack[++stack_top] = ephemeral[ind * data.size + i];
                  //if ( ponto == 1 ) {printf( "T_CONST: %d %f \n", stack_top, stack[stack_top]);}
                  break;
               default:
                  break;
            }
         }
         if( prediction_mode ) {vector[ponto] = stack[stack_top];}
         //else {sum += ERROR(stack[stack_top], data.inputs[ponto][data.ncol-1]);}
         else {sum += fabs(stack[stack_top] - data.inputs[ponto][data.ncol-1]);}
      }
      if ( !prediction_mode )
      {
         if( isnan( sum ) || isinf( sum ) ) {vector[ind] = std::numeric_limits<float>::max();}
         else 
         {
            vector[ind] = sum/data.nlin + alpha * size[ind];
         }
      }
   }

   std::priority_queue<std::pair<float, int> > q;
   for( int i = 0; i < nInd; ++i ) 
   {
      q.push( std::pair<float, int>(vector[i]*(-1), i) );
   }
   for( int i = 0; i < *best_size; ++i ) 
   {
      index[i] = q.top().second;
      q.pop();
   }
}

void seq_interpret_destroy() 
{
   for( int i = 0; i < data.nlin; ++i )
     delete [] data.inputs[i];
   delete [] data.inputs;
}
