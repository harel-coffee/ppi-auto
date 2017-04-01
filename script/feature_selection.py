#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
from numpy import genfromtxt
from numpy import linalg
import subprocess, os, argparse, csv, tempfile, shutil, sys

def RunCorrCoef(training_data):
   # Compute the correlation of the last column with all other columns
   correlations = np.corrcoef(training_data, rowvar=0)[-1][:-1]
   # Return 1.0 minus the maximum absolute correlation, so the greater the value, the less correlated the last column is with the remaining columns
   return 1.0 - np.max(map(abs, correlations))

def RunPEE(index, training_data, tmpdir):
   # Save the training dataset to a file (no need to save the header at this time, but doing it anyway)
   np.savetxt(args.output, training_data, delimiter=",", header=','.join([h for j, h in enumerate(header) if j in index]), comments='', fmt='%.8f')

   grammar = r"""   S = <exp>
   P = <exp> ::= <attribute> | <numeric_const> | <un_op> <exp> | <bin_op> <exp> <exp>
       <bin_op> ::= AND | OR | XOR | GREATER | GREATEREQUAL | LESS | LESSEQUAL | EQUAL | NOTEQUAL | ADD | SUB | MULT | DIV | MEAN | MAX | MIN | MOD | POW
       <un_op> ::= NOT | ABS | SQRT | POW2 | POW3 | POW4 | POW5 | NEG | ROUND | CEIL | FLOOR | EXP | EXP10 | EXP2 | LOG | LOG10 | LOG2 | SIN | COS | TAN | STEP | SIGN | LOGISTIC | GAMMA | GAUSS
       <numeric_const> ::= const | PI | PI_2 | PI_4 | 1_PI | 2_PI | 2_SQRTPI | SQRT2 | SQRT1_2 | E | LOG2E | LOG10E | LN2 | LN10 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | N1 | N2 | N3 | N4 | N5 | N6 | N7 | N8 | N9
       <attribute> ::= """
   for j in range(len(index)-1):
      grammar += "attr" + str(j)
      if j < len(index)-2:
         grammar += " | "
   grammar += '\n'

   # Check if we really need to rebuild PEE, i.e., if the grammar has changed
   try:
      with open(tmpdir+"/grammar.bnf", 'r') as g: old_grammar = g.read()
   except:
      old_grammar = None

   if old_grammar is None or old_grammar != grammar:
      with open(tmpdir+"/grammar.bnf", 'w') as g: g.write(grammar)
      os.system('cd ' + tmpdir + ' && cmake ' + args.pee + ' ' + args.cmake_args + ' -DGRAMMAR='+tmpdir+'/grammar.bnf -DLABEL=select -DCMAKE_BUILD_TYPE=RELEASE -DPROFILING=OFF > /dev/null && make -j -s')

   mae = float(subprocess.check_output(tmpdir + "/select -d " + args.output + " " + args.pee_args + " | grep -a '^> [0-9]' | cut -d';' -f3", shell=True))

   return mae

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--dataset", required=True, help="Dataset file")
parser.add_argument("-t", "--threshold", required=False, type=float, default=0.01, help="Threshold value")
parser.add_argument("-o", "--output", required=True, help="Output dataset with the selected features")
parser.add_argument("-p", "--pee", default='', help="PEE directory")
parser.add_argument("-pearson", "--pearson", action='store_true', default=False, help="Runs Pearson correlation instead of PEE")
parser.add_argument("-pa", "--pee-args", default='', help="Arguments for PEE")
parser.add_argument("-ca", "--cmake-args", default='', help="Arguments for CMAKE")
parser.add_argument("-hy", "--has-y", action='store_true', default=False, help="Has dependent variable")
parser.add_argument("-hh", "--has-header", action='store_true', default=False, help="Has header")
parser.add_argument("-nr", "--no-relative", action='store_true', default=False, help="Don't make the error relative by dividing it by the mean L1 norm of the feature")
args = parser.parse_args()

args.output = os.path.abspath(args.output)

if args.pee:
   args.pee = os.path.abspath(args.pee)

if args.pearson: # don't make sense to use relative norm with Pearson
   args.no_relative = True
else:
   if not args.pee:
      print >> sys.stderr, "> ERROR: PEE directory not given. Use -pearson if you would like to run a linear correlation analysis instead."
      sys.exit(1)
   # Create temporary directory for PEE
   TMPDIR = tempfile.mkdtemp(prefix='build-')
   print "The temporary directory is: %s" % (TMPDIR)


header = []
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

index = [0]
rmaes = [np.inf]
for i in range(1, nvar):
   # Make 'i' the dependent variable (y), because we want to check if it can
   # be expressed by the other features.
   index.append(i)

   # Prepare the sliced training data (and header) to be used as regression
   training_data = data[:,index]
   #######################
   if args.pearson:
      mae = RunCorrCoef(training_data)
   else:
      mae = RunPEE(index, training_data, TMPDIR)
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
      print "CORRELATED feature @ t=%s" % (args.threshold)
   else:
      rmaes.append(mae/mean_norm)
      print "NON-CORRELATED feature @ t=%s" % (args.threshold)
      print ":: Selected %d features (%.2f%%):" % (len(index), 100.0*len(index)/float(i+1)), index
      print ":: Relative entropies:", [float('{:.3}'.format(r)) for r in rmaes]
      if args.has_header:
         print ":: Selected feature names:", ','.join([h for j, h in enumerate(header) if j in index])
   print

if args.has_y:
   index.append(len(data[0])-1)

#print ":: Excluded feature indices:", list(set(range(nvar)).symmetric_difference(set(index)))
print ":: Excluded feature indices:", [j for j in range(0,nvar) if j not in index]
if args.has_header:
   print ":: Excluded feature names:", ','.join([h for j, h in enumerate(header) if j not in index])

print "\n:: Selected %d features indices (%.2f%%):" % (len(index), 100.0*len(index)/float(i+1)), index
print ":: Relative entropies:", [float('{:.3}'.format(r)) for r in rmaes]
if args.has_header:
   print ":: Selected feature names:", ','.join([h for j, h in enumerate(header) if j in index])

np.savetxt(args.output, data[:,index], delimiter=",", header=','.join([h for j, h in enumerate(header) if j in index]), comments='', fmt='%.8f')

# Remove temporary directory
if not args.pearson:
   shutil.rmtree(TMPDIR, ignore_errors=True)
