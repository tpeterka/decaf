#!/usr/bin/python

import subprocess
import os
import sys

print >> sys.stderr, "running", os.environ['COBALT_PARTNAME']

pid = subprocess.Popen(['runjob', 
                        '--np', '8', 
                        '--block', os.environ['COBALT_PARTNAME'],
                        '--ranks-per-node', '1', 
                        '--verbose', '2', 
                        '--timeout', '60',
                        '--mapping', './bgq_mapfile', 
                        '--envs', 'LD_LIBRARY_PATH=' + os.environ['LD_LIBRARY_PATH'],
                        '--envs', 'DECAF_PREFIX=' + os.environ['DECAF_PREFIX'], 
                        ':', './prod'])
pid.wait()
