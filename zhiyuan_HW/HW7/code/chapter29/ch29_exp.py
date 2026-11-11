#!/usr/bin/env python3
"""Run the chapter 29 benchmarks and plot them.

Each program prints one line per run that already says whether the result was
correct ("ok"); this script repeats runs, stores every line in a CSV file, and
stops if any run was WRONG.

usage:
  ch29_exp.py counter   --host NAME [--threads 1,2,4] [--reps 5]
  ch29_exp.py threshold --host NAME [--threads 4] [--reps 5]
  ch29_exp.py list      --host NAME [--threads 1,2,4] [--reps 3]
  ch29_exp.py hash      --host NAME [--threads 1,2,4] [--reps 5]
  ch29_exp.py plot                         (all CSV files in data/ -> PNGs)

NAME labels the machine (I used "mac" and "vm"); results go to
data/NAME_<experiment>.csv.
"""
import argparse
import csv
import statistics
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
DATA = HERE / "data"

INCREMENTS = 1_000_000                  # per thread (as in the chapter)
THRESHOLDS = [2 ** k for k in range(11)]  # 1 .. 1024
LIST_LENGTH, LIST_OPS = 1000, 2000      # 1000 nodes, 2000 lookups per thread
LIST_WORK = [0, 20, 200]                # busy-loop iterations per node visited
HASH_BUCKETS, HASH_KEYS, HASH_OPS = 1024, 4096, 1_000_000
HASH_LOCKS = [1, 4, 16, 64, 1024]


def run(cmd):
    out = subprocess.run([str(HERE / cmd[0])] + [str(c) for c in cmd[1:]],
                         capture_output=True, text=True, check=True).stdout.split()
    if out[-1] != "ok":
        sys.exit(f"WRONG result: {' '.join(map(str, cmd))} -> {' '.join(out)}")
    return out


def sweep(name, host, header, configs, reps):
    """configs: list of command lines. Runs each `reps` times, interleaved."""
    DATA.mkdir(exist_ok=True)
    path = DATA / f"{host}_{name}.csv"
    with open(path, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(header + ["rep"])
        for rep in range(reps):
            for cmd in configs:
                out = run(cmd)
                w.writerow(out + [rep])
                f.flush()
                print(" ".join(out), "rep", rep, flush=True)
    print("wrote", path)


def threads_arg(s):
    return [int(t) for t in s.split(",")]


def cmd_counter(a):
    configs = []
    for t in a.threads:
        configs.append(["counter", t, INCREMENTS])
        configs.append(["counter", "-s", 1024, t, INCREMENTS])
    sweep("counter", a.host, ["kind", "threads", "threshold", "increments", "seconds",
                              "final", "expected", "ok"], configs, a.reps)


def cmd_threshold(a):
    configs = [["counter", "-s", s, t, INCREMENTS] for t in a.threads for s in THRESHOLDS]
    sweep("threshold", a.host, ["kind", "threads", "threshold", "increments", "seconds",
                                "final", "expected", "ok"], configs, a.reps)


def cmd_list(a):
    configs = []
    for w in LIST_WORK:
        for t in a.threads:
            configs.append(["list", "-w", w, t, LIST_OPS, LIST_LENGTH])
            configs.append(["list", "-H", "-w", w, t, LIST_OPS, LIST_LENGTH])
    sweep("list", a.host, ["kind", "threads", "length", "ops", "work", "insert_pct", "seconds",
                           "found", "lookups", "size", "ok"], configs, a.reps)


def cmd_hash(a):
    configs = [["hash", "-l", l, t, HASH_OPS, HASH_BUCKETS, HASH_KEYS]
               for t in a.threads for l in HASH_LOCKS]
    sweep("hash", a.host, ["kind", "threads", "buckets", "keys", "locks", "update_pct", "ops",
                           "seconds", "updates", "sum", "ok"], configs, a.reps)


# --- plotting ------------------------------------------------------------------
def medians(path, key_fields):
    groups = defaultdict(list)
    with open(path) as f:
        for row in csv.DictReader(f):
            groups[tuple(row[k] for k in key_fields)].append(float(row["seconds"]))
    return {k: statistics.median(v) for k, v in groups.items()}


def cmd_plot(_):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        sys.exit("matplotlib is not installed; skipping plots")

    hosts = sorted({p.name.split("_")[0] for p in DATA.glob("*.csv")})
    titles = {"mac": "macOS, Apple M4 (4P + 6E cores)", "vm": "Ubuntu VM (4 vCPUs)"}

    def per_host(name, draw, fname, **kw):
        avail = [h for h in hosts if (DATA / f"{h}_{name}.csv").exists()]
        if not avail:
            return
        fig, axes = plt.subplots(1, len(avail), figsize=(6 * len(avail), 4.2), squeeze=False)
        for ax, h in zip(axes[0], avail):
            draw(ax, DATA / f"{h}_{name}.csv")
            ax.set_title(titles.get(h, h))
            ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(HERE / fname, dpi=130)
        print("wrote", HERE / fname)

    def counter(ax, path):
        m = medians(path, ["kind", "threads"])
        for kind, label in (("simple", "simple counter (one lock)"),
                            ("approx", "approximate counter, S = 1024")):
            pts = sorted((int(t), s) for (k, t), s in m.items() if k == kind)
            ax.plot([p[0] for p in pts], [p[1] for p in pts], "o-", label=label)
        ax.set_xlabel("threads (each does 1M increments)")
        ax.set_ylabel("time (seconds)")
        ax.legend()

    def threshold(ax, path):
        m = medians(path, ["threads", "threshold"])
        for t in sorted({int(k[0]) for k in m}):
            pts = sorted((int(s), v) for (tt, s), v in m.items() if int(tt) == t)
            ax.plot([p[0] for p in pts], [p[1] for p in pts], "o-", label=f"{t} threads")
        ax.set_xscale("log", base=2)
        ax.set_xlabel("approximation threshold S")
        ax.set_ylabel("time (seconds)")
        ax.legend()

    def lists(ax, path):
        m = medians(path, ["kind", "work", "threads"])
        styles = {"single": "-", "hoh": "--"}
        for w in LIST_WORK:
            for kind in ("single", "hoh"):
                pts = sorted((int(t), s) for (k, ww, t), s in m.items() if k == kind and int(ww) == w)
                if pts:
                    ax.plot([p[0] for p in pts], [p[1] for p in pts], "o" + styles[kind],
                            label=f"{'one lock' if kind == 'single' else 'hand-over-hand'}, work {w}")
        ax.set_yscale("log")
        ax.set_xlabel("threads (each does 2000 lookups, 1000-node list)")
        ax.set_ylabel("time (seconds, log scale)")
        ax.legend(fontsize=8)

    def hashes(ax, path):
        m = medians(path, ["locks", "threads"])
        for l in HASH_LOCKS:
            pts = sorted((int(t), s) for (ll, t), s in m.items() if int(ll) == l)
            label = {1: "1 lock (global)", HASH_BUCKETS: f"{l} locks (per bucket)"}.get(l, f"{l} locks")
            ax.plot([p[0] for p in pts], [p[1] for p in pts], "o-", label=label)
        ax.set_xlabel("threads (each does 1M operations)")
        ax.set_ylabel("time (seconds)")
        ax.legend()

    per_host("counter", counter, "counter.png")
    per_host("threshold", threshold, "threshold.png")
    per_host("list", lists, "list.png")
    per_host("hash", hashes, "hash.png")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    for name, fn, reps, threads in (("counter", cmd_counter, 5, "1,2,4"),
                                    ("threshold", cmd_threshold, 5, "4"),
                                    ("list", cmd_list, 3, "1,2,4"),
                                    ("hash", cmd_hash, 5, "1,2,4")):
        s = sub.add_parser(name)
        s.add_argument("--host", required=True)
        s.add_argument("--threads", type=threads_arg, default=threads_arg(threads))
        s.add_argument("--reps", type=int, default=reps)
        s.set_defaults(fn=fn)
    s = sub.add_parser("plot")
    s.set_defaults(fn=cmd_plot)
    a = p.parse_args()
    a.fn(a)


if __name__ == "__main__":
    main()
