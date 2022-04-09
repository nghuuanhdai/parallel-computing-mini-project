#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

# Reminder in shell 0 = true, 1 = false

#TODO: make sensor and group name configurable

# Init variables
DATE=`date '+%Y'`
AUTHOR="Your name goes here!"
PLUGIN_NAME=""
enableEntities=false

# parse input
while getopts "hp:e:a:" opt; do
	case "$opt" in
	h)
		echo "Usage:"
		echo "./generatePlugin.sh -p pluginName"
		echo "Options:"
		echo "-h      Print this help section"
		echo "-e xx   Generate files for entity"
		echo "-a xx   Specify name of the author which is  to be mentioned in the"
		echo "\tsource files"
		exit 0
		;;
	p)	PLUGIN_NAME=$OPTARG
		echo "Plugin named ${PLUGIN_NAME}"
		;;
	e)	enableEntities=true
	  ENTITY_NAME=$OPTARG
    echo "Generating entity ${ENTITY_NAME}"
		;;
	a)	AUTHOR=$OPTARG
		echo "Author: ${AUTHOR}"
		;;
	esac
done

# first check input parameters
if [ -z ${PLUGIN_NAME} ]
  then
    echo "Invalid plugin name!"
    exit 1
fi

# Set further variables
PLUGIN_NAME_UPC=`echo $PLUGIN_NAME | tr 'a-z' 'A-Z'`
PLUGIN_NAME_LWC=`echo $PLUGIN_NAME | tr 'A-Z' 'a-z'`
if [ "$enableEntities" = true ]
then
	ENTITY_NAME_UPC=`echo $ENTITY_NAME | tr 'a-z' 'A-Z'`
	ENTITY_NAME_LWC=`echo $ENTITY_NAME | tr 'A-Z' 'a-z'`
fi

if [ -e ${PLUGIN_NAME_LWC} ]
then
	echo "${PLUGIN_NAME_LWC}/ directory already present. Overwriting"
else
	echo "Creating ${PLUGIN_NAME_LWC}/ directory"
	mkdir ${PLUGIN_NAME_LWC}
fi
cd ${PLUGIN_NAME_LWC}

# Generate ...SensorBase.h
echo "Generating ${PLUGIN_NAME}SensorBase.h ..."
. ../SensorBase_h.sh

# Generate ...SensorGroup.h
echo "Generating ${PLUGIN_NAME}SensorGroup.h ..."
. ../SensorGroup_h.sh

# Generate ...SensorGroup.cpp
echo "Generating ${PLUGIN_NAME}SensorGroup.cpp ..."
. ../SensorGroup_cpp.sh

if [ "$enableEntities" = true ]
then
	# Generate ...$Entity.h
	echo "Generating ${PLUGIN_NAME}${ENTITY_NAME}.h ..."
	. ../Entity_h.sh

	# Generate ...$Entity.cpp
	echo "Generating ${PLUGIN_NAME}${ENTITY_NAME}.cpp ..."
	. ../Entity_cpp.sh
fi

# Generate ...Configurator.h
echo "Generating ${PLUGIN_NAME}Configurator.h ..."
. ../Configurator_h.sh

# Generate ...Configurator.cpp
echo "Generating ${PLUGIN_NAME}Configurator.cpp ..."
. ../Configurator_cpp.sh

# Generate Makefile code
echo "Generating code for Makefile ..."
. ../makefile.sh

# Generate ....conf
echo "Generating ${PLUGIN_NAME}.conf ..."
. ../config.sh

echo ""
echo "Plugin generator completed succefully"
echo ""
echo "Further steps to take from here:"
echo "* The generated files should go to their appropriate location:"
echo "  - the .conf goes to the other config files in config/"
echo "  - the source files should go in their own plugin directory at src/sensors/"
echo "  - copy required lines into Makefile as instructed in appendToMakefile.txt"
echo "* Complete the code in the generated source files. The TODOs are a good starting point, but you"
echo "  may need to make even more changes (and create more source files) for your plugin to work"
echo "* Extend the .conf and Makefile as required."
