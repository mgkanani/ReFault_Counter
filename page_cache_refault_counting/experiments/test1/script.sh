#!/bin/bash
for i in {1..4}; 
do
./a.out;
sync;
sudo sh -c 'echo 1 >/proc/sys/vm/drop_caches';
done
