#!/usr/bin/env python3
"""Experiments for the chapter 15 (address translation) homework.

All translations come from the official simulator (vm-mechanism/relocation.py -c);
this script only collects results.

  ./reloc_exp.py minlimit ARGS...    smallest -l that makes every generated VA valid
  ./reloc_exp.py maxbase  ARGS...    largest -b that still fits (searches the simulator's check)
  ./reloc_exp.py q5 [--seeds N] [--n N] [--asize A] [--png FILE]
                                     fraction of valid VAs vs bounds, many seeds (+ graph)
"""
import argparse
import os
import re
import subprocess
import sys

RELOC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                     '..', '..', '..', '..', 'vm-mechanism', 'relocation.py')
VA = re.compile(r'VA +\d+: 0x[0-9a-f]+ \(decimal: *(\d+)\) --> (VALID|SEGMENTATION VIOLATION)')


def run(args):
    p = subprocess.run([sys.executable, RELOC, *args, '-c'], capture_output=True, text=True)
    return p.stdout


def vas(args):
    return [(int(v), ok == 'VALID') for v, ok in VA.findall(run(args))]


def minlimit(args):
    # passing -l changes the random sequence (the limit is no longer drawn),
    # so measure the addresses that are generated when -l is given
    probe = vas([*args, '-l', '1'])
    m = max(v for v, _ in probe)
    print(f'largest VA generated: {m}  ->  smallest bounds: -l {m + 1}')
    for limit in (m, m + 1):
        res = vas([*args, '-l', str(limit)])
        print(f'  -l {limit}: {sum(ok for _, ok in res)}/{len(res)} valid')


def maxbase(args):
    lo, hi = 0, 1 << 40
    while lo < hi:                       # binary search on the simulator's own fit check
        mid = (lo + hi + 1) // 2
        if 'does not fit' in run([*args, '-b', str(mid)]):
            hi = mid - 1
        else:
            lo = mid
    print(f'largest base that fits: -b {lo}')
    print('  -b', lo + 1, '->', run([*args, '-b', str(lo + 1)]).strip().splitlines()[-2])


def q5(seeds, n, asize, png):
    size = int(asize[:-1]) * 1024 if asize.endswith('k') else int(asize)
    psize = str(4 * size)               # physical memory must be larger than the address space
    limits = list(range(0, size + 1, size // 32))
    rows = []
    print(f'Q5: -a {asize} -p {psize}, {seeds} seeds x {n} addresses per bounds value')
    for limit in limits:
        valid = total = 0
        for s in range(seeds):
            res = vas(['-s', str(s), '-n', str(n), '-a', asize, '-p', psize, '-l', str(limit)])
            valid += sum(ok for _, ok in res)
            total += len(res)
        rows.append((limit, valid / total))
        print(f'  limit {limit:6d} ({limit / size:5.3f} of address space): valid fraction {valid / total:.3f}')
    if png:
        plot(rows, size, asize, seeds * n, png)


def plot(rows, size, asize, samples, png):
    try:
        import matplotlib
    except ImportError:
        print('  matplotlib not installed; skipping the graph (sudo apt install python3-matplotlib)')
        return
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    blue, ink, ink2, grid, surf = '#2a78d6', '#0b0b0b', '#52514e', '#e4e3df', '#fcfcfb'
    fig, ax = plt.subplots(figsize=(6.4, 3.8), dpi=200)
    fig.patch.set_facecolor(surf)
    ax.set_facecolor(surf)
    ax.plot([0, size], [0, 1], color=ink2, lw=1, ls=(0, (4, 3)), zorder=1)
    ax.text(size * 0.55, 0.40, 'dashed: expected = bounds / address-space size', color=ink2, fontsize=8)
    ax.plot([r[0] for r in rows], [r[1] for r in rows], color=blue, lw=2, marker='o', ms=4.5,
            markeredgecolor=surf, markeredgewidth=1.2, zorder=3)
    ax.text(size * 0.02, 0.86, f'measured ({samples} random VAs per point)', color=ink, fontsize=9)
    ax.set_xlim(-size * 0.02, size * 1.02)
    ax.set_ylim(-0.02, 1.05)
    ax.set_xlabel(f'Bounds register value (address space = {asize} = {size} bytes)', color=ink2, fontsize=9)
    ax.set_ylabel('Fraction of valid addresses', color=ink2, fontsize=9)
    ax.set_title('Valid virtual addresses vs. bounds register', color=ink, fontsize=10.5, loc='left')
    ax.grid(axis='y', color=grid, lw=0.8)
    ax.tick_params(colors=ink2, labelsize=8, length=0)
    for s in ax.spines.values():
        s.set_visible(False)
    ax.spines['bottom'].set_visible(True)
    ax.spines['bottom'].set_color('#b5b4ad')
    fig.tight_layout()
    fig.savefig(png, facecolor=fig.get_facecolor())
    print(f'  wrote {png}')


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('minlimit', 'maxbase', 'q5'):
        sys.exit(__doc__)
    if sys.argv[1] == 'minlimit':
        minlimit(sys.argv[2:])
    elif sys.argv[1] == 'maxbase':
        maxbase(sys.argv[2:])
    else:
        ap = argparse.ArgumentParser()
        ap.add_argument('--seeds', type=int, default=50)
        ap.add_argument('--n', type=int, default=100)
        ap.add_argument('--asize', default='1k')
        ap.add_argument('--png', default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                      'valid_fraction.png'))
        a = ap.parse_args(sys.argv[2:])
        q5(a.seeds, a.n, a.asize, a.png)
