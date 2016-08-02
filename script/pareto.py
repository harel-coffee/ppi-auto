#!/usr/bin/env python

import os, sys
import argparse
import time
import subprocess
import simpleflock

# Format: 'generation;size;error;solution;genome;command-line;timings;...'
#                    |_________|
FIELD_START=1
FIELD_END=3

def UpdatePareto(pareto, candidate):
   if len(pareto) == 0: # Empty pareto, obviously 'candidate' dominates the empty
      return True, [candidate]

   # Check if the candidate is dominated by any of the current members; if so, then bye bye...
   cmd = [os.path.dirname(os.path.realpath(__file__))+"/"+"domination-many.py", "-t", "less", "-a", "dominated", ','.join(candidate.split(';')[FIELD_START:FIELD_END])]
   for p in pareto:
      if p.split(';')[FIELD_START:FIELD_END] == candidate.split(';')[FIELD_START:FIELD_END]:
         return False, pareto # candidate is already a pareto front member

      domination = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
      output, error = domination.communicate("%s\n"%(','.join(p.split(';')[FIELD_START:FIELD_END])))
      if error is not None:
         print >> stderr, error
      if domination.returncode == 0: # candidate is dominated by someone
         return False, pareto

   # Hmmm, dominated by none of them, it deserves inclusion as a new member of the pareto front
   print "\n> New pareto front member: [%s]" % (';'.join(candidate.split(';')[FIELD_START:FIELD_END]))

   new_pareto = []
   # Now, check if one or more members will leave the pareto front because of the introduction of candidate
   cmd = [os.path.dirname(os.path.realpath(__file__))+"/"+"domination-many.py", "-t", "less", "-a", "dominates", ','.join(candidate.split(';')[FIELD_START:FIELD_END])]
   for p in pareto:
      domination = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
      output, error = domination.communicate("%s\n"%(','.join(p.split(';')[FIELD_START:FIELD_END])))
      if error is not None:
         print >> stderr, error
      if domination.returncode == 1: # candidate does not dominate p
         new_pareto.append(p)
      else:
         print "\n> Member [%s] has left the pareto front due to the domination of the new member [%s]" % (';'.join(p.split(';')[FIELD_START:FIELD_END]),';'.join(candidate.split(';')[FIELD_START:FIELD_END]))

   # Add the new non-dominated member at the end of the list
   new_pareto.append(candidate)

   return True, new_pareto

def main():
   parser = argparse.ArgumentParser()
   parser.add_argument("-p", "--pareto-file", required=True, help="Pareto file path")
   parser.add_argument("-l", "--lock-file", required=False, default="pareto.lock", help="Lock file path")
   parser.add_argument("candidate", help="Candidate")
   args = parser.parse_args()

   #sys.stdout.write("(Processing: [%s]...)" % (';'.join(args.candidate.split(';')[FIELD_START:FIELD_END])))

   old_pareto = []; new_pareto = []; changed = None

   # Create the pareto file if it doesn't exist:
   with open(args.pareto_file, 'a+') as pareto:
      pass

   with open(args.pareto_file) as pareto:
      old_pareto = pareto.read().splitlines()
   changed, new_pareto = UpdatePareto(old_pareto, args.candidate)

   #print old_pareto, new_pareto

#time.sleep(10)
   if changed:
      #print "New pareto front member: %s" % (args.candidate)
      cur_pareto = []
      with simpleflock.SimpleFlock(args.lock_file):
         #print "lock acquired!"
         with open(args.pareto_file, 'r+') as pareto:
            cur_pareto = pareto.read().splitlines()
            pareto.seek(0)
            if cur_pareto == old_pareto:
               #print "Equal"
               #print '\n'.join(new_pareto)
               pareto.truncate()
               pareto.write('\n'.join(new_pareto)+"\n")
            else: # Hmmm, someone else wrote to the pareto_file in the meantime, recalculating the pareto...
               #print "Different"
               changed, new_pareto = UpdatePareto(cur_pareto, args.candidate)
               if changed:
                  #print "New: ", new_pareto
                  pareto.truncate()
                  pareto.write('\n'.join(new_pareto)+"\n")
#
## Raises an IOError in 3 seconds if unable to acquire the lock.
#with simpleflock.SimpleFlock("/tmp/foolock", timeout = 3):
#   # Do something.
#   pass
main()
