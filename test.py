import os
import shutil
import subprocess

# XXX This file is really stupid!! Just a 5-minute effort on my part...

if not os.path.exists('.temp'):
    os.mkdir('.temp')
for i in ['pythonc.py', 'syntax.py', 'transform.py', 'backend.cpp', 'alloc.h']:
    shutil.copy(i, '.temp')

for i in os.listdir('tests'):
    shutil.copy('tests/%s' % i, '.temp/test.py')
    subprocess.check_call('./pythonc.py test.py', cwd='.temp', shell=True)
