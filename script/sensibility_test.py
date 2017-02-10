#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, random, csv
import numpy as np
import argparse

#np.random.seed(1)

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


class Generator:
   def __init__(self, distribution, attributes, random=True, amount=None):
      self.attributes = attributes
      self.distribution = distribution
      self.amount = amount
      self.random = random
      self.indices = {}
      for a in self.attributes:
         maxlen = 0
         index = self.GetAttributeIndex(a)
         if index >= len(self.distribution):
            print >> sys.stderr, "> ERROR: 0-based index of attribute '%s' (index=%d) is out of range (dataset has only %d columns)" % (a, index, len(self.distribution))
            sys.exit(1)

         self.indices[a] = index
         if len(self.distribution[index]) > maxlen:
            maxlen = len(self.distribution[index])

      if amount is None: # Let's automatically set a reasonable amount
         self.amount = maxlen

   def GetAmount(self):
      return self.amount

   def __iter__(self):
      if self.random:
         count = 0
         while count < self.amount:
            sample = {}
            for a in self.attributes:
               index = self.indices[a]
               sample[a] = self.distribution[index][np.random.randint(0, len(self.distribution[index]))]
            yield sample
            count += 1

      else:  # Just iterate over all values
         count = 0
         counts = {} # Each attribute can potentially has a different number of values, so we need to store counting's separately
         while count < self.amount:
            sample = {}
            for a in self.attributes:
               index = self.indices[a]
               if counts.setdefault(index, 0) >= len(self.distribution[index]): # Rewind the count of an attribute when it reaches the end
                  counts[index] = 0
               sample[a] = self.distribution[index][counts[index]]
               counts[index] += 1
            yield sample
            count += 1
   @staticmethod
   def GetAttributeIndex(attr):
      # Extract only the digits from the given string (attr)
      number = ''.join([i for i in attr if i >= '0' and i <= '9'])
      if len(number) == 0: # For instance, X or ATTR
         return 0
      else:
         return int(number)


def print_stats(scenarios_stats, scenarios_stdev, attributes, nones, exps, total_iterations):
   exps_stats = {}
   exps_stdev = {}
   sum_exps_stats = 0.0
   for a in attributes:
      if scenarios_stats[a]:
         exps_stats[a] = expressions_stats_function(scenarios_stats[a])
         exps_stdev[a] = expressions_stats_function(scenarios_stdev[a])
         sum_exps_stats += exps_stats[a]

   out = ''
   for a in attributes:
      if scenarios_stats[a]:
         frequency=len(scenarios_stats[a])
         out += "%s: impact=%.2f%% (%.3f±%.1f), frequency=%.2f%% (%d/%d), none=%.2f%% (%d/%d)\n" % (a, 100.*exps_stats[a]/sum_exps_stats, exps_stats[a], exps_stdev[a], 100. * frequency/float(len(exps)), frequency, len(exps), 100.*none[a]/float(total_iterations), none[a], total_iterations)
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
      elif token == "XOR":
         cond1 = stack.pop(); cond2 = stack.pop();
         if bool(cond1) ^ bool(cond2):
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "NOT":
         stack.append(float(not(stack.pop())))
      elif token == ">":
         if stack.pop() > stack.pop(): stack.append(1.0)
         else: stack.append(0.0)
      elif token == ">=":
         if stack.pop() >= stack.pop():
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "<":
         if stack.pop() < stack.pop(): stack.append(1.0)
         else: stack.append(0.0)
      elif token == "<=":
         if stack.pop() <= stack.pop():
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "=":
         if stack.pop() == stack.pop():
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "!=":
         if stack.pop() != stack.pop():
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "+":
         stack.append(stack.pop() + stack.pop())
      elif token == "-":
         stack.append(stack.pop() - stack.pop())
      elif token == "*":
         res = stack.pop() * stack.pop()
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "MEAN":
         stack.append((stack.pop() + stack.pop()) / 2.0)
      elif token == "MAX":
         stack.append(max(stack.pop(),stack.pop()))
      elif token == "MIN":
         op1 = stack.pop(); op2 = stack.pop();
         stack.append(min(op1,op2))
      elif token == "MOD":
         op1 = stack.pop(); op2 = stack.pop();
         try:
            res = op1 % op2
         except ZeroDivisionError:
            return None
         stack.append(res)
      elif token == "POW":
         op1 = stack.pop(); op2 = stack.pop();
         try:
            res = pow(op1,op2)
         except ValueError:
            return None
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "ABS":
         stack.append(abs(stack.pop()))
      elif token == "POW2":
         try:
            res = pow(stack.pop(),2.0)
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "POW3":
         try:
            res = pow(stack.pop(),3.0)
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "POW4":
         try:
            res = pow(stack.pop(),4.0)
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "POW5":
         try:
            res = pow(stack.pop(),5.0)
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "NEG":
         stack.append(-stack.pop())
      elif token == "ROUND":
         stack.append(round(stack.pop()))
      elif token == "CEIL":
         stack.append(np.ceil(stack.pop()))
      elif token == "FLOOR":
         stack.append(np.floor(stack.pop()))
      elif token == "EXP":
         try:
            res = np.exp(stack.pop())
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "EXP10":
         try:
            res = pow(10., stack.pop())
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "EXP2":
         try:
            res = np.exp2(stack.pop())
         except OverflowError:
            return None
         if res == np.inf:
            return None
         else:
            stack.append(res)
      elif token == "SIN":
         stack.append(np.sin(stack.pop()))
      elif token == "COS":
         stack.append(np.cos(stack.pop()))
      elif token == "/":
         op1 = stack.pop(); op2 = stack.pop()
         try:
            res = op1/op2
         except ZeroDivisionError:
            return None
         stack.append(res)
      elif token == "SQRT":
         op = stack.pop()
         if op >= 0.0: stack.append(sqrt(op))
         else:
            return None
      elif token == "LOG":
         res = np.log(stack.pop())
         if res == -np.inf:
            return None
         else:
            stack.append(res)
      elif token == "LOG10":
         res = np.log10(stack.pop())
         if res == -np.inf:
            return None
         else:
            stack.append(res)
      elif token == "LOG2":
         res = np.log2(stack.pop())
         if res == -np.inf:
            return None
         else:
            stack.append(res)
      elif token == "TAN":
         stack.append(np.tan(stack.pop()))
      elif token in attr:
         stack.append(float(attr[token]))
      else:
         # It's probably a constant, so just stacking it
         stack.append(float(token))
   return stack.pop()


parser = argparse.ArgumentParser()
parser.add_argument('-e', '--exp', action='append', dest='exps', required=True, default=[], help="Set the expressions to be evaluated (can be given multiple times). Ex: -e '+ ATTR-0 ATTR-1' -e 'ATTR-1'")
parser.add_argument('-d', '--dataset', required=True, default='', help="CSV dataset file representing the distribution of each attribute (it is 0-based, so for example the set of valid values (distribution) for ATTR-5 is on the 6th column)")
parser.add_argument('-s', '--scenarios', required=False, type=int, default=None, help="The number of scenarios [default=number of rows of the dataset]")
parser.add_argument('-srs', action='store_true', default=False, help="Randomly sample each scenario instead of sequentially iterating over all rows of the dataset")
parser.add_argument('-p', '--perturbations', required=False, type=int, default=None, help="The number of perturbations for each attribute [default=number of values of the attribute]")
parser.add_argument('-prs', action='store_true', default=False, help="Randomly sample each attribute perturbation instead of iterating over all its values")
parser.add_argument('-pf', '--perturbation-function', required=False, default='lambda x,y: abs(x-y)', help="Perturbation measurement function (between scenario value and perturbed value) [default=lambda x,y: abs(x-y)]")
parser.add_argument('-psf', '--perturbations-stats-function', required=False, default='lambda x: np.median(x)', help="Perturbations statistics function used to aggregate multiple measurements of perturbations (between a scenario value and perturbed values) [default=lambda x: np.median(x)]")
parser.add_argument('-ssf', '--scenarios-stats-function', required=False, default='lambda x: np.mean(x)', help="Scenarios statistics function used to aggregate multiple scenarios measurements [default=lambda x: np.mean(x)]")
parser.add_argument('-esf', '--expressions-stats-function', required=False, default='lambda x: np.median(x)', help="Expressions statistics function used to aggregate scenarios statistics when an attribute is present in multiple expression [default=lambda x: np.median(x)]")

###
# Options table
###
#  -s      | -srs   -> behavior
#  -------- -----
#  None    | False  -> there will be N scenarios, where N is the number of rows of the given dataset (each scenario is a row)
#  M, M>0  | False  -> there will be M scenarios; if M <= N, the first M rows will be taken as scenarios; if M > N, it will wrap around and repeat scenarios
#  None    | True   -> there will be N scenarios, where N is the number of rows of the given dataset; but each attribute of the scenario will be randomly sampled (individually)
#  M, M>0  | True   -> there will be M scenarios; each attribute of the scenario will be randomly sampled (individually)

# For '-p' the behavior is analogous except that each attribute is always treated individually

args = parser.parse_args()

# The interpreter works from the end to the beginning (reversed order)
exps = [list(reversed(e.split())) for e in args.exps]

attrNames = []
for e in exps:
   for t in e: # Accepts one of: ATTR, ATTRN, ATTR-N, X, XN, VAR, VARN, VAR-N, V, VN; with N in [0,9]
      if t.upper()=='ATTR' or (t.upper().startswith('ATTR') and len(t)>4 and (t[4] >= '0' and t[4] <= '9')) or (t.upper().startswith('ATTR-') and len(t)>5 and (t[5] >= '0' and t[5] <= '9')):
         attrNames.append(t)
      elif t.upper()=='VAR' or (t.upper().startswith('VAR') and len(t)>3 and (t[3] >= '0' and t[3] <= '9')) or (t.upper().startswith('VAR-') and len(t)>4 and (t[4] >= '0' and t[4] <= '9')):
         attrNames.append(t)
      elif t.upper()=='X' or (t.upper().startswith('X') and len(t)>1 and (t[1] >= '0' and t[1] <= '9')):
         attrNames.append(t)
      elif t.upper()=='V' or (t.upper().startswith('V') and len(t)>1 and (t[1] >= '0' and t[1] <= '9')):
         attrNames.append(t)


attrNames = list(sorted(set(attrNames)))

distribution = {}
with open(args.dataset) as dist_file:
   for r, row in enumerate(csv.reader(dist_file)):
      for i, v in enumerate(row):
         try:
            distribution.setdefault(i, []).append(float(v))
         except ValueError:
            print >> sys.stderr, "> Warning: could not convert '%s' to float at line %d, column %d of '%s', skipping..." % (v, r+1, i+1, args.dataset)


partial = {}; temp = {}; scenarios_stats = {}; scenarios_stdev = {}; none = {};
for a in attrNames:
   partial[a] = []
   temp[a] = []
   scenarios_stats[a] = []
   scenarios_stdev[a] = []
   none[a] = 0

# perturbation_function = lambda...
exec("perturbation_function = " + args.perturbation_function)
exec("perturbations_stats_function = " + args.perturbations_stats_function)
exec("scenarios_stats_function = " + args.scenarios_stats_function)
exec("expressions_stats_function = " + args.expressions_stats_function)

count = 0
total_iterations = 0
for exp in exps:
   expAttrNames = [a for a in attrNames if a in exp] # Only the attributes in the expression
   if not expAttrNames:
      continue
   scenarios = Generator(distribution, expAttrNames, random=args.srs, amount=args.scenarios)
   for attr in scenarios:
      scenario_value = interpreter(exp, attr)

      if scenario_value is not None:
         for a in expAttrNames:
            original = attr[a] # Back up the original value for attr[a] before perturbation
            samples = Generator(distribution, [a], random=args.prs, amount=args.perturbations)
            for sample in samples:
               attr[a] = sample[a]
               perturbed_value = interpreter(exp, attr)
               if perturbed_value is not None:
                  partial[a].append(perturbation_function(scenario_value, perturbed_value))
               else:
                  none[a] += 1
            attr[a] = original # Restore the original value of attr[a]

            if partial[a]:
               temp[a].append(perturbations_stats_function(partial[a]))
               del partial[a][:]
            total_iterations += samples.GetAmount()
      else:
         for a in expAttrNames:
            samples = Generator(distribution, [a], random=args.prs, amount=args.perturbations)
            none[a] += samples.GetAmount() # all inner perturbations ended up being none (for each attribute)
            total_iterations += samples.GetAmount()

      count += 1
      ProgressBar(count, scenarios.GetAmount()*len(exps), out=sys.stderr)

   for a in expAttrNames:
      if temp[a]:
         stats = scenarios_stats_function(temp[a])
         if stats > 0.0: # is attribute 'a' statistically present? Maybe it is not even touched at all during the interpretation or it is something like "- X X"
            scenarios_stats[a].append(stats) # Append scenario stats (mean, median, ...) for expression exp
            scenarios_stdev[a].append(np.std(temp[a])) # Append scenario standard deviation for expression exp
         del temp[a][:]

print_stats(scenarios_stats, scenarios_stdev, attrNames, none, exps, total_iterations)
