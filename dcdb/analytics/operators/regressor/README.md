# Using the Wintermute Regressor Plugin

### Table of contents
1. [Introduction](#introduction)
2. [DCDB Configuration](#dcdbconfig)
    1. [Global Configuration](#globalconf)
    2. [PerfEvent Configuration](#perfeventconf)
    3. [ProcFS Configuration](#procfsconf)
    4. [SysFS Configuration](#sysfsconf)
3. [Wintermute Configuration](#wintermuteconfig)
    1. [Aggregator Configuration](#aggregatorconf)
    2. [Regressor Configuration](#regressorconf)
4. [Final Result](#result)


# Introduction <a name="introduction"></a>
In this Readme we present an end-to-end example for the usage of Wintermute, specifically for the purpose of power
consumption prediction in a HPC compute node. This example corresponds to _Case Study 1_ in the work by Netti et al. [1],
and numerical results as well as insights can be found there. Here, we focus on the configuration of the DCDB Pusher
component alone. Both sampling and regression are performed at a fine granularity of 250ms.

# DCDB Configuration <a name="dcdbconfig"></a>

We will first present the DCDB  Pusher configuration which supplies the necessary monitoring sensors to perform the regression.
We will first discuss the global DCDB Pusher configuration, and then we will discuss each single plugin separately.

## Global Configuration <a name="globalconf"></a>

The DCDB Pusher _global.conf_ configuration file simply contains global configuration parameters, as well as the list
of monitoring and operator plugins to be instantiated. To each of them will correspond a _pluginName.conf_ configuration
file that will be parsed. In this context, we use the _PerfEvent, ProcFS_ and _SysFS_ monitoring plugins, coupled by the
_Aggregator_ and _Regressor_ operator plugins.

``` 
global {
	mqttBroker    127.0.0.1:1883
	mqttPrefix    /regression-test
	threads       10
	verbosity     3
	qosLevel      0
	daemonize     false
	tempdir	      ./
	cacheInterval 120
}

restAPI {
	address     127.0.0.1:8100
	certificate deps/openssl-1.1.1c/test/certs/ca-cert.pem
	privateKey  deps/openssl-1.1.1c/test/certs/ca-key.pem
	dhFile      deps/openssl-1.1.1c/crypto/dh/dh2048.pem
}

plugins {
	plugin procfs {
			config
	}
	plugin perfevent {
			config
	}
	plugin sysfs {
			config
	}
}

operatorPlugins {
	operatorPlugin aggregator {
		config
	}
	operatorPlugin regressor {
		config
	}
}
``` 

## PerfEvent Configuration <a name="perfeventconf"></a>

We use the _PerfEvent_ plugin to sample CPU performance counters at a fine granularity. We sample most of the default
events provided by the _perfevent_ interface, omitting any architecture-specific raw events. For this test we are using
a Intel Knights Landing-based compute node, which is equipped with 64 CPU cores: for this reason, counters are sampled for
CPUs 0 to 63. However, for our regression experiment we will require compute node-level performance counters, which 
means we will have to add an aggregation stage later in the processing pipeline. For this same reason, we specify a
_subSampling_ value of 0: this setting means that the sensors are sampled and exposed locally, but are never sent out
over MQTT. This allows us to reduce the amount of data being sent out and stored in the database.

``` 
template_group def1 {
        interval        250
        minValues       10
        subSampling     0
        cpus            0-63
        htVal           64
}

group fixed {
        default     def1
        counter instructions {
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_INSTRUCTIONS
                mqttsuffix      /instructions
        }
}
group fixed2 {
        default     def1
        counter cpu-cycles {
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_CPU_CYCLES
                mqttsuffix      /cpu-cycles
        }
}
group fixed3 {
        default     def1
        counter ref-cycles {
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_REF_CPU_CYCLES
                mqttsuffix      /ref-cycles
        }
}
group branches {
        default     def1
        counter branch-instructions {
                mqttsuffix      /branch-instructions
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_BRANCH_INSTRUCTIONS
        }
}
group branches2 {
        default      def1
        counter branch-misses {
                mqttsuffix      /branch-misses
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_BRANCH_MISSES
        }
}
group cache {
        default def1
        counter references {
                mqttsuffix      /cache-references
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_CACHE_REFERENCES
        }
}
group cache2 {
        default def1
        counter misses {
                mqttsuffix      /cache-misses
                type            PERF_TYPE_HARDWARE
                config          PERF_COUNT_HW_CACHE_MISSES
        }
}
``` 

## ProcFS Configuration <a name="procfsconf"></a>

We use the _ProcFS_ plugin to sample several kernel-level indicators of CPU and memory performance. These come from
the _procstat_, _meminfo_ and _vmstat_ files respectively. Since we are only interested in obtaining compute node-level
CPU metrics, we sample the column metrics in the _procstat_ file with the _perCpu_ switch turned off. This removes any
necessity to perform aggregation later in the pipeline.

``` 
template_file def1 {
        interval    250
        minValues   10
}

file procstat {
        default     def1
        path        /proc/stat
        type        procstat

        metric col_user {
            type            col_user
            mqttsuffix      /col-user
            perCpu          off
        }
        metric col_nice {
            type            col_nice
            mqttsuffix      /col-nice
            perCpu          off
        }
        metric col_system {
            type            col_system
            mqttsuffix      /col-system
            perCpu          off
        }
        metric col_idle {
            type            col_idle
            mqttsuffix      /col-idle
            perCpu          off
        }
        metric col_iowait {
            type            col_iowait
            mqttsuffix      /col-iowait
            perCpu          off
        }
        metric intr {
            type            intr
            mqttsuffix      /intr
            perCpu          off
            delta		    true
        }
        metric ctxt {
            type            ctxt
            mqttsuffix      /ctxt
            perCpu          off
            delta           true
        }
        metric processes {
            type            processes
            mqttsuffix      /processes
            perCpu          off
        }
        metric procs_running {
            type            procs_running
            mqttsuffix      /procs-running
            perCpu          off
        }
        metric procs_blocked {
            type            procs_blocked
            mqttsuffix      /procs-blocked
            perCpu          off
        } 
        metric softirq {
            type            softirq
            mqttsuffix      /softirq
            perCpu          off
            delta           true
        }
}

file meminfo {
        default     def1
        path        /proc/meminfo
        type        meminfo

        metric MemFree {
            type            MemFree
            mqttsuffix      /memfree
        }
        metric Active {
            type            Active
            mqttsuffix      /active
        }
        metric AnonPages {
            type            AnonPages
            mqttsuffix      /anonpages
        }
        metric KernelStack {
            type            KernelStack
            mqttsuffix      /kernelstack
        }
}

file vmstat {
        default     def1
        path        /proc/vmstat
        type        vmstat

        metric pgfault {
            type            pgfault
            mqttsuffix      /pgfault
        }
        metric pgmajfault {
            type            pgmajfault
            mqttsuffix      /pgmajfault
        }
        metric thp_split {
            type            thp_split
            mqttsuffix      /thp-split
        }
}
``` 

## SysFS Configuration <a name="sysfsconf"></a>

Finally, we use the _SysFS_ plugin to sample remaining hardware and software sensors that will be useful in the regression
process. Among those there is a _power_ sensor, which will provide us the data to train a regression model, and which
corresponds to an hardware instantaneous power meter present in our KNL-based compute nodes.

``` 
template_single_sensor def1 {
    interval	250
    minValues	10
}

single_sensor frequency {
    default def1
    path /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
    mqttsuffix /frequency
}
single_sensor knltemp {
    default def1
    path	/sys/devices/virtual/thermal/thermal_zone0/temp
    mqttsuffix /knltemp
}
single_sensor hfi0temp {
    default	def1
    path	/sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/infiniband/hfi1_0/tempsense
    filter	"s/([[:digit:]]+).([[:digit:]]+).*/\\1\\20/"
    mqttsuffix /hfi0temp
}
single_sensor hfi1temp {
    default def1
    path    /sys/devices/pci0000:00/0000:00:02.0/0000:02:00.0/infiniband/hfi1_1/tempsense
    filter  "s/([[:digit:]]+).([[:digit:]]+).*/\\1\\20/"
    mqttsuffix /hfi1temp
}
single_sensor energy {
    default def1
    path 	/sys/devices/platform/sem/energy_j	
    mqttsuffix /energy
}
single_sensor power {
    default def1
    path   /sys/devices/platform/sem/power_mw
    mqttsuffix /power
}
``` 

# Wintermute Configuration <a name="wintermuteconfig"></a>

We now discuss the configuration of Wintermute, with all single plugins composing the ODA processing chain. For our
experiment we use the _Aggregator_ and _Regressor_ plugin, which are connected to form a pipeline.

## Aggregator Configuration <a name="aggregatorconf"></a>

The _Aggregator_ plugin is the first in the processing pipeline, and its sole purpose is to aggregate the performance
counters sampled by the _PerfEvent_ plugin, which are per-CPU core, using a sum operation in order to obtain compute 
node-level metrics, which will be used later by the _Regressor_ plugin. Note that all input sensors of the various
operators have the pattern expression _<bottomup, filter cpu>_, which means the per-CPU performance counters are all
at the bottom of the sensor tree, and all contain the "cpu" keyword. The output sensors, on the other hand, have the
pattern expression _<bottomup 1>_, so one level above the CPU cores and corresponding to the compute node itself. This
particular configuration can be deployed on both DCDB Pushers and Collect Agents without any alterations.

A second element that should be noted is the _delay_ parameter: this specifies an offset for triggering the
operator in online mode, in milliseconds, and its main purpose is to ensure that an operator always receives the latest
value from an input sensor that is sampled locally. In fact, by default in DCDB Pusher all sensor reading actions are
synchronized and fired at the same time periodically. 

``` 
template_aggregator def1 {
	interval    250
	delay		10
	minValues   10
	window	    0
	operation   sum
	streaming   true
	duplicate   false
}

aggregator avg1 {
	default     def1

	input {
		sensor "<bottomup, filter cpu>instructions"	
	}
	output {
		sensor "<bottomup 1>instructions" {
			mqttsuffix  /instructions
		}
	}
}
aggregator avg2 {
	default     def1

	input {
		sensor "<bottomup, filter cpu>cache-misses"
	}  
	output {
		sensor "<bottomup 1>cache-misses" {
			mqttsuffix  /cache-misses
		}
	}
}      
aggregator avg3 {
	default     def1

	input {
		sensor "<bottomup, filter cpu>branch-instructions"
	}
	output {
		sensor "<bottomup 1>branch-instructions" {
			mqttsuffix  /branch-instructions
		}
	}
}
aggregator avg4 {
    default     def1

	input {
	    sensor "<bottomup, filter cpu>cpu-cycles"
	}
	output {
		sensor "<bottomup 1>cpu-cycles" {
			mqttsuffix  /cpu-cycles
		}
	}
}
aggregator avg5 {
	default     def1

	input {
		sensor "<bottomup, filter cpu>branch-misses"
	}
	output {
		sensor "<bottomup 1>branch-misses" {
			mqttsuffix  /branch-misses
		}
	}
}
``` 

## Regressor Configuration <a name="regressorconf"></a>

We conclude the configuration with the _Regressor_ plugin itself, which will allow us to perform random forest-based
power consumption prediction. For the purpose of this configuration, the most important flag to note is _relaxed_: this
flag allows the configuration algorithm to relax certain consistency checks, and in turn enables pipelining of operators
which run concurrently in the same DCDB Pusher or Collect Agent instance. This is necessary, in this context, as the
_Regressor_ operator require the compute node-level CPU performance metrics calculated by the _Aggregator_ plugin.

It can then be seen that most of the sensors we configured previously are present in the list of input sensors. Among
these, the most important is _power_, which has the _target_ option enabled: this instructs the regressor to use this
specific sensor as source for the responses with which the model will be trained, thus achieving regression. Correspondingly,
there is only one output sensor, which will contain the output of the periodic regression process.

Similarly as done before, this _Regressor_ configuration can be deployed both in a DCDB Pusher and Collect Agent without
alterations. In this test we perform everything on the DCDB Pusher side for data liveness reasons: however, in a
production scenario and with more coarse-grained regression applications, users could also deploy the _Aggregator_ plugin
in the compute nodes' DCDB Pushers, and the _Aggregator_ plugin on a Collect Agent, thus distributing load. Then, the
_Aggregator_ plugin will automatically instantiate one independent model for each single compute node.

``` 
template_regressor def1 {
	interval    	250
	delay	        20
	minValues   	10
	duplicate       false
	streaming       true
	relaxed         true
	window          3000
	trainingSamples 30000
	targetDistance	1
}

regressor reg1 {
	default         def1
	getImportances  true
	
	input {
		sensor "<bottomup 1>knltemp"
		sensor "<bottomup 1>active"
		sensor "<bottomup 1>thp-split"
		sensor "<bottomup 1>kernelstack"
		sensor "<bottomup 1>frequency"
		sensor "<bottomup 1>intr"
		sensor "<bottomup 1>col-idle"
		sensor "<bottomup 1>col-user"
		sensor "<bottomup 1>col-system"
		sensor "<bottomup 1>cpu-cycles"
		sensor "<bottomup 1>instructions"
		sensor "<bottomup 1>branch-instructions"
		sensor "<bottomup 1>branch-misses"
		sensor "<bottomup 1>cache-misses"
		sensor "<bottomup 1>power" {
			target true
		}
	}
	output {
		sensor "<bottomup 1>power-pred" {
				mqttsuffix  /power-pred
		}
	}
}
``` 

# Final Result <a name="result"></a>

Once the configuration is completed and DCDB Pusher is started, all sensors and operators will be instantiated. At the
end of the start sequence, DCDB also prints the expected MQTT message rate. We expect this value to be very low, as 
we are not sending out any CPU-level metric, but only few compute node-level ones, even though at a fine granularity. 
The resulting log message has the following format:

``` 
[15:21:57] <info>: Signal handlers registered!
[15:21:57] <info>: Cleaning up...
[15:21:57] <info>: Setup complete!
[15:21:57] <info>: Mosquitto: Connection established!
[15:21:57] <info>: Mosquitto: No message cap enforced. Predicted message rate is 12.8 msg/s.
```

Here we omit any analytical results of the model itself. These can be found in the work by Netti et al. [1], where an
extensive discussion of the model's effectiveness and insights on the associated results are presented. Since
in this experiment we did not supply an input pre-trained model for the _Regressor_ plugin, its operators will start accumulating
sensor data in-memory, until a certain training set size (as specified in the _trainingSamples_ attribute) is reached,
after which training is performed and the actual regression starts. After the training is done, if the _getImportances_
flag is enabled, the model will also log the list of most important metrics for regression, as quantified by the
underlying random forest. This list looks like the following for our experiments:

``` 
[17:04:54] <info>: Operator reg1: model training performed.
[17:04:54] <info>: Operator reg1: listing feature importances for unit /regression-test/:
    /regression-test/col-user - diffsum - 0.098820
    /regression-test/power - latest - 0.076799
    /regression-test/power - qtl75 - 0.076525
    /regression-test/col-user - std - 0.072411
    /regression-test/power - qtl25 - 0.065518
    /regression-test/power - mean - 0.053110
    /regression-test/active - latest - 0.050822
    /regression-test/active - qtl75 - 0.049808
    /regression-test/active - mean - 0.046072
    /regression-test/instructions - mean - 0.040971
    /regression-test/col-idle - diffsum - 0.036893
    /regression-test/knltemp - qtl25 - 0.023455
    /regression-test/active - qtl25 - 0.023453
    /regression-test/knltemp - qtl75 - 0.022546
    /regression-test/cpu-cycles - latest - 0.021389
    /regression-test/knltemp - mean - 0.018785
    /regression-test/branch-misses - qtl75 - 0.017387
    /regression-test/instructions - latest - 0.017322
    /regression-test/knltemp - latest - 0.015392
    /regression-test/col-idle - std - 0.015035
    /regression-test/instructions - qtl75 - 0.011739
    /regression-test/cpu-cycles - qtl75 - 0.010702
    /regression-test/intr - mean - 0.009674
    /regression-test/instructions - qtl25 - 0.008478
    /regression-test/branch-instructions - diffsum - 0.008013
    /regression-test/branch-instructions - mean - 0.007568
    /regression-test/cache-misses - mean - 0.007557
    /regression-test/power - diffsum - 0.007256
    /regression-test/col-system - diffsum - 0.007174
    /regression-test/cache-misses - qtl25 - 0.005752
    /regression-test/col-system - std - 0.005485
    /regression-test/knltemp - diffsum - 0.005338
    /regression-test/branch-instructions - qtl25 - 0.005212
    /regression-test/branch-misses - latest - 0.004962
    /regression-test/intr - qtl75 - 0.004896
    /regression-test/branch-misses - qtl25 - 0.004131
    /regression-test/branch-misses - std - 0.003868
    /regression-test/cpu-cycles - mean - 0.003541
    /regression-test/active - diffsum - 0.003464
    /regression-test/instructions - diffsum - 0.002646
    /regression-test/intr - qtl25 - 0.002489
    /regression-test/knltemp - std - 0.002325
    /regression-test/branch-instructions - std - 0.002231
    /regression-test/intr - latest - 0.002051
    /regression-test/cache-misses - qtl75 - 0.001984
    /regression-test/branch-misses - mean - 0.001888
    /regression-test/cpu-cycles - qtl25 - 0.001722
    /regression-test/cache-misses - diffsum - 0.001657
    /regression-test/branch-instructions - latest - 0.001555
    /regression-test/instructions - std - 0.001431
    /regression-test/branch-instructions - qtl75 - 0.001334
    /regression-test/cache-misses - latest - 0.001326
    /regression-test/intr - diffsum - 0.001317
    /regression-test/power - std - 0.001294
    /regression-test/intr - std - 0.001103
    /regression-test/cache-misses - std - 0.001043
    /regression-test/kernelstack - mean - 0.000965
    /regression-test/col-system - mean - 0.000609
    /regression-test/active - std - 0.000517
    /regression-test/branch-misses - diffsum - 0.000217
    /regression-test/col-idle - latest - 0.000162
    /regression-test/kernelstack - qtl25 - 0.000157
    /regression-test/thp-split - diffsum - 0.000144
    /regression-test/kernelstack - std - 0.000103
    /regression-test/cpu-cycles - diffsum - 0.000087
    /regression-test/kernelstack - latest - 0.000070
    /regression-test/kernelstack - qtl75 - 0.000061
    /regression-test/kernelstack - diffsum - 0.000054
    /regression-test/col-system - qtl25 - 0.000045
    /regression-test/col-user - latest - 0.000031
    /regression-test/cpu-cycles - std - 0.000027
    /regression-test/thp-split - mean - 0.000015
    /regression-test/col-idle - qtl25 - 0.000009
    /regression-test/thp-split - std - 0.000008
    /regression-test/col-user - mean - 0.000003
    /regression-test/thp-split - latest - 0.000001
``` 

# References <a name="references"></a>

* [1] Alessio Netti, Micha Mueller, Carla Guillen, Michael Ott, Daniele Tafani, Gence Ozer and Martin Schulz. _DCDB Wintermute: Enabling Online and Holistic Operational Data Analytics on HPC Systems_. Proceedings of the International Parallel and Distributed Processing Symposium (IPDPS) 2020, _submitted_. [arXiv pre-print available here.](https://arxiv.org/abs/1910.06156) 
