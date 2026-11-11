#!/bin/bash
# Run every chapter 14 program three ways: plainly, under gdb, and under valgrind (Linux).
cd "$(dirname "$0")" && make -s || exit 1
export DEBUGINFOD_URLS=          # don't ask gdb/valgrind to download debug info
vg() { valgrind --leak-check=yes "$@" 2>&1 | grep -v -E 'Copyright|Using Valgrind|rerun with|^==[0-9]+== $|For lists'; }
gd() { gdb -q -batch -ex run -ex bt "$@" 2>&1 | grep -v -E 'libthread_db|^\[Thread'; }

for p in null leak overflow use_after_free bad_free; do
    echo "################ ./$p"
    ./$p; echo "exit status $?"
    echo "---- gdb ./$p (run, bt)"; gd ./$p
    echo "---- valgrind --leak-check=yes ./$p"; vg ./$p
done

echo "################ Q8: vector (realloc) vs linked list"
for n in 1000000 10000000; do echo "--- n=$n"; ./vector $n; done
echo "---- valgrind --leak-check=yes ./vector 100000"; vg ./vector 100000
