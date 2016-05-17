#!/usr/bin/env python

import os
import sys
import random


nargs = len(sys.argv)
if nargs < 13:
   print 'run.py -i <inputfile> -is <islandfile> -port <port> -ns <numberofsendings> -cl-d <device> -cl-p <platform>' 
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
   elif str(sys.argv[i]) == "-cl-d":
      device = sys.argv[i+1]
   elif str(sys.argv[i]) == "-cl-p":
      platform = sys.argv[i+1]
   else:
      print 'run.py -i <inputfile> -is <islandfile> -port <port> -ns <numberofsendings> -cl-d <device> -cl-p <platform>' 
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


text = [];
text.append("./main -v -e -acc -strategy PPCU -d " + inputfile + " -ncol " + str(ncol) + " -port " + port + " -peers \"" + ''.join(peers) + "\"" + " -cl-d " + device + " -cl-p " + platform + " -ps " + str(2**random.randint(10,15)) + " -g " + str(2**random.randint(6,17)) + " -cp " + str(random.random()) + " -mr " + str(random.uniform(0,0.5)) + " -ts " + str(random.randint(3,15)) + " -nb " + str(random.randint(16,2000)) + " -is " + str(random.randint(3,8)))

print "\n" + ''.join(text) + "\n"
os.system("cd ../build; make;" + ''.join(text) + ";cd -;")

