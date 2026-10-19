#!/bin/bash
# Reproduce every multi.py run used in the chapter 10 answers.
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../cpu-sched-multi" || exit 1

# run the simulator, dropping the ARG header lines
sim() { echo; echo "\$ ./multi.py $*"; python3 multi.py "$@" | sed -n '/^Job name/,$p' | grep -v '^$'; }
# just the finish time (and per-CPU stats) of one run
fin() { printf '%-60s %s\n' "$*" "$(python3 multi.py "$@" 2>&1 | grep -E 'Finished|Error' | tr -d '\n')"; }

echo "===== Q1: one job, one CPU, working set 200 > cache 100"
sim -n 1 -L a:30:200 -c -t

echo "===== Q2: cache big enough (-M 300), and different warm rates"
sim -n 1 -L a:30:200 -M 300 -c
for r in 1 2 3 4; do fin -n 1 -L a:30:200 -M 300 -r $r -c; done

echo "===== Q3: time-left trace (-T)"
sim -n 1 -L a:30:200 -M 300 -c -T

echo "===== Q4: cache trace (-C) and different warmup times (-w)"
sim -n 1 -L a:30:200 -M 300 -c -T -C
for w in 0 1 5 10 15 20 25 30 40; do fin -n 1 -L a:30:200 -M 300 -w $w -c; done

echo "===== Q5: three jobs, two CPUs, central round-robin queue"
sim -n 2 -L a:100:100,b:100:50,c:100:50 -c -t -T -C

echo "===== Q6: cache affinity (-A)"
sim -n 2 -L a:100:100,b:100:50,c:100:50 -A a:0,b:1,c:1 -c -t -T -C
for A in a:0,b:1,c:1 b:0,a:1,c:1 c:0,a:1,b:1 a:0,b:0,c:1 a:0.1,b:1,c:1; do
    fin -n 2 -L a:100:100,b:100:50,c:100:50 -A $A -c
done

echo "===== Q7: super-linear speedup"
for M in 50 100; do for n in 1 2 3; do fin -n $n -L a:100:100,b:100:100,c:100:100 -M $M -c; done; done
sim -n 3 -L a:100:100,b:100:100,c:100:100 -M 100 -c -t -C

echo "===== Q8: per-CPU scheduling queues (-p) and the peek interval (-P)"
sim -n 2 -L a:100:100,b:100:50,c:100:50 -p -c
for P in 0 1 2 5 10 20 25 30 40 50 100; do fin -n 2 -L a:100:100,b:100:50,c:100:50 -p -P $P -c; done
sim -n 2 -L a:100:100,b:100:50,c:100:50 -p -P 1 -c -t -T -C
echo "--- scaling the number of CPUs (3 jobs)"
for n in 1 2 3 4; do
    fin -n $n -L a:100:100,b:100:50,c:100:50 -c
    fin -n $n -L a:100:100,b:100:50,c:100:50 -p -c
done
echo "--- scaling the number of CPUs (6 jobs)"
L6=a:100:100,b:100:50,c:100:50,d:100:100,e:100:50,f:100:50
for n in 1 2 3 4 6; do
    fin -n $n -L $L6 -c
    fin -n $n -L $L6 -p -c
    [ $n -gt 1 ] && fin -n $n -L $L6 -p -P 5 -c
done
for s in 0 1 2 3 4; do fin -s $s -n 4 -L $L6 -p -P 5 -c; done
echo "--- quirk: per-CPU stealing with a single CPU crashes once a steal is attempted"
fin -n 1 -L a:100:100,b:100:50,c:100:50 -p -P 5 -c

echo "===== Q9: random workloads"
for s in 1 2 3; do
    echo; echo "seed $s jobs:"; python3 multi.py -s $s -j 4 | grep 'Job name'
    for n in 1 2 4; do for M in 100 200; do
        P=10; [ $n = 1 ] && P=0
        fin -s $s -j 4 -n $n -M $M -c
        fin -s $s -j 4 -n $n -M $M -p -P $P -c
    done; done
done
