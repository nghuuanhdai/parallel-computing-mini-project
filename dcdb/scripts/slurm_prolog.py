# Author: Micha Mueller 
# Date: 26.01.2019
# This script is intended to be run at PrologSlurmctld only!

# Grab available job data and insert it into the database. We use the current
# system time as job start time.
#TODO Logging?

import ctypes
import os
import time
from ctypes import cdll
from subprocess import Popen, Pipe

#TODO make lib-path and daba host configurable?
dcdbLib = cdll.LoadLibrary('./lib/libdcdb.so')

# Get current timestamp in ns since Unix epoch
startTime = int(round(time.time() * 1000 * 1000))
# alternative which requires python 3.7
#endTime = time.time_ns()

# Get JobId from environment
jobId = int(os.environ['SLURM_JOB_ID'])

# Get UserId from environment
userId = int(os.environ['SLURM_JOB_UID'])

# Get allocated nodes from environment
temp_list = []
proc = Popen(["scontrol", "show", "hostnames", stdout=PIPE)
for line in iter(proc.stdout.readline, b''):
    temp_list.append(line)
    
nodes = (ctypes.c_char_p * len(temp_list))()
nodes[:] = temp_list

# Preparation to insert job data
conn = dcdbLib.connectToDatabase(bytes("localhost", encoding="ascii"), 9042)
if not conn:
    print("Connection could not be established!")
    exit()

jds = dcdbLib.constructJobDataStore(conn)
if not jds:
    print("JobDataStore could not be constructed!")
    dcdbLib.disconnectFromDatabase(conn)
    exit()

# Insert job data
ret = dcdbLib.insertJobStart(jds, jobId, userId, startTime, nodes, len(temp_list)))

# Hopefully the DCDB_C_RESULT enum is not changed
if ret == 6:
    print("Tried to insert bad parameters")
elif ret != 0:
    print("Unknown error occured")

# Clean up
ret = dcdbLib.destructJobDataStore(jds)
ret = dcdbLib.disconnectFromDatabase(conn)
