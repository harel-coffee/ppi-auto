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

The `-dup` option allows duplicate candidates (w.r.t. complexity and error) into the Pareto file, provided that they differ in terms of the symbolic solution (model). Since the actual models differ, strictly speaking one doesn't dominate the other, thus using `-dup` option doesn't violate the principle of non-dominance.

### Through run.py ###

Within `build/`:

~~~~~~~~
   while true; do ../script/run.py -d ../problem/random/data.csv -e ./main -i islands.txt -p 9080 -n 4 -st 5000000 -cl-p 0 -cl-d 0 | grep -a '^> [0-9]' | cut -c3- | while read CANDIDATE; do ../script/pareto.py -p pareto.front -dup "$CANDIDATE"; done; sleep 1; done
~~~~~~~~

This will generate the Pareto front in file 'pareto.front'. Note that you have to run each instance separately (each one having its own port number (`-p <n>`) and ideally on a different accelerator (`-cl-p <p>`, `-cl-d <d>`); for example, you could start each instance on a separate 'screen' tab.

It is possible to pass parameters directly to the PEE's process; in order to do that, use the following convention:

~~~~~~~~
   ... ../script/run.py ... -- <parameters to be passed directly to PEE>
~~~~~~~~

For instance:

~~~~~~~~
   ... ../script/run.py ... -- -error '((X)!=(Y))'
~~~~~~~~

### Using the Pareto front as a repository ###

In this mode, the solutions contained in the Pareto file (actually, their genomes) are reintroduced into new running instances, aiming at the injection of diversity into those instances and, ultimately, improving the Pareto front. The script named `seeder.py` serves this purpose; for example:

~~~~~~~~
   while true; do ../script/seeder.py -i pareto.front -p 9080 -p 9081 -p 9082 -p 9083 -p 9084 -p 9085; sleep 10; done
~~~~~~~~

will periodically (each 10s) send a random genome from the pareto.front file ("Hall of Fame") to each one of the given running instances (`localhost:9080`, ...).


### Watching the Pareto front progress on-the-fly ###

~~~~~~~~
   watch -n 1 "cut -d';' -f2-4 pareto.front|sort -n"
~~~~~~~~
