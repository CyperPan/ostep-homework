#!/usr/bin/env python3
# Q8: two children connected by a pipe, like the shell command "ls -1 / | wc -l".
import os

r, w = os.pipe()

writer = os.fork()
if writer == 0:
    os.dup2(w, 1)  # stdout -> write end of pipe
    os.close(r)
    os.close(w)
    os.execlp("ls", "ls", "-1", "/")

reader = os.fork()
if reader == 0:
    os.dup2(r, 0)  # stdin <- read end of pipe
    os.close(r)
    os.close(w)
    os.execlp("wc", "wc", "-l")

# the parent must close both ends, or wc never sees end-of-file
os.close(r)
os.close(w)
os.waitpid(writer, 0)
os.waitpid(reader, 0)
