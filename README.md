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

### Manually (one instance) ###

~~~~~~~~
   ./main  -v -e -acc -strategy PP -d ../problem/iris/data.csv -ncol 5 -port 9080 -cl-d 0 -cl-p 0 -ps 30000 -cp 0.6 -mr 0.01 -ts 5 -nb 2000  -error '((X)!=(Y))' -g 1000000 -cl-mls 16 -machine | grep -a '^> [0-9]' | cut -c3- | while read CANDIDATE; do ../script/pareto.py -p pareto.front -dup "$CANDIDATE"; done
~~~~~~~~

### Through run.py ###

Within `build/`:

~~~~~~~~
   while true; do ../script/run.py -d ../problem/random/data.csv -e ./main -i islands.txt -p 9080 -n 4 -st 5000000 -cl-p 0 -cl-d 0 | grep -a '^> [0-9]' | cut -c3- | while read CANDIDATE; do ../script/pareto.py -p pareto.front "$CANDIDATE"; done; sleep 1; done
~~~~~~~~

This will generate the Pareto front in file 'pareto.front'. Note that you have to run each instance separately (each one having its own port number (`-p <n>`) and ideally on a different accelerator (`-cl-p <p>`, `-cl-d <d>`); for example, you could start each instance on a separate 'screen' tab.

### Watching the Pareto front progress on-the-fly ###

~~~~~~~~
   watch -n 1 "cut -d';' -f2-4 pareto.front|sort -n"
~~~~~~~~
