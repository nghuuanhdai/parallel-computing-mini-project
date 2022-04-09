# Wintermute, the DCDB Data Analytics Framework

### Table of contents
1. [Introduction](#introduction)
2. [DCDB Wintermute](#dcdbanalytics)
    1. [Global Configuration](#globalConfiguration)
    2. [Operators](#operators)
    	1. [The Sensor Tree](#sensorTree)
    	2. [The Unit System](#unitSystem)
    	3. [Operational Modes](#opModes)
		4. [Operator Configuration](#operatorConfiguration)
			1. [Configuration Syntax](#configSyntax)
			2. [Instantiating Units](#instantiatingUnits)
			3. [Instantiating Hierarchical Units](#instantiatingHUnits)
			4. [Job Operators](#joboperators)
			5. [MQTT Topics](#mqttTopics)
			6. [Pipelining Operators](#pipelining)
    3. [Rest API](#restApi)
        1. [List of resources](#listOfResources)
        2. [Examples](#restExamples)
3. [Plugins](#plugins)
	1. [Aggregator Plugin](#averagePlugin)
	2. [Job Aggregator Plugin](#jobaveragePlugin)
	3. [Smoothing Plugin](#smoothingPlugin)
	4. [Regressor Plugin](#regressorPlugin)
	5. [Classifier Plugin](#classifierPlugin)
	6. [Clustering Plugin](#clusteringPlugin)
	6. [CS Signatures Plugin](#csPlugin)
	7. [Tester Plugin](#testerPlugin)
4. [Sink Plugins](#sinkplugins)
	1. [File Sink Plugin](#filesinkPlugin)
	2. [Cooling Control Plugin](#coolingcontrolPlugin)
	3. [Health Checker Plugin](#healthcheckerPlugin)
5. [Writing Plugins](#writingPlugins)

### Additional Resources

* **End-to-end usage example** of the Wintermute framework to perform regression. Can be found in _operators/regressor_.

# Introduction <a name="introduction"></a>
In this Readme we describe Wintermute, the DCDB Data Analytics framework, and all data abstractions that are associated with it. 

# DCDB Wintermute <a name="dcdbanalytics"></a>
The Wintermute framework is built on top of DCDB, and allows to perform data analytics based on sensor data
in a variety of ways. Wintermute can be deployed both in DCDB Pusher and Collect Agent, with some minor
differences:

* **DCDB Pusher**: only sensor data that is sampled locally and that is contained within the sensor cache can be used for
data analytics. However, this is the preferable way to deploy simple models on a large-scale, as all computation is
performed within compute nodes, dramatically increasing scalability;
* **DCDB Collect Agent**: all available sensor data, in the local cache and in the Cassandra database, can be used for data
analytics. This approach is preferable for models that require data from multiple sources at once 
(e.g., clustering-based anomaly detection), or for models that are deployed in [on-demand](#operatorConfiguration) mode.

## Global Configuration <a name="globalConfiguration"></a>
Wintermute shares the same configuration structure as DCDB Pusher and Collect Agent, and it can be enabled via the 
respective (i.e., _dcdbpusher.conf_ or _collectagent.conf_) configuration file.  All output sensors of the frameworks 
are therefore affected by configuration parameters described in the global Readme. Additional parameters specific to 
this framework are the following:

| Value | Explanation |
|:----- |:----------- |
| **analytics** | Wrapper structure for the data analytics-specific values.
| hierarchy | Space-separated sequence of regular expressions used to infer the local (DCDB Pusher) or global (DCDB Collect Agent) sensor hierarchy. This parameter should be wrapped in quotes to ensure proper parsing. See the Sensor Tree [section](#sensorTree) for more details.
| filter | Regular expression used to filter the set of sensors in the sensor tree. Everything that matches is included, the rest is discarded.
| jobFilter | Regular expression used to filter the jobs processed by job operators. The expression is applied to all nodes of the job's nodelist to extract certain information (e.g., rack or island).
| jobMatch | String against which the node names filtered through the _jobFilter_ are checked, to determine if a job is to be processed (see this [section](#jobOperators)).
| jobIdFilter | Like the jobFilter, this is a regular expression used to filter out jobs that do not match it. In this case, the job ID is checked against the regex and the job is discarded if a match is not found.
| jobDomainId | Specifies the job domain ID (e.g., HPC system, SLURM partition) to query for jobs. 
| **operatorPlugins** | Block containing the specification of all data analytics plugin to be instantiated.
| plugin _name_ | The plugin name is used to build the corresponding lib-name (e.g. average --> libdcdboperator_average.1.0)
| path | Specify the path where the plugin (the shared library) is located. If left empty, DCDB will look in the default lib-directories (usr/lib and friends) for the plugin file.
| config | One can specify a separate config-file (including path to it) for the plugin to use. If not specified, DCDB will look up pluginName.conf (e.g. average.conf) in the same directory where global.conf is located.
| | |

## Operators <a name="operators"></a>
Operators are the basic building block in Wintermute. An Operator is instantiated within a plugin, performs a specific
task and acts on sets of inputs and outputs called _units_. Operators are functionally equivalent to _sensor groups_
in DCDB Pusher, but instead of sampling data, they process such data and output new sensors. Some high-level examples
of operators are the following:

* An operator that performs time-series regression on a particular input sensor, and outputs its prediction;
* An operator that aggregates a series of input sensors, builds feature vectors, and performs machine 
learning-based tasks using a supervised model;
* An operator that performs clustering-based anomaly detection by using different sets of inputs associated to different
compute nodes;
* An operator that outputs statistical features related to the time series of a certain input sensor.

### The Sensor Tree <a name="sensorTree"></a>
Before diving into the configuration and instantiation of operators, we introduce the concept of _sensor tree_. A  sensor
tree is simply a data structure expressing the hierarchy of sensors that are being sampled; internal nodes express
hierarchical entities (e.g., clusters, racks, nodes, cpus), whereas leaf nodes express actual sensors. In DCDB Pusher, 
a sensor tree refers only to the local hierarchy, while in the Collect Agent it can capture the hierarchy of the entire
system being sampled.

A sensor tree is built at initialization time of DCDB Wintermute, and is implemented in the _SensorNavigator_ class. 
By default, if no hierarchy string has been specified in the configuration, the tree is built automatically by assuming 
that each forward slash-separated part of the sensor name expresses a level in the hierarchy. The total depth of the 
tree is thus determined at runtime as well. This is, in most cases, the preferable configuration, as it complies with
the MQTT topic standard, and interprets each sensor name as if it was a path in a file system.

For spacial cases, the _hierarchy_ global configuration parameter can be used to enforce a specific hierarchy, with
a fixed number of levels. More in general, the following could be a set of forward slash-separated sensor names, from
which we can construct a sensor tree with three levels corresponding to racks, nodes and CPUs respectively:

```
/rack00/status
/rack00/node05/MemFree
/rack00/node05/energy
/rack00/node05/temp
/rack00/node05/cpu00/col_user
/rack00/node05/cpu00/instr
/rack00/node05/cpu00/branch-misses
/rack00/node05/cpu01/col_user
/rack00/node05/cpu01/instr
/rack00/node05/cpu01/branch-misses
/rack02/status
/rack02/node03/MemFree
/rack02/node03/energy
/rack02/node03/temp
/rack02/node03/cpu00/col_user
/rack02/node03/cpu00/instr
/rack02/node03/cpu00/branch-misses
/rack02/node03/cpu01/col_user
/rack02/node03/cpu01/instr
/rack02/node03/cpu01/branch-misses
``` 

Each sensor name is interpreted as a path within the sensor tree. Therefore, the _instr_ and _branch-misses_ sensors
will be placed as leaf nodes in the deepest level of the tree, as children of the respective cpu node they belong to.
Such cpu nodes will be in turn children of the nodes they belong to, and so on.

The generated sensor tree can then be used to navigate the sensor hierarchy, and perform actions such as _retrieving
all sensors belonging to a certain node, to a neighbor of a certain node, or to the rack a certain node belongs to_.
Please refer to the documentation of the _SensorNavigator_ class for more details.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; In the example above, if sensor names were formatted in a different format (e.g.,
rackXX.nodeXX.cpuXX.sensorName), we should have defined a hierarchy string explicitly in order to generate a sensor
tree correctly. Such string would be, in this case "_rack\d{2}.  node\d{2}.  cpu\d{2}._". 

> NOTE 2 &ensp;&ensp;&ensp; Sensor trees are always built from the names of sensors _as they are published_. Therefore,
please make sure to use the _-a_ option in DCDB Pusher appropriately, to build sensor names that express the desired hierarchy.


### The Unit System <a name="unitSystem"></a>
Each operator operates on one or more _units_. A unit represents an abstract (or physical) entity in the current system that
is the target of analysis. A unit could be, for example, a rack, a node within a rack, a CPU within a node or an entire HPC system.
Units are identified by three components:

* **Name**: The name of this unit, that corresponds to the entity it represents. For example, _/rack02/node03/_ or _/rack00/node05/cpu01/_ could be unit names. A unit must always correspond to an existing internal node in the current sensor tree;
* **Input**: The set of sensors that constitute the input for analysis conducted on this unit. The sensors must share a hierarchical relationship with the unit: that is, they can either belong to the node represented by this unit, to its subtree, or to one of its ancestors; 
* **Output**: The set of output sensors that are produced from any analysis conducted on this unit. The output sensors are always directly associated with the node represented by the unit.

Units are a way to define _patterns_ in the sensor tree and retrieve sensors that are associated to each other by a 
hierarchical relationship. See the configuration [section](#instantiatingUnits) for more details on how to create
templates in order to define units suitable for operators.

### Operational Modes <a name="opModes"></a>
Operators can operate in two different modes:

* **Streaming**: streaming operators perform data analytics online and autonomously, processing incoming sensor data at regular intervals.
The units of streaming operators are completely resolved and instantiated at configuration time. The type of output of streaming
operators is identical to that of _sensors_ in DCDB Pusher, which are pushed to a Collect Agent and finally to the Cassandra datastore,
resulting in a time-series representation;
* **On-demand**: on-demand operators do not perform data analytics autonomously, but only when queried by users. Unlike
for streaming operators, the units of on-demand operators are not instantiated at configuration, but only when a query is performed. When 
such an event occurs, the operator verifies that the queried unit belongs to its _unit domain_, and then instantiates it,
resolving its inputs and outputs. Then, the unit is stored in a local cache for future re-use. The outputs of a on-demand
operator are exposed through the REST API, and are never pushed to the Cassandra database.

Use of streaming operators is advised when a time-series-like output is required, whereas on-demand operators are effective
when data is required at specific times and for specific purposes, and when the unit domain's size makes the use of streaming
operators unfeasible.

### Operator Configuration <a name="operatorConfiguration"></a>
Here we describe how to configure and instantiate operators in Wintermute. The configuration scheme is very similar
to that of _sensor groups_ in DCDB Pusher, and a _global_ configuration block can be defined in each plugin configuration
file. The following is instead a list of configuration parameters that are available for the operators themselves:

| Value | Explanation |
|:----- |:----------- |
| default | Name of the template that must be used to configure this operator.
| interval | Specifies how often (in milliseconds) the operator will be invoked to perform computations, and thus the sampling interval of its output sensors. Only used for operators in _streaming_ mode.
| queueSize | Maximum number of readings to queue. Default is 1024.
| relaxed | If set to _true_, the units of this operator will be instantiated even if some of the respective input sensors do not exist.
| delay | Delay in milliseconds to be applied to the interval of the operator. This parameter can be used to tune how operator pipelines work, ensuring that the next computation stage is started only after the previous one has finished.
| unitCacheLimit | Defines the maximum size of the unit cache that is used in the on-demand and job modes. Default is 1000.
| minValues |   Minimum number of readings that need to be stored in output sensors before these are pushed as MQTT messages. Only used for operators in _streaming_ mode.
| mqttPart |    Part of the MQTT topic associated to this operator. Only used for the _root_ unit or when the _enforceTopics_ flag is set to true (see this [section](#mqttTopics)).
| enforceTopics | If set to _true_, mqttPart will be forcibly pre-pended to the MQTT topics of all output sensors in the operator (see this [section](#mqttTopics)). 
| sync | If set to _true_, computation will be performed at time intervals synchronized with sensor readings.
| disabled | If set to _true_, the operator will be instantiated but will not be started and will not be available for queries.
| duplicate | 	If set to _false_, only one operator object will be instantiated. Such operator will perform computation over all units that are instantiated, at every interval, sequentially. If set to _true_, the operator object will be duplicated such that each copy will have one unit associated to it. This allows to exploit parallelism between units, but results in separate models to avoid race conditions.
| streaming |	If set to _true_, the operator will operate in _streaming_ mode, pushing output sensors regularly. If set to _false_, the operator will instead operate in _on-demand_ mode.
| unitInput | Block of input sensors that must be used to instantiate the units of this operator. These can both be a list of strings, or fully-qualified _Sensor_ blocks containing specific attributes (see DCDB Pusher Readme).
| unitOutput | Block of output sensors that will be associated to this operator. These must be _Sensor_ blocks containing valid MQTT suffixes. Note that the number of output sensors is usually fixed depending on the type of operator.
| globalOutput | Block for _global_ output sensors that are not associated with a specific unit. If this is defined, all units described by the _unitInput_ and _unitOutput_ blocks will be grouped under a hierarchical _root_ unit that contains the output sensors described here.
| | |

#### Configuration Syntax <a name="configSyntax"></a>
In the following we show a sample configuration block for the _Aggregator_ plugin. For the full version, please refer to the
default configuration file in the _config_ directory:

```
global {
	mqttprefix /analytics
}

template_aggregator def1 {
interval	1000
minValues	3
duplicate 	false
streaming	true
}

aggregator avg1 {
default     def1
mqttPart    /avg1

	unitInput {
		sensor col_user
		sensor MemFree
	}

	unitOutput {
		sensor sum {
			operation 	sum
			mqttsuffix  /sum
		}

		sensor max {
			operation	maximum
			mqttsuffix  /max
		}

		sensor avg {
			operation	average
			mqttsuffix  /avg
		}
	}
}
``` 

The configuration shown above uses a template _def1_ for some configuration parameters, which are then applied to the
_avg1_ operator. This operator takes the _col_user_ and _MemFree_ sensors as input (which must be available under this name),
 and outputs _sum_, _max_, and _avg_ sensors. In this configuration, the Unit System and sensor hierarchy are not used, 
 and therefore only one generic unit (called the _root_ unit) will be instantiated.

#### Instantiating Units <a name="instantiatingUnits"></a>
Here we propose once again the configuration discussed above, this time making use of the Unit System to abstract from
the specific system being used and simplify configuration. The adjusted configuration block is the following: 

```
global {
	mqttprefix /analytics
}

template_aggregator def1 {
interval	1000
minValues	3
duplicate 	false
streaming	true
}

aggregator avg1 {
default     def1
mqttPart    /avg1

	unitInput {
		sensor "<bottomup>col_user"
		sensor "<bottomup 1>MemFree"
	}

	unitOutput {
		sensor "<bottomup, filter cpu00>sum" {
			operation 	sum
			mqttsuffix  /sum
		}

		sensor "<bottomup, filter cpu00>max" {
			operation 	maximum
			mqttsuffix  /max
		}

		sensor "<bottomup, filter cpu00>avg" {
			operation 	average
			mqttsuffix  /avg
		}
	}
}
``` 

In each sensor declaration, the _< >_ block is a placeholder that will be replaced with the name of the units that will
be associated to the operator, thus resolving the sensor names. Such block allows to navigate the current sensor tree,
and select nodes that will constitute the units. Its syntax is the following:

```
< bottomup|topdown X, filter Y >SENSORNAME 
``` 

The first section specified the _level_ in the sensor tree at which nodes must be selected. _bottomup X_ and _topdown X_
respectively mean _"search X levels up from the deepest level in the sensor tree"_, and _"search X levels down from the 
topmost level in the sensor tree"_. The _X_ numerical value can be omitted as well.

The second section, on the other hand, allows to search the sensor tree _horizontally_. Within the level specified in the
first section of the configuration block, only the nodes whose names match with the regular expression Y will be selected.
This way, we can navigate the current sensor tree both vertically and horizontally, and easily instantiate units starting 
from nodes in the tree. The set of nodes in the current sensor tree that match with the specified configuration block is
defined as the _unit domain_ of the operator.

The configuration algorithm then works in two steps:

1. The _output_ block of the operator is read, and its unit domain is determined; this implies that all sensors in the 
output block must share the same _< >_ block, and therefore match the same unit domain;
2. For each unit in the domain, its input sensors are identified. We start from the _unit_ node in the sensor tree, and 
navigate to the corresponding sensor node according to its _< >_ block, which identifies its level in the tree. Each 
unit, once its inputs and outputs are defined, is then added to the operator.

According to the sensor tree built in the previous [section](#sensorTree), the configuration above would result in
an operator with the following set of _flat_ units:

```
/rack00/node05/cpu00/ {
	Inputs {
		/rack00/node05/cpu00/col_user
		/rack00/node05/MemFree
	}
	
	Outputs {
		/rack00/node05/cpu00/sum
		/rack00/node05/cpu00/max
		/rack00/node05/cpu00/avg
	}
}

/rack02/node03/cpu00/ {
	Inputs {
		/rack02/node03/cpu00/col_user
		/rack02/node03/MemFree
	}
                     	
	Outputs {
		/rack02/node03/cpu00/sum
		/rack02/node03/cpu00/max
		/rack02/node03/cpu00/avg
	}
}
``` 

#### Instantiating Hierarchical Units <a name="instantiatingHUnits"></a>
A second level of aggregation beyond ordinary units can be obtained by defining sensors in the _globalOutput_ block. In
this case, a series of units will be created like in the previous example, and they will be added as _sub-units_ of a 
top-level _root_ unit, which will have as outputs the sensors defined in the _globalOutput_ block. This type of unit
is called a _hierarchical_ unit.
 
 
Computation for a hierarchical unit is always performed starting from the top-level unit. This means that all sub-units
will be processed sequentially in the same computation interval, and that they cannot be split. However, both the
top-level unit and the respective sub-units are exposed to the outside, and their sensors can be queried. Please
refer to the plugins' documentation to see whether hierarchical units are supported or not.

Recalling the previous example, a hierarchical unit can be constructed with the following configuration:

```
global {
	mqttprefix /analytics
}

template_aggregator def1 {
interval	1000
minValues	3
duplicate 	false
streaming	true
}

aggregator avg1 {
default     def1
mqttPart    /avg1

	unitInput {
		sensor "<bottomup>col_user"
		sensor "<bottomup 1>MemFree"
	}

	unitOutput {
		sensor "<bottomup, filter cpu00>sum" {
			operation 	sum
			mqttsuffix  /sum
		}

		sensor "<bottomup, filter cpu00>max" {
			operation 	maximum
			mqttsuffix  /max
		}

		sensor "<bottomup, filter cpu00>avg" {
			operation 	average
			mqttsuffix  /avg
		}
	}
	
	globalOutput {
		sensor globalSum {
			operation 	sum
			mqttsuffix  /globalSum
		}
    }
}
``` 

Note that hierarchical units can only have global outputs, but not global inputs, as they are meant to perform aggregation
of the results obtained on single sub-units. Such a configuration would result in the following unit structure:

```
__root__ {
	Outputs {
		/analytics/avg1/globalSum
	}
	
	Sub-units {
		/rack00/node05/cpu00/ {
			Inputs {
				/rack00/node05/cpu00/col_user
				/rack00/node05/MemFree
			}
			
			Outputs {
				/rack00/node05/cpu00/sum
				/rack00/node05/cpu00/max
				/rack00/node05/cpu00/avg
			}
		}
		
		/rack02/node03/cpu00/ {
			Inputs {
				/rack02/node03/cpu00/col_user
				/rack02/node03/MemFree
			}
								
			Outputs {
				/rack02/node03/cpu00/sum
				/rack02/node03/cpu00/max
				/rack02/node03/cpu00/avg
			}
		}
	}
}
``` 

> NOTE&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;&ensp; The _duplicate_ setting has no effect when hierarchical units are used 
(i.e., _globalOutput_ is defined).

> NOTE 2 &ensp;&ensp;&ensp;&ensp;&ensp; As of now the _aggregator_ plugin does not support hierarchical units. The
above is only meant as an example for how hierarchical units can be created in general.

#### Job Operators <a name="joboperators"></a>

_Job Operators_ are a class of operators which act on job-specific data. Such data is structured in _job units_. These
units are _hierarchical_, and work as described previously (see this [section](#instantiatingHUnits)). In particular,
 they are arranged as follows:

* The top unit is associated to the job itself and contains all of the required output sensors (_globalOutput_ block);
* One sub-unit for each node on which the job was running is allocated. Each of these sub-units contains all of the input
sensors that are required at configuration time (_unitInput_ block), along output sensors at the compute node level (_unitOutput_ block).

The computation algorithms driving job operators can then navigate freely this hierarchical unit design according to
their specific needs. Job-level sensors in the top unit do not require Unit System syntax (see this [section](#mqttTopics));
sensors that are defined in sub-units, if supported by the plugin, need however to be at the compute node level in the
current sensor tree, since all of the sub-units are tied to the nodes on which the job was running. This way, 
unit resolution is performed correctly by the Unit System.

Job operators also support the _streaming_ and _on-demand_ modes, which work like the following:

* In **streaming** mode, the job operator will retrieve the list of jobs that were running in the time interval starting
from the last computation to the present; it will then build one job unit for each of them, and subsequently perform computation;
* In **on-demand** mode, users can query a specific job id, for which a job unit is built and computation is performed.

A filtering mechanism can also be applied to select which jobs an operator should process. The default filtering policy uses
two parameters: a job _filter_ regular expression and a job _match_ string. When a job first appears in the system, the
job filter regex is applied to all of the node names in its nodelist. This regex could extract, for example, the portion
of the node name that encodes a certain _rack_ or _island_ in an HPC system. Then, frequencies are computed for each filtered
node name, and the mode is computed. If the mode corresponds to the job _match_ string, the job is assigned to the
operator. This policy can be overridden and changed on a per-plugin basis.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; The _duplicate_ setting does not affect job operators.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; In order to get units that operate at the _node_ level, the output sensors in the
configuration discussed earlier should have a unit block in the form of < bottomup 1 >.

#### MQTT Topics <a name="mqttTopics"></a>
The MQTT topics associated to output sensors of a certain operator are constructed in different ways depending
on the unit they belong to:

* **Root unit**: if the output sensors belong to the _root_ unit, that is, they do not belong to any level in the sensor
hierarchy and are uniquely defined, the respective topics are constructed like in DCDB Pusher sensors, by concatenating
the MQTT prefix, operator part and sensor suffix that are defined. The same happens for sensors defined in the _globalOutput_
block, which are part of the top level in a hierarchical unit, which also corresponds to the _root_ unit;
* **Job unit**: if the output sensors belong to a _job_ unit in a job operator (see below), the MQTT topic is constructed
by concatenating the MQTT prefix, the operator part, a job suffix (e.g., /job1334) and finally the sensor suffix;
* **Any other unit**: if the output sensor belongs to any other unit in the sensor tree, its MQTT topic is constructed
by concatenating the MQTT prefix associated to the unit (which is defined as _the portion of the MQTT topic shared by all sensors
belonging to such unit_) and the sensor suffix.

Even for units belonging to the last category, we can enforce arbitrary MQTT topics by enabling the _enforceTopics_ flag.
Using this, the MQTT prefix and operator part are pre-pended to the unit name and sensor suffix. This is enforced also
in the sub-units of hierarchical units (e.g., for job operators). Recalling the example above, this would lead to the following result:

``` 
MQTT Prefix	/analytics
MQTT Part	/avg1
Unit 		/rack02/node03/cpu00/

Without enforceTopics:
	/rack02/node03/cpu00/sum
With enforceTopics:
	/analytics/avg1/rack02/node03/cpu00/sum
``` 

Like ordinary sensors in DCDB Pusher, also operator output sensors can be published via the _auto-publish_ feature, and metadata can be specified for them. For more details, refer to the README of DCDB Pusher.

#### Pipelining Operators <a name="pipelining"></a>

The inputs and outputs of streaming operators can be chained so as to form a processing pipeline. To enable this, users
need to configure operators by enabling the _relaxed_ configuration parameter, and by selecting as input the output sensors
of other operators. This is necessary as the operators are instantiated sequentially at startup, and
the framework cannot infer the correct order of initialization so as to resolve all dependencies transparently.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; This feature is not supported when using operators in _on demand_ mode.

## Rest API <a name="restApi"></a>
Wintermute provides a REST API that can be used to perform various management operations on the framework. The 
API is functionally identical to that of DCDB Pusher, and is hosted at the same address. All requests that are targeted
at the data analytics framework must have a resource path starting with _/analytics_.

### List of resources <a name="listOfResources"></a>

Prefix `/analytics` left out!

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
    <td colspan="2">Return a cheatsheet of possible analytics REST API endpoints.</td>
  </tr>
  <tr>
  	<td colspan="4">No queries.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>GET /plugins</b></td>
    <td colspan="2">List all currently loaded data analytic plugins.</td>
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
  	<td>All operator plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operators of a plugin.</td>
  	<td>Yes</td>
  	<td>Restrict sensor list to an operator.</td>
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
    <td colspan="2"><b>GET /units</b></td>
    <td colspan="2">List all units of a specific plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All operator plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operators of a plugin.</td>
  	<td>Yes</td>
  	<td>Restrict unit list to an operator.</td>
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
    <td colspan="2"><b>GET /operators</b></td>
    <td colspan="2">List all operators of a specific plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All operator plugin names.</td>
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
    <td colspan="2"><b>PUT /reload</b></td>
    <td colspan="2">Reload configuration and initialization of all or only a specific operator plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>Yes</td>
  	<td>Reload only the specified plugin.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /navigator</b></td>
    <td colspan="2">Rebuild the Sensor Navigator used for instantiating operators.</td>
  </tr>
  <tr>
  	<td colspan="4">No queries.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>PUT /compute</b></td>
    <td colspan="2">Query the given operator for a certain input unit. Intended for "on-demand" operators, but works with "streaming" operators as well.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operator names of a plugin.</td>
  	<td>No</td>
  	<td>Specify the operator within the plugin.</td>
  </tr>
  <tr>
  	<td>unit</td>
  	<td>All units of a plugin.</td>
  	<td>Yes</td>
  	<td>Select the target unit. Defaults to the root unit if not specified.</td>
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
    <td colspan="2"><b>PUT /operator</b></td>
    <td colspan="2">Perform a custom REST PUT action defined at operator level. See operator plugin documenation for such actions.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>No</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>action</td>
  	<td>See operator plugin documentation.</td>
  	<td>No</td>
  	<td>Select custom action.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operators of a plugin.</td>
  	<td>Yes</td>
  	<td>Specify the operator within the plugin.</td>
  </tr>
  <tr>
  	<td colspan="4">Custom action may require or allow for more queries!</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>POST /start</b></td>
    <td colspan="2">Start all or only a specific plugin. Or only start a specific streaming operator within a specific plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>Yes</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operator names of a plugin.</td>
  	<td>Yes</td>
  	<td>Only start the specified operator. Requires a plugin to be specified. Limited to streaming operators.</td>
  </tr>
</table>

<table>
  <tr>
    <td colspan="2"><b>POST /stop</b></td>
    <td colspan="2">Stop all or only a specific plugin. Or only stop a specific streaming operator within a specific plugin.</td>
  </tr>
  <tr>
  	<td>plugin</td>
  	<td>All plugin names.</td>
  	<td>Yes</td>
  	<td>Specify the plugin.</td>
  </tr>
  <tr>
  	<td>operator</td>
  	<td>All operator names of a plugin.</td>
  	<td>Yes</td>
  	<td>Only stop the specified operator. Requires a plugin to be specified. Limited to streaming operators.</td>
  </tr>
</table>

> NOTE&ensp;&ensp;&ensp;&ensp;&ensp;&ensp;&ensp; Opt. = Optional

> NOTE 2 &ensp;&ensp;&ensp;&ensp;&ensp; The value of operator output sensors can be retrieved with the _compute_ resource, or with the _/average_ resource defined in the DCDB Pusher REST API.

> NOTE 3 &ensp;&ensp;&ensp;&ensp;&ensp; Developers can integrate their custom REST API resources that are plugin-specific, by implementing the _REST_ method in _OperatorTemplate_. To know more about plugin-specific resources, please refer to the respective documentation. 

> NOTE 4 &ensp;&ensp;&ensp;&ensp;&ensp; When operators employ a _root_ unit (e.g., when the Unit System is not used or a _globalOutput_ block is defined in regular operators) the _unit_ query can be omitted when performing a _/compute_ action.

### Rest Examples <a name="restExamples"></a>
In the following are some examples of REST requests over HTTPS:

* Listing the units associated to the _avgoperator1_ operator in the _aggregator_ plugin:
```bash
GET https://localhost:8000/analytics/units?plugin=aggregator;operator=avgOperator1
```
* Listing the output sensors associated to all operators in the _aggregator_ plugin:
```bash
GET https://localhost:8000/analytics/sensors?plugin=aggregator;
```
* Reloading the _aggregator_ plugin:
```bash
PUT https://localhost:8000/analytics/reload?plugin=aggregator
```
* Stopping the _avgOperator1_ operator in the _aggregator_ plugin:
```bash
PUT https://localhost:8000/analytics/stop?plugin=aggregator;operator=avgOperator1
```
* Performing a query for unit _/node00/cpu03/_ to the _avgOperator1_ operator in the _aggregator_ plugin:
```bash
PUT https://localhost:8000/analytics/compute?plugin=aggregator;operator=avgOperator1;unit=/node00/cpu03/
```

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; The analytics RestAPI requires authentication credentials as well.

# Plugins <a name="plugins"></a>
Here we describe available plugins in Wintermute, and how to configure them.

## Aggregator Plugin <a name="averagePlugin"></a>
The _Aggregator_ plugin implements simple data processing algorithms. Specifically, this plugin allows to perform basic
aggregation operations over a set of input sensors, which are then written as output.
The configuration parameters specific to the _Aggregator_ plugin are the following:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to retrieve recent readings for the input sensors, starting from the latest one.
| goBack | Value in milliseconds that can be used to shift back the aggregation window. Useful when very recent sensor data is not always available.

Additionally, output sensors in operators of the _Aggregator_ plugin accept the following parameters:

| Value | Explanation |
|:----- |:----------- |
| operation | Operation to be performed over the input sensors. Can be "sum", "average", "maximum", "minimum", "std", "percentiles" or "observations".
| percentile |  Specific percentile to be computed when using the "percentiles" operation. Can be an integer in the (0,100) range.
| relative | If true, the _relative_ query mode will be used. Otherwise the _absolute_ mode is used.


## Job Aggregator Plugin <a name="jobaveragePlugin"></a>

The _Job Aggregator_ plugin offers the same functionality as the _Aggregator_ plugin, but on a per-job basis. As such,
it performs aggregation of the specified input sensors across all nodes on which each job is running. Please refer
to the corresponding [section](#joboperators) for more details.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; The Job Aggregator plugin does not support the _relative_ option supported by the Aggregator plugin, and always uses the _absolute_ sensor query mode.

## Smoothing Plugin <a name="smoothingPlugin"></a>
The _Smoothing_ plugin performs similar functions as the _Aggregator_ plugin, but is optimized for performing running averages. It uses a single accumulator to compute approximate running averages, improving memory usage and reducing the amount of queried data.

Units ins the _Smoothing_ plugin are also instantiated differently compared to other plugins. As it is meant to perform running average on most or all sensors present in a system, there is no need to specify the input sensors of an operator: the plugin will automatically fetch all instantiated sensors in the system, and create separate units for them, each with their separate average output sensors. 
For this reason, pattern expressions specified on output sensors (e.g., _<bottomup 1, filter cpu>avg60_) are ignored and only the MQTT suffixes of the output sensors are used to construct units. If required, users can still specify input sensors using the Unit System syntax, to select subsets of the available sensors in the system. Operators of the _Smoothing_ plugin accept the following configuration parameters: 

| Value | Explanation |
|:----- |:----------- |
| separator | Character used to separate the MQTT prefix of output sensors (which is the input sensor's name) from the suffix (which is the average's identifier). Default is "#". 
| exclude | String containing a regular expression, defining which sensors must be excluded from the smoothing process.

Additionally, output sensors in operators of the _Smoothing_ plugin accept the following parameters:

| Value | Explanation |
|:----- |:----------- |
| range | Range in milliseconds of the average to be stored in this output sensor.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; The _Smoothing_ plugin will automatically update the metadata table of the input sensors in the units to expose the computed averages. This behavior can be altered by setting the _isOperation_ metadata attribute to _false_ for each output sensor. This way, the sensors associated to the averages will be published as independent entries.

## Regressor Plugin <a name="regressorPlugin"></a>

The _Regressor_ plugin is able to perform regression of sensors, by using _random forest_ machine learning predictors. The algorithmic backend is provided by the OpenCV library.
For each input sensor in a certain unit, statistical features from recent values of its time series are computed. These are, in this specific order, the average, standard deviation, sum of differences, 25th quantile, 75th quantile and latest value. These statistical features are then combined together in a single _feature vector_. 

In order to operate correctly, the models used by the regressor plugin need to be trained: this procedure is performed automatically online when using the _streaming_ mode, and can be triggered arbitrarily over the REST API. In _on demand_ mode, automatic training cannot be performed, and as such a pre-trained model must be loaded from a file.
The following are the configuration parameters available for the _Regressor_ plugin:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to retrieve recent readings for the input sensors, starting from the latest one.
| trainingSamples | Number of samples necessary to perform training of the current model.
| targetDistance | Temporal distance (in terms of lags) of the sample that is to be predicted.
| smoothResponses | If false, the regressor will attempt to predict one single sensor reading as specified by the _targetDistance_ parameter. If true, it will instead predict the _average_ of the upcoming _targetDistance_ sensor readings.
| inputPath | Path of a file from which a pre-trained random forest model must be loaded.
| outputPath | Path of a file to which the random forest model trained at runtime must be saved.
| getImportances | If true, the random forest will also compute feature importance values when trained, which are printed.
| rawMode | If true, only the average is used as feature for each of the sensor inputs.

> NOTE &ensp;&ensp;&ensp;&ensp;&ensp; When the _duplicate_ option is enabled, the _outputPath_ field is ignored to avoid file collisions from multiple regressors.

> NOTE 2 &ensp;&ensp;&ensp;&ensp;&ensp; When loading the model from a file and _getImportances_ is set to true, importance values will be printed only if the original model had this feature enabled upon training.

Additionally, input sensors in operators of the Regressor plugin accept the following parameter:

| Value | Explanation |
|:----- |:----------- |
| target | Boolean value. If true, this sensor represents the target for regression. Every unit in operators of the regressor plugin must have excatly one target sensor.

Finally, the Regressor plugin supports the following additional REST API actions:

| Action | Explanation |
|:----- |:----------- |
| train | Triggers a new training phase for the random forest model. Feature vectors are temporarily collected in-memory until _trainingSamples_ vectors are obtained. Until this moment, the old random forest model is still used to perform prediction.
| importances | Returns the sorted importance values for the input features, together with the respective labels, if available.

## Classifier Plugin <a name="classifierPlugin"></a>

The _Classifier_ plugin, as the name implies, performs machine learning classification. It is based on the Regressor plugin, and as such it also uses OpenCV random forest models. The plugin supplies the same options and has the same behavior as the Regressor plugin, with the following two exceptions:

* The _target_ parameter here indicates a sensor which stores the labels (as numerical integer identifiers) to be used for training and on which classification will be based. The mapping from the integer labels to their text equivalent is left to the users. Moreover, unlike in the
Regressor plugin, the target sensor is always excluded from the feature vectors.
* The _targetDistance_ parameter has a default value of 0. It can be set to higher values to perform predictive classification.

### Managing Models with the Classifier and Regressor Plugins <a name="classifierPluginModels"></a>

The _Classifier_ and _Regressor_ Wintermute plugins support saving models generated via online training, as well as loading pre-trained models from files.
The models to be loaded to not need to necessarily come from the Wintermute plugins themselves: the OpenCV pipeline is fully supported, and users can load _RTrees_ models
that have been generated and trained offline using the Python front-end. However, users need to make sure that the loaded models are _compatible_ with the specification in the corresponding Wintermute configuration.
The following aspects should be considered:

* The trained model must have the same number of inputs as the input specification in the Wintermute configuration file. Hence, the two models must use the same number of sensors and the same number of features (see _rawMode_ parameter).
* Currently, only models with a single output are supported.
* Both the scale and numerical format of the inputs and outputs should be taken into consideration. DCDB and Wintermute only support integers, hence this should be taken into account when training the model.
* Wintermute respects the order of sensors specified in the configuration files to build feature vectors. These are divided in consecutive blocks, each containing all features for a given sensor (again, see _rawMode_ parameter). In the case of classifier models, the _target_ sensor is never included in the feature vectors and is always skipped.
* If using a custom data processing step with different features from those available in the Wintermute plugins, it is advisable to set the _rawMode_ parameter to true and perform the necessary data processing in a separate plugin, constructing a pipeline.
* When loading a model and thus online training is not required, there is no need to specify a _target_ sensor.


## Clustering Plugin <a name="clusteringPlugin"></a>

The _Clustering_ plugin implements a gaussian mixture model for performance variation analysis and outlier detection. The plugin is based on the OpenCV library, similarly to the _Regressor_ plugin.
The input sensors of operators define the axes of the clustering space and hence the number of dimensions of the associated gaussian components. The units of the operator, instead, define the number of points in the clustering space: for this reason, 
the Clustering plugin employs hierarchical units, so that the clustering is performed once for all present sub-units, at each computation interval. When prediction is performed (after training of the GMM model, depending on the _reuseModel_ attribute) the
label of the gaussian component to which each sub-unit belongs is stored in the only output sensor of that sub-unit. Operators of the Clustering plugin support the following configuration parameters:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to retrieve recent readings for the input sensors, starting from the latest one.
| lookbackWindow | Enables a lookback window (whose length is expressed in milliseconds) to collect additional points from previous time windows in order to perform model training. The additional points will be accumulated in memory until training can be performed. Disabled by default.
| numComponents | Number of gaussian components in the mixture model.
| reuseModel | Boolean value. If false, the GMM model is trained at each computation, otherwise only once or when the "train" REST API action is used. Default is false.
| outlierCut | Threshold used to determine outliers when performing the Mahalanobis distance. 
| inputPath | Path of a file from which a pre-trained random forest model must be loaded.
| outputPath | Path of a file to which the random forest model trained at runtime must be saved.

The Clustering plugin does not provide any additional configuration parameters for its input and output sensors. 
However, it supports the following additional REST API actions:

| Action | Explanation |
|:----- |:----------- |
| train | Triggers a new training phase for the gaussian mixture model. At the next computation interval, the feature vectors of all units of the operator are combined to perform training, after which predicted labels are given as output.
| means | Returns the means of the generated gaussian components in the trained mixture model.
| covs | Returns the covariance matrices of the generated gaussian components in the trained mixture model.

## CS Signatures Plugin <a name="csPlugin"></a>

The _CS Signatures_ plugin computes signatures from sensor data, which aggregate data both in time and across sensors, and are composed by a specified number of complex-valued _blocks_. 
Each of the blocks is then stored in two separate sensors, which contain respectively the real and imaginary part of the block. Like in the _Regressor_ and _Classifier_ plugins, the CS algorithm is trained using a specified number of samples, which are accumulated in memory, subsequently learning the correlations between sensors.
Operators in this plugin support the following configuration parameters:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to retrieve recent readings for the input sensors, starting from the latest one, that are then aggregated in the signatures.
| trainingSamples | Number of samples for the sensors that are to be used to train the CS algorithm.
| numBlocks | Desired number of blocks in the signatures.
| scalingFactor | Scaling factor (and upper bound) used to compute the blocks. Default is 1000000.
| inputPath | Path of a file in which the order of the sensors and their upper/lower bounds are stored.
| outputPath | Path of a file to which the order of the sensors and their upper/lower bounds must be saved.

Additionally, the output sensors of the CS Signatures plugin support the following parameters:

| Value | Explanation |
|:----- |:----------- |
| imag | Boolean value. Specifies whether the sensor should store the imaginary or real part of a block.

The output sensors are automatically duplicated according to the specified number of blocks, and a numerical identifier is appended to their MQTT topics. If no sensor with the _imag_ parameter set to true is specified, the signatures will contain only their real parts.
Finally, the plugin supports the following REST API actions:

| Action | Explanation |
|:----- |:----------- |
| train | Triggers a new training phase for the CS algorithm. For practical reasons, only the sensor data from the first unit of the operator is used for training.

## Tester Plugin <a name="testerPlugin"></a>
The _Tester_ plugin can be used to test the functionality and performance of the query engine, as well as of the Unit System. It will perform a specified number of queries over the set of input sensors for each unit, and then output as a sensor the total number of retrieved readings. The following are the configuration parameters for operators in the _Tester_ plugin:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to retrieve recent readings for the input sensors, starting from the latest one.
| queries | Number of queries to be performed at each computation interval. If more than the number of input sensors per unit, these will be looped over multiple times.
| relative | If true, the _relative_ query mode will be used. Otherwise the _absolute_ mode is used.

# Sink Plugins <a name="sinkplugins"></a>
Here we describe available plugins in Wintermute that are devoted to the output of sensor data (_sinks_), and that do not perform any analysis.

## File Sink Plugin <a name="filesinkPlugin"></a>
The _File Sink_ plugin allows to write the output of any other sensor to the local file system. As such, it does not produce output sensors by itself, and only reads from input sensors.
The input sensors can either be fully qualified, or can be described through the Unit System. In this case, multiple input sensors can be generated automatically, and the respective output paths need to be adjusted by enabling the _autoName_ attribute described below, to prevent multiple sensors from being written to the same file. The file sink operators (named sinks) support the following attributes:

| Value | Explanation |
|:----- |:----------- |
| autoName | Boolean. If false, the output paths associated to sensors are interpreted literally, and a file is opened for them. If true, only the part in the path describing the current directory is used, while the file itself is named accordingly to the MQTT topic of the specific sensor.

Additionally, input sensors in sinks accept the following parameters:

| Value | Explanation |
|:----- |:----------- |
| path | The path to which the sensors's readings should be written. It is interpreted as described above for the _autoName_ attribute.

## Cooling Control Plugin <a name="coolingcontrolPlugin"></a>

The _Cooling Control_ plugin enables proactive control of the inlet temperature in a warm-water cooling system using the SNMP protocol. Its logic is simple: at each computation interval, operators fetch as input temperature readings for a set of sensors, as specified in the configuration.
Then, for each sensor, an operator establishes if the associated temperature is above a certain _hotThreshold_ value, in which case the sensor is flagged as _hot_. If the percentage of hot sensors is higher than a _hotPercentage_ value, the operator proceeds to decrease the inlet temperature. Otherwise, it increases it.
The plugin provides two different possible cooling strategies:

* _continuous_: at each computation interval the operator sets a new inlet temperature value. The more sensors are flagged as hot in comparison to _hotPercentage_, the steeper the decrease in temperature - conversely, the increase in temperature will be stronger if just a minimal amount of nodes is flagged as hot.
* _stepped_: this strategy functions similarly to _continuous_, with a more conservative approach. The main difference is that the space of possible inlet temperature settings is not continuous, but rather separated in evenly-spaced bins: the new inlet temperature setting is applied via SNMP only upon crossing boundaries between bins, and otherwise it is simply stored locally.

Operators in this plugin cannot have any output sensors. The operators themselves provide the following configuration parameters:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to query temperature sensors. In order for a sensor to be flagged as _hot_, all of the readings in the time window must be above _hotThreshold_. Defaults to 0. 
| hotPercentage | Percentage of sensors (from 0 to 100) to be flagged as _hot_ in order to trigger a decrease in the inlet temperature. Defaults to 20.
| minTemperature | Minimum allowed value to be set as inlet temperature. The scale must be appropriate for the corresponding SNMP sensor. Defaults to 350.
| maxTemperature | Maximum allowed value to be set as inlet temperature. Defaults to 450.
| bins | Number of bins in which the (_minTemperature_, _maxTemperature_)range is divided when using the _stepped_ strategy. Defaults to 4.
| strategy | Cooling strategy to be used. Can either be _continuous_ or _stepped_. Defaults to _stepped_.
| OIDPrefix | OID prefix of the SNMP sensor that can be used to control the inlet temperature.
| OIDSuffix | OID suffix of the SNMP sensor that can be used to control the inlet temperature.

Cooling Control operators additionally support all of the configuration parameters available in the _dcdbpusher_ SNMP plugin in order to set up connections to SNMP hosts. In particular, these are _Host_, _Community_, _Version_, _Username_, _SecLevel_, _AuthProto_, _PrivProto_, _AuthKey_ and _PrivKey_. 
Sensors in the Cooling Control plugin support the following parameters:

| Value | Explanation |
|:----- |:----------- |
| hotThreshold | Threshold value for the sensor to be considered _hot_. It must have the same scale as the readings of the sensor itself. Defaults to 70.
| critThreshold | Additional threshold value higher than _hotThreshold_. If a component reaches this threshold it is considered to be in a critical state (e.g., subject to thermal throttling) and hence a decrease in inlet temperature is triggered regardless of the current number of _hot_ nodes. Disabled by default.

Finally, the plugin supports the following REST API actions:

| Action | Explanation |
|:----- |:----------- |
| status | Displays the temperature settings currently in use, as well as the cooling strategy.

## Health Checker Plugin <a name="healthcheckerPlugin"></a>

The _Health Checker_ plugin allows to monitor sensors and raise alarms if anomalous conditions are detected - these are threshold-based, and can be defined on a per-sensor basis. 
Whenever an alarm is raised, the plugin can execute arbitrary scripts and programs to respond accordingly: for example, this can be used to send automatic emails to system administrators. The following is an example of alarm produced by the Health Checker plugin:

``` 
[11:45:55] <warning>: The following alarm conditions were detected by the DCDB Health Checker plugin:

    - Sensor /system/node1/power is not providing any data.
    - Sensor /system/node1/temp has a reading greater than threshold 95000.
``` 

Operators in this plugin cannot have any output sensors. The operators themselves provide the following configuration parameters:

| Value | Explanation |
|:----- |:----------- |
| window | Length in milliseconds of the time window that is used to query sensors. Defaults to 0. 
| command | Command to be executed when an alarm is raised. This must contain the _%s_ marker, which is replaced at runtime with a descriptive text of the current alarm. The command is executed in a shell environment (similarly to _popen_) and thus can contain most typical shell constructs (e.g., pipes or re-directs). Default is none.
| log | Boolean. If true, whenever an alarm is raised the event is written to the standard DCDB log on top of being transmitted to the external command. Default is true.
| cooldown | Length in milliseconds of a _cooldown_ time window, in which a given alarm cannot be raised again a second time. Defaults to 0.

Sensors in the Health Checker plugin support the following parameters:

| Value | Explanation |
|:----- |:----------- |
| condition | Condition at which an alarm is raised for a given sensor, with respect to a certain threshold. This can be _above_ (readings are greater than the threshold), _below_ (readings are smaller than the threshold), _equals_ (readings are equal to the threshold) or _exists_ (sensor data must be available at all times). Only one condition can be defined per sensor.
| threshold | Numerical value that is used to verify the condition explained by the _condition_ parameter.

# Writing Wintermute Plugins <a name="writingPlugins"></a>
Generating a DCDB Wintermute plugin requires implementing a _Operator_ and _Configurator_ class which contain all logic
tied to the specific plugin. Such classes should be derived from _OperatorTemplate_ and _OperatorConfiguratorTemplate_
respectively, which contain all plugin-agnostic configuration and runtime features. Please refer to the documentation 
of the _Aggregator_ plugin for an overview of how a basic plugin can be implemented.
