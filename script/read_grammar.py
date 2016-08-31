#!/usr/bin/env python

import sys, argparse, os

import numpy as np

def main(argv):
   parser = argparse.ArgumentParser()
   parser.add_argument("-o", "--output-dir", required=True, help="Output directory")
   parser.add_argument("-g", "--grammar", required=True, help="BNF grammar's path name")
   parser.add_argument("-i", "--interpreter", required=True, help="Interpreter core's path name")
   parser.add_argument("-p", "--interpreter-print", required=True, help="Interpreter print definitions' path name")

   args = parser.parse_args()

   return args


def interpreter(terminal):
      if terminal == ">":
         return "GREATER"
      elif terminal == ">=":
         return "GREATEREQUAL"
      elif terminal == "<":
         return "LESS"
      elif terminal == "<=":
         return "LESSEQUAL"
      elif terminal == "=":
         return "EQUAL"
      elif terminal == "!=":
         return "NOTEQUAL"
      elif terminal == "+":
         return "ADD"
      elif terminal == "-":
         return "SUB"
      elif terminal == "*":
         return "MULT"
      elif terminal == "/":
         return "DIV"
      else:
         return None


if __name__ == "__main__":
   args = main(sys.argv)


try:
   f = open(args.grammar,"r")
except IOError:
   print "Could not open file '" + args.grammar + "'"
lines = f.readlines()
f.close()

grammar = []
for i in range(0,len(lines)):
   if not lines[i].lstrip().startswith(('#')): # Skip comment lines (they start with '#')
      grammar += list(lines[i].split())

text0 = []; text1 = []; text2 = []; text3 = []; text4 = []; text5 = []; text6 = []; #text7 = []

i = 0; maxrule = 0;
while i < len(grammar):

   if grammar[i] == "S":
      for j in range(i+1,len(grammar)):
         if list(grammar[j])[0] == "<" and list(grammar[j])[-1] == ">":
            rule = 1; ini = j; j = j + 1
            while grammar[j] != "P":
              rule = rule + 1; j = j + 1
            fim = j; i = j - 1
            break

      text1.append("\nt_rule* regras_INICIAL[] = { &INICIAL_0")
      text2.append("\nt_rule** gramatica[] = { regras_INICIAL")
      text3.append("\nunsigned tamanhos[] = { sizeof(regras_INICIAL) / sizeof(t_rule*),")
      text4.append("NT_INICIAL"); text4.append(", ")

      if( rule > maxrule ): maxrule = rule;
      text0.append("t_rule INICIAL_0 = { " + str(rule) + ", {NT_" + grammar[ini][1 : 1 + (len(grammar[ini])-2)].upper())

      for j in range(ini+1,fim):
         text0.append(", NT_" + grammar[j][1 : 1 + (len(grammar[j])-2)].upper())
      text0.append("} };\n")


   if grammar[i] == "P":
      while i < len(grammar):

         if grammar[i] == "::=":

            if text1: 
               text1.append(" };")

            nterminal = grammar[i-1][1 : 1 + (len(grammar[i-1])-2)].upper()

            text0.append("\n")
            text1.append("\nt_rule* regras_" + nterminal + "[] = { ")
            text2.append(", regras_" + nterminal)
            text3.append(" sizeof(regras_" + nterminal + ")/sizeof(t_rule*),")
            if "NT_"+nterminal not in text4:
               text4.append("NT_" + nterminal); text4.append(", ")

            n = 0; i = i + 1; flag = 1
            while flag and i < len(grammar):

               rule = 0; ini = i
               while i < len(grammar) and grammar[i] != "|" and grammar[i] != "::=":
                  rule = rule + 1; i = i + 1
               fim = i; 
               if i >= len(grammar): i = len(grammar) - 1

               if grammar[i] == "::=":
                  rule = rule - 1; fim = fim - 1; i = i - 1
                  flag = 0
               else:
                  i = i + 1

               if n > 0:
                  text1.append(", &" + nterminal + "_" + str(n))
               else:
                  text1.append("&" + nterminal + "_" + str(n))

               if( rule > maxrule ): maxrule = rule;
               if list(grammar[ini])[0] == "<" and list(grammar[ini])[-1] == ">":
                  text0.append("t_rule " + nterminal + "_" + str(n) + " = { " + str(rule) + ", {NT_" + grammar[ini][1 : 1 + (len(grammar[ini])-2)].upper())
               else:
                  if grammar[ini] in ['+', '-', '*', '/', '<', '<=', '>', '>=', '=', '!=']:
                     value = interpreter(grammar[ini])
                     if value is None:
                        print "Problem with interpreter."
                        exit()
                     else:
                        text0.append("t_rule " + nterminal + "_" + str(n) + " = { " + str(rule) + ", {T_" + value)
                        if not text5:
                           text5.append("T_" + value); text5.append(" = TERMINAL_MIN")
                        else:
                           if "T_"+value not in text5:
                              text5.append(", "); text5.append("T_" + value)
                  else:
                     text0.append("t_rule " + nterminal + "_" + str(n) + " = { " + str(rule) + ", {T_" + grammar[ini].upper())
                     if "ATTR" in  grammar[ini].upper():
                        if not text6:
                           text6.append(", "); text6.append("T_ATTRIBUTE")
                           text6.append(", "); text6.append("T_" + grammar[ini].upper()); text6.append(" = ATTRIBUTE_MIN + " + grammar[ini][4:])
                        else:
                           if "T_"+grammar[ini].upper() not in text6:
                              text6.append(", "); text6.append("T_" + grammar[ini].upper()); text6.append(" = ATTRIBUTE_MIN + " + grammar[ini][4:])
                        #if grammar[ini][4:].upper() not in text7:
                           #text7.append(grammar[ini][4:].upper())
                     else:
                        if not text5:
                           text5.append("T_" + grammar[ini].upper()); text5.append(" = TERMINAL_MIN")
                        else:
                           if "T_"+grammar[ini].upper() not in text5:
                              text5.append(", "); text5.append("T_" + grammar[ini].upper())

               for j in range(ini+1,fim):
                  if list(grammar[j])[0] == "<" and list(grammar[j])[-1] == ">":
                     text0.append(", NT_" + grammar[j][1 : 1 + (len(grammar[j])-2)].upper())
                  else:
                     if grammar[j] in ['+', '-', '*', '/', '<', '<=', '>', '>=', '=', '!=']:
                        value = interpreter(grammar[j])
                        if value is None:
                           print "Problem with interpreter."
                           exit()
                        else:
                           text0.append(", T_" + value)
                        if not text5:
                           text5.append("T_" + value); text5.append(" = TERMINAL_MIN")
                        else:
                           if "T_"+value not in text5:
                              text5.append(", "); text5.append("T_" + value)
                     else:
                        text0.append(", T_" + grammar[j].upper())
                        if "ATTR" in  grammar[j].upper():
                           if not text6:
                              text6.append(", "); text6.append("T_ATTRIBUTE"); text6.append(", ");
                              text6.append("T_" + grammar[j].upper()); text6.append(" = ATTRIBUTE_MIN + " + grammar[j][4:]);
                           else:
                              if "T_"+grammar[j].upper() not in text6:
                                 text6.append(", "); text6.append("T_" + grammar[j].upper()); text6.append(" = ATTRIBUTE_MIN + " + grammar[j][4:]);
                           #if grammar[j][4:].upper() not in text7:
                              #text7.append(grammar[j][4:].upper())
                        else:
                           if not text5:
                              text5.append("T_" + grammar[j].upper()); text5.append(" = TERMINAL_MIN")
                           else:
                              if "T_"+grammar[j].upper() not in text5:
                                 text5.append(", "); text5.append("T_" + grammar[j].upper())


               text0.append("} };\n")

               n = n + 1
    
         i = i + 1

   i = i + 1


text1.append(" };\n")
text2.append(" };\n")
text3.append(" };\n")

text0 = ''.join(text0)
text1 = ''.join(text1)
text2 = ''.join(text2)
text3 = ''.join(text3)
text4 = ''.join(text4)
text5 = ''.join(text5)
text6 = ''.join(text6)

#text7 = map(int,text7)
#text7.sort()
#text7 = map(str,text7)
#text7 = [', T_ATTR' + s for s in text7]
#text7[0] = text7[0] + ' = ATTRIBUTE_MIN'
#text7 = [', T_ATTRIBUTE'] + text7
#text7 = ''.join(text7)


f = open(os.path.join(args.output_dir, "grammar"), 'w')
f.write("#define MAX_QUANT_SIMBOLOS_POR_REGRA " + str(maxrule) + "\n\nstruct t_rule { unsigned quantity; Symbol symbols[MAX_QUANT_SIMBOLOS_POR_REGRA]; };\n\n" + text0 + text1 + text2 + text3)
f.close()

symbol_head = r"""#ifndef __SYMBOL_H
#define __SYMBOL_H

#define UNPROTECTED_FUNCTIONS 1

#ifndef M_PI_F
   #define M_PI_F 3.14159274101257f
#endif
#ifndef M_PI_2_F
   #define M_PI_2_F 1.57079637050629f
#endif
#ifndef M_PI_4_F
   #define M_PI_4_F 0.78539818525314f
#endif
#ifndef M_1_PI_F
   #define M_1_PI_F 0.31830987334251f
#endif
#ifndef M_2_PI_F
   #define M_2_PI_F 0.63661974668503f
#endif
#ifndef M_2_SQRTPI_F
   #define M_2_SQRTPI_F 1.12837922573090f
#endif
#ifndef M_SQRT2_F
   #define M_SQRT2_F 1.41421353816986f
#endif
#ifndef M_SQRT1_2_F
   #define M_SQRT1_2_F 0.70710676908493f
#endif
#ifndef M_E_F
   #define M_E_F 2.71828174591064f
#endif
#ifndef M_LOG2E_F
   #define M_LOG2E_F 1.44269502162933f
#endif
#ifndef M_LOG10E_F
   #define M_LOG10E_F 0.43429449200630f
#endif
#ifndef M_LN2_F
   #define M_LN2_F 0.69314718246460f
#endif
#ifndef M_LN10_F
   #define M_LN10_F 2.30258512496948f
#endif

#define TERMINAL_MIN 10000
#define ATTRIBUTE_MIN 20000

typedef enum {"""

symbol_tail = r"""} Symbol;
"""


if "T_CONST" not in text5:
   symbol_tail = symbol_tail + r"""
#define NOT_USING_T_CONST 1
"""

symbol_tail = symbol_tail + r"""
#endif"""

f = open(os.path.join(args.output_dir, "symbol"), 'w')
#f.write(symbol_head + text4 + text5 + text7 + symbol_tail)
f.write(symbol_head + text4 + text5 + text6 + symbol_tail)
f.close()

try:
   f = open(args.interpreter,"r")
except IOError:
   print "Could not open file '" + args.interpreter + "'"
lines = f.readlines()
f.close()

lst = list(text5.split())
lst = [s.replace(',', '') for s in lst]

core = []; terminais = [];
index = [i for i, j in enumerate(lines) if 'case' in j]
for i in range(0,len(index)):
   terminais.append(lines[index[i]].split('case ')[1].split(':')[0])
   if lines[index[i]].split('case ')[1].split(':')[0] in lst:
      if i+1 > len(index)-1:
         core += lines[index[i]:len(lines)]
      else:
         core += lines[index[i]:index[i+1]]

f = open(os.path.join(args.output_dir, "interpreter_core"), 'w')
f.write(''.join(core))
f.close()

#lst -> contem os terminais fornecidos pela gramatica bnf
#terminais -> contem os terminais tratados no algoritmo (interpreter_core)
lst = [i for i in lst if not "=" in i]
lst = [i for i in lst if not "TERMINAL_MIN" in i]
#Imprime os terminais contidos em lst, mas que nao estao em terminais
missing = [i for i in lst if not i in terminais]
for i in range(0,len(missing)):
   if missing[i] != "T_CONST":
      print "Missing terminal:", missing[i], "\nPlease check the '" + args.interpreter + "' file"

try:
   f = open(args.interpreter_print,"r")
except IOError:
   print "Could not open file '" + args.interpreter_print + "'"
lines = f.readlines()
f.close()

lst = list(text5.split())
lst = [s.replace(',', '') for s in lst]

print_function = []; terminais = [];
index = [i for i, j in enumerate(lines) if 'case' in j]
for i in range(0,len(index)):
   terminais.append(lines[index[i]].split('case ')[1].split(':')[0])
   if lines[index[i]].split('case ')[1].split(':')[0] in lst:
      if i+1 > len(index)-1:
         print_function += lines[index[i]:len(lines)]
      else:
         print_function += lines[index[i]:index[i+1]]

icp_header = r"""void pee_individual_print( const Population* individual, int idx, FILE* out, int generation, int argc, char** argv, int print_mode )
{
   Symbol phenotype[data.max_size_phenotype];
   float ephemeral[data.max_size_phenotype];

   int allele = 0;
   int size = decode( individual->genome + (idx * data.number_of_bits), &allele, phenotype, ephemeral, 0, data.initial_symbol );
   if( !size ) { return; }

   if (print_mode)
      fprintf( out, "\n> %d;%d;%.12f;", generation, size, individual->fitness[idx] - ALPHA*size ); // Print the raw error, that is, without the penalization for complexity
   else
      fprintf( out, "\n[%d] %3d :: %.12f :: ", generation, size, individual->fitness[idx] - ALPHA*size ); // Print the raw error, that is, without the penalization for complexity

   for( int i = 0; i < size; ++i )
      switch( phenotype[i] )
      {
"""

icp_tail = r"""
         case T_ATTRIBUTE:
            fprintf( out, "ATTR-%d ", (int)ephemeral[i] );
            break;
      }

   if( print_mode )
   {
      fprintf(out, ";");
      for( int i = 0; i < size; ++i )
         if( phenotype[i] == T_CONST || phenotype[i] == T_ATTRIBUTE )
            fprintf( out, "%d %.12f ", phenotype[i], ephemeral[i] );
         else
            fprintf( out, "%d ", phenotype[i] );
      // Print the individual's genome, but only the active (no introns) region (useful for seeding new generations)
      fprintf(out, ";");
      for(int i=0; i<allele; ++i)
         fprintf(out, "%d", (individual->genome + (idx * data.number_of_bits))[i]);
      fprintf(out, ";");
      for (int i=0; i<argc; ++i)
         fprintf(out, "%s ", argv[i]);
   }
   else
      fprintf( out, ":: ");
}
"""

f = open(os.path.join(args.output_dir, "interpreter_core_print"), 'w')
f.write(icp_header + ''.join(print_function) + icp_tail)
f.close()


#lst -> contem os terminais fornecidos pela gramatica bnf
#terminais -> contem os terminais tratados no algoritmo (interpreter_core)
lst = [i for i in lst if not "=" in i]
lst = [i for i in lst if not "TERMINAL_MIN" in i]
#Imprime os terminais contidos em lst, mas que nao estao em terminais
missing = [i for i in lst if not i in terminais]
for i in range(0,len(missing)):
   print "Missing terminal:", missing[i], "\nPlease check the '" + args.interpreter_print + "' file"
 
