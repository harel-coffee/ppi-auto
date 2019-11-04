#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt
import csv, argparse, sys

from sklearn.ensemble import RandomForestRegressor
from sklearn.ensemble import ExtraTreesRegressor

from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import ExtraTreesClassifier

from sklearn.feature_selection import SelectFromModel

parser = argparse.ArgumentParser()
parser.add_argument("-d", "--dataset", required=True, help="Training dataset")
parser.add_argument("-tes", "--test-dataset", required=False, help="Test dataset with the selected features")
parser.add_argument("-norm", "--norm", action='store_true', default=False, help="Normalize the dataset")
parser.add_argument("-hh", "--has-header", action='store_true', default=False, help="Has header")
parser.add_argument("-v", "--verbose-mode", action='store_true', default=False, help="Print the feature importances in descending order")
parser.add_argument("-regression", "--regression", action='store_true', default=False, help="Regression problem")
parser.add_argument("-classification", "--classification", action='store_true', default=False, help="Classification problem")
parser.add_argument("-n", "--number-trees", default='2000', help="Number of trees in the forest for features selection")
parser.add_argument("-cr", "--criterion-regression", default='mse', help="Function to measure the quality of a split for the regression problem [default=mse] [another option=mae]")
parser.add_argument("-cc", "--criterion-classification", default='gini', help="Function to measure the quality of a split for the classification problem [default=gini] [another option=entropy]")
parser.add_argument("-t", "--threshold", type=float, default=0.005, help="Threshold value [default=0.005]. Features with importance values above threshold will be retained")
parser.add_argument("-num", "--number-features", nargs="?", type=int, const=True, action='store', required=False, default=None, help="Number of features retained")
parser.add_argument("-per", "--percentage-features", type=float, default=None, help="Percentage of features retained")
parser.add_argument("-o", "--output-dataset", nargs="?", const=True, action='store', required=False, default=False, help="Output dataset with the selected features")
parser.add_argument("-outest", "--output-test", required=False, help="Output test dataset with the selected features")
parser.add_argument("-fig", "--output-figure", required=False, help="Output figure. Plot the feature importances of the forest")
args = parser.parse_args()

if args.has_header:
   skip_header=1
   header = []
   with open(args.dataset) as input:
      for hh in csv.reader(input):
         header = hh
         break
else:
   skip_header=0

nlin = np.genfromtxt(args.dataset, delimiter=',', skip_header=skip_header)

if args.test_dataset:
   T = np.genfromtxt(args.test_dataset, delimiter=',', skip_header=skip_header)

if args.norm:
   max_val = np.empty(len(nlin[0])); min_val = np.empty(len(nlin[0]))
   for i in range(len(nlin[0])):
      max_val[i] = max(nlin[:,i])
      min_val[i] = min(nlin[:,i])
   for i in range(len(nlin)):
      for j in range(len(nlin[0])):
         if max_val[j]-min_val[j] == 0.:
            nlin[i,j] = 0.
         else:
            nlin[i,j] = (nlin[i,j]-min_val[j]) / (max_val[j]-min_val[j]) 

X = nlin[:,:-1]; y = nlin[:,-1]

# Build a forest and compute the feature importances
if args.regression:
   #forest = RandomForestRegressor(n_estimators=int(args.number_trees), criterion=args.criterion_regression, bootstrap=True, n_jobs=-1, random_state=None)
   forest = ExtraTreesRegressor(n_estimators=int(args.number_trees), criterion=args.criterion_regression, bootstrap=False, n_jobs=-1, random_state=0) #, max_features=None
elif args.classification:
   #forest = RandomForestClassifier(n_estimators=int(args.number_trees), random_state=None)
   forest = ExtraTreesClassifier(n_estimators=int(args.number_trees), criterion=args.criterion_classification, bootstrap=False, n_jobs=-1, random_state=0)
else:
   print("Is it a classification or regression problem? Define it.", file=sys.stderr)
   sys.exit(1)

forest = forest.fit(X, y)

importances = forest.feature_importances_

if args.number_features is not None: # -num was given 
   if args.number_features is True: # -num was given but without argument
      model = SelectFromModel(forest, prefit=True)
      num = model.transform(X).shape[1]
   else:
      num = args.number_features

# For each tree, it calculates the importance of each feature; 
# then, it calculates the std of each feature over the number of trees (for example 2000 trees).
if args.output_figure:
   std = np.std([tree.feature_importances_ for tree in forest.estimators_], axis=0)
   xaxis = [] 

indices = np.argsort(importances)[::-1]

if args.test_dataset:
   if args.number_features is not None: # -num was given 
      T_new = np.empty([T.shape[0], num+1])
   elif args.percentage_features is not None:
      T_new = np.empty([T.shape[0], int((args.percentage_features/100.)*T.shape[1])+1])
   else:
      T_new = np.empty([T.shape[0], len(importances[importances > args.threshold])+1])
if args.output_dataset:
   if args.number_features is not None: # -num was given 
      X_new = np.empty([X.shape[0], num+1])
   elif args.percentage_features is not None:
      X_new = np.empty([X.shape[0], int((args.percentage_features/100.)*X.shape[1])+1])
   else:
      X_new = np.empty([X.shape[0], len(importances[importances > args.threshold])+1])

# Print the feature ranking
if args.verbose_mode:
   print("Feature ranking:"); 

hh = []; i = 0
for f in range(X.shape[1]):

   if args.verbose_mode:
      print(("%d. %s(%d): %f" % (f + 1, header[indices[f]], indices[f], importances[indices[f]])))

   if args.output_figure:
      xaxis.append(header[indices[f]])

   if args.number_features is not None: # -num was given 
      if f < num:
         if args.output_dataset:
            X_new[:,i] = X[:,indices[f]]
         if args.test_dataset:
            T_new[:,i] = T[:,indices[f]] 
         hh.append(header[indices[f]])
         i = i  + 1
   elif args.percentage_features is not None:
      if f < int((args.percentage_features/100.)*X.shape[1]):
         if args.output_dataset:
            X_new[:,i] = X[:,indices[f]]
         if args.test_dataset:
            T_new[:,i] = T[:,indices[f]] 
         hh.append(header[indices[f]])
         i = i  + 1
   else:
      if importances[f] >= args.threshold:
         if args.output_dataset:
            X_new[:,i] = X[:,f]
         if args.test_dataset:
            T_new[:,i] = T[:,f] 
         hh.append(header[f])
         i = i  + 1

if args.output_dataset:
   X_new[:,i] = y
if args.test_dataset:
   T_new[:,i] = T[:,-1]
hh.append(header[-1])

if args.number_features is not None: # -num was given 
   print("\n"+str(num)+"/"+str(X.shape[1])+" features: "+' '.join(str(i+1) for i in indices[:num]))
elif args.percentage_features is not None:
   print("\n"+str(int((args.percentage_features/100.)*X.shape[1]))+"/"+str(X.shape[1])+" features: "+' '.join(str(i+1) for i in indices[:int((args.percentage_features/100.)*X.shape[1])]))
else:
   print("\n"+str(len(importances[importances > args.threshold]))+"/"+str(X.shape[1])+" features: "+' '.join(str(i+1) for i in indices[:len(importances[importances > args.threshold])]))
print("features names: "+' '.join(i for i in hh[:-1]))

if args.output_dataset != False:
   if args.output_dataset == True: # -o was given but without argument, using default
      np.savetxt(args.dataset+'.out', X_new , fmt='%.2f', delimiter=',', header=','.join(hh), comments='')
      print("\nSaved to file '%s'" % (args.dataset+'.out'))
   else:
      np.savetxt(args.output_dataset, X_new , fmt='%.2f', delimiter=',', header=','.join(hh), comments='')
      print("\nSaved to file '%s'" % (args.output_dataset))

if args.test_dataset:
   if args.output_test:
      np.savetxt(args.output_test, T_new , fmt='%.2f', delimiter=',', header=','.join(hh), comments='')
      print("\nSaved to file '%s'" % (args.output_test))
   else:
      np.savetxt(args.test_dataset+'.out', T_new , fmt='%.2f', delimiter=',', header=','.join(hh), comments='')
      print("\nSaved to file '%s'" % (args.test_dataset+'.out'))

# Plot the feature importances of the forest
if args.output_figure:
   fig = plt.figure(figsize=(16,8))
   plt.title("Feature importances", fontsize=14, fontweight='bold')
   plt.tick_params(axis='x', which='major', labelsize=5)
   plt.tick_params(axis='y', which='major', labelsize=10)

   if args.number_features is not None: # -num was given 
      pass
   elif args.percentage_features is not None:
      num = int((args.percentage_features/100.)*X.shape[1])
   else:
      num = len(importances[importances > args.threshold])

   #plt.bar(range(num), importances[indices[:num]], color="r", yerr=std[indices[:num]], align="center")
   plt.bar(list(range(num)), importances[indices[:num]], color="r", align="center")
   plt.xlim([-1, num])
   plt.xticks(list(range(num)), xaxis[:num], rotation=90)

   ticklabels = [t for t in plt.gca().get_xticklabels()]
   if args.number_features is not None: # -num was given 
      for i in range(num):
         ticklabels[i].set_fontweight("bold")
   elif args.percentage_features is not None:
      for i in range(int((args.percentage_features/100.)*X.shape[1])):
         ticklabels[i].set_fontweight("bold")
   else:
      for i in range(len(importances[importances > args.threshold])):
         ticklabels[i].set_fontweight("bold")
   #plt.show()
   
   fig.savefig(args.output_figure, dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
   print("\nPlotted to file '%s'" % (args.output_figure))
   plt.close(fig)
