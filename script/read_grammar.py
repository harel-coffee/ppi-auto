#!/usr/bin/env python

#Running: 
#          python script/read_grammar.py -i problem/random/grammar_bnf
#

# script/read_grammar.py -o output_dir -g grammar.bnf -i interpreter_core -p interpreter_core_print

import sys, argparse

import numpy as np


def main(argv):
   parser = argparse.ArgumentParser()
   #parser.add_argument("-o", "--output-dir", required=True, help="Output directory")
   parser.add_argument("-i", "--grammar", required=True, help="BNF grammar's path name")
   #parser.add_argument("-i", "--interpreter", required=True, help="Interpreter core's path name")
   #parser.add_argument("-p", "--interpreter-print", required=True, help="Interpreter print definitions' path name")

   args = parser.parse_args()

   #print args.output_dir, args.grammar, args.interpreter, args.interpreter_print

   return args.grammar


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
   inputfile = main(sys.argv[1:])


try:
   f = open(inputfile,"r")
except IOError:
   print "Could not open file " + inputfile + "!"
lines = f.readlines()
f.close()
 
grammar = []
for i in range(0,len(lines)):
   grammar += list(lines[i].split())

text0 = []; text1 = []; text2 = []; text3 = []; text4 = []; text5 = []; text6 = [];

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
      text4.append("NT_INICIAL, ")

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
            text4.append("NT_" + nterminal + ", ")

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
                           text5.append("T_" + value + " = TERMINAL_MIN")
                        else:
                           text5.append(", T_" + value)
                  else:
                     text0.append("t_rule " + nterminal + "_" + str(n) + " = { " + str(rule) + ", {T_" + grammar[ini].upper())
                     if "ATTR" in  grammar[ini].upper():
                        if not text6: 
                           text6.append(", T_ATTRIBUTE, T_" + grammar[ini].upper() + " = ATTRIBUTE_MIN")
                        else:
                           text6.append(", T_" + grammar[ini].upper())
                     else:
                        if not text5: 
                           text5.append("T_" + grammar[ini].upper() + " = TERMINAL_MIN")
                        else:
                           text5.append(", T_" + grammar[ini].upper())

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
                           text5.append("T_" + value + " = TERMINAL_MIN")
                        else:
                           text5.append(", T_" + value)
                     else:
                        text0.append(", T_" + grammar[j].upper())
                        if "ATTR" in  grammar[j].upper():
                           if not text6: 
                              text6.append(", T_ATTRIBUTE, T_" + grammar[j].upper() + " = ATTRIBUTE_MIN")
                           else:
                              text6.append(", T_" + grammar[j].upper())
                        else:
                           if not text5: 
                              text5.append("T_" + grammar[j].upper() + " = TERMINAL_MIN")
                           else:
                              text5.append(", T_" + grammar[j].upper())


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

f = open("src/grammar", 'w')
f.write("#define MAX_QUANT_SIMBOLOS_POR_REGRA " + str(maxrule) + "\n\nstruct t_rule { unsigned quantity; Symbol symbols[MAX_QUANT_SIMBOLOS_POR_REGRA]; };\n\n" + text0 + text1 + text2 + text3)
f.close()

f = open("src/symbol", 'w')
f.write("#ifndef __SYMBOL_H\n#define __SYMBOL_H\n\n#define TERMINAL_MIN 10000\n#define ATTRIBUTE_MIN 20000\n\ntypedef enum { " + text4 + text5 + text6 + " } Symbol;\n#endif")
#f.write("#ifndef __SYMBOL_H\n#define __SYMBOL_H\n\n#define TERMINAL_MIN 10000\n#define ATTRIBUTE_MIN 20000\n\ntypedef enum { " + text4 + "T_IF_THEN_ELSE = TERMINAL_MIN, T_AND, T_OR, T_XOR, T_NOT, T_GREATER, T_GREATEREQUAL, T_LESS, T_LESSEQUAL, T_EQUAL, T_NOTEQUAL, T_ADD, T_SUB, T_MULT, T_DIV, T_MEAN, T_MAX, T_MIN, T_MOD, T_POW, T_ABS, T_SQRT, T_POW2, T_POW3, T_POW4, T_POW5, T_NEG, T_ROUND, T_CONST, T_0, T_1, T_2, T_3, T_4, T_5, T_6, T_7, T_8, T_9, T_ATTRIBUTE, T_ATTR1 = ATTRIBUTE_MIN, T_ATTR2, T_ATTR3, T_ATTR4, T_ATTR5, T_ATTR6, T_ATTR7, T_ATTR8, T_ATTR9, T_ATTR10, T_ATTR11, T_ATTR12, T_ATTR13, T_ATTR14, T_ATTR15, T_ATTR16, T_ATTR17, T_ATTR18, T_ATTR19, T_ATTR20, T_ATTR21, T_ATTR22, T_ATTR23, T_ATTR24, T_ATTR25, T_ATTR26, T_ATTR27, T_ATTR28, T_ATTR29, T_ATTR30, T_ATTR31, T_ATTR32, T_ATTR33, T_ATTR34, T_ATTR35, T_ATTR36, T_ATTR37, T_ATTR38, T_ATTR39, T_ATTR40, T_ATTR41, T_ATTR42, T_ATTR43, T_ATTR44, T_ATTR45, T_ATTR46, T_ATTR47, T_ATTR48, T_ATTR49 } Symbol;\n#endif")
f.close()

name = "problem/interpreter_core"
try:
   f = open(name,"r")
except IOError:
   print "Could not open file " + name + "!"
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

f = open("src/interpreter/interpreter_core", 'w')
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
      print "Missing terminal:", missing[i], "\nPlease check the interpreter_core file"
     
name = "problem/pee_individual_print_function"
try:
   f = open(name,"r")
except IOError:
   print "Could not open file " + name + "!"
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

f = open("src/pee_individual_print_function", 'w')
f.write("void pee_individual_print( const Population* individual, int idx, FILE* out, int print_mode )\n{\nSymbol phenotype[data.max_size_phenotype];\nfloat ephemeral[data.max_size_phenotype];\n\nint allele = 0;\nint size = decode( individual->genome + (idx * data.number_of_bits), &allele, phenotype, ephemeral, 0, data.initial_symbol );\nif( !size ) { return; }\n\nif( print_mode )\n{\nfor( int i = 0; i < size; ++i )\nif( phenotype[i] == T_CONST || phenotype[i] == T_ATTRIBUTE )\nfprintf( out, \"%d %.12f \", phenotype[i], ephemeral[i] );\nelse\nfprintf( out, \"%d \", phenotype[i] );\nfprintf( out, \"\\n{%d} \", size );\n}\nelse\nfprintf( out, \"{%d} \", size );\n\nfor( int i = 0; i < size; ++i )\nswitch( phenotype[i] )\n{\n" + ''.join(print_function) + "case T_ATTRIBUTE:\nfprintf( out, \"ATTR-%d \", (int)ephemeral[i] );\nbreak;\n}\nfprintf( out, \":: %.12f\\n\", individual->fitness[idx] - ALPHA*size ); // Print the raw error, that is, without the penalization for complexity\n}")
f.close()

#lst -> contem os terminais fornecidos pela gramatica bnf
#terminais -> contem os terminais tratados no algoritmo (interpreter_core)
lst = [i for i in lst if not "=" in i]
lst = [i for i in lst if not "TERMINAL_MIN" in i]
#Imprime os terminais contidos em lst, mas que nao estao em terminais
missing = [i for i in lst if not i in terminais]
for i in range(0,len(missing)):
   print "Missing terminal:", missing[i], "\nPlease check the pee_individual_print file"
 
