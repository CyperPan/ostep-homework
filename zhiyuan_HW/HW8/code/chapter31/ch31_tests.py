#!/usr/bin/env python3
"""Runs the chapter 31 programs many times and checks their output.

usage: ch31_tests.py [q1|q2|q3|q4|q6 ...]     (no argument: all of them)

The programs only print; this script checks the order of what they print:
  q1  fork-join:  "child" must come before "parent: end"
  q2  rendezvous: both "before" lines before either "after" line
  q3  barrier:    all N "before" lines before any "after" line
  q4  both reader-writer locks: where the first write happens, longest waits
  q6  mutex-nostarve: longest "overtakes" (see mutex-nostarve.c)
The *-broken programs (built with -DBROKEN) leave out the semaphore wait that
matters, and show that the check really can fail.
"""
import os
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def run(prog, *args, env=None):
    e = dict(os.environ, **(env or {}))
    t = time.time()
    p = subprocess.run([str(HERE / prog)] + [str(a) for a in args], capture_output=True,
                       text=True, env=e, timeout=120)
    assert p.returncode == 0, (prog, args, p.returncode, p.stderr)
    return p.stdout.splitlines(), p.stderr, time.time() - t


def before_after_ok(lines, n):
    """n 'before' lines, n 'after' lines, and every before precedes every after."""
    b = [i for i, l in enumerate(lines) if l.endswith(": before")]
    a = [i for i, l in enumerate(lines) if l.endswith(": after")]
    return len(b) == n and len(a) == n and max(b) < min(a)


def count(prog, check, runs, *args, env=None):
    bad = 0
    for _ in range(runs):
        lines, _, _ = run(prog, *args, env=env)
        bad += not check(lines)
    return bad


def q1():
    print("== Q1 fork-join")
    lines, _, secs = run("fork-join")
    print(" ", " | ".join(lines), f"({secs:.2f} s; the child sleeps 1 s)")
    ok = lambda ls: ls.index("child") < ls.index("parent: end") if "child" in ls else False
    for prog in ("fork-join", "fork-join-broken"):
        print(f"  {prog:17s}: child printed before 'parent: end' in "
              f"{10 - count(prog, ok, 10)}/10 runs")


def q2():
    print("== Q2 rendezvous (argument: which child sleeps 1 s before arriving)")
    for arg, runs in ((0, 500), (1, 5), (2, 5)):
        bad = count("rendezvous", lambda ls: before_after_ok(ls, 2), runs, arg)
        lines, _, _ = run("rendezvous", arg)
        print(f"  arg {arg}: {runs - bad}/{runs} runs correct; e.g. " + " | ".join(lines[1:-1]))


def q3():
    print("== Q3 barrier (BARRIER_SLEEP_US: random delay before arriving)")
    for prog in ("barrier", "barrier-broken"):
        for n in (1, 2, 5, 10, 50):
            for us in (0, 2000):
                runs = 200
                bad = count(prog, lambda ls: before_after_ok(ls, n), runs, n,
                            env={"BARRIER_SLEEP_US": str(us)})
                print(f"  {prog:14s} N={n:2d} sleep<{us:4d}us: {runs - bad:3d}/{runs} correct")


def rw_stats(prog, readers, writers, loops, read_us, write_us):
    lines, err, secs = run(prog, readers, writers, loops,
                           env={"RW_READ_US": str(read_us), "RW_WRITE_US": str(write_us)})
    ops = [l for l in lines if l.startswith(("read", "write"))]
    first_write = next(i for i, l in enumerate(ops) if l.startswith("write"))
    reads = sum(l.startswith("read") for l in ops)
    m = re.search(r"reader ([\d.]+) ms .* writer ([\d.]+) ms", err)
    assert lines[-1] == f"end: value {writers * loops}", lines[-1]
    return first_write, reads, float(m.group(1)), float(m.group(2)), secs


def q4():
    print("== Q4/Q5 reader-writer locks")
    print("  readers writers loops hold_read_us hold_write_us | lock | reads before 1st write"
          " / total reads | max wait reader, writer (ms) | run time (s)")
    configs = [(5, 1, 200, 1000, 0), (10, 1, 100, 1000, 0), (5, 1, 200, 0, 0), (5, 1, 2000, 0, 0),
               (1, 5, 200, 0, 1000), (5, 5, 100, 1000, 1000)]
    for cfg in configs:
        for prog in ("reader-writer", "reader-writer-nostarve-weak", "reader-writer-nostarve"):
            res = [rw_stats(prog, *cfg) for _ in range(3)]
            fw = max(r[0] for r in res)
            print(f"  {cfg[0]:2d} {cfg[1]:2d} {cfg[2]:5d} {cfg[3]:5d} {cfg[4]:5d} | {prog:27s} | "
                  f"{fw:5d} / {res[0][1]:5d} | {max(r[2] for r in res):7.1f}, "
                  f"{max(r[3] for r in res):7.1f} | {max(r[4] for r in res):.2f}"
                  "   (worst of 3 runs)")


def q6():
    print("== Q6 no-starve mutex: worst case over 3 runs")
    for sem in ("lifo", "zem"):
        for mode in ("ns", "sem"):
            for n in (2, 3, 4, 8, 16):
                for loops in (2000, 20000):
                    worst = None
                    for _ in range(3):
                        lines, _, _ = run("mutex-nostarve", "-s", sem, "-m", mode, n, loops)
                        f = lines[1].split()
                        d = dict(zip(f[0::2], f[1::2]))
                        assert f[-1] == "ok", lines
                        if worst is None or int(d["max_overtakes"]) > int(worst["max_overtakes"]):
                            worst = d
                    print(f"  sem {sem:4s} lock {mode:3s} N={n:2d} loops={loops:6d}: "
                          f"max overtakes {worst['max_overtakes']:>6s} (2(N-1) = {worst['bound']:>2s}),"
                          f" max wait {worst['max_wait_ms']:>7s} ms, time {worst['time']} s")


if __name__ == "__main__":
    tests = {"q1": q1, "q2": q2, "q3": q3, "q4": q4, "q6": q6}
    for name in sys.argv[1:] or tests:
        tests[name]()
