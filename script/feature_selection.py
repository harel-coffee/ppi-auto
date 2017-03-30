#!/usr/bin/env python

import numpy as np
from numpy import genfromtxt
from numpy import linalg
import subprocess, os, argparse, csv, tempfile, shutil

def RunPEE(attr_ids, dataset, tmpdir):
   grammar = r"""   S = <exp>
   P = <exp> ::= <attribute> | <numeric_const> | <un_op> <exp> | <bin_op> <exp> <exp>
       <bin_op> ::= AND | OR | XOR | GREATER | GREATEREQUAL | LESS | LESSEQUAL | EQUAL | NOTEQUAL | ADD | SUB | MULT | DIV | MEAN | MAX | MIN | MOD | POW
       <un_op> ::= NOT | ABS | SQRT | POW2 | POW3 | POW4 | POW5 | NEG | ROUND | CEIL | FLOOR | EXP | EXP10 | EXP2 | LOG | LOG10 | LOG2 | SIN | COS | TAN | STEP | SIGN | LOGISTIC | GAMMA | GAUSS
       <numeric_const> ::= const | PI | PI_2 | PI_4 | 1_PI | 2_PI | 2_SQRTPI | SQRT2 | SQRT1_2 | E | LOG2E | LOG10E | LN2 | LN10 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | N1 | N2 | N3 | N4 | N5 | N6 | N7 | N8 | N9
       <attribute> ::= """
   for j in range(len(attr_ids)-1):
      grammar += "attr" + str(j)
      if j < len(attr_ids)-2:
         grammar += " | "

   with open(tmpdir+"/grammar.bnf", 'w') as f: f.write(grammar+'\n')

   os.system('cd ' + tmpdir + ' && cmake ' + args.pee + ' ' + args.cmake_args + ' -DGRAMMAR='+tmpdir+'/grammar.bnf -DLABEL=select -DCMAKE_BUILD_TYPE=RELEASE -DPROFILING=OFF > /dev/null && make -j -s')
   mae = float(subprocess.check_output(tmpdir + "/select -d " + dataset + " " + args.pee_args + " | grep -a '^> [0-9]' | cut -d';' -f3", shell=True))

   return mae

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--dataset", required=True, help="Dataset file")
parser.add_argument("-t", "--threshold", required=False, type=float, default=0.01, help="Threshold value")
parser.add_argument("-o", "--output", required=True, help="Output dataset with the selected attributes")
parser.add_argument("-p", "--pee", required=True, help="PEE directory")
parser.add_argument("-pa", "--pee-args", default='', help="Arguments for PEE")
parser.add_argument("-ca", "--cmake-args", default='', help="Arguments for CMAKE")
parser.add_argument("-hy", "--has-y", action='store_true', default=False, help="Has dependent variable")
parser.add_argument("-hh", "--has-header", action='store_true', default=False, help="Has header")
parser.add_argument("-nr", "--no-relative", action='store_true', default=False, help="Don't make the error relative by dividing it by the mean L1 norm of the attribute")
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

TMPDIR = tempfile.mkdtemp(prefix='build-')

print "The temporary directory is: %s" % (TMPDIR)

index = [0]
rmaes = [np.inf]
for i in range(1, nvar):
   index.append(i)
   if args.has_header:
      header_str = ','.join([h for j, h in enumerate(header) if j in index])
   else:
      header_str = ''
   np.savetxt(args.output, data[:,index], delimiter=",", header=header_str, comments='', fmt='%.8f')

   #######################
   mae = RunPEE(index, args.output, TMPDIR)
   #######################

   if args.no_relative:
      mean_norm = 1.0
   else:
      i_values = data[:,i]
      mean_norm = linalg.norm(i_values, 1)/len(i_values) # Compute the L1 norm divided by the number of values on the column
      if abs(mean_norm) <= 1e-8:
         mean_norm = 1.0
   print "\n:: [ATTR-%d]: MAE=%f, MEAN L1 NORM=%f, RELATIVE MAE=%f -> " % (i, mae, mean_norm, mae/mean_norm),

   if mae/mean_norm <= args.threshold: # Use Relative MAE to account for different magnitudes so the threshold can be applied to non-normalized datasets
      index.pop()
      print "CORRELATED attributed @ t=%s" % (args.threshold)
   else:
      rmaes.append(mae/mean_norm)
      print "NON-CORRELATED attributed @ t=%s" % (args.threshold)
      print ":: Selected %d features (%s%%):" % (len(index), 100.0*len(index)/float(i+1)), index
      print ":: Relative entropies:", [float('{:.3}'.format(r)) for r in rmaes]
      if args.has_header:
         print ":: Selected feature names:", [h for j, h in enumerate(header) if j in index]
   print

if args.has_y:
   index.append(len(data[0])-1)

print ":: Excluded feature indices:", [j for j in range(0,nvar) if j not in index]
if args.has_header:
   print ":: Excluded feature names:", [h for j, h in enumerate(header) if j not in index]
   header_str = ','.join([h for j, h in enumerate(header) if j in index])
else:
   header_str = ''

np.savetxt(args.output, data[:,index], delimiter=",", header=header_str, comments='', fmt='%.8f')

# Remove temporary directory
shutil.rmtree(TMPDIR, ignore_errors=True)
