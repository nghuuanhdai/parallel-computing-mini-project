#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > appendToMakefile.txt
Append to PLUGINS variable: ${PLUGIN_NAME_LWC}

Append at end of Makefile:

EOF
  
if [ "$enableEntities" = true ]
then
  echo "libdcdbplugin_${PLUGIN_NAME_LWC}.\$(LIBEXT): src/sensors/${PLUGIN_NAME_LWC}/${PLUGIN_NAME}SensorGroup.o src/sensors/${PLUGIN_NAME_LWC}/${PLUGIN_NAME}Configurator.o src/sensors/${PLUGIN_NAME_LWC}/${PLUGIN_NAME}${ENTITY_NAME}.o" >> appendToMakefile.txt
else
  echo "libdcdbplugin_${PLUGIN_NAME_LWC}.\$(LIBEXT): src/sensors/${PLUGIN_NAME_LWC}/${PLUGIN_NAME}SensorGroup.o src/sensors/${PLUGIN_NAME_LWC}/${PLUGIN_NAME}Configurator.o" >> appendToMakefile.txt
fi

cat << EOF >> appendToMakefile.txt
	\$(CXX) \$(LIBFLAGS)\$@ -o \$@ $^ -L\$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system

NOTE: Probably you will have to append further libraries to the linker for your plugin to compile
EOF
