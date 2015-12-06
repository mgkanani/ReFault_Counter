#!/bin/bash
cp f f1
for i in {1..23}; 
do
cp f1 f2
cat f2>>f1
done
rm f2
mv f1 test1.txt
gcc Correctness.c
sudo sh -c 'echo 1 >/proc/sys/vm/drop_caches';
