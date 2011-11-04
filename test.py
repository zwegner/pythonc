#!/usr/bin/python3
import os
import shutil
import subprocess
import time

# XXX This file is really stupid!! Just a 5-minute effort on my part...

if not os.path.exists('.temp'):
    os.mkdir('.temp')
for i in ['pythonc.py', 'syntax.py', 'transform.py', 'backend.cpp', 'alloc.h']:
    shutil.copy(i, '.temp')

for i in os.listdir('tests'):
    if i.endswith('.py'):
        shutil.copy('tests/%s' % i, '.temp/test.py')
        start = time.time()
        out_p = subprocess.check_output('./pythonc.py -q test.py', cwd='.temp', shell=True)
        mid = time.time()
        out_c = subprocess.check_output('python3 test.py', cwd='.temp', shell=True)
        end = time.time()
        if out_p != out_c:
            raise RuntimeError('%s mismatched!\nPythonc:\n%s\nCPython:\n%s' % (i,
                out_p.decode(), out_c.decode()))
        time_p = mid - start
        time_c = end - mid
        # XXX timing should factor out translation time
        print('%s: pythonc=%.3fs cpython=%.3fs (%.3fx)' % (i, time_p, time_c, time_p / time_c))
