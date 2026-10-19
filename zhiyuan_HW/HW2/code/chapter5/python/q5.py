#!/usr/bin/env python3
# Q5: what wait() returns in the parent, and in a child with no children.
import os
import sys

rc = os.fork()
if rc == 0:
    try:
        os.wait()
    except ChildProcessError as e:  # Python raises instead of returning -1
        print(f"child  (pid {os.getpid()}): wait() failed: errno {e.errno} ({e.strerror})")
    sys.stdout.flush()
    os._exit(42)
else:
    pid, status = os.wait()
    print(f"parent (pid {os.getpid()}): fork() returned {rc}, wait() returned {pid}")
    if os.WIFEXITED(status):
        print(f"parent: child exit status = {os.WEXITSTATUS(status)}")
