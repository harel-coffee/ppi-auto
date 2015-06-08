#!/bin/bash

home="$(pwd)"

i=1 # 0 a 9 conjuntos de treinamento e teste

TMPTEST=$(mktemp);TMPTRAINING=$(mktemp)
arquivo=$(mktemp)

file=$home/data/A701.txt
$home/script/funcoeszz zzshuffle $file > $arquivo
# NF-1 pq não considera a coluna do TRMM
ncol=`awk -F',' '{print NF-1}' $arquivo | head -1`
natr=22; nmod=$(echo "scale=0; $ncol - $natr" | bc) # 22 colunas iniciais fixas

nlin=`cat $arquivo | wc -l`
nlin_teste=$(echo "scale=0; (0.1 * $nlin) * 100 / 100" | bc)
nlin_treinamento=$(echo "scale=0; $nlin - $nlin_teste" | bc)

inicio=$(echo "scale=0; $nlin_treinamento + 1 - $i * $nlin_teste" | bc)
fim=$(echo "scale=0; $inicio + $nlin_teste - 1" | bc)
if [[ $i -eq 9 && $inicio -gt 1 ]]; then inicio=1; fi
cat $arquivo | sed ''$inicio','$fim'!d' > $TMPTEST
inicio=$(echo "scale=0; $inicio - 1" | bc)
fim=$(echo "scale=0; $fim + 1" | bc)
:> $TMPTRAINING 
if [ $inicio -gt 0 ]; then
  cat $arquivo | sed '1,'$inicio'!d' >> $TMPTRAINING
fi
if [ $fim -lt $nlin ]; then 
  cat $arquivo | sed ''$fim','$nlin'!d' >> $TMPTRAINING
fi

$home/build/main -v -d $TMPTRAINING -ni $natr -nm $nmod
# 2> $home/solution
#cat $home/solution
#$home/build/main -d $TMPTRAINING -ni $natr -nm $nmod -f $home/solution
#$home/build/main -d $file -ni $natr -nm $nmod 2> $home/solution
#cat $home/solution
#$home/build/main -d $file -ni $natr -nm $nmod -f $home/solution

rm $TMPTEST $TMPTRAINING 
rm $arquivo
