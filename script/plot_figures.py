#!/usr/bin/python


import numpy as np
import matplotlib.pyplot as plt
import subprocess
import argparse

################################################################################
# Usage:
################################################################################
#
# From the principal directory (don't forget to run the script 
# 'script/run_solution.sh PROBLEM_NAME' to generate the file
# 'IN_DIRECTORY/PROBLEM_NAME.err'):
#
#     script/plot_figures.py -in IN_DIRECTORY -out OUT_DIRECTORY 
#                            -p PROBLEM_NAME -f FORECAST_RANGE 
#                            -c CITY_NAME
#
# Example:
#
#    python script/plot_figures.py -in problem/vento -out ../vento/figs -p speed_A305_3 -f 72 -c Fortaleza
################################################################################


parser = argparse.ArgumentParser()
parser.add_argument("-in", "--in-directory", required=True, help="Directory of files")
parser.add_argument("-out", "--out-directory", required=True, help="Directory of figures")
parser.add_argument("-p", "--problem", required=True, help="Problem name")
parser.add_argument("-f", "--forecast-range", required=True, help="Forecast range")
parser.add_argument("-c", "--city", required=True, help="City name")
args = parser.parse_args()


try:
   f = open(args.in_directory + '/' + args.problem + '.front',"r")
except IOError:
   print "Could not open file '" + args.in_directory + '/' + args.problem + '.front' + "'"
lines = f.readlines()
f.close()


size = []; error = []; solution = []
for i in range(len(lines)):
    size.append(lines[i].split(';')[1])
    error.append(lines[i].split(';')[2])
    solution.append(lines[i].split(';')[4])

size = map(int,size)
error = map(float,error)
    

fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_title('Pareto Front - ' + args.city + ' \n Ensemble ' + args.forecast_range + '-hour forecast of wind speed', fontsize=14, fontweight='bold')
ax.set_xlabel('size (nodes)', fontweight='bold')
ax.set_ylabel('MAE (m/s)', fontweight='bold')

ax.scatter(size, error)
ax.axis([0, int(max(size)+0.02*max(size)), min(error)-0.05*min(error), max(error)+0.05*max(error)])
fig.savefig(args.out_directory + '/' + args.problem + '_pareto-front.pdf', dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
#plt.show()


try:
   f = open(args.in_directory + '/' + args.problem + '.tes',"r")
except IOError:
   print "Could not open file '" + args.in_directory + '/' + args.problem + '.tes' + "'"
nlin = f.readlines()
f.close()


ncol = len(nlin[0].split(','))


obs = np.empty(len(nlin))
for i in range(len(nlin)):
    obs[i] = nlin[i].split(',')[-1]


result = np.empty(len(nlin))
#for i in range(1):
for i in range(len(solution)):
    partial = subprocess.check_output(["build/" + args.problem, "-d", args.in_directory + '/' + args.problem + '.tes', "-ncol", str(ncol), "-sol", str(size[i] ) + " " + str(solution[i]), "-pred"])
    #error = subprocess.check_output(["../build/speed_A305_1", "-d", "../problem/vento/speed_A305_1.tes", "-ncol", "36", "-sol", str(size[i] ) + " " + str(solution[i])])
    result += abs(map(float,partial[:].split('\n')[:-1]) - obs)

    
fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_title('Ensemble ' + args.forecast_range + '-hour forecast of wind speed - ' + args.city, fontsize=14, fontweight='bold')
ax.set_xlabel('Test Days', fontweight='bold')
ax.set_ylabel('MAE (m/s)', fontweight='bold')

ax.scatter(range(1,len(nlin)+1), result/len(solution))
ax.axis([0,len(nlin)+2, 0, max(result/len(solution))+0.05*max(result/len(solution))])
fig.savefig(args.out_directory + '/' + args.problem + '_error-time.pdf', dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
#plt.show()


#######################################################################################################################

size = np.genfromtxt(args.in_directory + '/' + args.problem + '.err', delimiter=',', dtype=None, usecols=(0))
tra_error = np.genfromtxt(args.in_directory + '/' + args.problem + '.err', delimiter=',', dtype=None, usecols=(1))
tes_error = np.genfromtxt(args.in_directory + '/' + args.problem + '.err', delimiter=',', dtype=None, usecols=(2))

tra_min = min(tra_error); tra_max = max(tra_error)
tes_min = min(tes_error); tes_max = max(tes_error)
min_value = min(tra_min, tes_min)
max_value = max(tra_max, tes_max)

fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_title('Ensemble ' + args.forecast_range + '-hour forecast of wind speed - ' + args.city, fontsize=14, fontweight='bold')
ax.set_xlabel('Test MAE (m/s)', fontweight='bold')
ax.set_ylabel('Training MAE (m/s)', fontweight='bold')


ax.scatter(tes_error, tra_error)
ax.axis([min_value-0.05*min_value, max_value+0.05*max_value, min_value-0.05*min_value, max_value+0.05*max_value])
ax.plot([0, 1], [0, 1], transform=ax.transAxes, ls="--")
fig.savefig(args.out_directory + '/' + args.problem + '_errors.pdf', dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
#plt.show()


porc = (tra_error - tes_error) * 100 / tra_error

vector = np.empty([max(size)])
for i in range(1,max(size)+1):
    #print len(porc[size==i]), np.mean(porc[size==i])
    if len(porc[size==i]) > 0:
        vector[i-1] = np.mean(porc[size==i])
    else:
        vector[i-1] = 0.
 

fig = plt.figure()
ax = fig.add_subplot(111)
ax.set_title('Ensemble ' + args.forecast_range + '-hour forecast of wind speed - ' + args.city, fontsize=14, fontweight='bold')
ax.set_xlabel('size (nodes)', fontweight='bold')
ax.set_ylabel('Improvement (%)', fontweight='bold')


#ax.bar(range(1,max(size)+1),vector[idx])
idx = np.argwhere(vector > 0.)
ax.bar(idx+1,vector[idx],color='b')
idx = np.argwhere(vector < 0.)
ax.bar(idx+1,vector[idx],color='r')
ax.axis([0, int(max(size)+0.02*max(size)), min(0,min(vector))-0.05*min(0,min(vector)), max(vector)+0.05*max(vector)])
fig.savefig(args.out_directory + '/' + args.problem + '_improvement.pdf', dpi=300, facecolor='w', edgecolor='w', orientation='portrait', papertype='letter', bbox_inches='tight')
#plt.show()



