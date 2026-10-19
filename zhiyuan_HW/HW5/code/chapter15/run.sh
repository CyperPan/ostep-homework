#!/bin/bash
# Reproduce every relocation.py run used in the chapter 15 answers (Q5 takes ~1.5 minutes).
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../vm-mechanism" || exit 1
E="$here/reloc_exp.py"
run() { echo; echo "\$ ./relocation.py $* -c"; python3 relocation.py "$@" -c | sed -n '/Base-and-Bounds/,$p'; }

echo "===== Q1: seeds 1, 2, 3"
for s in 1 2 3; do run -s $s; done

echo "===== Q2: -s 0 -n 10"
run -s 0 -n 10
"$E" minlimit -s 0 -n 10
run -s 0 -n 10 -l 930

echo "===== Q3: -s 1 -n 10 -l 100"
run -s 1 -n 10 -l 100
"$E" maxbase -s 1 -n 10 -l 100
run -s 1 -n 10 -l 100 -b 16284

echo "===== Q4: larger address spaces and physical memories"
run -s 1 -n 10 -a 16k -p 64k
run -s 2 -n 10 -a 32m -p 1g
run -s 0 -n 10 -a 64k -p 32m -l 40k
"$E" minlimit -s 0 -n 10 -a 16k -p 64k
"$E" maxbase -s 1 -n 10 -a 16k -p 64k -l 4k
"$E" maxbase -s 1 -n 10 -a 32m -p 1g -l 10m

echo "===== Q5: fraction of valid addresses vs bounds"
"$E" q5                 # also writes valid_fraction.png next to this script
