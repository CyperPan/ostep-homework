#!/bin/bash
# Reproduce every fork.py run used in the chapter 5 simulation answers.
cd "$(dirname "$0")/../../../../cpu-api" || exit 1
run() { echo; echo "\$ ./fork.py $*"; python3 fork.py "$@" | sed -n '/Process Tree:/,$p'; }

echo "===== Q1"; run -s 10 -c
echo "===== Q2"
for f in 0.1 0.3 0.5 0.7 0.9; do
    echo; echo "\$ ./fork.py -s 1 -a 100 -f $f -F -c"
    python3 fork.py -s 1 -a 100 -f $f -F -c | sed -n '/Final Process Tree:/,$p'
done
echo "===== Q3"; run -s 5 -t -c
echo "===== Q4"; run -A a+b,b+c,c+d,c+e,c- -c; run -A a+b,b+c,c+d,c+e,c- -R -c
echo "===== Q5"; for s in 3 4 2; do run -s $s -a 8 -F -c; done
echo "===== Q6"; for s in 5 1; do run -s $s -t -F -c; done
