#!/usr/bin/env python3
"""Run malloc.py -c and summarize what happened.

usage:
  ./malloc_stats.py [malloc.py flags]             one run, summary
  ./malloc_stats.py --seeds N [malloc.py flags]   average over seeds 0..N-1
  ./malloc_stats.py --bysize [malloc.py flags]    failure rate by request size,
                                                  first vs last 25% of allocations
  ./malloc_stats.py --timeline [malloc.py flags]  free-list length and largest
                                                  free chunk after selected ops
"""
import os
import re
import subprocess
import sys

MALLOC = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      '..', '..', '..', '..', 'vm-freespace', 'malloc.py')
ALLOC = re.compile(r'Alloc\((\d+)\)\s+returned (-?\d+) \(searched (\d+) elements\)')
LIST = re.compile(r'Free List \[ Size (\d+) \]: (.*)')
CHUNK = re.compile(r'sz:(\d+)')


def run(args):
    out = subprocess.run([sys.executable, MALLOC, *args, '-c'],
                         capture_output=True, text=True, check=True).stdout
    header = int(re.search(r'^headerSize (\d+)', out, re.M).group(1))
    allocs = [(int(s), int(r) != -1 + header, int(n))    # (size, ok, searched)
              for s, r, n in ALLOC.findall(out)]
    lists = [(int(n), [int(c) for c in CHUNK.findall(rest)]) for n, rest in LIST.findall(out)]
    return allocs, lists


def summary(args):
    allocs, lists = run(args)
    lens = [n for n, _ in lists]
    final = lists[-1][1] if lists else []
    ok = [a for a in allocs if a[1]]
    return {
        'allocs': len(allocs),
        'failed': len(allocs) - len(ok),
        'fail%': 100 * (len(allocs) - len(ok)) / max(1, len(allocs)),
        'searched': sum(a[2] for a in allocs),
        'avg_searched': sum(a[2] for a in allocs) / max(1, len(allocs)),
        'avg_searched_ok': sum(a[2] for a in ok) / max(1, len(ok)),
        'avg_len': sum(lens) / max(1, len(lens)),
        'max_len': max(lens, default=0),
        'final_len': len(final),
        'final_free': sum(final),
        'final_largest': max(final, default=0),
    }


def fmt(s):
    return (f"allocs {s['allocs']:6.1f}  failed {s['failed']:6.1f} ({s['fail%']:5.1f}%)  "
            f"avg searched {s['avg_searched']:5.2f} (ok {s['avg_searched_ok']:5.2f})  list len avg {s['avg_len']:5.1f} "
            f"max {s['max_len']:5.1f} final {s['final_len']:5.1f}  "
            f"final free {s['final_free']:5.1f} largest {s['final_largest']:5.1f}")


def averaged(n, args):
    runs = [summary(['-s', str(i), *args]) for i in range(n)]
    return {k: sum(r[k] for r in runs) / n for k in runs[0]}


def bysize(args):
    allocs, _ = run(args)
    q = len(allocs) // 4
    for name, part in (('first 25%', allocs[:q]), ('last 25%', allocs[-q:])):
        cells = []
        for size in sorted({a[0] for a in allocs}):
            got = [a for a in part if a[0] == size]
            if got:
                cells.append(f'{size}:{100 * sum(not a[1] for a in got) / len(got):.0f}%')
        print(f'  failure rate by size, {name} of allocations: ' + ' '.join(cells))


def timeline(args):
    _, lists = run(args)
    points = [p for p in (10, 50, 100, 250, 500, 750, 1000) if p <= len(lists)]
    print('  after op:      ' + ''.join(f'{p:>7d}' for p in points))
    print('  list length:   ' + ''.join(f'{lists[p - 1][0]:>7d}' for p in points))
    print('  largest chunk: ' + ''.join(f'{max(lists[p - 1][1], default=0):>7d}' for p in points))


if __name__ == '__main__':
    a = sys.argv[1:]
    if a and a[0] == '--seeds':
        print('  ' + fmt(averaged(int(a[1]), a[2:])) + f'   (mean of {a[1]} seeds)')
    elif a and a[0] == '--timeline':
        timeline(a[1:])
    elif a and a[0] == '--bysize':
        bysize(a[1:])
    else:
        print('  ' + fmt(summary(a)))
