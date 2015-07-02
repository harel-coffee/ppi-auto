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
               case T_GREATER:
                  if( pilha[topo] > pilha[topo - 1] ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_LESS:
                  if( pilha[topo] < pilha[topo - 1] ) { pilha[topo - 1] = 1.0; --topo; }
                  else { pilha[topo - 1] = 0.0; --topo; }
                  break;
               case T_EQUAL:
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
               case T_1P:
                  pilha[++topo] = 1;
                  break;
               case T_2P:
                  pilha[++topo] = 2;
                  break;
               case T_3P:
                  pilha[++topo] = 3;
                  break;
               case T_4P:
                  pilha[++topo] = 4;
                  break;
               case T_CONST:
                  pilha[++topo] = ephemeral[ind * data.size + i];
                  break;
            }
         }
