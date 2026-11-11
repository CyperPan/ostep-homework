#!/bin/bash
# Reproduce the chapter 13 measurements (run on Linux).
cd "$(dirname "$0")" && make -s || exit 1
mem() { free -m | awk '/Mem:/{printf "used %5d  free %5d  buff/cache %5d  available %5d", $3, $4, $6, $7}'; }

echo "===== Q2: free -m"; free -m

echo "===== Q4: free while memory-user runs"
for mb in 500 1000 2000 4000; do
    echo "--- $mb MB"; echo "  before : $(mem)"
    ./memory-user $mb 8 > /dev/null & pid=$!
    sleep 5
    echo "  running: $(mem)   [memory-user VmRSS $(awk '/VmRSS/{print $2/1024}' /proc/$pid/status) MB]"
    wait $pid; sleep 1; echo "  after  : $(mem)"
done
echo "--- 20000 MB (more than RAM + swap)"; ./memory-user 20000 3; echo "  exit status $?"
echo "--- 2000 MB inside a 1 GB memory cgroup (needs a systemd user session)"
systemd-run --user --scope -q -p MemoryMax=1G -p MemorySwapMax=0 ./memory-user 2000 5
echo "  exit status $? (137 = killed by the OOM killer)"

echo "===== Q6/Q7: biggest processes and gnome-shell's address space"
ps auxw --sort=-rss | head -4
if g=$(pgrep -x gnome-shell | head -1); then
    pmap -x "$g" | tail -1; echo "  mappings: $(pmap -q "$g" | wc -l)"
fi

echo "===== Q8: pmap of memory-user"
for mb in 100 1000; do
    ./memory-user $mb 6 > /dev/null & pid=$!; sleep 3
    echo "--- $mb MB"; pmap -x $pid | sed -n '2p'; pmap -x $pid | awk 'NR>2' | sort -k2 -nr | head -3; pmap -x $pid | tail -1
    wait $pid
done
