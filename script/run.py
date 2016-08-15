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
parser.add_argument("-st", "--stagnation-tolerance", type=int, required=True, help="How many new individuals without improvement until the algorithm terminates")
parser.add_argument("-cl-p", "--cl-platform-id", required=False, default=0, help="OpenCL platform id [default=0]")
parser.add_argument("-cl-d", "--cl-device-id", required=False, default=0, help="OpenCL device id [default=0]")
parser.add_argument("-s", "--strategy", required=False, default="PPCU", choices=['PPCU', 'ppcu', 'PP', 'pp', 'FP', 'fp'], help="Parallelization strategy")
parser.add_argument("args", nargs=argparse.REMAINDER, help="Extra arguments to be passed to the executable; use after -- (ex: ... -- -error '((X)!=(Y))')")

args = parser.parse_args()

if args.stagnation_tolerance and args.stagnation_tolerance < 1000000:
   parser.error("Minimum stagnation tolerance (-st) is 1000000")

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


##############################
##### Parameters' values #####
##############################
P = {}
P['ps']  = 2**random.randint(9,16)
P['st']  = args.stagnation_tolerance/P['ps'] # Transform population-based into individual-based (this is fairer when using different population sizes
P['cp']  = random.uniform(0.5,1.0)
P['mr']  = random.uniform(0,0.1)
P['ts']  = random.randint(2,30)
P['nb']  = random.randint(200,4000)
P['is']  = random.randint(1,1)
P['iat'] = random.uniform(0.0,0.8)
##############################

cmd = [];
cmd.append(args.exe + " -v -machine -e -acc -strategy " + args.strategy.upper() + " -d " + args.dataset + " -ncol " + str(ncol) + " -port " + str(args.port) + " -cl-d " + str(args.cl_device_id) + " -cl-p " + str(args.cl_platform_id) + " -ps " + str(P['ps']) + " -g 100000000" + " -cp " + str(P['cp']) + " -mr " + str(P['mr']) + " -ts " + str(P['ts']) + " -nb " + str(P['nb']) + " -is " + str(P['is']) + " -st " + str(P['st']) + " -iat " + str(P['iat']))
if peers:
   cmd.append(" -peers " + ''.join(peers))
if args.args: # Adds extra arguments to the executable if any
   cmd.append(" " + ' '.join(args.args[1:]))

print "\n" + ''.join(cmd) + "\n"

try:
   process = subprocess.Popen(''.join(cmd).split(), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
   #for out in iter(lambda: process.stdout.read(1), ''): # Outputs char by char
   for out in iter(process.stdout.readline, ''): # Outputs line by line
      sys.stdout.write(out)
      sys.stdout.flush()
      sys.stderr.write(out)
   process.wait() # Wait for the completion and sets the returncode (if used communicate() this wouldn't be necessary)
   if process.returncode != 0:
      raise Exception("'%s (return code: %d)'" % (''.join(process.stderr.readlines()), process.returncode))
except Exception as e:
   print >> sys.stderr, "<> ", "The following error occurred when running the command ('%s'): %s" % (''.join(cmd), e)

sys.exit(0) # Always exit with SUCCESS!
