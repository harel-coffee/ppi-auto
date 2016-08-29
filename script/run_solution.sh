#!/bin/ksh
 
home="$(pwd)"

if [ $# -le 0 ]; then
   echo "Usage 'script/run_solution.sh PROBLEM_NAME'"
fi

problem=$1

TMP0=$(mktemp)

cat $home/problem/vento/"$problem".front | sed 's/ ;/;/g' | sed 's/; /;/g' | cut -d';' -f2,5 | sed 's/;/ /g' > $TMP0

rm -f $home/problem/vento/"$problem".err

nsol=`cat $TMP0 | wc -l`
for sol in $(seq 1 $nsol); do #solution

   solution=`cat $TMP0 | sed -n ''$sol'p'`

   cd $home/build
   ncol=`cat $home/problem/vento/"$problem".tes | awk -F',' '{print NF}'`
   tes_error=`./$problem -d $home/problem/vento/"$problem".tes -ncol $ncol -sol "$solution"`

   tra_error=`cat $home/problem/vento/"$problem".front | sed -n ''$sol'p' | cut -d';' -f3`
   size=`cat $home/problem/vento/"$problem".front | sed -n ''$sol'p' | cut -d';' -f2`
   echo $size","$tra_error","$tes_error >> $home/problem/vento/"$problem".err

done
 
rm $TMP0


