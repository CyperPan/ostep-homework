#!/bin/bash
# Reruns every x86.py command used in the chapter 26 write-up.
# Usage: ./run.sh          (from any directory)
cd "$(dirname "$0")/../../../../threads-intro" || exit 1
X="python3 ./x86.py"

run() { echo; echo "\$ ./x86.py $*"; $X "$@"; }

echo "===== Q1"; run -p loop.s -t 1 -i 100 -R dx -c
echo "===== Q2"; run -p loop.s -t 2 -i 100 -a dx=3,dx=3 -R dx -c
echo "===== Q3"
for s in 0 1 2; do run -p loop.s -t 2 -i 3 -r -R dx -a dx=3,dx=3 -s $s -c; done
echo "===== Q4"; run -p looping-race-nolock.s -t 1 -M 2000 -c
echo "===== Q5"; run -p looping-race-nolock.s -t 2 -a bx=3 -M 2000 -c
echo "===== Q6"
for s in 0 1 2; do run -p looping-race-nolock.s -t 2 -M 2000 -i 4 -r -s $s -c; done
echo "===== Q6 sweep: final value for seeds 0..19, and the instructions each thread ran before its first interrupt"
for s in $(seq 0 19); do
    out=$($X -p looping-race-nolock.s -t 2 -M 2000 -i 4 -r -s $s -c)
    v=$(echo "$out" | tail -1 | awk '{print $1}')
    first=$(echo "$out" | awk '/Interrupt/{print n; exit} /^ +[0-9]+ +[0-9]+ /{n++}')
    echo "seed $s: value=$v, thread 0 ran $first instructions before the first interrupt"
done
echo "===== Q7"
for i in 1 2 3; do run -p looping-race-nolock.s -a bx=1 -t 2 -M 2000 -i $i -c; done
echo "===== Q7/Q8 sweep: final value at 2000 for -i 1..40 (expected 2*bx)"
for bx in 1 100; do
    printf "bx=%-4s" $bx
    for i in $(seq 1 40); do
        v=$($X -p looping-race-nolock.s -a bx=$bx -t 2 -M 2000 -i $i -c | tail -1 | awk '{print $1}')
        printf " %s:%s" $i $v
    done
    echo
done
echo "===== Q9"; run -p wait-for-me.s -a ax=1,ax=0 -R ax -M 2000 -c
echo "===== Q10"; run -p wait-for-me.s -a ax=0,ax=1 -R ax -M 2000 -c
echo "===== Q10 with other interrupt intervals (instructions executed)"
for i in 50 100 1000; do
    n=$($X -p wait-for-me.s -a ax=0,ax=1 -R ax -M 2000 -i $i -c | grep -c -E '^ +[0-9]+ +[0-9-]+ +[0-9]+ ')
    echo "-i $i: $n trace lines"
done
