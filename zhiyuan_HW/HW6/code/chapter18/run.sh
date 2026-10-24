#!/bin/bash
# Reproduce every paging-linear-translate.py run used in the chapter 18 answers.
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../vm-paging" || exit 1
S="$here/paging_stats.py"
run() { echo; echo "\$ ./paging-linear-translate.py $* -c"; python3 paging-linear-translate.py "$@" -c | sed -n '/Page Table/,$p'; }

echo "===== Q1: page-table size vs address-space size and page size"
for a in 1m 2m 4m; do python3 "$S" size -P 1k -a $a -p 512m -v; done
for P in 1k 2k 4k; do python3 "$S" size -P $P -a 1m -p 512m -v; done

echo "===== Q2: -u 0, 25, 50, 75, 100"
for u in 0 25 50 75 100; do run -P 1k -a 16k -p 32k -v -u $u; done
for u in 25 50 75; do python3 "$S" valid $u; done

echo "===== Q3: crazy parameters"
run -P 8 -a 32 -p 1024 -v -s 1
run -P 8k -a 32k -p 1m -v -s 2
python3 paging-linear-translate.py -P 1m -a 256m -p 512m -v -s 3 -c | sed -n '/Virtual Address Trace/,$p'
python3 "$S" size -P 1m -a 256m -p 512m -s 3

echo "===== Q4: limits"
try() { echo; echo "\$ ./paging-linear-translate.py $*"; python3 paging-linear-translate.py "$@" -c 2>&1 | grep -E 'Error|-->' | tail -2; }
try -P 1k -a 32k -p 32k            # physical memory must be bigger than the address space
try -P 1k -a 64k -p 32k
try -a 512m -p 1g                  # 1 GB limit
try -P 1k -a 12k -p 32k            # address space must be a power of 2
try -P 32k -a 16k -p 64k           # page bigger than the address space
try -P 0 -a 16k -p 64k             # crashes: division by zero
try -P 1k -a 16k -p 32k -A 20000   # crashes: VA outside the address space
try -P 1k -a 16k -p 32k -u 150 -n 1
try -P 1 -a 16 -p 32 -n 2          # 1-byte pages still work
