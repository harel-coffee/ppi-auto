#!/usr/bin/env python

import sys
import argparse

def less(a, b):
   return a < b
def greater(a, b):
   return a > b

better = less

def dominated(x, y):
   if len(x) != len(y):
      print "Error: size mismatch!"
      return None
   dominated = False
   for i,j in zip(x,y):
      if better(i, j):
         return False
      if better(j, i):
         dominated = True
   return dominated

def dominates(x, y):
   return dominated(y, x)

# Usage:
# echo '1,0,1[;0,0,0...]' | ./domination.py [-h] -t {less,greater} -a {dominated,dominates} '0,1,0[;1,1,1...]'

# Reading the input (accepts either '1,0,0;0,0,0;1,1,1' or '1,0,0;0,0,0\n1,1,1', for instance)
tmp = [i.split(';') for i in sys.stdin.read().splitlines()]
points_dataset = []
for i in tmp:
   for j in i:
      if len(j) == 0: continue
      points_dataset.append([float(k) for k in j.split(',')])
#print points_dataset

parser = argparse.ArgumentParser()
parser.add_argument("-t", "--type", required=True, choices=['less','greater'], help="Comparison type: less or greater")
parser.add_argument("-a", "--action", required=True, choices=['dominated','dominates'], help="Action type: dominated or dominates")
parser.add_argument("point", help="the point to compare against the dataset of points; format: 'x1,...,xN'")
args = parser.parse_args()

if args.type=='less':
   better = less
elif args.type=='greater':
   better = greater

if len(args.point.split(';')) > 1:
   raise Exception("Only one point is accepted! For instance: domination.py '0,1,0'")
point = [float(i) for i in args.point.split(',')]

result = None
exit = 1 # Either the point does not dominate a single one or it isn't dominated by one of them
if args.action=='dominated':
   for y in points_dataset:
      result = dominated(point,y)
      if result: exit = 0
      print "Is", point, "dominated by", y, "? ->", result
elif args.action=='dominates':
   for y in points_dataset:
      result = dominates(point,y)
      if result: exit = 0
      print "Does", point, "dominate", y, "? ->", result

sys.exit(exit)
