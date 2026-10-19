#!/usr/bin/env python3
# Q2: parent and child share a file descriptor opened before fork().
import os
import sys

LINES = int(sys.argv[1]) if len(sys.argv) > 1 else 5

fd = os.open("q2.output", os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o600)
rc = os.fork()

who = "child " if rc == 0 else "parent"
for i in range(LINES):
    os.write(fd, f"{who} line {i}\n".encode())

if rc > 0:
    os.waitpid(rc, 0)
    print(f"file offset after both wrote: {os.lseek(fd, 0, os.SEEK_CUR)}")
os.close(fd)
