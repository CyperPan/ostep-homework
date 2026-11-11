#!/usr/bin/env python3
"""Run the chapter 28 x86.py simulator and summarize the trace.

It only reads x86.py's -c output; nothing is simulated here. For every run it
reports:

  count         final value of the shared variable `count`
  cs            how many times a thread entered the critical section
                (count == cs means no update was lost)
  max_in_cs     the most threads that were inside the critical section at
                the same time (more than 1 = mutual exclusion is broken)
  instr         instructions executed in total
  acquire       instructions executed in the lock-acquire code
  spin          acquire instructions beyond the uncontended fast path,
                i.e. instructions wasted waiting for the lock
  xchg / yield  how many atomic exchanges / yields were executed
  maxwait       the longest time (in instructions, all threads counted) any
                thread waited from its first acquire instruction until it
                got into the critical section

usage:
  lock_stats.py run   PROG [x86.py options]          one run, with a summary
  lock_stats.py sweep PROG FIRST LAST [x86.py options]  one row per -i value
  lock_stats.py schedules PROG N LEN [x86.py options]
                runs N random -P schedules of LEN slots each (seeded, so the
                same schedules every time) and reports how many broke mutual
                exclusion, lost an update, or did not finish
"""
import random
import re
import subprocess
import sys
from pathlib import Path

X86_DIR = Path(__file__).resolve().parents[4] / "threads-locks"

# Instructions in the acquire code when the lock is free (no waiting)
FAST_PATH = {
    "flag.s": 4,                   # mov, test, jne, mov $1
    "test-and-set.s": 4,           # mov $1, xchg, test, jne
    "test-and-test-and-set.s": 7,  # mov, test, jne, mov $1, xchg, test, jne
    "ticket.s": 5,                 # mov $1, fetchadd, mov turn, test, jne
    "yield.s": 4,                  # mov $1, xchg, test, je
    "peterson.s": 5,               # 2 stores, mov flag[other], test, jne
}


def layout(prog):
    """Address of every instruction, and the acquire / critical-section regions."""
    addr = 1000
    acquire = cs_start = release = None
    want_cs = want_release = False
    for line in (X86_DIR / prog).read_text().splitlines():
        code = line.split("#")[0].strip()
        comment = line.lower()
        if "critical section" in comment:
            want_cs = True
        if "release" in comment:
            want_release = True
        if not code or code.startswith(".var") or code == ".main":
            continue
        if code.startswith("."):
            if code == ".acquire":
                acquire = addr
            continue
        if want_cs and cs_start is None:
            cs_start = addr
        if want_release and release is None and cs_start is not None:
            release = addr
        addr += 1
    return acquire, cs_start, release


def run_x86(prog, args):
    cmd = ["python3", "./x86.py", "-p", prog, "-M", "count", "-c"] + args
    # a timeout so that a schedule that never finishes cannot hang us
    return subprocess.run(cmd, cwd=X86_DIR, capture_output=True, text=True, check=True,
                          timeout=30).stdout


def summarize(prog, args):
    out = run_x86(prog, args)
    acquire, cs_start, release = layout(prog)
    header = next(l for l in out.splitlines() if "Thread 0" in l)
    columns = [m.start() for m in re.finditer(r"Thread \d+", header)]

    stats = dict(count=0, cs=0, max_in_cs=0, instr=0, acquire=0, xchg=0, yld=0, maxwait=0)
    in_cs = set()
    wait_start = {}
    for line in out.splitlines():
        m = re.match(r"^\s*(-?\d+)\s+(.*?)(\d{4}) (.*)$", line)
        if not m or "Thread" in line:
            continue
        stats["count"] = int(m.group(1))
        pc, text = int(m.group(3)), m.group(4)
        tid = min(range(len(columns)), key=lambda t: abs(columns[t] - 7 - m.start(3)))
        stats["instr"] += 1
        if acquire <= pc < cs_start:
            stats["acquire"] += 1
            wait_start.setdefault(tid, stats["instr"])
        if text.startswith("xchg"):
            stats["xchg"] += 1
        if text.startswith("yield"):
            stats["yld"] += 1
        if pc == cs_start:
            stats["cs"] += 1
            in_cs.add(tid)
            stats["maxwait"] = max(stats["maxwait"], stats["instr"] - wait_start.pop(tid))
            stats["max_in_cs"] = max(stats["max_in_cs"], len(in_cs))
        if pc == release:
            in_cs.discard(tid)
    stats["spin"] = stats["acquire"] - FAST_PATH[prog] * stats["cs"]
    return stats, out


def row(stats):
    return (f"count={stats['count']:<5} cs={stats['cs']:<5} max_in_cs={stats['max_in_cs']} "
            f"instr={stats['instr']:<6} acquire={stats['acquire']:<6} spin={stats['spin']:<6} "
            f"xchg={stats['xchg']:<5} yield={stats['yld']:<4} maxwait={stats['maxwait']}")


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("run", "sweep", "schedules"):
        sys.exit(__doc__)
    if sys.argv[1] == "run":
        prog, args = sys.argv[2], sys.argv[3:]
        print(row(summarize(prog, args)[0]))
    elif sys.argv[1] == "schedules":
        prog, n, length, args = sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5:]
        threads = int(args[args.index("-t") + 1]) if "-t" in args else 2
        rng = random.Random(0)
        broken = lost = hung = 0
        worst = None
        for _ in range(n):
            # every thread must appear at least once in a schedule
            while True:
                sched = "".join(str(rng.randrange(threads)) for _ in range(length))
                if len(set(sched)) == threads:
                    break
            try:
                stats, _ = summarize(prog, args + ["-P", sched])
            except subprocess.TimeoutExpired:
                hung += 1
                continue
            if stats["max_in_cs"] > 1:
                broken += 1
                worst = worst or sched
            if stats["count"] != stats["cs"]:
                lost += 1
        print(f"{prog}: {n} random schedules of {length} slots: "
              f"{broken} broke mutual exclusion, {lost} lost an update, {hung} did not finish"
              + (f" (first bad schedule: -P {worst})" if worst else ""))
    else:
        prog, first, last, args = sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5:]
        for i in range(first, last + 1):
            print(f"-i {i:<3} " + row(summarize(prog, args + ["-i", str(i)])[0]))


if __name__ == "__main__":
    main()
