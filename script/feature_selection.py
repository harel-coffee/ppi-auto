#!/usr/bin/env python

import numpy as np
from numpy import genfromtxt
import subprocess, os, argparse, csv

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--dataset", required=True, help="Dataset file")
parser.add_argument("-t", "--threshold", required=False, type=float, default=0.01, help="Threshold value")
parser.add_argument("-o", "--output", required=True, help="Output dataset with the selected attributes")
parser.add_argument("-p", "--pee", required=True, help="PEE directory")
parser.add_argument("-pa", "--pee-args", default='', help="Arguments for PEE")
parser.add_argument("-ca", "--cmake-args", default='', help="Arguments for CMAKE")
parser.add_argument("--has-y", action='store_true', default=False, help="Has dependent variable")
parser.add_argument("--has-header", action='store_true', default=False, help="Has header")
args = parser.parse_args()

args.output = os.path.abspath(args.output)
args.pee = os.path.abspath(args.pee)

if args.has_header: 
   skip_header=1
   with open(args.dataset) as input:
      for h in csv.reader(input):
         header = h
         break
else:
   skip_header=0

data = genfromtxt(args.dataset, delimiter=',', skip_header=skip_header)

if args.has_y: 
   nvar = len(data[0])-1
else:
   nvar = len(data[0])

index = [0];
for i in range(1,nvar):
   index.append(i)
   if args.has_header:
      header_str = ','.join([h for j, h in enumerate(header) if j in index])
   else:
      header_str = ''
   np.savetxt(args.output, data[:,index], delimiter=",", header=header_str, comments='', fmt='%.8f')

   text = []; text.append("    <attribute> ::= ");
   for j in range(len(index)-1):
      text.append("attr" + str(j))
      if j < len(index)-2:
         text.append(" | ")
   text = ''.join(text)

   grammar = r"""S = <exp>
   P = <exp> ::= <attribute> | <numeric_const> | <un_op> <exp> | <bin_op> <exp> <exp>
       <bin_op> ::= AND | OR | XOR | GREATER | GREATEREQUAL | LESS | LESSEQUAL | EQUAL | NOTEQUAL | ADD | SUB | MULT | DIV | MEAN | MAX | MIN | MOD | POW
       <un_op> ::= NOT | ABS | SQRT | POW2 | POW3 | POW4 | POW5 | NEG | ROUND | CEIL | FLOOR | EXP | EXP10 | EXP2 | LOG | LOG10 | LOG2 | SIN | COS | TAN | STEP | SIGN | LOGISTIC | GAMMA | GAUSS
       <numeric_const> ::= const | PI | PI_2 | PI_4 | 1_PI | 2_PI | 2_SQRTPI | SQRT2 | SQRT1_2 | E | LOG2E | LOG10E | LN2 | LN10 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | N1 | N2 | N3 | N4 | N5 | N6 | N7 | N8 | N9
"""

   f = open("/tmp/grammar.bnf", 'w')
   f.write(grammar+text)
   f.close()

   os.system('mkdir -p /tmp/build; cd /tmp/build; cmake ' + args.pee + ' ' + args.cmake_args + ' -DGRAMMAR=/tmp/grammar.bnf -DLABEL=select -DCMAKE_BUILD_TYPE=RELEASE -DPROFILING=OFF > /dev/null; make -j -s')
   mae = subprocess.check_output("/tmp/build/select -d " + args.output + " " + args.pee_args + " | grep -a '^> [0-9]' | cut -d';' -f3", shell=True)
   print i, mae

   if float(mae) <= args.threshold:
      index.pop()
   else:
      print index

if args.has_y:
   index.append(len(data[0])-1)

if args.has_header:
   header_str = ','.join([h for j, h in enumerate(header) if j in index])
else:
   header_str = ''

np.savetxt(args.output, data[:,index], delimiter=",", header=header_str, comments='', fmt='%.8f')
