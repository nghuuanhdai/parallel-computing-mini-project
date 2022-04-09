# DCDB Tools

### Table of contents
1. [Introduction](#introduction)
2. [The dcdbconfig Tool](#dcdbconfig)
    1. [Examples](#dcdbconfigexamples)
3. [The dcdbquery Tool](#dcdbquery)
    1. [Examples](#dcdbqueryexamples)
4. [The dcdbslurmjob Tool](#dcdbslurmjob)
    1. [Examples](#dcdbslurmjobexamples)
5. [Other Tools](#othertools)


# Introduction <a name="introduction"></a>
DCDB includes a series of tools to access stored sensor data as well as perform various types of actions. In this 
document we review the main tools supplied with DCDB and their capabilities, along with several usage examples. 

All
of the DCDB tools presented in the following interact directly with an Apache Cassandra instance, and do not require
communication with Pusher or Collect Agent components. Please remember that, to use any of the tools in the following, 
the shell environment must be set properly using the supplied *dcdb.bash* script in the DCDB install directory:

```bash
$ source install/dcdb.bash
```

# The dcdbconfig Tool <a name="dcdbconfig"></a>

The *dcdbconfig* tool is used to perform database and metadata management actions. Its syntax is the following:

```bash
dcdbconfig [-h <host>] <command> [<arguments> ... ]
``` 

The *-h* argument specifies the hostname associated to the Apache Cassandra instance to which dcdbconfig should connect.
*\<command\>*, instead, indicates different types of actions and can be one of the following:

* *job*: allows to access actions that are job-related, e.g. to list the jobs stored in the database, visualize the ones
that are running, or show the details for a certain job ID. The insertion of the job data itself is performed via the
*dcdbslurmjob* tool.
* *sensor*: allows to perform several actions at the sensor level. For example, users can visualize and set the metadata 
associated to a certain sensor name, list available sensors, or publish new ones. 
* *db*: allows to perform certain database management actions, such as truncating obsolete data or manually inserting new data.
* *help*: visualizes a help message. More information for each of the commands above can be visualized by typing *help*
followed by the name of the specific command (e.g., *dcdbconfig help sensor*).

For each of the commands above a series of actions can be performed, which are specified with the subsequent arguments. 
Some actions may further require additional arguments (e.g., the name of the sensor that the user wishes to visualize). 

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; A *published* sensor in DCDB terminology is one that has its metadata correctly
populated in the Cassandra database. This process is usually handled transparently by Pushers that have the *-a* option
enabled. Publishing is not strictly necessary to store and query the sensor data itself.

## Examples <a name="dcdbconfigexamples"></a>

Visualizing the list of published sensors in the database:

 ```bash
 dcdbconfig -h 127.0.0.1 sensor listpublic
 ``` 
 
Sample output, showing the mapping between the *public* sensor names (left) and their internal MQTT *topics*:

```
dcdbconfig 0.4.135-gb57cedf (libdcdb 0.4.135-gb57cedf)

/test/node1/AnonPages : /test/node1/AnonPages
/test/node1/Hugepagesize : /test/node1/Hugepagesize
/test/node1/MemFree : /test/node1/MemFree
/test/node1/col_nice : /test/node1/col_nice
/test/node1/col_system : /test/node1/col_system
/test/node1/col_user : /test/node1/col_user
 ``` 
 
---
 
Showing all available information for a specific sensor:
 
  ```bash
  dcdbconfig -h 127.0.0.1 sensor show /test/node1/AnonPages 
  ``` 
  
Sample output:
 
```
dcdbconfig 0.4.135-gb57cedf (libdcdb 0.4.135-gb57cedf)

Details for public sensor /test/node1/AnonPages:
Pattern: /test/node1/AnonPages
Unit: aaa
Scaling factor: 1
Operations: -avg10,-avg300,-avg3600
Interval: 500000000
TTL: 0
Sensor Properties: 
``` 
 
---
 
Listing all jobs stored in the database that are currently running:
 
  ```bash
  dcdbconfig -h 127.0.0.1 job running SYSTEM1
  ``` 
  
Sample output:
 
```
dcdbconfig 0.4.160-g48ce9d0 (libdcdb 0.4.161-gf25fe15)

Domain ID, Job ID, User ID, Start Time, End Time, #Nodes
SYSTEM1,800475,di726bwe,1597992340671657000,0,128
SYSTEM1,800499,ga526ppd,1597992340522786000,0,16
``` 
 
---
 
Showing the information associated to a specific job ID:
 
```bash
dcdbconfig -h 127.0.0.1 job show 800475 SYSTEM1
``` 
  
Sample output:
 
 ```
dcdbconfig 0.4.160-g48ce9d0 (libdcdb 0.4.161-gf25fe15)

Domain ID:  SYSTEM1
Job ID: 800475
User ID: di726bwe
Start Time: 2020-07-21T06:47:30.854049000 (1595314022854049000)
End Time: 1970-01-01T01:00:00 (3600000000000)
Node List: /mpp2/r03/c04/s02/, /mpp2/r04/c04/s04/, /mpp2/r05/c05/s02/
 ``` 

# The dcdbquery Tool <a name="dcdbquery"></a>

The *dcdbquery* tool is used to perform database queries and fetch sensor data. Its syntax is the following:

```bash
dcdbquery -h <host> <Sensor 1> [<Sensor 2> ...] <Start> <End>
``` 

Like in dcdbconfig, the *-h* argument specifies the hostname associated to an Apache Cassandra instance. Then, the tool
accepts a list of sensor names to be queried (represented by the *\<Sensor\>* arguments) and a time range for the query 
(represented by the *\<Start\>* and *\<End\>* arguments). The range of the query can either be supplied with a pair of 64-bit timestamps
(expressed in nanoseconds), or with two relative timestamps (e.g., *now-1h* as *\<Start\>* and *now* as *\<End\>*). Moreover, a
series of operations can be automatically performed over the returned set of sensor values. Operations can be specified
like in the following:

```bash
dcdbquery -h <host> operation(<Sensor 1>) [operation(<Sensor 2>) ...] <Start> <End>
``` 

The currently supported operations are *delta*, *delta_t*, *derivative* and *integral*.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp;&ensp; Quoting of the dcdbquery command in the shell might be required to ensure proper parsing
 when using sensor operations.
 
 
> NOTE 2 &ensp;&ensp;&ensp;&ensp;&ensp; When using the *-j* option to query sensors for specific jobs, the *\<Start\>* and
*\<End\>* arguments are omitted, since they are inferred automatically from the job's start and end times.

## Examples <a name="dcdbqueryexamples"></a>

Querying sensor data in the last hour for a power sensor:

```bash
dcdbquery -h 127.0.0.1 /i01/r01/c01/s09/PKG_POWER now-1h now
``` 

Sample output:

```
dcdbquery 0.4.136-g233b476 (libdcdb 0.4.136-g233b476)

Sensor,Time,Value
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:28:00.000233000,135
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:30:00.000403000,198
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:32:00.000410000,190
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:34:00.000530000,84
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:36:00.002756000,259
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:38:00.000873000,239
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:40:00.000497000,96
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:42:00.000813000,256
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:44:00.002443000,256
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:46:00.002659000,261
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:48:00.001602000,259
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:50:00.001135000,260
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:52:00.000684000,264
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:54:00.001350000,261
/i01/r01/c01/s09/PKG_POWER,2020-08-04T09:56:00.001860000,255
``` 

---

Visualizing the delta_t for the successive readings of a DRAM power sensor:

```bash
dcdbquery -h 127.0.0.1 "delta_t(/i01/r01/c01/s09/DRAM_POWER)" now-2h now
``` 

Sample output:

```
dcdbquery 0.4.136-g233b476 (libdcdb 0.4.136-g233b476)

Sensor,Time,Delta_t
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:14:00.000455000,
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:16:00.000426000,119999971000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:18:00.000420000,119999994000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:20:00.000405000,119999985000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:22:00.000385000,119999980000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:24:00.000403000,120000018000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:26:00.000458000,120000055000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:28:00.000438000,119999980000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:30:00.000403000,119999965000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:32:00.000430000,120000027000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:34:00.000467000,120000037000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:36:00.000491000,120000024000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:38:00.000664000,120000173000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:40:00.000649000,119999985000
/i01/r01/c01/s09/DRAM_POWER,2020-08-04T09:42:00.001142000,120000493000
``` 

---

Visualizing an unpublished sensor containing job-aggregated performance metrics:

```bash
dcdbquery -h 127.0.0.1 /job800745/FLOPS now-20m now
``` 

Sample output:

```
dcdbquery 0.4.136-g233b476 (libdcdb 0.4.136-g233b476)

Sensor,Time,Value
/job800745/FLOPS,2020-08-04T09:58:00.000138000,6509354081
/job800745/FLOPS,2020-08-04T10:00:00.000364000,32726063414
/job800745/FLOPS,2020-08-04T10:02:00.000109000,32114478744
/job800745/FLOPS,2020-08-04T10:04:00.000376000,32899551609
/job800745/FLOPS,2020-08-04T10:06:00.000384000,32769676466
/job800745/FLOPS,2020-08-04T10:08:00.000368000,31317069762
/job800745/FLOPS,2020-08-04T10:10:00.000460000,32422604906
/job800745/FLOPS,2020-08-04T10:12:00.000423000,32143846805
/job800745/FLOPS,2020-08-04T10:14:00.000709000,32193801528
/job800745/FLOPS,2020-08-04T10:16:00.000722000,32887149173
``` 

---

Visualizing all of the *DRAM_POWER* sensor readings for the compute nodes on which job 781742 was running, throughout its runtime:

```bash
dcdbquery -h 127.0.0.1 -j 781742 DRAM_POWER
``` 

Sample output:

```
dcdbquery 0.4.144-g777a70c (libdcdb 0.4.144-g777a70c)

Sensor,Time,Value
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:28:00.001173000,22
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:30:00.000168000,19
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:32:00.000867000,20
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:34:00.000446000,20
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:36:00.000098000,20
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:38:00.000814000,21
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:40:00.000843000,21
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:42:00.000090000,22
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:44:00.000408000,21
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:46:00.000086000,19
/i01/r01/c01/s11/DRAM_POWER,2020-08-10T07:48:00.000440000,20
Sensor,Time,Value
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:28:00.001173000,20
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:30:00.000168000,20
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:32:00.000867000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:34:00.000446000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:36:00.000098000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:38:00.000814000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:40:00.000843000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:42:00.000090000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:44:00.000408000,20
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:46:00.000086000,21
/i01/r01/c01/s12/DRAM_POWER,2020-08-10T07:48:00.000440000,20
``` 

# The dcdbslurmjob Tool <a name="dcdbconfig"></a>

The *dcdbslurmjob* tool allows to manually insert job information in the Apache Cassandra database. Its syntax is the
following:

```bash
  dcdbslurmjob [-b <host>] [-n <nodelist>] [-j <jobid>] [-i <userid>] start|stop
``` 

The job's information in terms of job ID, user ID and node list can be supplied via the *-j*, *-i* and *-n* arguments
respectively, the last of which being a comma-separated list. A timestamp (for the start or end of the job) can be 
also optionally supplied with the *-t* argument: if not provided, this will default to the current time. 

The dcdbslurmjob tool accepts two main commands, which are *start* and *stop*: the *start* command is invoked whenever
a new job must be inserted into the database (either at submission time, or at start time), whereas the *stop* command
is invoked to update the end time of an existing job with a specific ID. 

There are two modes of transport for the dcdbslurmjob tool: if the *-b* argument is specified, the tool will attempt to
connect to the MQTT broker at the specified host, and transmit the job's information as an MQTT packet. On the other hand,
if the *-c* argument is used (optionally with the *-u* and *-p* arguments to specify a username and password respectively), 
the tool will attempt to connect directly to the Apache Cassandra instance at the specified host.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp;&ensp; While the dcdbslurmjob tool was designed primarily for HPC jobs orchestrated by
the SLURM resource manager, it is general enough to be applied to other environments as well.

## Examples <a name="dcdbslurmjobexamples"></a>

Inserting a new job into the database for the first time, by connecting directly to Cassandra: 

```bash
dcdbslurmjob -c 127.0.0.1 -j 675222 -i po977yfg -n "/mpp3/r03/c05/s06/,/mpp3/r03/c05/s02/" start
``` 

Sample output:

```
dcdbslurmjob 0.4.135-gb57cedf

Connected to Cassandra server 127.0.0.1:9042
JOBID    = 675222
USER     = po977yfg
START    = 1596543564693158000
NODELIST = /mpp3/r03/c05/s06/,/mpp3/r03/c05/s02/
SUBST    = 
NODES    = /mpp3/r03/c05/s06/ /mpp3/r03/c05/s02/
``` 

---

Stopping the same job and setting its end timestamp:

```bash
dcdbslurmjob -c 127.0.0.1 -j 675222 stop
``` 

Sample output:

```
dcdbslurmjob 0.4.135-gb57cedf

Connected to Cassandra server 127.0.0.1:9042
JOBID = 675222
STOP  = 1596543717613674000
``` 

# Other Tools <a name="othertools"></a>

Other tools, which are not covered in this document, are supplied with DCDB for specific use cases. These
are the following:

* *dcdbunitconv*: a simple unit conversion tool for sensor data values.
* *dcdbcsvimport*: allows to convert and insert CSV sensor data into an Apache Cassandra database.
