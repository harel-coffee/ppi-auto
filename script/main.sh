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

#$home/build/main -v -e -d $TMPTRAINING -ni $natr -nm $nmod
#$home/build/main -d $TMPTRAINING -ni $natr -nm $nmod -run $home/solution

cd $home/build; ./main -port 32768 -peers "localhost:8080,0.3;sabatini@lncc.br:10524,1.0" -d $file -ni $natr -nm $nmod -ps 10000 -g 5; cd -
#cd $home/build; ./main -acc -type CPU -strategy PP -d $file -ni $natr -nm $nmod -ps 1000 -g 50; cd -
#cd $home/build; ./main -acc -type GPU -strategy PP -d $file -ni $natr -nm $nmod -ps 1000 -g 50; cd -
#cd $home/build; ./main -acc -type GPU -strategy FP -d $file -ni $natr -nm $nmod -ps 1000 -g 50; cd -
#cd $home/build; ./main -acc -type GPU -strategy PPCU -d $file -ni $natr -nm $nmod -ps 1000 -g 50; cd -

#cd $home/build; ./main -d $file -ni $natr -nm $nmod -ps 1000 -g 50 1> $home/solution; cd -
#cd $home/build; ./main -acc -type GPU -strategy PPCU -d $file -ni $natr -nm $nmod -p 1000 -g 50 1> $home/solution; cd -
#cd $home/build; ./main -acc -type GPU -strategy PPCU -d $file -ni $natr -nm $nmod -ps 1000 -g 50 1> $home/solution; cd -
#more solution
#cd $home/build; ./main -v -d $file -ni $natr -nm $nmod p 1000 -g 10 -s 1; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution -acc -strategy PPCU; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution -acc -strategy FP; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution -acc -strategy PP; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution; cd -
#cd $home/build; ./main -d $file -pred -ni $natr -nm $nmod -run $home/solution -acc -strategy PPCU; cd -
#cd $home/build; ./main -d $file -ni $natr -nm $nmod -run $home/solution -acc -platform-id 1 -device-id 0; cd -

rm $TMPTEST $TMPTRAINING 
rm $arquivo
