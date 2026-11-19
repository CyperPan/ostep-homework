#!/bin/bash
# Builds the chapter 31 programs and reruns every test in the write-up.
# Usage: ./run.sh       (from any directory; about 15 minutes in the 4-vCPU VM)
set -e
cd "$(dirname "$0")"
make
mkdir -p data
python3 ch31_tests.py q1 q2 q3 | tee data/q1-q3.txt
python3 ch31_tests.py q4       | tee data/q4.txt
python3 ch31_tests.py q6       | tee data/q6.txt
