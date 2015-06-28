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
//      if( !size[ind] && !mode )
//      {
//         vector[ind] = std::numeric_limits<float>::max();
//         break;
//      }

      sum = 0.0;
      for( int ponto = 0; ponto < data.nlin; ++ponto )
      {
         topo = -1;
         for( int i = size[ind] - 1; i >= 0; --i )
         {
            switch( phenotype[ind * data.size + i] )
            {
               case T_IF_THEN_ELSE:
                  if( pilha[topo] == 1.0 ) { --topo; }
                  else { topo = topo - 2; }
                  break;
               case T_AND:
                  if( pilha[topo] == 1.0 && pilha[topo - 1] == 1.0 ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_OR:
                  if( pilha[topo] == 1.0 || pilha[topo - 1] == 1.0 ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_NOT:
                  pilha[topo] == !pilha[topo];
                  break;
               case T_MAIOR:
                  if( pilha[topo] > pilha[topo - 1] ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_MENOR:
                  if( pilha[topo] < pilha[topo - 1] ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_IGUAL:
                  if( pilha[topo] == pilha[topo - 1] ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_ADD:
                  pilha[topo - 1] = pilha[topo] + pilha[topo - 1]; --topo;
                  break;
               case T_SUB:
                  pilha[topo - 1] = pilha[topo] - pilha[topo - 1]; --topo;
                  break;
               case T_MULT:
                  pilha[topo - 1] = pilha[topo] * pilha[topo - 1]; --topo;
                  break;
               case T_DIV:
                  pilha[topo - 1] = pilha[topo] / pilha[topo - 1]; --topo;
                  break;
               case T_MEAN:
                  pilha[topo - 1] = (pilha[topo] + pilha[topo - 1]) / 2; --topo;
                  break;
               case T_MAX:
                  pilha[topo - 1] = fmax(pilha[topo], pilha[topo - 1]); --topo;
                  break;
               case T_MIN:
                  pilha[topo - 1] = fmin(pilha[topo], pilha[topo - 1]); --topo;
                  break;
               case T_ABS:
                  pilha[topo] = fabs(pilha[topo]);
                  break;
               case T_SQRT:
                  pilha[topo] = sqrt(pilha[topo]);
                  break;
               case T_POW2:
                  pilha[topo] = pow(pilha[topo], 2);
                  break;
               case T_POW3:
                  pilha[topo] = pow(pilha[topo], 3);
                  break;
               case T_NEG:
                  pilha[topo] = -pilha[topo];
                  break;
               case T_ATTRIBUTE:
                  pilha[++topo] = data.inputs[ponto][(int)ephemeral[ind * data.size + i]];
                  break;
               case T_1:
                  pilha[++topo] = 1;
                  break;
               case T_2:
                  pilha[++topo] = 2;
                  break;
               case T_3:
                  pilha[++topo] = 3;
                  break;
               case T_4:
                  pilha[++topo] = 4;
                  break;
               case T_EFEMERO:
                  pilha[++topo] = ephemeral[ind * data.size + i];
                  break;
            }
         }
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
