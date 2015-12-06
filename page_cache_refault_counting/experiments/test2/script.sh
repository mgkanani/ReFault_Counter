#!/bin/bash
for i in {1..10}; 
do
sync;
./a.out;
sync;
#sudo sh -c 'echo 1 >/proc/sys/vm/drop_caches';
sleep 15;
done
