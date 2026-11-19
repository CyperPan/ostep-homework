#!/usr/bin/env python3
"""Runs the chapter 32 experiments.

usage: ch32_exp.py [q1 q2 q3 q5 q6 q7 q8 q9 q10 ...]   (no argument: all)

In Q1-Q3, a run of vector-deadlock that has not finished after TIMEOUT
seconds counts as deadlocked (the others take well under a second) and is
killed. The timed runs of the other versions get 120 s. Times are the median of
REPS runs of the program's own "-t" timer.
"""
import statistics
import subprocess
import sys
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
TIMEOUT = 1.5
REPS = 5
PROGS = ["vector-global-order", "vector-try-wait", "vector-try-wait-block",
         "vector-avoid-hold-and-wait", "vector-nolock"]
THREADS = [1, 2, 4, 8, 16]


def run(prog, *args, timeout=TIMEOUT):
    """stdout of the run, or None if it was still running after `timeout` s."""
    try:
        p = subprocess.run([str(HERE / prog)] + [str(a) for a in args],
                           capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None
    assert p.returncode == 0, (prog, args, p.stderr)
    return p.stdout


def timed(prog, *args):
    """median time and median retry count (try-wait only) over REPS runs."""
    times, retries = [], []
    for _ in range(REPS):
        out = run(prog, "-t", *args, timeout=120)  # these versions cannot deadlock
        assert out is not None, ("no result after 120 s", prog, args)
        for line in out.splitlines():
            if line.startswith("Time:"):
                times.append(float(line.split()[1]))
            if line.startswith("Retries:"):
                retries.append(int(line.split()[1]))
    return statistics.median(times), (statistics.median(retries) if retries else None)


def hangs(prog, runs, *args):
    return sum(run(prog, *args) is None for _ in range(runs))


def q1():
    print("== Q1: vector-deadlock -n 2 -l 1 -v, 200 runs: distinct outputs")
    c = Counter(run("vector-deadlock", "-n", 2, "-l", 1, "-v") for _ in range(200))
    for out, k in c.most_common():
        print(f"  {k:3d} runs:")
        print("    " + out.rstrip().replace("\n", "\n    "))


def q2():
    print("== Q2: vector-deadlock -n 2 -d -l L: runs that deadlock")
    for loops in (1, 10, 100, 1000, 10000, 100000):
        runs = 50
        print(f"  -l {loops:6d}: {hangs('vector-deadlock', runs, '-n', 2, '-d', '-l', loops):3d}/{runs} deadlocked")


def q3():
    print("== Q3: vector-deadlock -d -n N: runs that deadlock")
    for loops in (1, 100, 10000):
        for n in (1, 2, 3, 4, 8, 16, 32):
            runs = 20
            print(f"  -l {loops:5d} -n {n:2d}: {hangs('vector-deadlock', runs, '-n', n, '-d', '-l', loops):3d}/{runs} deadlocked")
    print("  without -d, -n 8 -l 10000:",
          hangs("vector-deadlock", 20, "-n", 8, "-l", 10000), "/ 20 deadlocked")


def table(title, progs, flags_list, threads=THREADS, loops=100000):
    print(f"== {title}  (median of {REPS} runs, seconds; -l {loops})")
    for flags in flags_list:
        print(f"  flags: {' '.join(flags) or '(none)'}")
        print("    " + f"{'program':28s}" + "".join(f"  n={n:<6d}" for n in threads))
        for prog in progs:
            cells = []
            for n in threads:
                t, r = timed(prog, "-n", n, "-l", loops, *flags)
                cells.append(f"{t:8.4f}")
            print("    " + f"{prog:28s}" + "".join(f"  {c}" for c in cells))


def q5():
    print("== Q5: vector-global-order -t -n 2 -l 100000 -d:",
          f"{timed('vector-global-order', '-n', 2, '-l', 100000, '-d')[0]:.4f} s")
    print("  loops (n = 2, -d):")
    for loops in (100000, 200000, 400000, 800000):
        print(f"    -l {loops:6d}: {timed('vector-global-order', '-n', 2, '-l', loops, '-d')[0]:.4f} s")
    table("Q5: threads", ["vector-global-order"], [["-d"]])


def q6():
    table("Q6: -p", ["vector-global-order"], [["-d"], ["-d", "-p"]])


def q7():
    print(f"== Q7: try-wait retries (median of {REPS} runs, -l 100000)")
    for flags in (["-d"], ["-d", "-p"]):
        for prog in ("vector-try-wait", "vector-try-wait-block"):
            cells = []
            for n in THREADS:
                t, r = timed(prog, "-n", n, "-l", 100000, *flags)
                cells.append(f"n={n}: {t:.4f}s/{int(r)}")
            print(f"  {' '.join(flags):5s} {prog:22s} " + "  ".join(cells))
    table("Q7: try-wait vs global order", ["vector-global-order", "vector-try-wait",
                                           "vector-try-wait-block"], [["-d"], ["-d", "-p"]])


def q8():
    table("Q8: avoid-hold-and-wait", ["vector-global-order", "vector-try-wait",
                                      "vector-avoid-hold-and-wait"], [["-d"], ["-d", "-p"]])


def q9():
    print("== Q9: -c, 20 runs each: runs where some vector is not uniform, and runs")
    print("   that end with v[0] = v[1] = all zeros (impossible for whole vector adds)")
    for prog in PROGS:
        for flags in ([], ["-d"]):
            for n in (2, 4, 8):
                for loops in (1000, 100000):
                    bad = zero = 0
                    for _ in range(20):
                        out = run(prog, "-c", "-n", n, "-l", loops, *flags, timeout=120)
                        bad += "Uniform: no" in out
                        zero += "v[0].values[0] = 0, v[1].values[0] = 0" in out
                    print(f"  {prog:28s} -n {n} -l {loops:6d} {' '.join(flags):3s}: "
                          f"{bad:2d}/20 not uniform, {zero:2d}/20 all zeros")


def q10():
    table("Q10: all versions", PROGS, [["-d"], ["-d", "-p"]])


if __name__ == "__main__":
    tests = {"q1": q1, "q2": q2, "q3": q3, "q5": q5, "q6": q6, "q7": q7, "q8": q8,
             "q9": q9, "q10": q10}
    for name in sys.argv[1:] or tests:
        tests[name]()
        sys.stdout.flush()
