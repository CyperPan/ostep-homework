#!/usr/bin/env python3
"""Experiments for the chapter 9 (lottery scheduling) homework.

Every lottery result comes from running the official simulator
(cpu-sched-lottery/lottery.py -c) with many random seeds; this script only
collects statistics. A small stride scheduler is implemented here for Q5.

usage: ./lottery_exp.py q2|q3|q4|q5|all [--trials N]
"""
import argparse
import os
import re
import statistics
import subprocess
import sys

LOTTERY = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', '..', '..', '..', 'cpu-sched-lottery', 'lottery.py')
DONE = re.compile(r'JOB (\d+) DONE at time (\d+)')
WIN = re.compile(r'-> Run (\d+)')


def lottery(jlist, seed, quantum=1):
    """Run lottery.py; return ({job: finish time}, [winner of each lottery])."""
    out = subprocess.run([sys.executable, LOTTERY, '-l', jlist, '-s', str(seed),
                          '-q', str(quantum), '-c'],
                         capture_output=True, text=True, check=True).stdout
    finish = {int(j): int(t) for j, t in DONE.findall(out)}
    winners = [int(j) for j in WIN.findall(out)]
    return finish, winners


def fairness(finish):
    """F = time the first job completes / time the second job completes."""
    first, second = sorted(finish.values())
    return first / second


def stride(jobs, quantum=1, big=10000):
    """Stride scheduling: run the job with the lowest pass; pass += big/tickets.
    jobs is a list of (runtime, tickets); ties go to the lower job index."""
    left = [r for r, _ in jobs]
    strides = [big / t for _, t in jobs]
    passes = [0.0] * len(jobs)
    clock, finish = 0, {}
    while len(finish) < len(jobs):
        j = min((p, i) for i, p in enumerate(passes) if i not in finish)[1]
        left[j] = max(0, left[j] - quantum)
        clock += quantum
        passes[j] += strides[j]
        if left[j] == 0:
            finish[j] = clock
    return finish


def q2(trials):
    print(f'Q2: -l 10:1,10:100, {trials} seeds')
    early, counts = 0, []
    for s in range(trials):
        finish, winners = lottery('10:1,10:100', s)
        # number of times job 0 won before job 1's 10th (final) win
        wins1, n0 = 0, 0
        for w in winners:
            if w == 1:
                wins1 += 1
                if wins1 == 10:
                    break
            else:
                n0 += 1
        counts.append(n0)
        early += n0 > 0
    print(f'  job 0 ran at least once before job 1 finished: {early}/{trials} '
          f'= {100 * early / trials:.1f}%  (theory 1-(100/101)^10 = {100 * (1 - (100 / 101) ** 10):.1f}%)')
    for k in sorted(set(counts)):
        print(f'  job 0 ran {k} time(s) before job 1 finished: {counts.count(k)} runs')
    print(f'  job 1 finish time over seeds 0-19: {[lottery("10:1,10:100", s)[0][1] for s in range(20)]}')


def q3(trials):
    print(f'Q3: -l 100:100,100:100, {trials} seeds')
    fs, gaps = [], []
    for s in range(trials):
        finish, _ = lottery('100:100,100:100', s)
        fs.append(fairness(finish))
        gaps.append(abs(finish[0] - finish[1]))
    qs = statistics.quantiles(fs, n=20)
    print(f'  F: mean {statistics.mean(fs):.3f}, median {statistics.median(fs):.3f}, '
          f'min {min(fs):.3f}, max {max(fs):.3f}, 5th pct {qs[0]:.3f}, 95th pct {qs[-1]:.3f}')
    print(f'  finish-time gap (second - first): mean {statistics.mean(gaps):.1f}, '
          f'median {statistics.median(gaps)}, max {max(gaps)}')
    for lo, hi in ((0.95, 1.01), (0.9, 0.95), (0.8, 0.9), (0, 0.8)):
        n = sum(lo <= f < hi for f in fs)
        print(f'  {lo:.2f} <= F < {min(hi, 1):.2f}: {n} ({100 * n / trials:.1f}%)')
    print('  seeds 0-9:', ', '.join(f'{fairness(lottery("100:100,100:100", s)[0]):.3f}' for s in range(10)))


def q4(trials):
    print(f'Q4: -l 100:100,100:100 with growing quantum, {trials} seeds each')
    for q in (1, 2, 5, 10, 20, 25, 50, 100):
        fs = [fairness(lottery('100:100,100:100', s, q)[0]) for s in range(trials)]
        print(f'  q={q:<4d} lotteries per job ~{100 // q:<4d} mean F {statistics.mean(fs):.3f}  '
              f'min F {min(fs):.3f}')


def q5(trials, png):
    lengths = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000]
    print(f'Q5: fairness vs job length R, two jobs with 100 tickets, {trials} trials')
    rows = []
    for r in lengths:
        jl = f'{r}:100,{r}:100'
        fl = statistics.mean(fairness(lottery(jl, s)[0]) for s in range(trials))
        fs = fairness(stride([(r, 100), (r, 100)]))
        rows.append((r, fl, fs))
        print(f'  R={r:<5d} lottery F {fl:.3f}   stride F {fs:.3f}')
    print('  unequal tickets 2:1 (R=1000): lottery share check')
    fin, w = lottery('1000:200,1000:100', 0)
    first = min(fin.values())
    print(f'    while both ran (first {first} ticks): job0 won {w[:first].count(0)}, '
          f'job1 won {w[:first].count(1)} (ideal 2:1)')
    fin = stride([(1000, 200), (1000, 100)])
    print(f'    stride finish times: {fin}')
    if png:
        plot(rows, trials, png)


def plot(rows, trials, png):
    try:
        import matplotlib
    except ImportError:
        print('  matplotlib not installed; skipping the graph (sudo apt install python3-matplotlib)')
        return
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    blue, orange = '#2a78d6', '#eb6834'          # categorical slots 1 and 2
    ink, ink2, grid = '#0b0b0b', '#52514e', '#e4e3df'
    r = [x[0] for x in rows]
    fig, ax = plt.subplots(figsize=(6.4, 3.8), dpi=200)
    fig.patch.set_facecolor('#fcfcfb')
    ax.set_facecolor('#fcfcfb')
    ax.axhline(1.0, color=ink2, lw=1, ls=(0, (4, 3)), zorder=1)
    ax.text(1.05, 1.015, 'perfectly fair (F = 1)', color=ink2, fontsize=8, va='bottom')
    ax.plot(r, [x[1] for x in rows], color=blue, lw=2, marker='o', ms=4.5,
            markeredgecolor='#fcfcfb', markeredgewidth=1.2, label=f'Lottery (mean of {trials} trials)', zorder=3)
    ax.plot(r, [x[2] for x in rows], color=orange, lw=2, marker='s', ms=4.5,
            markeredgecolor='#fcfcfb', markeredgewidth=1.2, label='Stride (deterministic)', zorder=3)
    ax.text(r[2] * 1.15, rows[2][1] - 0.07, 'Lottery', color=ink, fontsize=9)
    ax.text(2.6, 0.885, 'Stride', color=ink, fontsize=9)
    ax.set_xscale('log')
    ax.set_xticks([1, 10, 100, 1000], ['1', '10', '100', '1000'])
    ax.minorticks_off()
    ax.set_xlim(0.8, 1300)
    ax.set_ylim(0, 1.1)
    ax.set_xlabel('Job length R (time slices)', color=ink2, fontsize=9)
    ax.set_ylabel('Fairness F', color=ink2, fontsize=9)
    ax.set_title('Fairness of two equal jobs (100 tickets each)', color=ink, fontsize=10.5, loc='left')
    ax.grid(axis='y', color=grid, lw=0.8)
    ax.tick_params(colors=ink2, labelsize=8, length=0)
    for s in ax.spines.values():
        s.set_visible(False)
    ax.spines['bottom'].set_visible(True)
    ax.spines['bottom'].set_color('#b5b4ad')
    ax.legend(loc='lower right', frameon=False, fontsize=8, labelcolor=ink)
    fig.tight_layout()
    fig.savefig(png, facecolor=fig.get_facecolor())
    print(f'  wrote {png}')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('question', choices=['q2', 'q3', 'q4', 'q5', 'all'])
    ap.add_argument('--trials', type=int, default=None)
    ap.add_argument('--png', default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                   'fairness.png'))
    a = ap.parse_args()
    todo = ['q2', 'q3', 'q4', 'q5'] if a.question == 'all' else [a.question]
    for q in todo:
        if q == 'q2':
            q2(a.trials or 1000)
        elif q == 'q3':
            q3(a.trials or 1000)
        elif q == 'q4':
            q4(a.trials or 200)
        else:
            q5(a.trials or 100, a.png)
        print()
