#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, random
import numpy as np
import argparse

#random.seed(100)

# -e '+ X1 * 3.14 / X2 X3' -e '+ X1 * 3.14 / X3 X2' -e '+ X2 * 3.14 / X1 X3' -e '+ X2 * 3.14 / X3 X1' -e '+ X3 * 3.14 / X1 X2' -e '+ X3 * 3.14 / X2 X1' -e '+ X1 X1' -e '/ X1 0.0'

################################################################################
# Nice progress bar
################################################################################
def ProgressBar(count, total, suffix='', out=sys.stdout):
    bar_len = 50
    filled_len = int(round(bar_len * count / float(total)))

    percents = 100.0 * count / float(total)
    bar = '#' * filled_len + '-' * (bar_len - filled_len)

    out.write('[%s] %.2f%s (%d/%d)%s\r' % (bar, percents, '%', count, total, suffix))
    out.flush()

def get_sample(attrname):
   return np.random.uniform(args.min,args.max) # TODO: uses a different distribution depending on the attribute

def print_stats(means, stds, attributes, nones, exps):
   stat_means = {}
   stat_stds = {}
   sum_stat_means = 0.0
   for i, attr in enumerate(attributes):
      if means[i]:
         stat_means[i] = np.median(means[i])
         stat_stds[i] = np.median(stds[i])
         sum_stat_means += stat_means[i]

   out = ''
   for i, attr in enumerate(attributes):
      if means[i]:
         frequency=len(means[i])
         out += "%s: impact=%.2f%% (%.3f±%.1f), frequency=%.2f%% (%d/%d), none=%.2f%% (%d/%d)\n" % (attr, 100.*stat_means[i]/sum_stat_means, stat_means[i], stat_stds[i], 100. * frequency/float(len(exps)), frequency, len(exps), 100.*none[i]/float(args.iterations*args.scenarios*len(exps)), none[i], args.iterations*args.scenarios*len(exps))
   sys.stdout.write(out)

def interpreter(exp, attr):
   stack = []
   for token in exp:
      if token == "IF-THEN-ELSE":
         cond = stack.pop(); true = stack.pop(); false = stack.pop();
         if cond: stack.append(true)
         else: stack.append(false)
      elif token == "AND":
         cond1 = stack.pop(); cond2 = stack.pop();
         if cond1 == 1.0 and cond2 == 1.0: stack.append(1.0)
         else: stack.append(0.0)
      elif token == "OR":
         cond1 = stack.pop(); cond2 = stack.pop();
         if cond1 == 1.0 or cond2 == 1.0: stack.append(1.0)
         else: stack.append(0.0)
      elif token == "NOT":
         stack.append(float(not(stack.pop())))
      elif token == ">":
         if stack.pop() > stack.pop(): stack.append(1.0)
         else: stack.append(0.0)
      elif token == "<":
         if stack.pop() < stack.pop(): stack.append(1.0)
         else: stack.append(0.0)
      elif token == "=":
         #value1 = stack.pop()
         #value2 = stack.pop()
         #print value1 
         #print value2
         #if value1 == value2:
         #   stack.append(1.0)
         #   print "TRUE"
         #else:
         #   stack.append(0.0)
         #   print "FALSE"
         if stack.pop() == stack.pop(): stack.append(1.0)
         else: stack.append(0.0)
      elif token == "+":
         stack.append(stack.pop() + stack.pop())
      elif token == "-":
         stack.append(stack.pop() - stack.pop())
      elif token == "*":
         value = stack.pop() * stack.pop()
         if value == np.inf:
            return None 
         else: 
            stack.append(value)
      elif token == "/":
         num = stack.pop(); den = stack.pop()
         if abs(den) > 0.0: stack.append(num / den)
         else:
            return None
      elif token == "MEAN":
         stack.append((stack.pop() + stack.pop()) / 2.0)
      elif token == "MAX":
         stack.append(max(stack.pop(),stack.pop()))
      elif token == "MIN":
         value1 = stack.pop(); value2 = stack.pop();
         #print value1, value2, min(value1,value2)
         stack.append(min(value1,value2))
      elif token == "ABS":
         stack.append(abs(stack.pop()))
      elif token == "SQRT":
         value = stack.pop()
         if value >= 0.0: stack.append(sqrt(value))
         else:
            return None
      elif token == "POW2":
         try:
            value = pow(stack.pop(),2.0)
         except OverflowError:
            return None
         if value == np.inf:
            return None 
         else: 
            stack.append(value)
      elif token == "POW3":
         try:
            value = pow(stack.pop(),3.0)
         except OverflowError:
            return None
         if value == np.inf:
            return None 
         else: 
            stack.append(value)
      elif token == "NEG":
         stack.append(-stack.pop())
      elif token == "1":
         stack.append(1.0)
      elif token == "2":
         stack.append(2.0)
      elif token == "3":
         stack.append(3.0)
      elif token == "4":
         stack.append(4.0)
      elif token in attr:
         stack.append(float(attr[token]))
      else:
         # Nenhum dos casos acima, empilha diretamente o valor da constante
         stack.append(float(token))
   #print (stack)
   return stack.pop()


parser = argparse.ArgumentParser()
parser.add_argument('-e', '--exp', action='append', dest='exps', required=True, default=[], help="Set the expressions to be evaluated. Ex: -e '+ ATTR0 ATTR1' -e 'ATTR1'")
parser.add_argument('-s', '--scenarios', required=False, type=int, default=100, help="The number of scenarios [default=100]")
parser.add_argument('-i', '--iterations', required=False, type=int, default=30, help="The number of iterations [default=30]")
parser.add_argument('-min', '--min', required=False, type=float, default=0.0, help="Minimum value while sampling [default=0.0]")
parser.add_argument('-max', '--max', required=False, type=float, default=1.0, help="Maximum value while sampling [default=1.0]")
args = parser.parse_args()

# The interpreter works from the end to the beginning (reversed order)
exps = [list(reversed(e.split())) for e in args.exps]

attrNames = []
for e in exps:
   for t in e: # Accepts one of: ATTR, ATTRN, X, XN, VAR, VARN, V, VN; with N in [0,9]
      if t.upper()=='ATTR' or (t.upper().startswith('ATTR') and len(t)>4 and t[4] in [str(i) for i in range(10)]):
         attrNames.append(t)
      elif t.upper()=='VAR' or (t.upper().startswith('VAR') and len(t)>3 and t[3] in [str(i) for i in range(10)]):
         attrNames.append(t)
      elif t.upper()=='X' or (t.upper().startswith('X') and len(t)>1 and t[1] in [str(i) for i in range(10)]):
         attrNames.append(t)
      elif t.upper()=='V' or (t.upper().startswith('V') and len(t)>1 and t[1] in [str(i) for i in range(10)]):
         attrNames.append(t)


attrNames = list(sorted(set(attrNames)))

partial = []; temp = []; means = []; stds = []; none = {};
for i in range(0, len(attrNames)):
   partial.append([])
   temp.append([])
   means.append([])
   stds.append([])
   none[i] = 0

count = 0
for exp in exps:
   for scenario in range(0,args.scenarios):
      attr = {}; attr = {a : get_sample(a) for a in attrNames}

      value1 = interpreter(exp, attr)

      if value1 is not None:
         for i, a in enumerate(attrNames):
            if a in exp:
               copy = dict.copy(attr)
         
               for j in range(0,args.iterations):
         
                  copy[a] = get_sample(a)

                  value2 = interpreter(exp, copy)
                  if value2 is not None:
                     partial[i].append(abs(value2 - value1))
                  else:
                     none[i] += 1

               if partial[i]:
                  temp[i].append(np.median(partial[i]))
                  del partial[i][:]
      else:
         for i, a in enumerate(attrNames):
            if a in exp:
               none[i] += args.iterations # all inner iterations are none (for each attribute)

      count += 1
      ProgressBar(count, args.scenarios*len(exps), out=sys.stderr)

   for i in range(0,len(attrNames)):
      if temp[i]:
         if np.mean(temp[i]) > 0.0:
            means[i].append(np.mean(temp[i]))
            stds[i].append(np.std(temp[i]))
         del temp[i][:]


print_stats(means, stds, attrNames, none, exps)
