#!/usr/bin/env python3
"""Fraction of valid (non-violating) virtual addresses for segmentation.py.

usage: ./seg_exp.py SEEDS N ARGS...
  runs ../../../../vm-segmentation/segmentation.py ARGS -n N -s S -c for seeds
  0 .. SEEDS-1 and reports how many of the generated addresses were valid.
"""
import os
import subprocess
import sys

SEG = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   '..', '..', '..', '..', 'vm-segmentation', 'segmentation.py')


def valid_fraction(args, seeds, n):
    valid = total = 0
    for s in range(seeds):
        out = subprocess.run([sys.executable, SEG, *args, '-n', str(n), '-s', str(s), '-c'],
                             capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            if line.strip().startswith('VA '):
                total += 1
                valid += 'VALID' in line
    return valid, total


if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    seeds, n, args = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3:]
    v, t = valid_fraction(args, seeds, n)
    print(f'  {" ".join(args)}: {v}/{t} valid = {100 * v / t:.1f}%')
