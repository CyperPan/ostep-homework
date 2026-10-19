#!/usr/bin/env python3
# Q1: a variable set before fork() is copied into the child.
import os
import sys

x = 100
print(f"before fork: x = {x} (pid {os.getpid()})")
sys.stdout.flush()  # otherwise the buffered line may be copied into the child

try:
    rc = os.fork()
except OSError as e:
    print(f"fork failed: {e}", file=sys.stderr)
    sys.exit(1)

if rc == 0:
    print(f"child:  x = {x} (inherited)")
    x = 200
    print(f"child:  set x = {x}")
else:
    x = 300
    print(f"parent: set x = {x}")
    os.waitpid(rc, 0)
    print(f"parent: after child exits, x = {x}")
