#!/usr/bin/python

import os
import subprocess
import sys

if len(sys.argv) < 2:
    print('usage: %s <input.py> [args...]' % sys.argv[0])
    sys.exit(1)

path = sys.argv[1]
base = os.path.splitext(path)[0]

subprocess.check_call(['python', 'transform.py', path, '%s.cpp' % base])
subprocess.check_call(['c++', '-g', '%s.cpp' % base, '-o', base])
subprocess.check_call(['./%s' % base] + sys.argv[2:])
