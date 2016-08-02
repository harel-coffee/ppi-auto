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
   while true; do ../script/run.py -d ../problem/random/data.csv -e ./main -i islands.txt -p 9080 -n 4 -st 5000000 -cl-p 0 -cl-d 0 | grep -a '^> [0-9]' | cut -c3- | while read CANDIDATE; do ../script/pareto.py -p pareto.front "$CANDIDATE"; done; sleep 1; done
~~~~~~~~

This will generate the Pareto front in file 'pareto.front'
