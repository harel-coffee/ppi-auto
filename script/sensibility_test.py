#!/usr/bin/env python
##!/usr/bin/python

from pylab import *

import random
import numpy as np

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


#random.seed(100)

alpha = 0.5; beta = 5 

rainNames = ["BMA", "CHUVA_ONTEM", "CHUVA_ANTEONTEM", "CHUVA_LAG1P", "CHUVA_LAG1N", "CHUVA_LAG2P", "CHUVA_LAG2N", "CHUVA_LAG3P", "CHUVA_LAG3N", "CHUVA_PADRAO", "CHUVA_HISTORICA", "MEAN_MODELO", "MAX_MODELO", "MIN_MODELO", "STD_MODELO", 'ETAm2', 'ETAm3', 'ETAm1', 'GP213', 'ETAm4', 'RAMSC', 'SFAVN', 'CPTEC', 'RPSAS', 'ETA20', 'ETAcr', 'SFENM', 'T299x', 'ACOPL']
attrNames = ["BMA", "CHUVA_ONTEM", "CHUVA_ANTEONTEM", "CHUVA_LAG1P", "CHUVA_LAG1N", "CHUVA_LAG2P", "CHUVA_LAG2N", "CHUVA_LAG3P", "CHUVA_LAG3N", "CHUVA_PADRAO", "CHUVA_HISTORICA", "MEAN_MODELO", "MAX_MODELO", "MIN_MODELO", "STD_MODELO", "CHOVE", "PADRAO_MUDA", "PAD", "K", "TT", "SWEAT", 'ETAm2', 'ETAm3', 'ETAm1', 'GP213', 'ETAm4', 'RAMSC', 'SFAVN', 'CPTEC', 'RPSAS', 'ETA20', 'ETAcr', 'SFENM', 'T299x', 'ACOPL'] 

prev = "1"
grammar = "nlinear"
diretorio = "saida/prev_" + prev + "/" + grammar + "/"
name = "stations_sorted.txt"
#name = "stations_" + grammar + ".txt"

print prev, grammar

try:
   stations = np.genfromtxt(diretorio + name, dtype=None, usecols=(0))
except IOError:
   print "Could not open file " + diretorio + name + "!"

partial = []; temp = []; result = []; none = {}; freq = {}
for i in range(0, len(attrNames)):
   partial.append([])
   temp.append([])
   result.append([])
   none[i] = 0
   freq[i] = 0

#soma = 0

#nstation = 1
nstation = len(stations) 

for station in stations[0:nstation]:
   #station = '83630'
   #for run in range(6,7):
   for run in range(0,10):

      try:
         f = open(diretorio + "modificada/" + str(station) + '.' + str(run),"r")
      except IOError:
         print "Could not open file " + diretorio + "modificada/" + str(station) + '.' + str(run) + "!"
         continue
      lines = f.readlines()
      f.close()
      
      try:
         exp = list(reversed(lines[1].split()))[2:-1]; #print exp
      except:
         print str(station) + '.' + str(run)
         continue
      #exp = ['0.1', 'BMA', '*', '0.5', 'CHUVA_ONTEM', '*', '+']

      for scenario in range(0,100):
         attr = {}; attr = {a : random.gammavariate(alpha,beta) for a in rainNames}
         attr["CHOVE"] = random.randint(0,1)
         attr["PADRAO_MUDA"] = random.randint(0,1)
         attr["PAD"] = random.randint(0,4)
         attr["K"] = np.random.uniform(0,30)
         attr["TT"] = np.random.uniform(0,60)
         attr["SWEAT"] = np.random.uniform(0,600)
   
         #soma = attr["CHOVE"] + soma

         value1 = interpreter(exp, attr)

         if value1 is None:
            for i in range(0,len(attrNames)):
               if attrNames[i] == "PAD": 
                  niter = 4
               elif attrNames[i] == "CHOVE" or attrNames[i] == "PADRAO_MUDA": 
                  niter = 2
               else: niter = 30
               none[i] = none[i] + niter
            
         if value1 is not None:
            for i in range(0,len(attrNames)):
               if attrNames[i] in exp:
                  copy = dict.copy(attr)
            
                  if attrNames[i] == "PAD": 
                     niter = 4
                     copy["PAD"] = 0.0
                  elif attrNames[i] == "CHOVE" or attrNames[i] == "PADRAO_MUDA":
                     niter = 2
                     copy[attrNames[i]] = -1.0
                  else: niter = 30 
            
                  for j in range(0,niter):
            
                     #while True:
                     if attrNames[i] == "CHOVE" or attrNames[i] == "PADRAO_MUDA" or attrNames[i] == "PAD": copy[attrNames[i]] = copy[attrNames[i]] + 1.0
                     elif attrNames[i] == "K": copy["K"] = np.random.uniform(0,30)
                     elif attrNames[i] == "TT": copy["TT"] = np.random.uniform(0,60)
                     elif attrNames[i] == "SWEAT": copy["SWEAT"] = np.random.uniform(0,600)
                     else: copy[attrNames[i]] = random.gammavariate(alpha,beta)

                     #if abs(copy[attrNames[i]] - attr[attrNames[i]]) >= (0.1 * attr[attrNames[i]]):          
                        #break

                     value2 = interpreter(exp, copy)
                     if value2 is not None:
                        partial[i].append(abs(value2 - value1))
                     else:
                        none[i] = none[i] + 1

                  if partial[i]:
                     #print attrNames[i]
                     temp[i].append(median(partial[i]))
                     del partial[i][:]

      for i in range(0,len(attrNames)):
         if temp[i]:
         #if attrNames[i] in exp:
            if mean(temp[i]) > 0.0:
            #if sum(j > 0.0 for j in partial[i]) > 0:
               result[i].append(mean(temp[i]))
               freq[i] = freq[i] + 1
               #print station, run, attrNames[i]
               #print attrNames[i], len(temp[i])
            del temp[i][:]

      #print

#print soma

saida = []
for i in range(0,len(attrNames)):
   saida.append([])

soma = 0.0
for i in range(0,len(attrNames)):
   soma = median(result[i]) + soma

for i in range(0,len(attrNames)):
   if result[i]:
      saida[i].append((median(result[i]) / soma) * 100)
      saida[i].append((freq[i] / (nstation * 10.) * 100))
      saida[i].append(len(result[i]))
      print attrNames[i], saida[i], none[i]
