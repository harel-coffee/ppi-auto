#!/usr/bin/env python

#Running: 
#          python script/run.py -i problem/random/data.csv -is script/islandfile.txt -port 8080 -ns 4 -st 100 -cl-d 0 -cl-p 0
#

import os
import sys
import random


nargs = len(sys.argv)
if nargs < 13:
   print 'run.py -i <inputfile> -is <islandfile> -port <port> -ns <numberofsendings> -st <stagnation> -cl-d <device> -cl-p <platform>' 
   exit();

i = 1;
while i < nargs:
   if str(sys.argv[i]) == "-i":
      inputfile = sys.argv[i+1]
   elif str(sys.argv[i]) == "-is":
      islandfile = sys.argv[i+1]
   elif str(sys.argv[i]) == "-port":
      port = sys.argv[i+1]
   elif str(sys.argv[i]) == "-ns":
      nsend = sys.argv[i+1]
   elif str(sys.argv[i]) == "-st":
      stagnation = sys.argv[i+1]
   elif str(sys.argv[i]) == "-cl-d":
      device = sys.argv[i+1]
   elif str(sys.argv[i]) == "-cl-p":
      platform = sys.argv[i+1]
   else:
      print 'run.py -i <inputfile> -is <islandfile> -port <port> -ns <numberofsendings> -st <stagnation> -cl-d <device> -cl-p <platform>' 
      break
   i = i + 2


try:
   f = open(islandfile,"r")
except IOError:
   print "Could not open file " + islandfile + "!"
lines = f.readlines()
f.close()
lines = [s.replace('\n', '') for s in lines]

i = 0; peers = [];
while i < int(nsend):
   address = random.choice(lines)
   if address.split(':')[1] != port:
      peers.append(address + "," + str(random.random()))
      if i < int(nsend)-1:
         peers.append(";")
      i = i + 1


try:
   f = open(inputfile,"r")
except IOError:
   print "Could not open file " + inputfile + "!"
lines = f.readlines()
f.close()
ncol = len(lines[1].split(','))


# TODO: aproveitar texto no caso sem peers
text = [];
if peers:
   text.append("./main -v -e -acc -strategy PPCU -d ../" + inputfile + " -ncol " + str(ncol) + " -port " + port + " -peers \"" + ''.join(peers) + "\"" + " -cl-d " + device + " -cl-p " + platform + " -ps " + str(2**random.randint(10,17)) + " -g 100000000" + " -cp " + str(random.random()) + " -mr " + str(random.uniform(0,0.1)) + " -ts " + str(random.randint(2,30)) + " -nb " + str(random.randint(500,3000)) + " -is " + str(random.randint(3,8)) + " -st " + stagnation)
else:
   text.append("./main -e -acc -strategy PPCU -d ../" + inputfile + " -ncol " + str(ncol) + " -port " + port + " -cl-d " + device + " -cl-p " + platform + " -ps " + str(2**random.randint(10,15)) + " -g " + str(2**random.randint(6,17)) + " -cp " + str(random.random()) + " -mr " + str(random.uniform(0,0.5)) + " -ts " + str(random.randint(3,15)) + " -nb " + str(random.randint(100,2000)) + " -is " + str(random.randint(3,8)) + " -st " + stagnation)

print "\n" + ''.join(text) + "\n"
os.system("cd build; make;" + ''.join(text) + ";cd -;")

