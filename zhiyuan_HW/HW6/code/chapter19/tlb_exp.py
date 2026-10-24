#!/usr/bin/env python3
"""Experiments for OSTEP chapter 19 (TLB measurement). Runs ./tlb many times.

  tlb_exp.py sweep  [--max N] [--flags "-o"] [--reps R] [--csv FILE]   Q3: ns/access vs pages
  tlb_exp.py trials [--pages N]                                         Q3: how many trials?
  tlb_exp.py pin    [--pages N]                                         Q6: pinned vs not pinned
  tlb_exp.py init   [--pages N]                                         Q7: initialized or not
  tlb_exp.py plot   PNG LABEL=CSV[:FLAGS] ...                           Q4: graph the sweeps
"""
import argparse
import os
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TLB = os.path.join(HERE, 'tlb')
TARGET = 1 << 24          # about 16M accesses per measurement


def run(pages, trials, flags=()):
    out = subprocess.run([TLB, *flags, str(pages), str(trials)],
                         capture_output=True, text=True, check=True).stdout
    return float(out.split()[2])


def page_counts(maxp):
    # 1, 2, 3, 4, 6, 8, 12, 16, ... : powers of two plus the point halfway between
    p, out = 1, []
    while p <= maxp:
        out.append(p)
        if p >= 2 and p * 3 // 2 <= maxp:
            out.append(p * 3 // 2)
        p *= 2
    return sorted(set(out))


def sweep(a):
    flags = a.flags.split()
    rows = []
    print(f'  flags {flags or "(none)"}, median of {a.reps} runs, ~{TARGET >> 20}M accesses each')
    print(f'  {"pages":>7} {"trials":>9} {"ns/access":>10}')
    for p in page_counts(a.max):
        trials = max(1, TARGET // p)
        ns = statistics.median(run(p, trials, flags) for _ in range(a.reps))
        rows.append((p, ns))
        print(f'  {p:7d} {trials:9d} {ns:10.3f}')
    if a.csv:
        with open(a.csv, 'w') as f:
            f.write('pages,ns\n')
            f.writelines(f'{p},{ns:.4f}\n' for p, ns in rows)
        print(f'  wrote {a.csv}')


def spread(xs):
    m = statistics.mean(xs)
    return m, statistics.stdev(xs) / m * 100, min(xs), max(xs)


def trials(a):
    print(f'  {a.pages} pages, 10 runs per row')
    print(f'  {"trials":>9} {"mean ns":>8} {"stdev %":>8} {"min":>7} {"max":>7}')
    t = 1
    while t <= 10 ** 6:
        m, sd, lo, hi = spread([run(a.pages, t, ['-o']) for _ in range(10)])
        print(f'  {t:9d} {m:8.3f} {sd:7.1f}% {lo:7.3f} {hi:7.3f}')
        t *= 10


def pin(a):
    t = max(1, TARGET // a.pages)
    print(f'  {a.pages} pages x {t} trials, 20 runs each')
    for name, flags in (('pinned to CPU 0', ['-o', '-c', '0']), ('not pinned', ['-o', '-c', '-1'])):
        m, sd, lo, hi = spread([run(a.pages, t, flags) for _ in range(20)])
        print(f'  {name:16s} mean {m:.3f} ns  stdev {sd:4.1f}%  min {lo:.3f}  max {hi:.3f}')


def init(a):
    print(f'  {a.pages} pages, median of 5 runs')
    print(f'  {"trials":>7} {"initialized":>12} {"not init (-n)":>14}')
    for t in (1, 10, 100, 1000):
        i = statistics.median(run(a.pages, t, ['-o']) for _ in range(5))
        n = statistics.median(run(a.pages, t, ['-o', '-n']) for _ in range(5))
        print(f'  {t:7d} {i:10.2f} ns {n:11.2f} ns')


def plot(a):
    try:
        import matplotlib
    except ImportError:
        print('  matplotlib not installed; skipping the graph')
        return
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    colors = ['#2a78d6', '#eb6834', '#1a9e77', '#7d5ba6']
    markers = ['o', 's', '^', 'D']
    ink, ink2, grid = '#0b0b0b', '#52514e', '#e4e3df'
    fig, ax = plt.subplots(figsize=(6.4, 3.8), dpi=200)
    fig.patch.set_facecolor('#fcfcfb')
    ax.set_facecolor('#fcfcfb')
    for k, spec in enumerate(a.series):
        label, csv = spec.split('=', 1)
        rows = [l.split(',') for l in open(csv).read().split()[1:]]
        ax.plot([int(r[0]) for r in rows], [float(r[1]) for r in rows], color=colors[k], lw=2,
                marker=markers[k], ms=4, markeredgecolor='#fcfcfb', markeredgewidth=1,
                label=label, zorder=3)
    ax.set_xscale('log', base=2)
    ticks = [1 << i for i in range(0, 16, 2)]
    ax.set_xticks(ticks, [str(t) for t in ticks])
    ax.minorticks_off()
    ax.set_ylim(bottom=0)
    ax.set_xlabel('Number of pages accessed', color=ink2, fontsize=9)
    ax.set_ylabel('Time per access (ns)', color=ink2, fontsize=9)
    ax.set_title(a.title, color=ink, fontsize=10.5, loc='left')
    ax.grid(axis='y', color=grid, lw=0.8)
    ax.tick_params(colors=ink2, labelsize=8, length=0)
    for s in ax.spines.values():
        s.set_visible(False)
    ax.spines['bottom'].set_visible(True)
    ax.spines['bottom'].set_color('#b5b4ad')
    ax.legend(loc='upper left', frameon=False, fontsize=8, labelcolor=ink)
    fig.tight_layout()
    fig.savefig(a.png, facecolor=fig.get_facecolor())
    print(f'  wrote {a.png}')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('sweep')
    s.add_argument('--max', type=int, default=16384)
    s.add_argument('--flags', default='')
    s.add_argument('--reps', type=int, default=5)
    s.add_argument('--csv')
    for name in ('trials', 'pin', 'init'):
        s = sub.add_parser(name)
        s.add_argument('--pages', type=int, default=16 if name == 'trials' else 1024)
    s = sub.add_parser('plot')
    s.add_argument('png')
    s.add_argument('title')
    s.add_argument('series', nargs='+', help='LABEL=CSV')
    a = ap.parse_args()
    if not os.path.exists(TLB) and a.cmd != 'plot':
        sys.exit('build first: make')
    globals()[a.cmd](a)
