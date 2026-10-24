#!/usr/bin/env python3
"""Statistics on top of paging-linear-translate.py (OSTEP chapter 18).

  paging_stats.py size  ARGS...          number of page-table entries (and valid ones)
  paging_stats.py valid U [--seeds N]    mean fraction of valid entries for -u U
"""
import os
import subprocess
import sys

SIM = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   '../../../../vm-paging/paging-linear-translate.py')


def table(args):
    out = subprocess.run([sys.executable, SIM, *args, '-n', '0'],
                         capture_output=True, text=True).stdout
    body = out.split('Page Table (from entry 0 down to the max size)')[1]
    return [l.split()[-1] for l in body.split('Virtual Address Trace')[0].split('\n') if '0x' in l]


if __name__ == '__main__':
    if len(sys.argv) < 3 or sys.argv[1] not in ('size', 'valid'):
        sys.exit(__doc__)
    if sys.argv[1] == 'size':
        pt = table(sys.argv[2:])
        valid = sum(1 for e in pt if int(e, 16) & 0x80000000)
        print(f'  {" ".join(sys.argv[2:])}: {len(pt)} entries x 4 bytes = {len(pt) * 4} bytes, {valid} valid')
    else:
        u, seeds = sys.argv[2], int(sys.argv[4]) if len(sys.argv) > 4 else 200
        n = v = 0
        for s in range(seeds):
            pt = table(['-P', '1k', '-a', '16k', '-p', '32k', '-u', u, '-s', str(s)])
            n += len(pt)
            v += sum(1 for e in pt if int(e, 16) & 0x80000000)
        print(f'  -u {u}: {v / n * 100:.1f}% of entries valid (over {seeds} seeds)')
