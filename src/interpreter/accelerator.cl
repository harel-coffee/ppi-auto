#include "symbol"

//TODO cpu: pilha[++topo] = inputs[(gr_id * local_size + lo_id) * ncol + 2];

__kernel void
evaluate_gpu( __global const Symbol* phenotype, __global const float* ephemeral, __global const int* size, __global const float* inputs, __global float* vector, __local float* EP, int nlin, int ncol, int prediction_mode, int nInd )
{
   float pilha[TAM_MAX];
   int lo_id = get_local_id(0);
   int gl_id = get_global_id(0);
   int gr_id = get_group_id(0);
   int lo_size = get_local_size(0);
   int next_power_of_2 = (pown(2.0f, (int) ceil(log2((float)lo_size))))/2;

   for( int ind = 0; ind < nInd; ++ind )
   {
      barrier(CLK_LOCAL_MEM_FENCE);
   
      if( size[ind] == 0 && !prediction_mode )
      {
         if( lo_id == 0 ) {vector[ind * get_num_groups(0) + gr_id] = MAXFLOAT;}
         continue;
      }

      EP[lo_id] = 0.0f;

      if( gl_id < nlin )
      {
         int topo = -1;
         for( int i = size[ind] - 1; i >= 0; --i )
         {
            switch( phenotype[ind * TAM_MAX + i] )
            {
               case T_IF_THEN_ELSE:
                  topo = ( (pilha[topo] == 1.0f) ? topo - 1 : topo - 2 );
                  break;
               case T_AND:
                  pilha[topo - 1] = ( (pilha[topo] == 1.0f && pilha[topo - 1] == 1.0f) ? 1.0f : 0.0f );
                  --topo;
                  break;
               case T_OR:
                  pilha[topo - 1] = ( (pilha[topo] == 1.0f || pilha[topo - 1] == 1.0f) ? 1.0f : 0.0f );
                  --topo;
                  break;
               case T_NOT:
                  pilha[topo] = !pilha[topo];
                  break;
               case T_MAIOR:
                  pilha[topo - 1] = ( (pilha[topo] > pilha[topo - 1]) ? 1.0f : 0.0f );
                  --topo;
                  break;
               case T_MENOR:
                  pilha[topo - 1] = ( (pilha[topo] < pilha[topo - 1]) ? 1.0f : 0.0f );
                  --topo;
                  break;
               case T_IGUAL:
                  pilha[topo - 1] = ( (pilha[topo] == pilha[topo - 1]) ? 1.0f : 0.0f );
                  --topo;
                  break;
               case T_ADD:
                  pilha[topo - 1] = pilha[topo] + pilha[topo - 1];
                  --topo;
                  break;
               case T_SUB:
                  pilha[topo - 1] = pilha[topo] - pilha[topo - 1];
                  --topo;
                  break;
               case T_MULT:
                  pilha[topo - 1] = pilha[topo] * pilha[topo - 1];
                  --topo;
                  break;
               case T_DIV:
                  pilha[topo - 1] = pilha[topo] / pilha[topo - 1];
                  --topo;
                  break;
               case T_MEAN:
                  pilha[topo - 1] = (pilha[topo] + pilha[topo - 1]) / 2.0f;
                  --topo;
                  break;
               case T_MAX:
                  pilha[topo - 1] = fmax(pilha[topo], pilha[topo - 1]);
                  --topo;
                  break;
               case T_MIN:
                  pilha[topo - 1] = fmin(pilha[topo], pilha[topo - 1]);
                  --topo;
                  break;
               case T_ABS:
                  pilha[topo] = fabs(pilha[topo]);
                  break;
               case T_SQRT:
                  pilha[topo] = sqrt(pilha[topo]);
                  break;
               case T_POW2:
                  pilha[topo] = pilha[topo] * pilha[topo];
                  break;
               case T_POW3:
                  pilha[topo] = pilha[topo] * pilha[topo] * pilha[topo];
                  break;
               case T_NEG:
                  pilha[topo] = -pilha[topo];
                  break;
               case T_ATTRIBUTE:
                  pilha[++topo] = inputs[(gr_id * lo_size + lo_id) + nlin * (int)ephemeral[ind * TAM_MAX + i]];
                  break;
               case T_1:
                  pilha[++topo] = 1.0f;
                  break;
               case T_2:
                  pilha[++topo] = 2.0f;
                  break;
               case T_3:
                  pilha[++topo] = 3.0f;
                  break;
               case T_4:
                  pilha[++topo] = 4.0f;
                  break;
               case T_EFEMERO:
                  pilha[++topo] = ephemeral[ind * TAM_MAX + i];
                  break;
               default:
                  break;
            }
         }
         if( !prediction_mode )
         {
            EP[lo_id] = fabs( pilha[topo] - inputs[(gr_id * lo_size + lo_id) + nlin * (ncol - 1)] );
         }
         else
         {
            vector[gl_id] = pilha[topo];
         }
      }
      if( !prediction_mode )
      {
         for( int s = next_power_of_2; s > 0; s/=2 )
         {
            barrier(CLK_LOCAL_MEM_FENCE);
            if( (lo_id < s) && (lo_id + s < lo_size) ) { EP[lo_id] += EP[lo_id + s]; }
         }
         if( lo_id == 0) { vector[ind * get_num_groups(0) + gr_id] = EP[0]; }
      }
   } 
}
