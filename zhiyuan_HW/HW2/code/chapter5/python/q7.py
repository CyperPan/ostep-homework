#!/usr/bin/env python3
# Q7: the child closes standard output, then calls print().
import os
import sys

print("parent: before fork")
sys.stdout.flush()

rc = os.fork()
if rc == 0:
    os.close(1)  # STDOUT_FILENO
    try:
        print("child: can you see me?")
        sys.stdout.flush()
    except OSError as e:
        # stderr (fd 2) is still open, so report what happened there
        print(f"child (stderr): print failed: {e}", file=sys.stderr)
    os._exit(0)  # skip interpreter cleanup, which would try to flush stdout again
else:
    os.waitpid(rc, 0)
    print("parent: stdout still works")
