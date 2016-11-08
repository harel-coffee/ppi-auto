#!/bin/sh

EXEDIR="`dirname "$(readlink -f $0)"`"

OUTPUT="merged.front"

DUPLICATE=""
FILES=""
while [ $# -ge 1 ]; do
   case $1 in
      -dup) DUPLICATE="-dup";;
      -p)   shift; OUTPUT="$1";;
      -h)   { echo "Usage: pareto-merge.sh [-dup] [-p <merged pareto front>] [-h] <pareto1.front> [pareto2.front ... paretoN.front]"; exit 0; };;
      *)    FILES="${FILES}$1\n";;
   esac
   shift
done

/usr/bin/printf  "${FILES}" | while read FILE
do
   cat "$FILE" | while read CANDIDATE
   do
      "${EXEDIR}/"pareto.py -p "${OUTPUT}" ${DUPLICATE} "$CANDIDATE"
   done
done
