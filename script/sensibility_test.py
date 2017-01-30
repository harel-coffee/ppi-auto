#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, random
import numpy as np

niter = 30
scenarios = 100

#random.seed(100)

attrNames = ["X1", "X2", "X3"] 

#exps = [['+', 'X1', '*', '3.14', '/', 'X2', 'X3'],['+', 'X1', '*', '3.14', '/', 'X2', 'X3'],['+', 'X1', '*', '3.14', '/', 'X2', 'X3']]

exps = [
['+', 'X1', '*', '3.14', '/', 'X2', 'X3'],
['+', 'X1', '*', '3.14', '/', 'X3', 'X2'],
['+', 'X2', '*', '3.14', '/', 'X1', 'X3'],
['+', 'X2', '*', '3.14', '/', 'X3', 'X1'],
['+', 'X3', '*', '3.14', '/', 'X1', 'X2'],
['+', 'X3', '*', '3.14', '/', 'X2', 'X1'], ['+', 'X1', 'X1'], ['/', 'X1', '0.0']]

#exps = [['+', '+', 'X1', 'X2', 'X3']]

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
   return np.random.uniform(0.0,1.0) # TODO: uses a different distribution depending on the attribute

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
         out += "%s: impact=%.2f%% (%.3f±%.1f), frequency=%.2f%% (%d/%d), none=%.2f%% (%d/%d)\n" % (attr, 100.*stat_means[i]/sum_stat_means, stat_means[i], stat_stds[i], 100. * frequency/float(len(exps)), frequency, len(exps), 100.*none[i]/float(niter*scenarios*len(exps)), none[i], niter*scenarios*len(exps))
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


partial = []; temp = []; means = []; stds = []; none = {};
for i in range(0, len(attrNames)):
   partial.append([])
   temp.append([])
   means.append([])
   stds.append([])
   none[i] = 0

# The interpreter works from the end to the beginning (reversed order)
exps = [list(reversed(e)) for e in exps]

count = 0
for exp in exps:
   for scenario in range(0,scenarios):
      attr = {}; attr = {a : get_sample(a) for a in attrNames}

      value1 = interpreter(exp, attr)

      if value1 is not None:
         for i, a in enumerate(attrNames):
            if a in exp:
               copy = dict.copy(attr)
         
               for j in range(0,niter):
         
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
               none[i] += niter # all inner iterations are none (for each attribute)

      count += 1
      ProgressBar(count, scenarios*len(exps), out=sys.stderr)

   for i in range(0,len(attrNames)):
      if temp[i]:
         if np.mean(temp[i]) > 0.0:
            means[i].append(np.mean(temp[i]))
            stds[i].append(np.std(temp[i]))
         del temp[i][:]


print_stats(means, stds, attrNames, none, exps)
