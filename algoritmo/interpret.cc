#include <stdio.h> 
#include <stdlib.h>
#include <cmath>    
#include "interpret.h"

/** ****************************************************************** **/
/** ***************************** TYPES ****************************** **/
/** ****************************************************************** **/

static struct t_data { double** input; double** model; double* obs; unsigned max_size_phenotype; int nlin; } data;


/** ****************************************************************** **/
/** ************************* MAIN FUNCTION ************************** **/
/** ****************************************************************** **/

void interpret_init( const unsigned max_size_phenotype, double** input, double** model, double* obs, int nlin, int ninput, int nmodel ) 
{
   data.max_size_phenotype = max_size_phenotype;
   data.nlin = nlin;

   data.input = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     data.input[i] = new double[ninput];
   data.model = new double*[nlin];
   for( int i = 0; i < nlin; i++ )
     data.model[i] = new double[nmodel];
   data.obs = new double[nlin];

   for( int i = 0; i < nlin; i++ )
   {
     for( int j = 0; j < ninput; j++ )
     {
       data.input[i][j] = input[i][j];
     }
     for( int j = 0; j < nmodel; j++ )
     {
       data.model[i][j] = model[i][j];
     }
     data.obs[i] = obs[i];
   }
}

double interpret( Symbol* phenotype, double* ephemeral, int size )
{
   double pilha[data.max_size_phenotype];
   double erro = 0.0; double soma = 0.0;
   int topo;
   for( int ponto = 0; ponto < data.nlin; ++ponto )
   {
      topo = -1;
      for( int i = size - 1; i >= 0; --i )
      {
         switch( phenotype[i] )
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
            case T_BMA:
               pilha[++topo] = data.input[ponto][0];
               break;
            case T_CHUVA_ONTEM:
               pilha[++topo] = data.input[ponto][1];
               break;
            case T_CHUVA_ANTEONTEM:
               pilha[++topo] = data.input[ponto][2];
               break;
            case T_MEAN_MODELO:
               pilha[++topo] = data.input[ponto][3];
               break;
            case T_MAX_MODELO:
               pilha[++topo] = data.input[ponto][4];
               break;
            case T_MIN_MODELO:
               pilha[++topo] = data.input[ponto][5];
               break;
            case T_STD_MODELO:
               pilha[++topo] = data.input[ponto][6];
               break;
            case T_CHOVE:
               pilha[++topo] = data.input[ponto][7];
               break;
            case T_CHUVA_LAG1P:
               pilha[++topo] = data.input[ponto][8];
               break;
            case T_CHUVA_LAG1N:
               pilha[++topo] = data.input[ponto][9];
               break;
            case T_CHUVA_LAG2P:
               pilha[++topo] = data.input[ponto][10];
               break;
            case T_CHUVA_LAG2N:
               pilha[++topo] = data.input[ponto][11];
               break;
            case T_CHUVA_LAG3P:
               pilha[++topo] = data.input[ponto][12];
               break;
            case T_CHUVA_LAG3N:
               pilha[++topo] = data.input[ponto][13];
               break;
            case T_PADRAO_MUDA:
               pilha[++topo] = data.input[ponto][14];
               break;
            case T_PERTINENCIA:
               pilha[++topo] = data.input[ponto][15];
               break;
            case T_CHUVA_PADRAO:
               pilha[++topo] = data.input[ponto][16];
               break;
            case T_CHUVA_HISTORICA:
               pilha[++topo] = data.input[ponto][17];
               break;
            case T_K:
               pilha[++topo] = data.input[ponto][18];
               break;
            case T_TT:
               pilha[++topo] = data.input[ponto][19];
               break;
            case T_SWEAT:
               pilha[++topo] = data.input[ponto][20];
               break;
            case T_PAD:
               pilha[++topo] = data.input[ponto][21];
               break;
            case T_MOD1:
               pilha[++topo] = data.model[ponto][0];
               break;
            case T_MOD2:
               pilha[++topo] = data.model[ponto][1];
               break;
            case T_MOD3:
               pilha[++topo] = data.model[ponto][2];
               break;
            case T_MOD4:
               pilha[++topo] = data.model[ponto][3];
               break;
            case T_MOD5:
               pilha[++topo] = data.model[ponto][4];
               break;
            case T_MOD6:
               pilha[++topo] = data.model[ponto][5];
               break;
            case T_MOD7:
               pilha[++topo] = data.model[ponto][6];
               break;
            case T_MOD8:
               pilha[++topo] = data.model[ponto][7];
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
               pilha[++topo] = ephemeral[i];
               break;
         }
      }
      erro += fabs(pilha[topo] - data.obs[ponto]); 
   }

   return erro;
}
