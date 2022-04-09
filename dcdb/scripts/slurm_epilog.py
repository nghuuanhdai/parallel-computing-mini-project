# Author: Micha Mueller 
# Date: 26.01.2019
# This script is intended to be run at EpilogSlurmctld only!

# The current system time is used as job end time. We update the job record's
# end time, which was previously inserted in the job prolog.
#TODO Logging?

import ctypes
import os
import time
from ctypes import cdll

#TODO make lib-path and daba host configurable?
dcdbLib = cdll.LoadLibrary('./lib/libdcdb.so')

# Get current timestamp in ns since Unix epoch
endTime = int(round(time.time() * 1000 * 1000))
# alternative which requires python 3.7
#endTime = time.time_ns()

# Get JobId from environment
jobId = int(os.environ['SLURM_JOB_ID'])

# Preparation to insert endTime
conn = dcdbLib.connectToDatabase(bytes("localhost", encoding="ascii"), 9042)
if not conn:
    print("Connection could not be established!")
    exit()

jds = dcdbLib.constructJobDataStore(conn)
if not jds:
    print("JobDataStore could not be constructed!")
    dcdbLib.disconnectFromDatabase(conn)
    exit()

# Update record with end time
ret = dcdbLib.updateJobEnd(jds, jobId, endTime)

# Hopefully the DCDB_C_RESULT enum is not changed
if ret == 6:
    print("JobId not present")
elif ret != 0:
    print("Unknown error occured")

# Clean up
ret = dcdbLib.destructJobDataStore(jds)
ret = dcdbLib.disconnectFromDatabase(conn)
