#!/usr/bin/python3

import os
import subprocess
import sys

def usage():
    print('usage: %s [-O] <input.py> [args...]' % sys.argv[0])
    exit(1)

args = sys.argv[1:]

gcc_flags = ['-g']
if args and args[0] == '-O':
    gcc_flags = ['-O3']
    args = args[1:]

if not args:
    usage()

path = args[0]
base = os.path.splitext(path)[0]

subprocess.check_call(['python3', 'transform.py', path, '%s.cpp' % base])
subprocess.check_call(['c++'] + gcc_flags + ['%s.cpp' % base, '-o', base])
subprocess.check_call(['./%s' % base] + args[1:])
