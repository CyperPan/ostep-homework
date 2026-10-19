#!/bin/bash
# Reproduce every lottery.py run used in the chapter 9 answers (takes ~2 minutes).
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../cpu-sched-lottery" || exit 1

echo "===== Q1: 3 jobs, seeds 1, 2, 3"
for s in 1 2 3; do
    echo; echo "\$ ./lottery.py -s $s -j 3 -c"
    python3 lottery.py -s $s -j 3 -c | sed -n '/Here is the job list/,$p'
done

echo "===== Q2: -l 10:1,10:100"
echo "\$ ./lottery.py -l 10:1,10:100 -c"
python3 lottery.py -l 10:1,10:100 -c | sed -n '/Solutions/,$p'
echo
"$here/lottery_exp.py" q2
"$here/lottery_exp.py" q3
"$here/lottery_exp.py" q4
"$here/lottery_exp.py" q5     # also writes fairness.png next to this script
