# The Data Center Data Base

The *Data Center* *Data Base* (DCDB) is a modular, continuous and holistic monitoring framework targeted at HPC environments. The framework consists of the three main parts:

* _Storage_ _Backend_: Here all the collected data is stored. By default a Cassandra database is used but the framework is intended to support usage of other data storage solutions as well.
* _Collect_ _Agent_: The collect agent functions as intermediary between one storage backend and one or multiple pushers.
* _Pusher_: The main part for collecting data is the pusher framework. It allows to run arbitrary data collection plugins and pushes the data to the collect agent via MQTT messages.

Other features included in DCDB:
* libdcdb: a library to access the storage backend with external tools.
* A set of tools leveraging libdcdb (e.g. a command line tool to query the storage backend).
* Built-in data analytics framework.
* Pusher plugins for a variety of typical HPC data sources.

## Build, install and run

Prerequisites:
* cmake
* C++11 compiler
* Doxygen (to build documentation)

```bash
//Create a new directory for DCDB
$ mkdir DCDB && cd DCDB

//Download the DCDB sources
$ git clone https://gitlab.lrz.de/dcdb/dcdb.git

//Start building and installation of DCDB and all of its dependencies.
$ cd dcdb
$ make depsinstall install
//OR use this make command instead to also build the doxygen documentation
$ make depsinstall install doc
```

If you would like to enable DCDB's advanced caching features, add the -DUSE_SENSOR_CACHE parameter to the CXXFLAGS string in the config.mk file. To run DCDB locally with the included default configuration files:

```bash
//Change to the DCDB directory you created during installation
$ cd DCDB
//Set required environment variables
$ source install/dcdb.bash
//Start one storage backend and one collect agent locally
$ install/etc/init.d/dcdb start

//Start a dcdbpusher instance
$ cd dcdb/dcdbpusher
$ ./dcdbpusher config/
```

## References

* Alessio Netti, Micha Mueller, Axel Auweter, Carla Guillen, Michael Ott, Daniele Tafani and Martin Schulz. _From Facility to Application Sensor Data: Modular, Continuous and Holistic Monitoring with DCDB_. Proceedings of the International Conference for High Performance Computing, Networking, Storage, and Analysis (SC) 2019. [arXiv pre-print available here.](https://arxiv.org/abs/1906.07509) 
* Alessio Netti, Micha Mueller, Carla Guillen, Michael Ott, Daniele Tafani, Gence Ozer and Martin Schulz. _DCDB Wintermute: Enabling Online and Holistic Operational Data Analytics on HPC Systems_. Proceedings of the International Symposium on High-Performance Parallel and Distributed Computing (HPDC) 2020. [arXiv pre-print available here.](https://arxiv.org/abs/1910.06156) 


## Contact, Copyright and License

DCDB was created at Leibniz Supercomputing Centre (LRZ).

For questions and/or suggestions please contact info@dcdb.it

Copyright (C) 2011-2020 Leibniz Supercomputing Centre

DCDB (except for the libdcdb parts) is licensed under the terms
and conditions of the GNU GENERAL PUBLIC LICENSE (Version 2 or later).

Libdcdb is licensed under the terms and conditions of the GNU LESSER
GENERAL PUBLIC LICENSE (Version 2.1 or later).

See the `LICENSE` file for more information.

<!---
 TODO:
 write pages for:
 -how to use dcdb
 -architecture of dcdb
 -develop plugin for pusher/analyzer
 -develop for framework itself
 -dig deeper into doxygen to get a beatiful documentation. perhaps use
  markdown files for pages (for tutorials, architecture explanation)
-->
