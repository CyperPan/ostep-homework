#!/bin/bash
# Reproduce every scheduler.py run used in the chapter 7 answers.
cd "$(dirname "$0")/../../../../cpu-sched" || exit 1
run() { echo; echo "\$ ./scheduler.py $* -c"; python3 scheduler.py "$@" -c | sed -n '/Final statistics/,$p'; }
avg() { printf '%-36s %s\n' "$*" "$(python3 scheduler.py "$@" -c | grep Average)"; }

echo "===== Q1"; run -p FIFO -l 200,200,200; run -p SJF -l 200,200,200
echo "===== Q2"; run -p FIFO -l 100,200,300; run -p SJF -l 100,200,300
avg -p FIFO -l 300,200,100; avg -p SJF -l 300,200,100
echo "===== Q3"; run -p RR -q 1 -l 200,200,200; run -p RR -q 1 -l 100,200,300
echo "===== Q4"; avg -p FIFO -l 300,200,100; avg -p SJF -l 300,200,100
echo "===== Q5"
for q in 1 100 200 300; do avg -p RR -q $q -l 100,200,300; done
avg -p SJF -l 100,200,300; avg -p RR -q 200 -l 200,200,200; avg -p RR -q 300 -l 300,200,100
echo "===== Q6"
for L in 10 100 200 500 1000; do avg -p SJF -l $L,$L,$L; done
for L in 100 200 400; do avg -p SJF -l $L,$((2*L)),$((3*L)); done
echo "===== Q7"
for q in 1 10 50 100 200; do avg -p RR -q $q -l 200,200,200,200,200; done
run -p RR -q 10 -l 200,200,200,200,200
