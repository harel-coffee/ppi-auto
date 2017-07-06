#!/usr/bin/env python

import sys
import random
import argparse
import subprocess

################################################################################
# Usage:
################################################################################
#
# From the build directory (this is required due to the OpenCL kernels which
# are compiled at runtime):
#
# usage: run.py [-h] [-e EXE] -d DATASET [-i ISLANDS_FILE] -p PORT
#               [-n NUMBER_TARGET_ISLANDS] [-st STAGNATION_TOLERANCE]
#               [-cl-p CL_PLATFORM_ID] [-cl-d CL_DEVICE_ID]
#               [-s {PDP,pdp,PP,pp,DP,dp}]
#               ...
#
# positional arguments:
#   args                  Extra arguments to be passed to the executable; use
#                         after -- (ex: ... -- -min -1.0 -max 1.0)
#
# optional arguments:
#   -h, --help            show this help message and exit
#   -e EXE, --exe EXE     Executable filename [default=./main]
#   -d DATASET, --dataset DATASET
#                         Training dataset
#   -i ISLANDS_FILE, --islands-file ISLANDS_FILE
#                         Island file
#   -p PORT, --port PORT  Port number
#   -n NUMBER_TARGET_ISLANDS, --number-target-islands NUMBER_TARGET_ISLANDS
#                         Number of islands to send individuals [default=0]
#   -st STAGNATION_TOLERANCE, --stagnation-tolerance STAGNATION_TOLERANCE
#                         How many new individuals without improvement until the
#                         algorithm terminates [default=1000000]
#   -cl-p CL_PLATFORM_ID, --cl-platform-id CL_PLATFORM_ID
#                         OpenCL platform id [default=0]
#   -cl-d CL_DEVICE_ID, --cl-device-id CL_DEVICE_ID
#                         OpenCL device id [default=0]
#   -s {PDP,pdp,PP,pp,DP,dp}, --strategy {PDP,pdp,PP,pp,DP,dp}
#                         Parallelization strategy [default=PDP]
#   -seq, --sequential    Enables sequential mode instead of accelerated
#                         [default=false]
#   -t, --threads         Number of OpenMP threads for the evolutionary part
#                         [default=1]
#   -min, --min-constant  Minimum constant [default=-10]
#
#   -max, --max-constant  Maximum constant [default=10]
#
# Example (don't forget to change the port number for each instance):
#
#    cd build
#    ../script/run.py -d ../problem/random/data.csv -i islands.txt -p 9080 -n 3 -st 5000000
################################################################################


parser = argparse.ArgumentParser()
parser.add_argument("-e", "--exe", required=False, default="./main", help="Executable filename [default=./main]")
parser.add_argument("-d", "--dataset", required=True, help="Training dataset")
parser.add_argument("-i", "--islands-file", required=False, help="Island file")
parser.add_argument("-p", "--port", required=True, help="Port number")
parser.add_argument("-n", "--number-target-islands", type=int, default=0, help="Number of islands to send individuals [default=0]")
parser.add_argument("-st", "--stagnation-tolerance", type=int, default=1000000, help="How many new individuals without improvement until the algorithm terminates [default=1000000]")
parser.add_argument("-cl-p", "--cl-platform-id", required=False, default=0, help="OpenCL platform id [default=0]")
parser.add_argument("-cl-d", "--cl-device-id", required=False, default=0, help="OpenCL device id [default=0]")
parser.add_argument("-s", "--strategy", required=False, default="PDP", choices=['PDP', 'pdp', 'PP', 'pp', 'DP', 'dp'], help="Parallelization strategy [default=PDP]")
parser.add_argument("args", nargs=argparse.REMAINDER, help="Extra arguments to be passed to the executable; use after -- (ex: ... -- -min -1.0 -max 1.0)")
parser.add_argument('-seq', '--sequential', required=False, action='store_true', default=False, help="Sequential evaluation mode instead of accelerated [default=accelerated]")
parser.add_argument("-t", "--threads", type=int, default=1, help="Number of OpenMP threads for the evolutionary part [default=1]")
parser.add_argument("-min", "--min-constant", type=float, default=-10, help="Minimum constant [default=-10]")
parser.add_argument("-max", "--max-constant", type=float, default=10, help="Maximum constant [default=10]")

args = parser.parse_args()

if args.stagnation_tolerance and args.stagnation_tolerance < 500000:
   parser.error("Minimum stagnation tolerance (-st) is 500000")

if args.number_target_islands > 0:
   if args.islands_file is None:
      parser.error("-n is not zero (-n %d) but no islands file (-i) was given" % (args.number_target_islands))
   try:
      f = open(args.islands_file,"r")
   except IOError:
      print "Could not open file '" + args.islands_file + "'"
      sys.exit(1)
   lines = f.readlines()
   f.close()
   islands = []
   for i in lines:
      i = i.strip()
      if not i.startswith('#'):
         islands.append(i)
#else:
#   if not args.islands_file is None:
#      parser.error("Given islands file (-i %s) but the number of target islands (-n) is zero" % (args.islands_file))

i = 0; peers = [];
while i < args.number_target_islands and len(islands) > 0:
   address = random.choice(islands)
   if address != "localhost:"+args.port and address != "127.0.0.1:"+args.port and address != "0.0.0.0:"+args.port:
      peers.append(address + "," + str(random.random()))
      islands.remove(address)
      if i < args.number_target_islands-1:
         peers.append(";")
      i = i + 1
   else:
      islands.remove(address)

# Remove the trailing ';' that might be left in some cases
if peers and peers[-1] == ';':
   peers.pop()

##############################
##### Parameters' values #####
##############################
P = {}
P['ps']  = 2**random.randint(9,16)
P['st']  = args.stagnation_tolerance/P['ps'] # Transform population-based into individual-based (this is fairer when using different population sizes
P['cp']  = random.uniform(0.5,1.0)
P['mr']  = random.uniform(0.002,0.05)
P['ts']  = random.randint(2,35)
P['nb']  = random.randint(64,3000)
P['is']  = random.randint(1,1)
P['iat'] = random.uniform(0.0,0.8)
##############################

if args.sequential:
   acc = ""
else:
   acc = "-acc -strategy " + args.strategy.upper() + " -cl-d " + str(args.cl_device_id) + " -cl-p " + str(args.cl_platform_id)

cmd = [];
cmd.append(args.exe + " -v -machine -e " + acc + " -d " + args.dataset + " -port " + str(args.port) + " -ps " + str(P['ps']) + " -g 100000000" + " -cp " + str(P['cp']) + " -mr " + str(P['mr']) + " -ts " + str(P['ts']) + " -nb " + str(P['nb']) + " -is " + str(P['is']) + " -st " + str(P['st']) + " -iat " + str(P['iat']) + " -t " + str(args.threads) + " -min " + str(args.min_constant) + " -max " + str(args.max_constant))
if peers:
   cmd.append(" -peers " + ''.join(peers))
if args.args: # Adds extra arguments to the executable if any
   cmd.append(" " + ' '.join(args.args[1:]))

print "\n" + ''.join(cmd) + "\n"

try:
   process = subprocess.Popen(''.join(cmd).split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
   for out in iter(lambda: process.stdout.read(1), ''): # Outputs char by char
   #for out in iter(process.stdout.readline, ''): # Outputs line by line
      sys.stdout.write(out)
      sys.stdout.flush()
      sys.stderr.write(out)
   process.wait() # Wait for the completion and sets the returncode (if used communicate() this wouldn't be necessary)
   if process.returncode != 0:
      raise Exception("'%s (return code: %d)'" % (''.join(process.stderr.readlines()), process.returncode))
except Exception as e:
   print >> sys.stderr, "<> ", "The following error occurred when running the command ('%s'): %s" % (''.join(cmd), e)

sys.exit(0) # Always exit with SUCCESS!
