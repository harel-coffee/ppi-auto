#include "symbol"

//TODO cpu: pilha[++topo] = inputs[(gr_id * local_size + lo_id) * ncol + 2];

__kernel void
evaluate_gpu( __global const Symbol* phenotype, __global const float* ephemeral, __global const int* size, __global const float* inputs, __global float* vector, __local float* EP, int nlin, int ncol, int prediction_mode, int nInd )
{
   float pilha[TAM_MAX];
   int lo_id = get_local_id(0);
   int gl_id = get_global_id(0);
   int gr_id = get_group_id(0);
   int local_size = get_local_size(0);

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
         Symbol phenotypeLocal;
         float ephemeralLocal;
         for( int i = size[ind] - 1; i >= 0; --i )
         {
            phenotypeLocal = phenotype[ind * TAM_MAX + i];
            // TODO: Não precisa ephemeralLocal, ou seja, não precisa ler da memória global toda hora (colocar no case)
            ephemeralLocal = ephemeral[ind * TAM_MAX + i];
            switch( phenotypeLocal )
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
// TODO: Criar um "T_ATTR" e usar o ephemeralLocal para guardar o índice do atributo
//               case T_ATTR
//                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) * ncol + (int) ephemeralLocal];
//                  break;
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
               case T_BMA:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id)];
                  break;
               case T_CHUVA_ONTEM:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 1];
                  break;
               case T_CHUVA_ANTEONTEM:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 2];
                  break;
               case T_MEAN_MODELO:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 3];
                  break;
               case T_MAX_MODELO:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 4];
                  break;
               case T_MIN_MODELO:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 5];
                  break;
               case T_STD_MODELO:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 6];
                  break;
               case T_CHOVE:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 7];
                  break;
               case T_CHUVA_LAG1P:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 8];
                  break;
               case T_CHUVA_LAG1N:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 9];
                  break;
               case T_CHUVA_LAG2P:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 10];
                  break;
               case T_CHUVA_LAG2N:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 11];
                  break;
               case T_CHUVA_LAG3P:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 12];
                  break;
               case T_CHUVA_LAG3N:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 13];
                  break;
               case T_PADRAO_MUDA:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 14];
                  break;
               case T_PERTINENCIA:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 15];
                  break;
               case T_CHUVA_PADRAO:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 16];
                  break;
               case T_CHUVA_HISTORICA:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 17];
                  break;
               case T_K:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 18];
                  break;
               case T_TT:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 19];
                  break;
               case T_SWEAT:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 20];
                  break;
               case T_PAD:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 21];
                  break;
               case T_MOD1:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 22];
                  break;
               case T_MOD2:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 23];
                  break;
               case T_MOD3:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 24];
                  break;
               case T_MOD4:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 25];
                  break;
               case T_MOD5:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 26];
                  break;
               case T_MOD6:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 27];
                  break;
               case T_MOD7:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 28];
                  break;
               case T_MOD8:
                  pilha[++topo] = inputs[(gr_id * local_size + lo_id) + nlin * 29];
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
                  pilha[++topo] = ephemeralLocal;
                  break;
               default:
                  break;
            }
         }
         if( !prediction_mode )
         {
            EP[lo_id] = fabs( pilha[topo] - inputs[(gr_id * local_size + lo_id) + nlin * (ncol - 1)] );
         }
         else
         {
            vector[gl_id] = pilha[topo];
         }
      }
      if( !prediction_mode )
      {
// TODO: Colocar só uma vez lá em cima (já pode fazer ele dividido por dois também)
         int next_power_of_2 = pown(2.0f, (int) ceil(log2((float)local_size)));
         for( int s = next_power_of_2/2; s > 0; s/=2 )
         {
            barrier(CLK_LOCAL_MEM_FENCE);
            if( (lo_id < s) && (lo_id + s < local_size) ) { EP[lo_id] += EP[lo_id + s]; }
         }
         if( lo_id == 0) { vector[ind * get_num_groups(0) + gr_id] = EP[0]; }
      }
   } 
}
