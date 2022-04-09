# Collect Agent

### Table of contents
1. [Introduction](#introduction)
2. [collectagent](#collectagent)
    1. [Configuration](#configuration)
    2. [Rest API](#restApi)
        1. [List of resources](#listOfResources)
        2. [Examples](#restExamples)

## Introduction <a name="introduction"></a>
DCDB (Data Center Data Base) is a database to collect sensor data associated to data center operation for further analysis. 
The collectagent serves the role of data broker in the DCDB hierarchy.

# collectagent <a name="collectagent"></a>

The collectagent acts as an MQTT broker that subscribes to all topics, and has the task of forwarding the data sent by DCDB Pushers into an Apache Cassandra database.
It will try to batch database inserts, depending on the incoming MQTT packets, in order to optimize performance.


Build it by simply running
```bash
make
```
or alternatively use
```bash
make debug
```
within the `collectagent` directory to build a version which will print additional debug-information during runtime.

You can run a collectagent by executing
```bash
./collectagent path/to/configfile/
```
or run
```bash
./collectagent -h
```
to print the help-section of the collectagent.

The collectagent will check the given file-path for the global configuration file which has to be named `collectagent.conf`.

### Configuration  <a name="configuration"></a>

The configuration specifies various general settings for the collectagent. Please have a look at the provided `config/collectagent.conf` example to get familiar with the file scheme. The different sections and values are explained in the following table:

| Value | Explanation |
|:----- |:----------- |
| global | Wrapper structure for the global values.
| threads | Specifies how many threads should be created for the local thread pool. This is used by the Wintermute plugins as well as the REST API. Default value of threads is 1.
| messageThreads | Number of threads to be used by the MQTT broker.
| messageSlots | Maximum number of connections allowed per MQTT thread.
| cacheInterval | Size (in seconds) of the local cache for each incoming sensor.
| verbosity | Level of detail in the log file. Set to a value between 5 (all log-messages, default) and 0 (only fatal messages). NOTE: level of verbosity for the command-line log can be set via the -v flag independently when invoking the collectagent.
| daemonize | Set to 'true' if the collectagent should run detached as daemon. Default is false.
| tempdir | One can specify a writeable directory where the collectagent can write its temporary and logging files to. Default is the current (' ./ ' ) directory.
| statistics | Boolean. If true, the collectagent will periodically print out statistic about data and insert rates.
| mqttListenAddresss | Address on which the MQTT broker must listen for incoming connections. Default is 127.0.0.1:1883.
| cleaningInterval | Periodic interval (in seconds) over which stale entries are purged from the sensor cache.
| | |
| cassandra | Wrapper for the Cassandra DB settings.
| address | Define address and port of the Cassandra DB server into which sensor data should be stored. Default is 127.0.0.1:9042.
| user | Username for Cassandra authentication. Default is empty.
| password | Password for Cassandra authentication. Default is empty.
| numThreadsIo | Number of I/O threads in the Cassandra driver. Default is 1.
| queueSizeIo | Size of the request queue for each I/O thread. Default is 4096.
| coreConnPerHost | Maximum number of connection per Cassandra database. Default is 1.
| ttl | Default *time to live* to be used for sensor data, if none is provided in the respective metadata. 
| debugLog | Outputs verbose error messages if enabled. Default is false.
| | |
| restAPI | Bundles all values related to the RestAPI. See the corresponding [section](#restApi) for more information on supported functionality.
| address | Define (IP-)address and port where the REST API server should run on. Default is 127.0.0.1:8080.
| certificate | Provide the (path and) file which the HTTPS server should use as certificate.
| privateKey | Provide the (path and) file which should be used as corresponding private key for the certificate. If private key and certificate are stored in the same file one should nevertheless provide the path to the cert-file here again.
| dhFile | Provide the (path and) file where Diffie-Hellman parameters for the key exchange are stored.
| user | This struct is used to define username and password for users of the REST API, along with their respective allowed REST operations (i.e., GET, PUT, POST).
| | |

An example configuration file can be found in the `config/` directory. An explanation of how to deploy Wintermute 
data analytics plugins can be found in the corresponding readme document.

## REST API <a name="restApi"></a>

The collectagent runs an HTTPS server which provides some functionality to be controlled over a RESTful API. 
The API is by default hosted at port 8080 on 127.0.0.1 but the address can be changed in [`collectagent.conf`](#configuration).

An HTTPS request to the collectagent should have the following format: `[GET|PUT|POST] host:port[resource]?[queries]`.
Tables with allowed resources sorted by REST methods can be found below. A query consists of a key-value pair of the format `key=value`. Multiple queries are separated by semicolons (';'). For all requests (except /help) basic authentication credentials must be provided.

### List of resources <a name="listOfResources"></a>

<table>
  <tr>
    <td colspan="2"><b>Resource</b></td>
    <td colspan="2">Description</td>
  </tr>
  <tr>
  	<td>Query</td>
  	<td>Value</td>
  	<td>Opt.</td>
  	<td>Description</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>GET /help</b></td>
    <td colspan="2">Return a cheatsheet of possible REST API endpoints.</td>
  </tr>
  <tr>
  	<td colspan="4">No queries.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>GET /average</b></td>
    <td colspan="2">Get the average of the last readings of a sensor in the local cache. Also allows access to analytics sensors.</td>
  </tr>
  <tr>
  	<td>sensor</td>
  	<td>All sensor names that are available.</td>
  	<td>No</td>
  	<td>Specify a sensor stored in the local cache.</td>
  </tr>
  <tr>
  	<td>interval</td>
  	<td>Number of seconds.</td>
  	<td>Yes</td>
  	<td>Use only readings more recent than (now - interval) for average calculation. Defaults to zero, i.e. all cached sensor readings are included in average calculation.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /quit</b></td>
    <td colspan="2">Exits the collectagent with a user-specified return code.</td>
  </tr>
  <tr>
  	<td>code</td>
  	<td>Return code.</td>
  	<td>Yes</td>
  	<td>Return code to be used when exiting.</td>
  </tr>
</table>

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; Opt. = Optional

### Examples <a name="restExamples"></a>

Two examples for HTTPS requests (authentication credentials not shown):

```bash
GET https://localhost:8080/average?sensor=/clust06/node55/freq1;interval=15
```
```bash
PUT https://localhost:8080/quit?code=11
```