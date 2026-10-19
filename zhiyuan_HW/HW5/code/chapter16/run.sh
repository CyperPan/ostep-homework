#!/bin/bash
# Reproduce every segmentation.py run used in the chapter 16 answers.
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../vm-segmentation" || exit 1
run() { echo; echo "\$ ./segmentation.py $* -c"; python3 segmentation.py "$@" -c | sed -n '/Segment register/,$p' | grep -v '^$'; }
X="python3 $here/seg_exp.py"

echo "===== Q1: translate addresses (seeds 0, 1, 2)"
for s in 0 1 2; do run -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -s $s; done

echo "===== Q2: boundaries of the tiny address space"
run -a 128 -p 512 -b 0 -l 20 -B 512 -L 20 -A 0,19,20,63,64,107,108,127

echo "===== Q3: valid, valid, violation ... violation, valid, valid"
run -a 16 -p 128 -A 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 --b0 0 --l0 2 --b1 128 --l1 2

echo "===== Q4: ~90% valid addresses (10 seeds x 1000 addresses)"
$X 10 1000 -a 1k -p 16k -b 0 -l 461 -B 16k -L 461
$X 10 1000 -a 1k -p 16k -b 8k -l 461 -B 4k -L 461
$X 10 1000 -a 1k -p 16k -b 0 -l 512 -B 16k -L 410
$X 10 1000 -a 128 -p 512 -b 0 -l 58 -B 512 -L 57
$X 10 1000 -a 1k -p 16k -b 0 -l 256 -B 16k -L 256
$X 10 1000 -a 1k -p 16k

echo "===== Q5: no valid addresses"
run -a 128 -p 512 -b 0 -l 0 -B 512 -L 0 -n 10 -s 3
$X 5 128 -a 128 -p 512 -b 0 -l 0 -B 512 -L 0
