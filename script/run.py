#!/usr/bin/env python

import os
import sys
import random
import argparse

################################################################################
# Usage:
################################################################################
#
# From the build directory (this is required due to the OpenCL kernels which
# are compiled at runtime):
#
#     ./run.py [-h] [-e EXE] -d DATASET -i ISLANDS_FILE -p PORT -n
#               NUMBER_TARGET_ISLANDS -st STAGNATION_TOLERANCE
#               [-cl-p CL_PLATFORM] [-cl-d CL_DEVICE]
#               [-s {PPCU,ppcu,PP,pp,FP,fp}]
#
# Example (don't forget to change the port number for each instance):
#
#    cd build
#    ../script/run.py -d ../problem/random/data.csv -i islands.txt -p 9080 -n 3 -st 1000
################################################################################


parser = argparse.ArgumentParser()
parser.add_argument("-e", "--exe", required=False, default="./main", help="Executable filename [default=./main]")
parser.add_argument("-d", "--dataset", required=True, help="Training dataset")
parser.add_argument("-i", "--islands-file", required=True, help="Island file")
parser.add_argument("-p", "--port", required=True, help="Port number")
parser.add_argument("-n", "--number-target-islands", type=int, required=True, help="Number of islands to send individuals")
parser.add_argument("-st", "--stagnation-tolerance", type=int, required=True, help="Stagnation tolerance")
parser.add_argument("-cl-p", "--cl_platform", required=False, default=0, help="OpenCL platform id [default=0]")
parser.add_argument("-cl-d", "--cl_device", required=False, default=0, help="OpenCL device id [default=0]")
parser.add_argument("-s", "--strategy", required=False, default="PPCU", choices=['PPCU', 'ppcu', 'PP', 'pp', 'FP', 'fp'], help="Parallelization strategy")

args = parser.parse_args()

try:
   f = open(args.islands_file,"r")
except IOError:
   print "Could not open file '" + args.islands_file + "'"
lines = f.readlines()
f.close()
lines = [s.replace('\n', '') for s in lines]

i = 0; peers = [];
while i < args.number_target_islands and len(lines) > 0:
   address = random.choice(lines)
   if address != "localhost:"+args.port and address != "127.0.0.1:"+args.port and address != "0.0.0.0:"+args.port:
      peers.append(address + "," + str(random.random()))
      lines.remove(address)
      if i < args.number_target_islands-1:
         peers.append(";")
      i = i + 1
   else:
      lines.remove(address)

# Remove the trailing ';' that might be left in some cases
if peers and peers[-1] == ';':
   peers.pop()


try:
   f = open(args.dataset,"r")
except IOError:
   print "Could not open file '" + dataset + "'"
lines = f.readlines()
f.close()
ncol = len(lines[1].split(','))

text = [];
text.append(args.exe + " -v -e -acc -strategy " + args.strategy.upper() + " -d " + args.dataset + " -ncol " + str(ncol) + " -port " + str(args.port) + " -cl-d " + str(args.cl_device) + " -cl-p " + str(args.cl_platform) + " -ps " + str(2**random.randint(9,16)) + " -g 100000000" + " -cp " + str(random.random()) + " -mr " + str(random.uniform(0,0.1)) + " -ts " + str(random.randint(2,30)) + " -nb " + str(random.randint(800,2000)) + " -is " + str(random.randint(1,1)) + " -st " + str(args.stagnation_tolerance) + " -iat " + str(random.uniform(0.0,0.8)) )
if peers:
   text.append(" -peers \"" + ''.join(peers) + "\"")

print "\n" + ''.join(text) + "\n"
#os.system("cd build; make;" + ''.join(text) + ";cd -;")
os.system(''.join(text))
