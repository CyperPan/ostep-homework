#!/bin/bash
# Builds the chapter 29 programs and reruns every measurement in the write-up.
# Usage: ./run.sh HOST            e.g. ./run.sh mac   or   ./run.sh vm
#   results go to data/HOST_*.csv; graphs (*.png) are drawn from all data/*.csv
#   takes about 15 minutes on the M4; plotting needs matplotlib
#   (the thread counts below are chosen for the 10-core M4, or for a 4-CPU Linux box)
set -e
cd "$(dirname "$0")"
HOST=${1:?usage: ./run.sh HOST}
make
mkdir -p data
./timer | tee data/${HOST}_timer.txt

N=$(getconf _NPROCESSORS_ONLN)
if [ "$N" -ge 10 ]; then          # the 10-core M4
    COUNTER=1,2,3,4,5,6,8,10,12,16; THRESHOLD=4,10; OTHER=1,2,4,6,8,10
else                              # the 4-vCPU VM
    COUNTER=1,2,3,4,5,6,8;          THRESHOLD=4;    OTHER=1,2,3,4,6,8
fi
python3 ch29_exp.py counter   --host $HOST --threads $COUNTER
python3 ch29_exp.py threshold --host $HOST --threads $THRESHOLD
python3 ch29_exp.py list      --host $HOST --threads $OTHER
python3 ch29_exp.py hash      --host $HOST --threads $OTHER
python3 ch29_exp.py plot || true
