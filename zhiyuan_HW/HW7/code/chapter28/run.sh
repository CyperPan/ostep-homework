#!/bin/bash
# Reruns every experiment in the chapter 28 write-up.
# Traces come straight from x86.py; lock_stats.py only summarizes them.
# Usage: ./run.sh          (from any directory; takes about 2 minutes)
HERE="$(cd "$(dirname "$0")" && pwd)"
S="$HERE/lock_stats.py"
cd "$HERE/../../../../threads-locks" || exit 1
X="python3 ./x86.py"

trace() { echo; echo "\$ ./x86.py $*"; $X "$@" | grep -v '^ARG'; }

echo "===== Q2: flag.s with the defaults"
trace -p flag.s -M flag,count -R ax,bx -c
echo "===== Q3: flag.s with bx=2"
$S run flag.s -a bx=2,bx=2
echo "===== Q4: flag.s, bx=100, -i 1..40 (correct: count=200, max_in_cs=1)"
$S sweep flag.s 1 40 -a bx=100,bx=100
echo "===== Q6: test-and-set.s, bx=100, -i 1..40"
$S sweep test-and-set.s 1 40 -a bx=100,bx=100
echo "===== Q7: hand-made schedules for test-and-set.s"
echo "--- T0 takes the lock (mov, xchg), then T1 tries to acquire it"
trace -p test-and-set.s -M mutex,count -R ax -a bx=1,bx=1 -P 00111111110000000000 -c
echo "--- T0 and T1 alternate every instruction (both race for the xchg)"
$S run test-and-set.s -a bx=1,bx=1 -P 01
trace -p test-and-set.s -M mutex,count -R ax -a bx=1,bx=1 -P 01 -c
echo "--- T0 is preempted inside the critical section"
$S run test-and-set.s -a bx=1,bx=1 -P 000001111111111111
echo "--- random schedules, 2 and 3 threads"
$S schedules test-and-set.s 300 12 -a bx=3,bx=3
$S schedules test-and-set.s 300 15 -t 3 -a bx=2
$S schedules flag.s 300 12 -a bx=3,bx=3
echo "===== Q9: peterson.s, -i 1..20"
$S sweep peterson.s 1 20 -a bx=0,bx=1
echo "===== Q10: hand-made schedules for peterson.s"
echo "--- T0 runs alone first: no contention"
$S run peterson.s -a bx=0,bx=1 -P 0000000000000000000011
echo "--- both set their flag before either reads the other's (strict alternation)"
trace -p peterson.s -M flag,turn,count -R cx -a bx=0,bx=1 -P 01 -c
echo "--- T1 arrives while T0 is in the critical section"
$S run peterson.s -a bx=0,bx=1 -P 00000000000111111111
echo "--- random schedules"
$S schedules peterson.s 500 12 -a bx=0,bx=1
$S schedules peterson.s 500 6 -a bx=0,bx=1
echo "===== Q11: ticket.s, bx=1000"
$S run ticket.s -a bx=1000,bx=1000
for i in 10 100 500; do printf -- "-i %-4s" $i; $S run ticket.s -a bx=1000,bx=1000 -i $i; done
echo "===== Q12: more threads, bx=100 each, -i 50: ticket.s vs test-and-set.s (maxwait = fairness)"
for t in 2 3 4 6 8; do
    printf "ticket       -t %s " $t; $S run ticket.s -t $t -a bx=100
    printf "test-and-set -t %s " $t; $S run test-and-set.s -t $t -a bx=100
done
echo "===== Q13: test-and-set.s vs yield.s, T0 preempted inside the critical section"
trace -p yield.s -M mutex,count -a bx=2,bx=2 -i 16 -c
for p in test-and-set.s yield.s; do printf "%-15s -i 16 " $p; $S run $p -a bx=2,bx=2 -i 16; done
echo "--- bx=100, different intervals and thread counts"
for t in 2 4; do for i in 5 16 50 100; do
    for p in test-and-set.s yield.s; do printf "%-15s -t %s -i %-4s" $p $t $i; $S run $p -t $t -a bx=100 -i $i; done
done; done
echo "===== Q14: test-and-test-and-set.s vs test-and-set.s, bx=100"
for t in 2 4; do for i in 5 16 50 100; do
    for p in test-and-set.s test-and-test-and-set.s; do printf "%-24s -t %s -i %-4s" $p $t $i; $S run $p -t $t -a bx=100 -i $i; done
done; done
