#!/bin/bash
# Builds the chapter 32 programs and reruns every experiment in the write-up.
# Usage: ./run.sh       (from any directory; about 20 minutes in the VM, mostly
#                        waiting for deadlocked runs to time out)
set -e
cd "$(dirname "$0")"
make
mkdir -p data
python3 ch32_exp.py q1 q2 q3      | tee data/q1-q3.txt
python3 ch32_exp.py q5 q6 q7 q8   | tee data/q5-q8.txt
python3 ch32_exp.py q9 q10        | tee data/q9-q10.txt
