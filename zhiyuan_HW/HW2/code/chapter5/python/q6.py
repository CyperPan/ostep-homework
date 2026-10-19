#!/usr/bin/env python3
# Q6: waitpid() lets the parent wait for one specific child.
# The first child is slow and the second is fast, but we reap the slow one first.
import os
import sys
import time

slow = os.fork()
if slow == 0:
    time.sleep(2)
    print(f"slow child (pid {os.getpid()}) exiting")
    sys.stdout.flush()
    os._exit(1)

fast = os.fork()
if fast == 0:
    print(f"fast child (pid {os.getpid()}) exiting")
    sys.stdout.flush()
    os._exit(2)

# WNOHANG: poll without blocking
pid, status = os.waitpid(slow, os.WNOHANG)
print(f"parent: waitpid(slow, WNOHANG) returned {pid} (still running)")

pid, status = os.waitpid(slow, 0)
print(f"parent: waitpid(slow) returned {pid}, exit status {os.WEXITSTATUS(status)}")
pid, status = os.waitpid(fast, 0)
print(f"parent: waitpid(fast) returned {pid}, exit status {os.WEXITSTATUS(status)}")
