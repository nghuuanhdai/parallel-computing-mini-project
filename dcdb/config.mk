DCDBBASEPATH ?= $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)
DCDBDEPSPATH ?= $(DCDBBASEPATH)/deps
DCDBDEPLOYPATH ?= $(DCDBBASEPATH)/install

# dcdbpusher plugins to be built
PLUGINS = sysfs ipmi pdu bacnet snmp procfs tester gpfsmon msr rest

# data analytics plugins to be built
OPERATORS = aggregator smoothing regressor classifier clustering cssignatures job_aggregator testeroperator filesink smucngperf persystsql coolingcontrol healthchecker

DEFAULT_VERSION = 0.4
GIT_VERSION = $(shell git describe --tags 2>/dev/null|sed -e 's/-\([0-9]*\)/.\1/'|tr -d '\n'; if ! git diff-index --quiet HEAD --; then echo "~"; fi)
VERSION := $(if $(GIT_VERSION),$(GIT_VERSION),$(DEFAULT_VERSION))

CXXFLAGS = -std=c++11 -O2 -g -Wall -fmessage-length=0 \
           -DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG \
           -DBOOST_LOG_DYN_LINK -DVERSION=\"$(VERSION)\" \
           -Wno-unused-function \
           -Wno-unused-variable
FULL_CC = $(shell which $(CC))
FULL_CXX = $(shell which $(CXX))

OS = $(shell uname -s)
MAKETHREADS ?= $(if $(findstring $(shell uname),Darwin),$(shell sysctl machdep.cpu.thread_count | cut -b 27-),\
               $(if $(findstring $(shell uname),Linux),$(shell cat /proc/cpuinfo | grep processor | wc -l),4))
               
ifneq ($(OS),Darwin)
	PLUGINS += perfevent
endif

