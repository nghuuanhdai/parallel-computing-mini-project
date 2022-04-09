# Grafana Server

### Table of contents
1. [Introduction](#introduction)
2. [grafanaserver](#grafanaserver)
	1. [Configuration](#configuration)
	2. [Rest API](#restApi)
		1. [List of resources](#listOfResources)
		2. [Examples](#restExamples)
3. [DCDB Grafana Plugin](#grafanaPlugin)

## Introduction <a name="introduction"></a>
DCDB (Data Center Data Base) is a database to collect various (sensor-)values of a data center for further analysis. 
The grafanaserver is a REST HTTPS server that accepts and processes requests from the DCDB data source plugin of the Grafana web interface. It essentially resolves Grafana queries for visualizing timeserie data of systems monitored with DCDB.

# grafanaserver <a name="grafanaserver"></a>

This is a general REST HTTPS server that processes requests from a Grafana web interface frontend. All requests are expected to originate from web panels based on a custom Grafana DCDB data source plugin. Of course, grafanaserver can still be used to serve generic REST HTTPS requests, though it would be far more convenient to use the REST API servers built in the collectagent or the dcdbpusher for this specific purpose.

Build it by simply running
```bash
make
```
or alternatively use
```bash
make debug
```
within the `grafanaserver` directory to build a version which will print additional debug-information during runtime.

You can run grafanaserver by executing
```bash
./grafanaserver path/to/configfile/
```
or run
```bash
./grafanaserver -h
```
to print the help-section of grafanaserver.

Grafanaserver will check the given file-path for the global configuration file which has to be named `grafana.conf`.

### Configuration  <a name="configuration"></a>

The configuration specifies various settings for grafanaserver in general. Please have a look at the provided `config/grafana.conf` example to get familiar with the file scheme. The different sections and values are explained in the following table:

| Value | Explanation |
|:----- |:----------- |
| global | Wrapper structure for the global values.
| threads | Specify how many threads should be created to handle the requests. Default value of threads is 1.
| verbosity | Level of detail in the logfile (dcdb.log). Set to a value between 5 (all log-messages, default) and 0 (only fatal messages). NOTE: level of verbosity for the command-line log can be set via the -v flag independently when invoking grafanaserver.
| daemonize | Set to 'true' if grafanaserver should run detached as daemon. Default is false.
| tempdir | One can specify a writeable directory where grafanaserver can write its temporary and logging files to. Default is the current (' ./ ' ) directory.
| | |
| cassandra | Wrapper for the Cassandra DB settings.
| address | Define address and port of the Cassandra DB server which stores the time series data. Default is 127.0.0.1:9042.
| user | Username for Cassandra authentication. Default is empty.
| password | Password for Cassandra authentication. Default is empty.
| numThreadsIo | Number of I/O threads in the Cassandra driver. Default is 1.
| queueSizeIo | Size of the request queue for each I/O thread. Default is 4096.
| coreConnPerHost | Maximum number of connection per Cassandra database. Default is 1.
| debugLog | Outputs verbose error messages if enabled. Default is false.
| | |
| hierarchy | Wrapper for the definition of he hierarchical structure of the sensor tree.
| regex | Regular expression representing the full hierarchy of sensor names, where each level is separated by a provided "separator" character. If left empty, '/' are considered default separators of the different levels composing the name (e.g., /system/rack/chassis/node/sensor). If the published sensor names are different from their corresponding MQTT topics, the provided regex needs to reflect it. For example, the regex "mySystem.,rack\\d{2}.,chassis\\d{2}.,server\\d{2}" could represent a custom sensor name such as "mySystem.rack01.chassis03.server10" (see the documentation of the Sensor Navigator for more details). However, it is highly encouraged to leave the default configuration settings, whereas published sensor names exactly match their correspondant MQTT topics.
| separator | Specifies a custom separator for the different levels composing the sensor name. Default is emtpy.
| filter | Regular expression to filter the set of nodes used to build the sensor navigator. Default is none.
| smootherRegex | Regular expression to identify the sensor operations that represent smoothers. The latter need to contain the sampling interval of the smoother, in seconds, in their names. Default is empty. 
| | |
| restAPI | Bundles all values related to the RestAPI. See the corresponding [section](#restApi) for more information on supported functionality.
| address | Define (IP-)address and port where the REST API server should run on. Default is 127.0.0.1:8081.
| certificate | Provide the (path and) file which the HTTPS server should use as certificate.
| privateKey | Provide the (path and) file which should be used as corresponding private key for the certificate. If private key and certificate are stored in the same file one should nevertheless provide the path to the cert-file here again.
| dhFile | Provide the (path and) file where Diffie-Hellman parameters for the key exchange are stored.
| user | This struct is used to define username and password for users of the REST API, along with their respective allowed REST operations (i.e., GET, PUT, POST).
| | |

An example configuration file can be found in the `config/` directory.


## REST API <a name="restApi"></a>

Grafanaserver runs a HTTPS server which provides some functionality to be controlled over a RESTful API. The API is by default hosted at port 8081 on 127.0.0.1 but the address can be changed in [`grafana.conf`](#configuration).

Grafanaserver is designed to expect requests from a Grafana web interface, but it is of course possible to send generic HTTPS requests from different tools a well (e.g., curl). In this regard, a HTTPS request to grafanaserver should have the following format: `[GET|PUT|POST] host:port[resource]?[queries]`.
Tables with allowed resources sorted by REST methods can be found below. A query consists of a key-value pair of the format `key=value`. Multiple queries are separated by semicolons(';'). For all requests, basic authentication credentials must be provided.

### List of Resources <a name="listOfResources"></a>

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
<td colspan="2"><b>GET /</b></td>
<td colspan="2">Dummy request to define a data source in Grafana.</td>
</tr>
<tr>
<td colspan="4">No queries.</td>
</tr>
</table>

<table>
<tr>
<td colspan="2"><b>POST /levels</b></td>
<td colspan="2">Returns the maximum depth of the sensor tree.</td>
</tr>
<tr>
<td colspan="4">No queries.</td>
</tr>
</table>

<table>
<tr>
<td colspan="2"><b>POST /search</b></td>
<td colspan="2">Returns a list of metrics from a specific parent node (level).</td>
</tr>
<tr>
<td>node</td>
<td>node name</td>
<td>Yes</td>
<td>Specifiy the parent node name (level) from which all metrics are listed.</td>
</tr>
<tr>
<td>sensors</td>
<td>"true"</td>
<td>Yes</td>
<td>Returns all sensors belonging to the specified parent node.</td>
</tr>
</table>

<table>
<tr>
<td colspan="2"><b>POST /query</b></td>
<td colspan="2">Returns the timeserie data associated with a set of sensors and time range. Note that the Grafana web interface encapsulates the information related to sensors and time range within the body of the request in a JSON format. If you want to use this endpoint from outside Grafana, make sure to format the payload appropriately (see the Examples section). </td>
</tr>
<tr>
<td colspan="4">No queries.</td>
</tr>
</table>

<table>
<tr>
<td colspan="2"><b>PUT /navigator</b></td>
<td colspan="2">Rebuilds the internal sensor navigator.</td>
</tr>
<tr>
<td colspan="4">No queries.</td>
</tr>
</table>

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; Opt. = Optional

### Examples <a name="restExamples"></a>

Here are some examples for HTTPS requests outside of Grafana (authentication credentials not shown):

```bash
GET https://localhost:8081/
```
```bash
POST https://localhost:8081/levels
```
```bash
POST https://localhost:8081/search?node=/my/node/name;sensors=true
```
```bash
POST https://localhost:8081/query
```
A more concrete example of usage of the query endpoint with curl:

```bash
curl -k -v -u user:password -d '{"targets": [{"target":"/my/node/name/sensorname"}], "range":{"from":"2019-01-03T08:33:03", "to":"2019-01-05T15:48:36"}}' -H "Content-Type: application/json" -X POST "https://127.0.0.1:8081/query"
```

# DCDB Grafana Data Source Plugin <a name ="plugins"></a>

In order to visualize data collected by DCDB with Grafana we need...Grafana itself! 
You can find detailed instructions on how to download, install and configure Grafana at the official [website](https://grafana.com/) of GrafanaLabs. Please note that the DCDB data source plugin supports Grafana version 6.2.2.

To install the data source plugin you can manually copy and paste the entire folder `grafana-dcdb-datasource`  with all its content into the Grafana plugins directory, that is by default:

On Linux:
```bash
/var/lib/grafana/plugins/
```
On macOS:
```bash
/usr/local/var/lib/grafana/plugins/
```
Once done, restart Grafana so it can load the DCDB data source plugin. If everything worked well,  you should be able to see the DCDB plugin from the list of available data sources. To get started with a simple test setup of the data source plugin, please refer to the following configuration example:

- URL: https://127.0.0.1:8081 (default URL from the `grafana.conf` file)
- Access: Server (Default).
- Check "Basic Auth" and "Skip TLS Verify" under the "Auth" table.
- Provide your username and password as defined in the `grafana.conf` file.

Enjoy visualising data!
