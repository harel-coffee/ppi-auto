#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, random, csv
import numpy as np
import math
import argparse

#np.random.seed(1)

# Make numpy ignore the following exceptions: 'divide', 'invalid', 'over', 'under'.
# Replacing 'ignore' with 'raise' will make the interpreter more intransigent and
# will return 'None' more frequently (in some cases, for instance, instead of
# Inf, None will be returned).
np.seterr(all='ignore')
#np.seterr(all='raise')


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


def GetAttributeIndex(attr):
   # Extract only the digits from the given string (attr)
   number = ''.join([i for i in attr if i >= '0' and i <= '9'])
   if len(number) == 0: # For instance, X or ATTR
      return 0
   else:
      return int(number)


class Generator:
   def __init__(self, distribution, attributes, random=True, amount=None):
      self.attributes = attributes
      self.distribution = distribution
      self.amount = amount
      self.random = random
      for a in self.attributes:
         maxlen = 0
         if len(self.distribution[a]) > maxlen:
            maxlen = len(self.distribution[a])

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
               sample[a] = self.distribution[a][np.random.randint(0, len(self.distribution[a]))]
            yield sample
            count += 1

      else:  # Just iterate over all values
         count = 0
         counts = {} # Each attribute can potentially has a different number of values, so we need to store counting's separately
         while count < self.amount:
            sample = {}
            for a in self.attributes:
               if counts.setdefault(a, 0) >= len(self.distribution[a]): # Rewind the count of an attribute when it reaches the end
                  counts[a] = 0
               sample[a] = self.distribution[a][counts[a]]
               counts[a] += 1
            yield sample
            count += 1


def print_stats(scenarios_stats, scenarios_stdev, attributes, invalids, exps, total_iterations):
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
         out += "%s: impact=%.2f%% (%.3f±%.1f), frequency=%.2f%% (%d/%d), invalids=%.2f%% (%d/%d)\n" % (a, 100.*exps_stats[a]/sum_exps_stats, exps_stats[a], exps_stdev[a], 100. * frequency/float(len(exps)), frequency, len(exps), 100.*invalids[a]/float(total_iterations), invalids[a], total_iterations)
   sys.stdout.write(out)


def interpreter(exp, attr):
   stack = []
   for token in exp:
      if token == "IF-THEN-ELSE":
         cond = stack.pop(); true = stack.pop(); false = stack.pop();
         if cond:
            stack.append(true)
         else:
            stack.append(false)
      elif token == "AND":
         cond1 = stack.pop(); cond2 = stack.pop();
         if cond1 and cond2:
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "OR":
         cond1 = stack.pop(); cond2 = stack.pop();
         if cond1 or cond2:
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "XOR":
         if float(not(stack.pop())) != float(not(stack.pop())):
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
         stack.append(stack.pop() * stack.pop())
      elif token == "MEAN":
         stack.append((stack.pop() + stack.pop()) / 2.0)
      elif token == "MAX":
         stack.append(max(stack.pop(),stack.pop()))
      elif token == "MIN":
         op1 = stack.pop(); op2 = stack.pop();
         stack.append(min(op1,op2))
      elif token == "MOD":
         try:
            stack.append(stack.pop() % stack.pop())
         except ZeroDivisionError:
            return None
      elif token == "POW":
         try:
            stack.append(math.pow(stack.pop(),stack.pop()))
         except ValueError:
            return None
         except OverflowError:
            stack.append(np.inf)
         except ZeroDivisionError:
            return None
      elif token == "ABS":
         stack.append(abs(stack.pop()))
      elif token == "POW2":
         try:
            stack.append(pow(stack.pop(),2.0))
         except OverflowError:
            stack.append(np.inf)
      elif token == "POW3":
         try:
            stack.append(pow(stack.pop(),3.0))
         except OverflowError:
            stack.append(np.inf)
      elif token == "POW4":
         try:
            stack.append(pow(stack.pop(),4.0))
         except OverflowError:
            stack.append(np.inf)
      elif token == "POW5":
         try:
            stack.append(pow(stack.pop(),5.0))
         except OverflowError:
            stack.append(np.inf)
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
            stack.append(np.exp(stack.pop()))
         except OverflowError:
            stack.append(np.inf)
         except FloatingPointError:
            return None
      elif token == "EXP10":
         try:
            stack.append(pow(10., stack.pop()))
         except OverflowError:
            stack.append(np.inf)
         except FloatingPointError:
            return None
      elif token == "EXP2":
         try:
            stack.append(np.exp2(stack.pop()))
         except OverflowError:
            stack.append(np.inf)
         except FloatingPointError:
            return None
      elif token == "SIN":
         stack.append(np.sin(stack.pop()))
      elif token == "COS":
         stack.append(np.cos(stack.pop()))
      elif token == "STEP":
         if stack.pop() >= 0.0:
            stack.append(1.0)
         else:
            stack.append(0.0)
      elif token == "SIGN":
         op = stack.pop()
         if op > 0.0:
            stack.append(1.0)
         elif op < 0.0:
            stack.append(-1.0)
         else:
            stack.append(0.0)
      elif token == "LOGISTIC":
         try:
            stack.append((1.0/(1.0 + np.exp(-stack.pop()))))
         except OverflowError:
            stack.append(np.inf)
         except FloatingPointError:
            return None
      elif token == "GAMMA":
         op = stack.pop()
         try:
            stack.append(math.pow((op/2.71828174591064)*np.sqrt(op*np.sinh(1.0/op)),op)*np.sqrt(2.0*3.14159274101257/op))
         except ValueError:
            return None
         except OverflowError:
            stack.append(np.inf)
         except ZeroDivisionError:
            return None
      elif token == "GAUSS":
         op = stack.pop()
         try:
            stack.append(np.exp(-op*op));
         except OverflowError:
            stack.append(np.inf)
         except FloatingPointError:
            return None
      elif token == "/":
         try:
            stack.append(stack.pop()/stack.pop())
         except ZeroDivisionError:
            return None
      elif token == "SQRT":
         try:
            stack.append(np.sqrt(stack.pop()))
         except FloatingPointError:
            return None
      elif token == "LOG":
         try:
            stack.append(np.log(stack.pop()))
         except FloatingPointError:
            return None
      elif token == "LOG10":
         try:
            stack.append(np.log10(stack.pop()))
         except FloatingPointError:
            return None
      elif token == "LOG2":
         try:
            stack.append(np.log2(stack.pop()))
         except FloatingPointError:
            return None
      elif token == "TAN":
         try:
            stack.append(np.tan(stack.pop()))
         except FloatingPointError:
            return None
      elif token == "pi":
         stack.append(3.14159274101257)
      elif token == "pi/2":
         stack.append(1.57079637050629)
      elif token == "pi/4":
         stack.append(0.78539818525314)
      elif token == "1/pi":
         stack.append(0.31830987334251)
      elif token == "2/pi":
         stack.append(0.63661974668503)
      elif token == "2/sqrt(pi)":
         stack.append(1.12837922573090)
      elif token == "sqrt(2)":
         stack.append(1.41421353816986)
      elif token == "sqrt(2)/2":
         stack.append(0.70710676908493)
      elif token == "e":
         stack.append(2.71828174591064)
      elif token == "log2(e)":
         stack.append(1.44269502162933)
      elif token == "log10(e)":
         stack.append(0.43429449200630)
      elif token == "ln(2)":
         stack.append(0.69314718246460)
      elif token == "ln(10)":
         stack.append(2.30258512496948)
      elif token in attr:
         stack.append(float(attr[token]))
      else:
         # It's probably a constant, so just stacking it
         stack.append(float(token))
   res = stack.pop()
   if math.isnan(res) or math.isinf(res):
      return None
   else:
      return res


parser = argparse.ArgumentParser()
parser.add_argument('-e', '--exp', action='append', dest='exps', required=True, default=[], help="Set the expressions to be evaluated (can be given multiple times). If given '-e -' then expressions from stdin will be read as well (one per line). Ex: -e '+ ATTR-0 ATTR-1' -e 'ATTR-1'")
parser.add_argument('-d', '--dataset', required=True, default='', help="CSV dataset file representing the distribution of each attribute (it is 0-based, so for example the set of valid values (distribution) for ATTR-5 is on the 6th column)")
parser.add_argument('-s', '--scenarios', required=False, type=int, default=None, help="The number of scenarios [default=number of rows of the dataset]")
parser.add_argument('-srs', action='store_true', default=False, help="Randomly sample each scenario instead of sequentially iterating over all rows of the dataset")
parser.add_argument('-p', '--perturbations', required=False, type=int, default=None, help="The number of perturbations for each attribute [default=number of values of the attribute]")
parser.add_argument('-prs', action='store_true', default=False, help="Randomly sample each attribute perturbation instead of iterating over all its values")
parser.add_argument('-pf', '--perturbation-function', required=False, default='lambda x,y: abs(x-y)', help="Perturbation measurement function (between scenario value and perturbed value) [default='lambda x,y: abs(x-y)']")
parser.add_argument('-psf', '--perturbations-stats-function', required=False, default='lambda x: np.median(x)', help="Perturbations statistics function used to aggregate multiple measurements of perturbations (between a scenario value and perturbed values) [default='lambda x: np.median(x)']")
parser.add_argument('-ssf', '--scenarios-stats-function', required=False, default='lambda x: np.mean(x)', help="Scenarios statistics function used to aggregate multiple scenarios measurements [default='lambda x: np.mean(x)']")
parser.add_argument('-esf', '--expressions-stats-function', required=False, default='lambda x: np.median(x)', help="Expressions statistics function used to aggregate scenarios statistics when an attribute is present in multiple expression [default='lambda x: np.median(x)']")
parser.add_argument('-st', '--significance-threshold', required=False, type=float, default=0.0, help="Attributes having a 'scenarios statistics' value greater than that are considered statistically present in the expression [default=0.0]")
parser.add_argument('-a', '--attributes', nargs='+', dest='attributes', type=str, required=False, help="Subset of attributes to be considered [default=all]")
parser.add_argument('-c', '--correlation', nargs="?", const=True, action='store', required=False, default=False, help="Enable generation of correlation graphics for each attribute using the given function to aggregate predictions [if -c is given, default='lambda x: np.mean(x)']")

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
exps = []
for e in args.exps:
   if e == '-': # "-e -" will make the script read also from stdin
      for i in sys.stdin.read().splitlines():
         if i.strip():
            exps.append(list(reversed(i.split())))
   else:
      exps.append(list(reversed(e.split())))


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
attrNamesFiltered = [a for a in attrNames if (not args.attributes or a in args.attributes)] # Only the attributes the user want (via '-a' option)

attrToIndices = {a: GetAttributeIndex(a) for a in attrNames}
indicesToAttr = {v: k for k, v in attrToIndices.items()} # Invert the mapping: from ATTR-X:X to X:ATTR-X

distribution = {}
with open(args.dataset) as dist_file:
   for r, row in enumerate(csv.reader(dist_file)):
      for i, v in enumerate(row):
         if i in indicesToAttr:
            try:
               distribution.setdefault(indicesToAttr[i], []).append(float(v))
            except ValueError:
               print >> sys.stderr, "> [%s]: could not convert '%s' to float at line %d, column %d of '%s', skipping..." % (indicesToAttr[i], v, r+1, i+1, args.dataset)

perturbations = {}; perturbations_stats = {}; scenarios_stats = {}; scenarios_stdev = {}; invalids = {}; correlations = {}
for a in attrNamesFiltered:
   perturbations[a] = []
   perturbations_stats[a] = []
   scenarios_stats[a] = []
   scenarios_stdev[a] = []
   invalids[a] = 0
   if args.correlation != False:
      correlations[a] = {}

# perturbation_function = lambda...
exec("perturbation_function = " + args.perturbation_function)
exec("perturbations_stats_function = " + args.perturbations_stats_function)
exec("scenarios_stats_function = " + args.scenarios_stats_function)
exec("expressions_stats_function = " + args.expressions_stats_function)

count = 0
total_iterations = 0
for exp in exps:
   expAttrNames = [a for a in attrNames if a in exp] # Only the attributes in the expression
   expAttrNamesFiltered = [a for a in attrNamesFiltered if a in exp] # Only the attributes in the expression that we want to compute stats
   if not expAttrNamesFiltered: # Some expressions don't have any desired attribute at all
      continue
   scenarios = Generator(distribution, expAttrNames, random=args.srs, amount=args.scenarios)
   for attr in scenarios:
      scenario_value = interpreter(exp, attr)
      #print scenario_value

      if scenario_value is not None:
         for a in expAttrNamesFiltered:
            original = attr[a] # Back up the original value for attr[a] before perturbation
            samples = Generator(distribution, [a], random=args.prs, amount=args.perturbations)
            for sample in samples:
               attr[a] = sample[a]
               perturbed_value = interpreter(exp, attr)
               if perturbed_value is not None:
                  perturbations[a].append(perturbation_function(scenario_value, perturbed_value))
                  if args.correlation != False:
                     # For a given attribute (a), this records all output values (perturbed_value's) for each perturbation value (attr[a])
                     # attr_i: perturbation_a -> {output_1, ..., output_n}
                     #         ...
                     #         perturbation_z -> {output_1, ..., output_m}
                     # ...
                     correlations[a].setdefault(attr[a], []).append(perturbed_value)
               else:
                  invalids[a] += 1
            attr[a] = original # Restore the original value of attr[a]

            if perturbations[a]:
               perturbations_stats[a].append(perturbations_stats_function(perturbations[a]))
               del perturbations[a][:]
            total_iterations += samples.GetAmount()
      else:
         for a in expAttrNamesFiltered:
            samples = Generator(distribution, [a], random=args.prs, amount=args.perturbations)
            invalids[a] += samples.GetAmount() # all inner perturbations ended up being invalid (for each attribute)
            total_iterations += samples.GetAmount()

      count += 1
      ProgressBar(count, scenarios.GetAmount()*len(exps), out=sys.stderr)

   for a in expAttrNamesFiltered:
      if perturbations_stats[a]:
         stats = scenarios_stats_function(perturbations_stats[a])
         if stats > args.significance_threshold: # is attribute 'a' statistically present? Maybe it is not even touched at all during the interpretation or it is something like "- X X"
            scenarios_stats[a].append(stats) # Append scenario stats (mean, median, ...) for expression exp
            scenarios_stdev[a].append(np.std(perturbations_stats[a])) # Append scenario standard deviation for expression exp
         del perturbations_stats[a][:]

print_stats(scenarios_stats, scenarios_stdev, attrNamesFiltered, invalids, exps, total_iterations)

################################################################################
####### Correlation plots (attribute values x corresponding predictions)
################################################################################
if args.correlation != False: # -c was given
   import matplotlib.pyplot as plt

   if args.correlation == True: # -c was given but without argument, using default
      args.correlation = 'lambda x: np.mean(x)' # No correlation function was given, using default np.mean

   exec("correlation_function = " + args.correlation)

   for a in correlations:
      if not scenarios_stats[a]: # Skip if the current attribute 'a' is not significantly present (according to the significance threshold)
         continue
      xs = []
      ys = []
      #sz = []
      for x in correlations[a]:
         stats = correlation_function(correlations[a][x])
         #stdev = np.std(correlations[a][x])
         if type(stats) is list:
            correlations[a][x] = stats
         else:
            correlations[a][x] = [stats]
         for y in correlations[a][x]:
            xs.append(x)
            ys.append(y)
            #sz.append(stdev)
            #print (x, y)

      plt.title(a)
      plt.xlabel('Attribute range')
      plt.ylabel('Predicted value')
      plt.scatter(xs, ys, s=[15.0], c=ys, cmap=plt.cm.jet)
      plt.show()
#######
