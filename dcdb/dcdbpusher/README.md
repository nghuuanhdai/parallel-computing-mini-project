# DCDB Pusher

### Table of contents
1. [Introduction](#introduction)
2. [dcdbpusher](#dcdbpusher)
    1. [Global Configuration](#globalConfiguration)
    2. [Rest API](#restApi)
        1. [List of resources](#listOfResources)
        2. [Examples](#restExamples)
    3. [MQTT topic](#mqttTopic)
3. [Plugins](#plugins)
    1. [IPMI](#ipmi)
    2. [Perf-event](#perf)
        1. [type and config](#perfTypeConfig)
        2. [Footnotes](#perfFootnotes)
    3. [SNMP](#snmp)
    4. [SysFS](#sysfs)
    5. [PDU](#pdu)
    6. [BACnet](#bacnet)
    7. [OPA](#opa)
        1. [counterData](#opaCounterData)
    8. [ProcFS](#procfs)
    9. [Caliper](#caliper)
    10. [Metadata Management](#metadataManagement)
    11. [Writing own plugins](#writingOwnPlugins)

## Introduction <a name="introduction"></a>
DCDB (DataCenter DataBase) is a database to collect various (sensor-)values of a datacenter for further analysis.
Harvesting of the data is task of the dcdbpusher.

# dcdbpusher <a name="dcdbpusher"></a>

This is a general MQTT pusher which sends values of various sensors to the DCDB-database.
It ships with plugins for BACnet, IPMI, PDU (proprietary Power Delivery Unit, but could be used as XML plugin), perfcounter, SNMP and sysFS, among others.

Build it by simply running
```bash
make
```
or alternatively use
```bash
make debug
```
within the `dcdbpusher` directory to build a version which will print additional debug-information during runtime.

The logic for the various sensors is encapsulated into plugins (shared dynamic libraries; the makefile will take care of compiling them for you). The dcdbpusher will dynamically open the libraries if they are specified in the [global configuration](#GC) file. Vice versa, if selected sensor-functionality, e.g. sysFS is not specified, the corresponding shared library libdcdbplugin_sysfs.so does not have to be present. 

You can run dcdbpusher by executing
```bash
./dcdbpusher path/to/configfile/
```
or run
```bash
./dcdbpusher -h
```
to print the help-section of dcdbpusher.

Dcdbpusher will check the given file-path for the global configuration file which has to be named `dcdbpusher.conf`.

### Global Configuration  <a name="globalConfiguration"></a>

The global configuration specifies various settings for dcdbpusher in general, e.g. which plugins should be loaded etc.
Please have a look at the provided `config/dcdbpusher.conf` example to get familiar with the file scheme. The example also forms a good starting point for writing a custom `dcdbpusher.conf`. The different sections and values are explained in the following table:

| Value | Explanation |
|:----- |:----------- |
| global | Wrapper structure for the global values.
| mqttBroker | Define address and port of the MQTT-broker which collects the messages (sensor values) send by dcdbpusher.
| mqttprefix | To not rewrite a full MQTT-topic for every sensor one can specify here a consistent prefix.
| sensorpattern | pattern used to perform automatic sensor name publishing. See the corresponding [section](#autopublish) for more information.
| threads | Specify how many threads should be created to handle the sensors asynchronously, as well as the Wintermute plugins. Default value of threads is 1. Note that the MQTT Pusher always starts an extra thread. So the actual number of started threads is always one more than defined here. Specifying not enough threads can result in a delay for some sensors until they are read.
| maxMsgNum | To avoid publishing too many MQTT messages at once you can define here a maximum count of values that are published in one turn. After reaching this limit the MQTT Pusher will be forced to sleep for a short time before continuing.
|maxInflightMsgNum|Maximum number of messages that can be "inflight". This is a MQTT term and should match the broker's setting. Set to 0 for unlimited.
|maxQueuedMsgNum|Maximum number of MQTT messages (including "inflight") that should be queued. This is to limit the amount of memory that is used for buffering. Set to 0 for unlimited.
| verbosity | Level of detail in the logfile (dcdb.log). Set to a value between 5 (all log-messages, default) and 0 (only fatal messages). NOTE: level of verbosity for the command-line log can be set via the -v flag independently when invoking dcdbpusher.
| daemonize | Set to 'true' if dcdbpusher should run detached as daemon. Default is false.
| tempdir | One can specify a writeable directory where dcdbpusher can write its temporary and logging files to. Default is the current (' ./ ' ) directory.
| cacheInterval | Define a time interval in seconds. The last sensor readings within this time interval will be kept. This value can be overwritten by plugins.
| | |
| restAPI | Bundles all values related to the RestAPI. See the corresponding [section](#restApi) for more information on supported functionality.
| address | Define (IP-)address and port where the REST API server should run on.
| certificate | Provide the (path and) file which the HTTPS server should use as certificate.
| privateKey | Provide the (path and) file which should be used as corresponding private key for the certificate. If private key and certificate are stored in the same file one should nevertheless provide the path to the cert-file here again.
| dhFile | Provide the (path and) file where Diffie-Hellman parameters for the key exchange are stored.
| user | This struct is used to define username and password for users of the REST API, along with their respective allowed REST operations (i.e., GET, PUT, POST).
| | |
| plugins | In this section one can specify the plugins which should be used.
| plugin _name_ | The plugin name is used to build the corresponding lib-name (e.g. sysfs --> libdcdbplugin_sysfs.1.0)
| path | Specify the path where the plugin (the shared library) is located. If left empty, dcdbpusher will look in the default lib-directories (usr/lib and friends) for the plugin-file.
| config | One can specify a separate config-file (including path to it) for the plugin to use. If not specified, dcdbpusher will look up pluginName.conf (e.g. sysfs.conf) in the same directory where dcdbpusher.conf is located.
| | |

Formats of the other sensor-specific config-files are explained in the corresponding [subsections](#ipmi), 
 while example configuration-files can be found in the `config/` directory. An explanation of how to deploy Wintermute 
 data analytics plugins can be found in the corresponding readme document.


## REST API <a name="restApi"></a>

Dcdbpusher runs a HTTPS server which provides some functionality to be controlled over a RESTful API. The API is by default hosted at port 8000 on 127.0.0.1 but the address can be changed in [`dcdbpusher.conf`](#globalConfiguration).

A HTTPS request to dcdbpusher should have the following format: `[GET|PUT|POST] host:port[resource]?[queries]`.
Tables with allowed resources sorted by REST methods can be found below. A query consists of a key-value pair of the format `key=value`. Multiple queries are separated by semicolons(';'). For all requests (except /help) basic authentication credentials must be provided.

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
    <td colspan="2"><b>GET /plugins</b></td>
    <td colspan="2">List all loaded dcdbpusher plugins.</td>
  </tr>
  <tr>
  	<td>json</td>
  	<td>"true"</td>
  	<td>Yes</td>
  	<td>Format response as json.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>GET /sensors</b></td>
    <td colspan="2">List all sensors of a specific plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>json</td>
  	<td>"true"</td>
  	<td>Yes</td>
  	<td>Format response as json.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>GET /average</b></td>
    <td colspan="2">Get the average of the last readings of a sensor. Also allows access to analytics sensors.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>sensor</td>
  	<td>All sensor names of the plugin or the operator manager.</td>
  	<td>No</td>
  	<td>Specify the sensor within the plugin.</td>
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
    <td colspan="2">Exits the Pusher with a user-specified return code.</td>
  </tr>
  <tr>
  	<td>code</td>
  	<td>Return code.</td>
  	<td>Yes</td>
  	<td>Return code to be used when exiting.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /load</b></td>
    <td colspan="2">Load and intitialize a new plugin but do not start it. Use the /start request to kick off the plugin's data collection.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>Plugin name.</td>
  	<td>No</td>
  	<td>Name of the new plugin. Is used to build the shared library file name which holds the plugin. Shared lib file name is of the form libdcdbplugin_PLUGINNAME.so (or .dylib for Apple).</td>
  </tr>
  <tr>
  	<td>path</td>
  	<td>A file path.</td>
  	<td>Yes</td>
  	<td>Path to where the shared library for the plugin is located. If not specified the default library directories (urs/lib and friends) are searched.</td>
  </tr>
  <tr>
  	<td>config</td>
  	<td>A file path including file name.</td>
  	<td>Yes</td>
  	<td>Path and name of the plugin configuration file. If not specified we will search for "./PLUGINNAME.conf".</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /unload</b></td>
    <td colspan="2">Unload a plugin, removing it completely from dcdbpusher. To use the plugin again one has to /load it first.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /reload</b></td>
    <td colspan="2">Reload a plugin's configuration (includes fresh creation of a plugin's sensors and a plugin restart).</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>POST /start</b></td>
    <td colspan="2">Start a plugin, i.e. its sensors start polling.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>POST /stop</b></td>
    <td colspan="2">Stop a plugin, i.e. its sensors stop polling.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
</table>

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; Opt. = Optional

### Examples <a name="restExamples"></a>

Two examples for HTTPS requests (authentication credentials not shown):

```bash
GET https://localhost:8000/average?plugin=sysfs;sensor=freq1;interval=15
```
```bash
PUT https://localhost:8000/stop?plugin=bacnet
```

## MQTT topic <a name="mqttTopic"></a>

For communication between the different DCDB-components (database, dcdbpusher) the [MQTT protocol](https://mqtt.org/) is used. In order to identify each sensor, each has to have a unique MQTT topic assigned. The topic for a sensor is built by appending up to 4 parts:
1. mqttprefix    (e.g. /mysystem)
2. mqttpart of entity (if supported by plugin, e.g. /host0)
3. mqttpart of group    (e.g. /eth0)
4. mqttsuffix    (e.g. /xmitdata)

Then the topic for the sensor is /mysystem/host0/eth0/xmitdata.

Additionally, sensors can be published automatically to the Storage Backend under their specified MQTT topics, by using the _auto-publish_ feature. Such feature is enabled via the _-a_ switch to DCDB Pusher. This way, the metadata tables in the Storage Backend will be populated with the information of all instantiated sensors, and these will become visible for queries.



# Plugins <a name ="plugins"></a>

The core of dcdbpusher is responsible of collecting all the values read by the sensors and sending them to the database. However, the main functionality of the sensors comes from the various plugins. Every plugin corresponds to a special sensor functionality.
All the different plugins share some same general principles in common regarding the sensor structure and configuration. Those principles should also be obeyed when [writing own plugins](#writingOwnPlugins).
1. There are three hierarchical levels (from bottom up):
    1. Sensors
    2. Groups
    3. Entities (optional)
2. There are no sensors on its own. Every sensor belongs to a group.
3. Multiple groups may or may not be aggregated by an entity. Entities can be optionally used by the plugin developer to aggregate groups which belong together, e.g. because they all query the same host.
4. Every hierarchical level is associated with some attributes. In the following are some hints on how one (when developing own plugins) should decide which attributes are associated with which level. Also for every level the common base attributes are listed (with explanation), which are specified independently of a plugin:
    1. Entities (if present) hold all attributes which are required to query the represented entity or all its associated groups have in common. Common entity attributes:
        * __default__     (One can define the name of a template group (see below) whose values and groups should be used as default)
        * Other entity attributes could be: mqttPart, protocol-version, host address and port.
    2. Groups hold all attributes which multiple sensors belonging to it share in common. Common group attributes:
        * __interval__    (Time in [ms] between two consecutive sensor reads. Default is 1000[ms] = 1[s])
        * __queueSize__   (Maximum number of sensor readings to queue to bridge connectivity issues with the CollectAgent. Default is 1024.
	* __minValues__   (Minimum number of sensor reads the sensors in a group should gather before they are sent together to the database. Useful to reduce MQTT-overhead. Default is 1 (every sensor value is sent on its own))
        * __mqttPart__    (Part for the [mqtt-topic](#mqttTopic) all sensors in this group should share in common)
        * __default__     (One can define the name of a template group (see below) whose values and sensors should be used as default)
    3. Sensors hold only those attributes which are necessary to uniquely identify the target sensor. Common base attributes:
        * __mqttsuffix__  (to make its [mqtt-topic](#mqttTopic) unique)
        * __delta__ (identifies a monotonic sensor. If set to "true", differences between successive readings are collected)
        * __deltaMax__ (used only for monotonic sensors. Establishes the maximum wrap-around value for the accumulator. Default is LLONG_MAX.)
        * __subSampling__ (subsampling factor S. If S>=1, only one reading every S is sent over MQTT, and the others are kept locally. If S<1, readings are never sent out and only kept locally)
		* __publish__ (if set to "true", the sensor will be published when the auto-publish feature is enabled. Otherwise it is omitted. Default is "true".)
5. Be aware that naming of sensor/group/entity is not fixed. A plugin developer can name them as he likes, e.g. counter/multicounter/host.
6. It is possible to define template sensors, groups, or entities in the config file. To specify a template sensor/group/entity simply prefix its definition with `template_` (see the example below). You can reference them later by using the `default` attribute. A template entity can consist of groups and these in turn can consist of sensors. When using a template, all of its attribute values are copied to the actual sensor. Copied attributes can be overwritten in the actual entity/group/sensor (some of them even should be overwritten, e.g. the mqttPart). Groups/sensors associated with a template are copied to the actual entity/group. One can specify further groups/sensors which are then added to those copied from the template. If a group's/sensor's name is identical to one of the groups/sensors introduced by the template, it will not be added but instead overwrites the corresponding group/sensor of the template (overwrite means: specified attributes replace template attributes. Otherwise template values are kept). This can be used to purposefully overwrite single (attributes of) groups/sensors introduced by a template. Template entitys/groups/sensors themself are never used in live operation of the plugin. They are purely cosmetic for convenient configuration.
 
In the following two abstract config files are shown to visualize the structure, one with the optional entity level and one without. A real example configuration file for every plugin should be provided in the `/config` directory. One should use them as a starting point to write own configuration files. 
```
 Without entity:
------------------------------------------------

global {
	mqttprefix /myprefix
	cacheInterval 120
	...
}

template_group temp1 {			;template group named temp1 (is not used in live operation)
	interval	1000			;While it is possible define entities/groups/sensors without
	minValues	3				;name it is strictly disregarded. Naming entities/groups/sensors
	mqttPart	/aa				;simplifies debugging and especially enables one to reference
								;templates later on. Also names should be always unique.
	sensor s1 {
		mqttsuffix		/s1
		...						;usually the sensor would require additional attributes
	}

	sensor s2 {
		mqttsuffix		/s2
		...
	}
}

group g1 {
	default		temp1			;use temp1 as template group
	mqttPart	/bb				;overwrite the mqttPart from temp1, to avoid identical
								;mqtt-topics if another group uses the same template
	sensor s3 {					;g1 has now 3 sensors: s1, s2 (both taken over from temp1)
		mqttsuffix		/s3		;and s3
		...
	}
}

group g2 {						;g2 consists of only one sensor (s21) and uses
 	sensor s21 {				;for every attribute the default value
		mqttsuffix	/s21		;by using a longer mqttsuffix we do not need a
		...						;group mqtt-part
	}
}

...
```

```
 With entity:
------------------------------------------------

global {
	...
}

template_entity temp1 {				;template entity which is not used in live operation
	...								;here go entity attributes

	group g1 {						
		interval	1000
		minValues	3
		mqttPart	/aa
		
		sensor s1 {
			mqttsuffix		/s1
			...						;usually the sensor would require additional attributes
		}
	
		sensor s2 {
			mqttsuffix		/s2
			...
		}
	}
}

entity ent1 {
	default		temp1				;use temp1 as template entity
	
	group g2 {						;ent1 has now two groups (g1 and g2) with a total of
	 	sensor s21 {				;3 sensors (s1, s2, s21)
			mqttsuffix	/s21
			...
		}
	}
}

...
```
One should have noticed the global section in the examples which was not mentioned before. In this section the user can (but is not obligated to) overwrite values from the `dcdbpusher.conf` for this plugin or specify other settings which are global for this plugin.

## IPMI <a name="ipmi"></a>

The [IPMI](https://en.wikipedia.org/wiki/Intelligent_Platform_Management_Interface) plugin enables dcdbpusher to collect sensor values offered by a baseboard management controller (BMC).

Explanation of the values specific for the IPMI plugin:

| Value | Explanation |
|:----- |:----------- |
| sessiontimeout | Session timeout value for the IPMI-connection
| retransmissiontimeout | Retransmission timeout value for the IPMI-connection
| username | For the remote IPMI-connection login credentials are required
| password | For the remote IPMI-connection login credentials are required
| ipmiversion | IPMI version to use for LAN connections (1 or 2)
| cipher | Cipher to use for IPMI 2.0 LAN connections (currently supported: 0, 1, 2, 3, 6, 7, 8, 11, 12)
| cmd | One can define a raw IPMI-command (in hex-notation) to be sent. In this case also the start and stop fields for the response have to be defined. Alternatively, one can define the record-ID of the sensor (see below).
| lsb | Offset where the least significant byte of the wanted return value of an IPMI raw command in the IPMI response<sup>[1](#ipmifn1)</sup>
| msb | Offset where the most significant byte of the wanted return value of an IPMI raw command in the IPMI response<sup>[1](#ipmifn1)</sup>
| recordId | Define the record-ID of the sensor to be read. One can look up the corresponding record-IDs for every sensor with the "ipmi-sensors" command line tool (ships with the freeipmi-library). Alternatively, one can define a raw IPMI-command (see above).
| factor | One can specify a factor to scale the read value before it is stored in the database (to adjust precision).
#### Footnotes <a name="ipmiFootnotes"></a>

<a name="ipmifn1">**1**</a>: &ensp; Use lsb > msb values if response is Little-endian (LSB first), use lsb < msb values if response is Big-Endian (MSB first). Maximum length is 8 bytes.  

## Perf-event <a name="perf"></a>

The Perfevent functionality is tasked with collecting data from the CPUs various performance counters (PMUs).
> NOTE &ensp;&ensp;&ensp; The perf-event plugin measures PMUs for all processes running on a specific CPU. Therefore a value of less than 1 is required in `/proc/sys/kernel/perf_event_paranoid`. Other values (>=1) restrict the access to PMUs. See this [footnote](#fn1) for additional information.

Explanation of the values specific for the perfevent plugin:

| Value | Explanation |
|:----- |:----------- |
| type | Type of which the counter should be. Each type determines different possible values for the config-field. Possible type-values are described below.
| config | Together with the type-field config determines which performance counter should be read. Possible values and what they measure are listed below.
| cpus | One can define a comma-separated list of cpu numbers (also value ranges can be specified, e.g. 2-4 equals 2,3,4). The hardware counter will then be only opened on the specified cpus.
| htVal | Specify multiplier for CPU aggregation. All CPUs where (CPU-number % htVal) has the same result are aggregated together. Only CPUs which are included in the "cpus" field (or all CPUs if the "cpus" field is not present) are aggregated. Background: To reduce the amount of pushed sensor data, it is possible to aggregate cpu readings. This feature is specifically aimed at processors which are hyper-threading enabled but can also come in handy for other use cases. Only the values pushed via the MQTT-Pusher are aggregated. There still exist sensors for each CPU and they store unaggregated readings in their local caches.
| mqttsufffix | In the context of the perfevent plugin the CPU id is integrated in the suffix. Sensors will be duplicated in order to open hardware counter for each CPU. Therefore an identifier in the style of "/cpuxx" will be pre-prended to the mqttSuffix when building the topics.


> NOTE &ensp;&ensp;&ensp; As perfevent counters are usually always monotonic, the delta attribute is by default set to true for all sensors. One has to explicitly set delta to "off" for a sensor to overwrite this behaviour.


### type and config <a name="perfTypeConfig"></a>

(see the [perf_event_open man-page](http://man7.org/linux/man-pages/man2/perf_event_open.2.html) for more detailed explanations)

| Type | Config | Explanation |
|:----:|:------ |:----------- |
| PERF_TYPE_HARDWARE | | generalized hardware CPU events
| " | PERF_COUNT_HW_CPU_CYCLES | total cycles (affected by frequency scaling)
| " | PERF_COUNT_HW_INSTRUCTIONS | retired instructions
| " | PERF_COUNT_HW_CACHE_REFERENCES | cache accesses (usually last level)
| " | PERF_COUNT_HW_CACHE_MISSES | cache misses (usually last level)
| " | PERF_COUNT_HW_BRANCH_INSTRUCTIONS | retired branch instructions
| " | PERF_COUNT_HW_BRANCH_MISSES | mispredicted branch instructions
| " | PERF_COUNT_HW_BUS_CYCLES | bus cycles
| " | PERF_COUNT_HW_STALLED_CYCLES_FRONTEND | stalled cycles during issue
| " | PERF_COUNT_HW_STALLED_CYCLES_BACKEND  | stalled cycles during retirement
| " | PERF_COUNT_HW_REF_CPU_CYCLES | total cycles (unaffected by frequency scaling)
| | | |
| PERF_TYPE_SOFTWARE | | software events provided by the kernel
| " | PERF_COUNT_SW_CPU_CLOCK | reports CPU clock
| " | PERF_COUNT_SW_TASK_CLOCK | clock count specific to the running task
| " | PERF_COUNT_SW_PAGE_FAULTS | number of page faults
| " | PERF_COUNT_SW_CONTEXT_SWITCHES | count of context switches
| " | PERF_COUNT_SW_CPU_MIGRATIONS | times the process has migrated to a new CPU
| " | PERF_COUNT_SW_PAGE_FAULTS_MIN | number of minor page faults (no disk-I/O)
| " | PERF_COUNT_SW_PAGE_FAULTS_MAJ | number of major page faults (disk-I/O was required)
| " | PERF_COUNT_SW_ALIGNMENT_FAULTS | alignment faults when accessing unaligned memory
| " | PERF_COUNT_SW_EMULATION_FAULTS | number of unimplemented instructions which had to be emulated
| " | PERF_COUNT_SW_DUMMY | placeholder which counts nothing
| | | |
| PERF_TYPE_TRACEPOINT | | not yet implemented
| PERF_TYPE_HW_CACHE | | not yet implemented
| | | |
| PERF_TYPE_RAW | | user can define architecture-specific raw events here.
| " | *XXXX* | Config must be a raw event config value, see <sup>[2](#fn2)</sup>
| | | |
| PERF_TYPE_BREAKPOINT | --- | config not required, any values will be ignored. However config must still be specified (even if empty)
|<Custom>|<Custom>| dynamic PMU event, see <sup>[3](#fn3)</sup>

#### Footnotes <a name="perfFootnotes"></a>

Taken from the [perf_event_open man-page](http://man7.org/linux/man-pages/man2/perf_event_open.2.html):

<a name="fn1">**1**</a>: &ensp; The pid and cpu arguments allow specifying which process and CPU to monitor:  
[...]  
pid == -1 and cpu >= 0  
This measures all processes/threads on the specified CPU. This requires CAP_SYS_ADMIN capability or a /proc/sys/kernel/perf_event_paranoid value of less than 1.

[...]

The perf_event_paranoid file can be set to restrict access to the performance counters.

| Value | Restriction |
|:-----:|:----------- |
| 2 | allow only user-space measurements (default since Linux 4.6) |
| 1 | allow both kernel and user measurements (default before Linux 4.6) |
| 0 | allow access to CPU-specific data but not raw trace-point samples |
| -1 | no restrictions |
	
The existence of the perf_event_paranoid file is the official method for determining if a kernel supports perf_event_open()

<a name="fn2">**2**</a>: &ensp; If type is *PERF_TYPE_RAW*, then a custom "raw" config value is needed. Most CPUs support events that are not covered by the "generalized" events. These are implementation defined; see your CPU manual (for example the Intel Volume 3B documentation or the AMD BIOS and Kernel Developer Guide). The libpfm4 library can be used to translate from the name in the architectural manual to the raw hex value perf_event_open() expects in this field.

<a name="fn3">**3**</a>: &ensp; Custom type and Config values can be specified to use the PMU of a specific device. The necessary configuration parameters can be obtained from the type and config files the respective in /sys/devices/<device> tree.

## snmp <a name="snmp"></a>

The SNMP plugin enables dcdbpusher to talk with devices which have an SNMP agent running and query requests from them. A SNMP sensor corresponds to a single value as identified by the unique OID. Sensors are aggregated by connections. See the exemplary snmp.conf file in the `config/` directory.
> NOTE &ensp;&ensp;&ensp; In the SNMP context the word privacy is used synonymously for encryption.

Explanation of the values specific for the SNMP plugin:

| Value | Explanation |
|:----- |:----------- |
| connection | An aggregating connection
| Type | Type of the SNMP application which runs on the device queried by the connection. Currently only the type Agent is supported.
| Host | Host name of the device which is to be queried. Follows net-snmp's [<transport-specifier>:]<transport-address> format, e.g. udp:hostname:161
| OIDPrefix | This OIDPrefix is used for all following sensors.
| |
| Version | Which SNMP version to use (either 2 (maps to 2c) or 3).
| Community | Which SNMP community to use (required only if version 2 is used).
| Username | Username to authenticate with (only required for version 3).
| SecLevel | The security level to be used (only required for version 3). Can be either `noAuthNoPriv` for no authentication and privacy ("privacy" is SNMPs synonym for encryption), `authNoPriv` for only authentication and `authPriv` for authentication and privacy.
| AuthProto | Which protocol to use for authentication (only required for version 3 and if SecLevel != noAuthNoPriv). Can be MD5 or SHA1.
| AuthKey | The passphrase for authentication (only required for version 3 and if SecLevel != noAuthNoPriv). Must be at least 8 characters long.
| PrivProto | Which protocol to use for privacy (only required for version 3 and if SecLevel = AuthPriv). Can be DES or AES.
| PrivKey | The passphrase for privacy encryption (only required for version 3 and if SecLevel = AuthPriv). Must be at least 8 characters long.
| mqttPart | Connection specific MQTT-part which is appended to the MQTT-prefix and succeded by the sensor specific suffix.
| |
| OID | OID suffix which together with the OIDPrefix forms the unique OID identifying a value to query.
| passphrase | has to be at least 8 characters long

## sysFS <a name="sysfs"></a>

SysFS sensors read data from sysFS files. The configuration file of the plugin corresponds to the generic plugin configuration with standalone sensors. Additionally for a sysFS sensor the following parameters are mandatory/possible:

Explanation of the values specific for the sysFS plugin:

| Value | Explanation |
|:----- |:----------- |
| path | Path to the sysFS file the sensor should read from. This parameter is mandatory.
| filter | One can define an optional filter if the sysFS file consists of more than only the sensor value. Please note the following points for filters: <br> 1.  The filter supports substitutions. For substitution sed syntax ("s/.../.../") is used. Therefore extended regular expressions (ERE) are used as regex-syntax. ERE is closest to Basic RE (BRE), which is actually used by sed, but requires less escaping. <br> 2.  If a \ ("backslash") is needed in the regex (for escaping), always use \\ ("double backslash") as the regex is read in as string and strings also escape with backslash <br> 3.  Whitespaces are actually used as value separators in the config files. If your filter requires whitespaces either use [[:space:]] in the regex or put it in quotation marks ("") <br> 4.  To be able to reference parts of the match (for substitution) use groups. Groups are created with parentheses. <br>  5.  If using character classes like [[:digit:]] always make sure to use double brackets ("[[" and "]]") or they will not be recognized. <br>  See [ERE-syntax](https://www.gnu.org/software/sed/manual/html_node/ERE-syntax.html#ERE-syntax) <br>  See [substitution syntax](http://www.boost.org/doc/libs/1_65_1/libs/regex/doc/html/boost_regex/format/sed_format.html)
| retain | If False, the file is closed and re-opened at every sampling interval. Default is True.

## PDU <a name="pdu"></a>

The Power Delivery Unit (PDU) plugin is in charge of sending a network-request to the PDUs and gathering specified sensor data from the XML-file response.

Explanation of the values specific for the PDU plugin:

| Value | Explanation |
|:----- |:----------- |
| host | Hostname and (optional) port where to fetch the XML-file with sensor data from. If no port is specified, 443 is used. The plugin requests the file via HTTPS.
| request | Define the request to be sent to the host via HTTPS as a string. One should put the request in quotation marks (' " ') to enable the use of whitespaces within the request. Special characters (like usage of ' " ' within the request) should be escaped (' " ' --> ' \" '; ' \ ' --> ' \\\\ '; newline --> ' \n '; ...).
| path | Define a dot-separated path to the value to be read in the XML file. One can specify attribute values a node has to fulfil in brackets after the node. Even multiple (comma-separated) attributes can be given, however no whitespaces should be used (!) as they will not be filtered and could therefore be treat as part of the attributes name.

## BACnet <a name="bacnet"></a>

The BACnet plugin enables dcdbpusher to communicate and request data from devices which communicate via the BACnet protocol. A so called "read property" request is sent by the plugin to the BACnet devices as configured in the config file. The response value is then stored in the database. Usually one is only interested in collecting the current reading of a BACnet device (property PROP_PRESENT_VALUE, ID 85). However, also reading of other properties is supported.
> NOTE &ensp;&ensp;&ensp; On startup BACnet plugin does no device discovery. Instead it relies on the user providing a file with addresses of all required BACnet devices. One can generate such an address-file for example by using the `bacwi` demo tool provided by the BACnet-Stack.

Explanation of the values specific for the BACnet plugin:

| Value | Explanation |
|:----- |:----------- |
| address_cache | (Path to and) filename of the address cache file where the addresses of BACnet devices are stored (as noted above).
| interface | Network interface (IPv4) which is to be used by the plugin to send its "Read Property" requests.
| port | Port to use on the interface
| timeout |	Value of µ-seconds to wait for a response packet.
| apdu_timeout | Value of µ-seconds before sending a request times out.
| apdu_retries | How often should sending a request be retried.
| templates | One can define template properties in this section for convenience.
| factor | Described in the section for the [IPMI-plugin](#ipmi).
| devices | Starts the part in the config file where the actual BACnet devices are configured. A BACnet device consists of multiple nested parts: device > objects > properties.
| instance (device) | Instance of the BACnet-device.
| type | Type of the object within the device.
| instance (object) | Instance of the object within the device.
| id | ID of the property to be read from the BACnet device-object. Assignment of numbers to properties is done according to the enum as defined in `bacenum.h`.

## Opa (Intel Omni-Path Architecture) <a name="opa"></a>

The Opa plugin enables dcdbpusher to query various counters from omni-path interconnects.

Explanation of the values specific for the Opa plugin:

| Value | Explanation |
|:----- |:----------- |
| hfiNum | Number of which omni-path Host Fabric Interface to query (starting with 1)
| portNum | Number of which omni-path port to query (starting with 1)
| cntData | Name which data counter to query. A list of possible values can be found below.


> NOTE &ensp;&ensp;&ensp; As opa counters are usually always monotonic, the delta attribute is by default set to true for all sensors. One has to explicitly set delta to "off" for a sensor to overwrite this behaviour.

### counterData <a name="opaCounterData"></a>

Possible values for cntData:
* portXmitData
* portRcvData
* portXmitPkts
* portRcvPkts
* portMulticastXmitPkts
* portMulticastRcvPkts
* localLinkIntegrityErrors
* fmConfigErrors
* portRcvErrors
* excessiveBufferOverruns
* portRcvConstraintErrors
* portRcvSwitchRelayErrors
* portXmitDiscards
* portXmitConstraintErrors
* portRcvRemotePhysicalErrors
* swPortCongestion
* portXmitWait
* portRcvFECN
* portRcvBECN
* portXmitTimeCong
* portXmitWastedBW
* portXmitWaitData
* portRcvBubble
* portMarkFECN
* linkErrorRecovery
* linkDowned
* uncorrectableErrors

## ProcFS (/proc filesystem) <a name="procfs"></a>

The ProcFS plugin enables dcdbpusher to sample resource usage metrics from a variety of files in the /proc virtual filesystem generated by the Linux kernel. Each defined sensor group is assigned to a specific file, which is periodically parsed. Currently supported files for sampling are:
* /proc/vmstat: contains virtual memory-related usage metrics;
* /proc/meminfo: contains RAM memory-related usage metrics (note that some of the metrics overlap with /proc/vmstat);
* /proc/stat: contains CPU usage-related metrics, both at system and core levels.

Note that the ProcFS plugin can operate in two distinct modes, with respect to MQTT topics:
* Automatic: if no sensors are specified, all metrics discovered in the underlying parsed file are acquired; sensors and MQTT topics are generated for them. Please be careful when configuring the plugin so that its MQTT topics do not overlap with those of other plugins.
* Manual: If at least one sensor is specified, only the corresponding metrics are acquired, and all other metrics in the parsed file are discarded. MQTT topics are assigned accordingly, using the mqttPrefix, mqttPart and mqttSuffix fields.

Explanation of the values specific for the ProcFS plugin:

| Value | Explanation |
|:----- |:----------- |
| type | The type of the file parsed by the sensor group. Can be either "vmstat", "meminfo", "procstat" or "sar"
| path | Path of the file, if different from the default path in the /proc filesystem
| cpus | Defines the set of CPU cores for which metrics must be collected. Only affects extraction of core-specific metrics (e.g. those in /proc/stat), whereas system-level metrics are acquired regardless of this setting. If no CPU cores set is defined, metrics for all available CPU cores will be collected. This parameter follows the same syntax as in the Perf-event plugin.
| htVal | Specify a multiplier for CPU aggregation. All CPUs where (CPU-number % htVal) has the same result are aggregated together. If specified, only CPUs which are included in the "cpus" field are aggregated. See Perf-event plugin for more details.
| sarMax | A scaling factor to be applied to ratio-like metrics with the sar parser. Default is 1000000.
| mqttSuffix | the mqttSuffix field in the ProcFS plugin, for sensors that are CPU-related such as the ones in procstat files, behaves as described for the perf-event plugin.

Additionally, sensors in the ProcFS plugin (defined with the "metric" keyword) support the following additional values:

| Value | Explanation |
|:----- |:----------- |
| type | The type of the specific metric associated to the sensor. This field must match the exact name of a metric in the underlying parsed file. If such a match does not exist, the sensor is discarded.
| perCpu | Boolean. If set to "on", the metric will be collected for each CPU core specified with the "cpus" sensor group parameter, or for all CPU cores if none is specified. Otherwise, the metric will be collected only at system level. This parameter has no effect on metrics that are not acquired at CPU core level (e.g. those in /proc/vmstat and /proc/meminfo).

The "type" field can be inferred for each sensor by simply checking the underlying file parsed by the sensor group. For /proc/stat files, on the other hand, CPU core-related metrics are collected in separate columns, which adopt the following naming scheme that can be used to define sensors: 
* col_user 
* col_nice 
* col_system 
* col_idle 
* col_iowait
* col_irq
* col_softirq
* col_steal
* col_guest
* col_guest_nice

Additional CPU-related metrics (that may be introduced in future versions of the Linux kernel) are not supported by the DCDB ProcFS plugin.
Note that for /proc/meminfo instances, an additional synthetic sensor of type "MemUsed" can be defined. This sensor will automatically extract the amount of used memory from the MemTotal and MemFree values present in meminfo files.

## Caliper <a name="caliper"></a>

The Caliper plugin collects application introspection data and therefore allows for application performance analysis in retrospect. This plugin is special as it does not work on its own but also requires a corresponding Caliper framework service running on application side. Please see Caliper's [official documentation](https://software.llnl.gov/Caliper/) for an exhaustive introduction.
The Caliper plugin supports two use cases:
* **Sampling** Low overhead automatic sampling of program counter (PC) values. Allows to analyze how much time was spent in a function in retrospect. Is the default case and enabled at all times.
* **Instrumentation** The user can instrument its application with Caliper annotations. The event data generated by the annotations is picked up by Pusher in addition to the sampling data and stored within DCDB. The annotation data can then be correlated with other monitoring data and allows for more fine-grained introspection than the sampling approach. On the downside, this usually induces more overhead.

### Caliper framework side
Caliper is an application introspection system. Its functionality stems from so called services. To work with the Pusher plugin the custom Dcdbpusher service for Caliper is required as well as the stock timestamp, pthread, and sampler service. For instrumentation, the event service is required as well.
Caliper has to be integrated into the application. This can be done either manually from the application developer or more automated by the system administrator by "hijacking" applications, e.g. overwriting main methods before execution. For the sampling case it is sufficient to use the Caliper framework just once, i.e. initialize it somewhere. However, one can still use the full functionality of Caliper services at own will in parallel.
The dcdbpusher service retrieves all relevant data from snapshots (Timestamp, CPU, PC (sampling), annotation data (instrumentation)). In case of sampling, the PC value is resolved to the actual function name via the binary's symbol data. Retrieved data is temporarily stored in a thread-local buffer. Eventually it gets written to a shared-memory queue which is used to communicate with the Pusher plugin.

The Caliper services can be controlled by the environment variables listed below:

| Value | Explanation |
|:----- |:----------- |
| CALI_SERVICES_ENABLE | Specify which Caliper services to enable. Should be at least `event:sampler:timestamp:pthread:dcdbpusher`.
| CALI_SAMPLER_FREQUENCY | Frequency of the sampler service in Hz.
| CALI_TIMER_TIMESTAMP | Must be set to `true` to enrich all snapshots with timestamps of their creation.
| CALI_DCDBPUSHER_SUS_CYCLE | Symbol update service (SUS) cycle. To resolve PC values to function names the symbol data of the binary and loaded libraries is locally buffered in a so called "symbol index". In case a symbol could not be resolved by the dcdbpusher service (e.g. because the PC points to a newly loaded library that has not yet been indexed) it informs the background SUS thread to update the symbol index. Updating the symbol index is a heavy blocking task. To limit overhead and avoid continuous rebuild of the symbol index this environment variable can be used to set the cycle interval of the SUS in seconds (e.g. `export CALI_DCDBPUSHER_SUS_CYCLE=x`). The SUS only checks every x seconds if a symbol data update is requested. Increasing this value reduces overhead of repeated symbol index rebuilds but decreases responsiveness if rebuilds are requested seldomly. Default is 15 seconds.

### Pusher plugin side
The pusher plugin serves as data sink for the snapshot data from the Caliper service. It can handle multiple different applications at once. However, it is mainly intended for only one application with multiple threads/(MPI-)processes. 
The plugin consumes the snapshot data from the shared-memory queue. For each unique snapshot data a new sensor is created. Subsequent encounters of the same data (function name or annotation) a reading value of 1 is stored with the sensor.
After an application terminates/timeouts or the maxSensors value is reached all sensors get cleared.

Explanation of the values specific for this plugin:

| Value | Explanation |
|:----- |:----------- |
| interval | In case of sampling, the interval value of a SensorGroup (or SingleSensor) has a small side effect. Within the same read cycle, multiple encounters of the same function name will be aggregated. Instead of a value of one for each encounter, only the aggregated value at the end of the read cycle will be actually stored with the corresponding sensor. Therefore the read cycle interval also determines the granularity of the sampling data. A lower interval results in more fine-grained sampling data resolution but also requires more memory in the storage backend. 
| maxSensors | To limit indefinite memory usage by the creation of new Sensor object one can specify a threshold here. If the number of sensors exceeds this value, they will be cleared. Default is 500.
| timeout | Number of read cycles after which an Caliper-application is assumed to be terminated if no new values have been received. Connection (shared memory) is teared down on timeout. Default is 15.

### Shortcomings
Usage of the Caliper plugin is currently obstructed by a few shortcomings:
* The Caliper framework has to be integrated manually by the user into its application for this plugin to work.
* The Caliper framework seems to interfere with Intel libraries, which may cause [application crashes](https://github.com/LLNL/Caliper/issues/223).

## Metadata Management <a name="metadataManagement"></a>

Sensor metadata can be included in Pusher configurations, and will be published to the Storage Backend if the _auto-publish_ feature is enabled. A metadata block looks like the following:

```
...

group g2 {						
 	sensor pw {				
		mqttsuffix	/power		
		...
		
		metadata {
			unit	      Watt
			scale	      1000
			ttl           3600000
			operations    avg5,min,max
		}		
	}
}

...
```

Available fields that can be published as metadata are the following:

| Value | Explanation |
|:----- |:----------- |
| unit | String containing the unit of measure for the sensor, if any. |
| scale | Scaling factor (as a floating point value) to be applied to readings of the sensor upon queries. |
| ttl | Time to live for the readings of this sensor in milliseconds, after which they are automatically deleted from the Storage Backend. |
| monotonic | Boolean flag specifying whether the sensor is monotonic or not. |
| integrable | Boolean flag specifying whether the sensor's time series can be integrated or not. |
| interval | Sampling interval in milliseconds of the sensor. |
| operations | Comma-separated lists of operations available for the sensor, whose values can be retrieved by appending their names to the sensor name. |

An additional _isOperation_ field is available for the output sensors of operators in the Wintermute framework. If these output sensors are generated starting from a single input, this field allows to publish them as _operations_ of the latter, and will be listed in the associated database entry. For this to apply, however, the MQTT topic of the output sensor must be identical to that of the input, plus a suffix that describes the operation. Enabling this option invalidates all other metadata fields.

## Writing own plugins <a name="writingOwnPlugins"></a>
First make sure you read the [plugins](#plugins) section. 

It is recommended to use the `pluginGenerator/generatePlugin.sh` script to kick off plugin development. Running `./generatePlugin.sh -h` gives instructions on how to use the script. On success, the script generates all required source files for a new plugin with instructions on how to continue from there.

