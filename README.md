# PEE #

## Building ##

### Grammar assembling ###

~~~~~~~~
   cd build/
   cmake .. -DGRAMMAR=<path to the actual grammar> [-DLABEL=<label>] [-DPoco_DIR=<poco dir>] [-DCMAKE_BUILD_TYPE=<build type>]
~~~~~~~~

For instance:

~~~~~~~~
   cd build/
   cmake .. -DGRAMMAR=../problem/random/grammar.bnf -DLABEL=random -DPoco_DIR=/opt/poco -DCMAKE_BUILD_TYPE=RELEASE
~~~~~~~~


### Compilation ###

~~~~~~~~
   make
~~~~~~~~

## Running (example) ##

### Through run.py ###

Within `build/`:

~~~~~~~~
   while true; do  ../script/run.py -d ../problem/vento/wndm00_1s.txt -e ./vento -i ./islands.txt -p 9080 -n 4 -st 1000000 -cl-p 0 -cl-d 0 |grep -a -A 1 'Overall best' | tail -n 1 >> out; sleep 1; done
~~~~~~~~

### Generating pareto front ###

~~~~~~~~
   cat out | tr -s ';' ',' | sort -t',' -k1,1n -k2,2 -u > tmp.domination && cat tmp.domination | while read L; do cut -d',' -f1-2 tmp.domination | if ! ../script/domination-many.py -t less -a dominated $(echo $L|cut -d',' -f1-2) > /dev/null; then echo $L; fi; done
~~~~~~~~
