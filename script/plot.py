#!/usr/bin/python


import numpy as np
import matplotlib.pyplot as plt
import subprocess
import sys
import argparse

from matplotlib.cbook import get_sample_data

parser = argparse.ArgumentParser()
parser.add_argument("-in", "--in-directory", required=True, help="Directory of files")
parser.add_argument("-out", "--out-name", required=True, help="Name of figures")
parser.add_argument("-d", "--dataset", required=True, help="Test Dataset")
parser.add_argument("-p", "--problems", required=True, help="Problem names")
parser.add_argument("-l", "--labels", required=True, help="Labels of problems")
args = parser.parse_args()

import os
os.chdir("/home/amanda/public/projetos/pee/build/")

problems = args.problems.split(';')
labels = args.labels.split(';')
colors = ['b','r','g']

for idx in range(len(problems)):

   try:
      f = open(args.in_directory + '/' + problems[idx] + '.front',"r")
   except IOError:
      print "Could not open file '" + args.in_directory + '/' + problems[idx] + '.front' + "'"
   lines = f.readlines()
   f.close()

   size      = np.empty(len(lines)); 
   tra_error = np.empty(len(lines))
   solution = [];

   for i in range(len(lines)):
       size[i] = lines[i].split(';')[1]
       tra_error[i] = lines[i].split(';')[2]
       solution.append(lines[i].split(';')[4])
   size = size.astype(int)

   try:
      f = open(args.dataset,"r")
   except IOError:
      print "Could not open file '" + args.dataset + "'"
   lines = f.readlines()
   f.close()

   ncol = len(lines[0].split(','))

   tes_error = np.empty(len(solution)); 
   for i in range(len(solution)):
       tes_error[i] = subprocess.check_output(["./" + problems[idx], "-d", args.dataset, "-ncol", str( ncol), "-sol", str(size[i]) + " " + str(solution[i].rstrip())])


   min_value = sys.float_info.max; max_value = 0.

   fig = plt.figure(figsize=(18,4))
   ax = fig.add_subplot(111)
   ax.set_title('Ensemble ' + str(labels[idx]) + ' forecast of wind intensity - Fortaleza', fontsize=14, fontweight='bold')
   ax.set_xlabel('Complexity (nodes)', fontweight='bold', fontsize=12)
   ax.set_ylabel('MSE (m/s)', fontweight='bold', fontsize=12)

   width = 0.4

   #value = tra_error[tes_error<1.e+30] - tes_error[tes_error<1.e+30]
   #value = (tra_error - tes_error) * 100 / tra_error

   vector = []; marks = []
   for i in range(1,max(size[tes_error<1.e+30])+1):
       if len(tra_error[(tes_error<1.e+30) & (size==i)]) > 0:
           marks.append(i)
           vector.append(np.mean(tra_error[(tes_error<1.e+30) & (size==i)]))
   ax.bar(range(1,len(marks)+1), vector, width, color='b', label='Training set')

   vector = []
   for i in range(1,max(size[tes_error<1.e+30])+1):
       if len(tes_error[(tes_error<1.e+30) & (size==i)]) > 0:
           vector.append(np.mean(tes_error[(tes_error<1.e+30) & (size==i)]))
    
   ax.bar([x + width for x in range(1,len(marks)+1)], vector, width, color='r', label='Test set')

   #pos = np.argwhere(vector > 0.)
   #ax.bar(pos+1,vector[pos], width, color='b')
   #pos = np.argwhere(vector < 0.)
   #ax.bar(pos+1,vector[pos], width, color='r')

   if min(tra_error) < min_value: min_value = min(tra_error)
   if max(tra_error) > max_value: max_value = max(tra_error)
   if min(tes_error) < min_value: min_value = min(tes_error)
   if max(tes_error[tes_error<1.e+30]) > max_value: max_value = max(tes_error[tes_error<1.e+30])
 
   ax.set_ylim(min_value-0.03*min_value,max_value+0.03*max_value)
   ax.set_xlim(marks[0]-3*width,len(marks)+3*width)
   ax.set_xticks([x + width for x in range(1,len(marks)+1)])  
   xtickNames = ax.set_xticklabels(marks)
   plt.setp(xtickNames, rotation=45, fontsize=8)

   ax.legend(loc='upper right', scatterpoints=1, ncol=1, fontsize=12)
   fig.savefig(args.out_name + '_improvement_' + str(labels[idx]) + '.pdf', dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
   #plt.show()



