#!/bin/bash
# Build and run every chapter 19 measurement (takes ~5 minutes).
# Run it on Linux to get pinning (-c); on macOS the -c flag is ignored.
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here" || exit 1
make -s || exit 1
E=./tlb_exp.py
os=$(uname -s | tr A-Z a-z)

echo "===== Q1: timer precision";        ./timer
echo "===== Q3: sweep, -o (staggered)";   $E sweep --max 32768 --flags=-o --reps 3 --csv data/${os}_o.csv
echo "===== Q3: sweep, plain a[i*jump]";  $E sweep --max 32768 --reps 3 --csv data/${os}_plain.csv
echo "===== Q3: how many trials?";        $E trials --pages 16
echo "===== Q5: -O2 vs -O0 vs naive"
for b in tlb tlb-O0 tlb-naive; do echo "$b"; for p in 16 1024 8192; do ./$b -o $p $((16777216 / p)); done; done
echo "===== Q6: pinning";                 $E pin --pages 1024; $E pin --pages 8192
echo "===== Q7: initialization";          $E init --pages 1024; $E init --pages 8192
echo "===== Q4: graphs (from the saved VM and macOS data)"
$E plot tlb.png "Cost per access vs number of pages (one access per page, -o)" \
    "Ubuntu VM, 4 KB pages=data/vm_o.csv" "macOS, 16 KB pages=data/mac_o.csv"
$E plot tlb_conflict.png "Without -o: every access hits the same cache sets" \
    "Ubuntu VM, 4 KB pages=data/vm_plain.csv" "macOS, 16 KB pages=data/mac_plain.csv"
