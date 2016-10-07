#!/usr/bin/python

import subprocess
import os
import sys

print >> sys.stderr, "running   :" , os.environ['COBALT_PARTNAME']
print >> sys.stderr, "NUM_PROCS =" , os.environ['NUM_PROCS']
print >> sys.stderr, "PPN       =" , os.environ['PPN']
print >> sys.stderr, "TIME      =" , os.environ['TIME']
print >> sys.stderr, "EXE1      =" , os.environ['EXE1']

pid = subprocess.Popen(['runjob'          , 
                        '--np'            , os.environ['NUM_PROCS'], 
                        '--block'         , os.environ['COBALT_PARTNAME'],
                        '--ranks-per-node', os.environ['PPN'], 
                        '--verbose'       , '2', 
                        '--timeout'       , os.environ['TIME'],
                        '--mapping'       , './bgq_mapfile', 
                        '--envs'          , 'LD_LIBRARY_PATH=' + os.environ['LD_LIBRARY_PATH'],
                        '--envs'          , 'DECAF_PREFIX=' + os.environ['DECAF_PREFIX'], 
                        ':'               , os.environ['EXE1']
                        ])
pid.wait()
