#!/bin/bash
# Reproduce every malloc.py run used in the chapter 17 answers (takes ~1 minute).
here="$(cd "$(dirname "$0")" && pwd)"
cd "$here/../../../../vm-freespace" || exit 1
S="python3 $here/malloc_stats.py"
show() { echo; echo "\$ ./malloc.py $* -c"; python3 malloc.py "$@" -c | sed -n '/^$/,$p' | grep -v '^$'; }

echo "===== Q1-Q3: seed 0, 10 ops, no header, BEST / WORST / FIRST"
for p in BEST WORST FIRST; do show -n 10 -H 0 -p $p -s 0; done

echo; echo "===== Q4: policy x list order"
for p in BEST WORST FIRST; do for o in ADDRSORT SIZESORT+ SIZESORT-; do
    printf '%-6s %-10s' $p $o
    python3 malloc.py -n 10 -H 0 -p $p -l $o -s 0 -c |
        awk '/searched/ {match($0, /searched [0-9]+/); s += substr($0, RSTART + 9, RLENGTH - 9)}
             END {print "  seed 0, 10 ops: total elements searched", s}'
done; done
for p in BEST WORST FIRST; do for o in ADDRSORT SIZESORT+ SIZESORT-; do
    printf '%-6s %-10s' $p $o; $S --seeds 5 -n 1000 -H 0 -p $p -l $o
done; done

echo; echo "===== Q5: 1000 ops, with and without coalescing"
for p in BEST WORST FIRST; do for C in "" -C; do
    printf '%-6s %-3s' $p "$C"; $S --seeds 5 -n 1000 -H 0 -p $p $C
done; done
for C in "" -C; do echo "--- BEST, seed 0 $C"; $S --timeline -s 0 -n 1000 -H 0 -p BEST $C; $S --bysize -s 0 -n 1000 -H 0 -p BEST $C; done
for p in BEST FIRST; do for o in ADDRSORT SIZESORT+ SIZESORT- INSERT-FRONT INSERT-BACK; do
    printf '%-6s %-12s -C' $p $o; $S --seeds 5 -n 1000 -H 0 -p $p -l $o -C
done; done

echo; echo "===== Q6: percent of operations that are allocations (-P)"
for C in -C ""; do echo "--- BEST ADDRSORT $C"; for P in 1 10 30 50 60 70 80 90 99 100; do
    printf 'P=%-4d' $P; $S --seeds 5 -n 1000 -H 0 -p BEST -P $P $C
done; done

echo; echo "===== Q7: hand-made fragmented free lists (-A)"
show -H 0 -C -A +10,+10,+10,+10,+10,+10,+10,+10,+10,+10,-0,-2,-4,-6,-8,+20
show -H 0 -C -A +10,+10,+10,+10,+10,+10,+10,+10,+10,+10,-0,-2,-4,-6,-8,-1,+20
for p in BEST WORST FIRST; do show -H 0 -p $p -A +20,+5,+15,+5,+30,+5,+20,-0,-2,-4,+12,+18,+28; done
for o in ADDRSORT SIZESORT-; do show -H 0 -C -l $o -A +10,+20,+30,+40,-0,-1,-2; done
