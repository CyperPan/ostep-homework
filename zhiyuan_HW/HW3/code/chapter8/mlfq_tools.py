#!/usr/bin/env python3
"""Helpers for reading mlfq.py -c output.

  segments  ARGS...          compress the per-ms trace into "start-end  job J  Qp" runs
  share LO HI ARGS...        CPU ticks used by each job in the time window [LO, HI)
"""
import os
import re
import subprocess
import sys

MLFQ = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                    '..', '..', '..', '..', 'cpu-sched-mlfq', 'mlfq.py')
RUN = re.compile(r'\[ time (\d+) \] Run JOB (\d+) at PRIORITY (\d+)')


def trace(args):
    out = subprocess.run([sys.executable, MLFQ, *args, '-c'],
                         capture_output=True, text=True, check=True).stdout
    return out, [tuple(map(int, m.groups())) for m in RUN.finditer(out)]


def segments(args):
    out, ticks = trace(args)
    segs = []
    for t, j, p in ticks:
        if segs and segs[-1][2:] == [j, p] and segs[-1][1] == t:
            segs[-1][1] = t + 1
        else:
            segs.append([t, t + 1, j, p])
    for line in out.splitlines():
        if line.strip().startswith(('Job ', 'Avg')) and 'response' in line:
            print('   ', line.strip())
    for s, e, j, p in segs:
        print(f'  {s:4d}-{e:<4d} job {j}  Q{p}')


def share(lo, hi, args):
    _, ticks = trace(args)
    cnt = {}
    for t, j, _ in ticks:
        if lo <= t < hi:
            cnt[j] = cnt.get(j, 0) + 1
    print(f'  CPU share in [{lo},{hi}): ' + ', '.join(
        f'job {j}: {c} ms ({100 * c / (hi - lo):.1f}%)' for j, c in sorted(cnt.items())))


if __name__ == '__main__':
    if len(sys.argv) < 2 or sys.argv[1] not in ('segments', 'share'):
        sys.exit(__doc__)
    if sys.argv[1] == 'segments':
        segments(sys.argv[2:])
    else:
        share(int(sys.argv[2]), int(sys.argv[3]), sys.argv[4:])
