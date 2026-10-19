#!/usr/bin/env python3
# Q3: make the child print first without calling wait() in the parent.
# The parent blocks on read() from a pipe until the child writes to it.
import os
import sys

r, w = os.pipe()
rc = os.fork()

if rc == 0:
    os.close(r)
    print("hello")
    sys.stdout.flush()
    os.write(w, b"x")  # signal the parent
    os.close(w)
else:
    os.close(w)
    os.read(r, 1)      # blocks until the child has printed
    os.close(r)
    print("goodbye")
