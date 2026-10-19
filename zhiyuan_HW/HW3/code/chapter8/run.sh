#!/bin/bash
# Reproduce every mlfq.py run used in the chapter 8 answers.
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../cpu-sched-mlfq" || exit 1
T="python3 $here/mlfq_tools.py"
cmd() { echo; echo "\$ ./mlfq.py $* -c"; }
seg() { cmd "$@"; $T segments "$@"; }
share() { lo=$1 hi=$2; shift 2; cmd "$@"; $T share $lo $hi "$@"; }

echo "===== Q1: random problems, 2 jobs, 2 queues, short jobs, no I/O"
for s in 1 2 3; do cmd -s $s -n 2 -j 2 -m 20 -M 0; python3 mlfq.py -s $s -n 2 -j 2 -m 20 -M 0 -c | sed -n '/Execution Trace/,$p'; done

echo "===== Q2: examples from the chapter"
echo "--- Figure 8.2: a single long-running job";            seg -n 3 -q 10 -l 0,200,0
echo "--- Figure 8.3 (left): along came a short job";        seg -n 3 -q 10 -l 0,180,0:100,20,0
echo "--- Figure 8.3 (right): interactive job with I/O";    seg -n 3 -q 10 -i 5 -S -l 0,175,0:50,25,1
echo "--- Figure 8.4 (left): no boost, A starves";          seg -n 3 -q 10 -i 1 -S -l 0,200,0:100,50,1:100,50,1
echo "--- Figure 8.4 (right): boost every 100 ms";          seg -n 3 -q 10 -i 1 -S -B 100 -l 0,200,0:100,100,1:100,100,1
echo "--- Figure 8.5 (left): gaming with old Rules 4a/4b";  share 100 300 -n 3 -q 10 -i 1 -S -l 0,300,0:80,200,9
echo "--- Figure 8.5 (right): new Rule 4 (no -S)";          share 100 300 -n 3 -q 10 -i 1 -l 0,300,0:80,200,9
echo "--- Figure 8.6: lower priority, longer quanta";       seg -n 3 -Q 10,20,40 -A 2,2,2 -l 0,140,0:0,140,0

echo "===== Q3: behave like round robin"
cmd -n 1 -q 10 -l 0,100,0:0,200,0:0,300,0; python3 mlfq.py -n 1 -q 10 -l 0,100,0:0,200,0:0,300,0 -c | grep -E '^  (Job|Avg).*response'
echo; echo "\$ ../cpu-sched/scheduler.py -p RR -q 10 -l 100,200,300 -c"; python3 ../cpu-sched/scheduler.py -p RR -q 10 -l 100,200,300 -c | sed -n '/Final statistics/,$p'

echo "===== Q4: gaming the scheduler to get 99% of the CPU"
share 100 1100 -n 3 -q 100 -i 1 -S -l 0,1000,0:0,1000,99
share 100 1100 -n 3 -q 100 -i 1 -l 0,1000,0:0,1000,99

echo "===== Q5: how often to boost so a long job gets >= 5%"
for B in 50 100 200 300 400; do share 0 2000 -n 3 -q 10 -i 1 -S -B $B -l 0,2000,0:0,2000,1:0,2000,1; done
share 0 2000 -n 3 -q 10 -i 1 -S -l 0,2000,0:0,2000,1:0,2000,1

echo "===== Q6: -I (I/O-finished jobs go to the front of their queue)"
seg -n 1 -q 10 -i 5 -l 0,100,0:0,100,0:0,30,2
seg -n 1 -q 10 -i 5 -I -l 0,100,0:0,100,0:0,30,2
