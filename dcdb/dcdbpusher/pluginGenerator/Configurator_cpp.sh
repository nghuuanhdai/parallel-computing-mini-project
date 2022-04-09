#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}Configurator.cpp
//================================================================================
// Name        : ${PLUGIN_NAME}Configurator.cpp
// Author      : ${AUTHOR}
// Contact     :
// Copyright   :
// Description : Source file for ${PLUGIN_NAME} plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) ${DATE} Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

#include "${PLUGIN_NAME}Configurator.h"

${PLUGIN_NAME}Configurator::${PLUGIN_NAME}Configurator() {
    /* 
     * TODO
     * If you want sensor or group to be named differently in the config file, you can change it here
     */
EOF

if [ "$enableEntities" = true ]
then
  echo "    _entityName = \"${ENTITY_NAME_LWC}\";" >> ${PLUGIN_NAME}Configurator.cpp
fi

cat << EOF >> ${PLUGIN_NAME}Configurator.cpp
    _groupName = "group";
    _baseName = "sensor";
}

${PLUGIN_NAME}Configurator::~${PLUGIN_NAME}Configurator() {}

void ${PLUGIN_NAME}Configurator::sensorBase(${PLUGIN_NAME}SensorBase& s, CFG_VAL config) {
    ADD {
        /* 
         * TODO
         * Add ATTRIBUTE macros for sensorBase attributes
         */
         //ATTRIBUTE("attributeName", attributeSetter);
    }
}

void ${PLUGIN_NAME}Configurator::sensorGroup(${PLUGIN_NAME}SensorGroup& s, CFG_VAL config) {
    ADD {
        /* 
         * TODO
         * Add ATTRIBUTE macros for sensorGroup attributes
         */
    }
}

EOF

if [ "$enableEntities" = true ]
then
  cat << EOF >> ${PLUGIN_NAME}Configurator.cpp
void ${PLUGIN_NAME}Configurator::sensorEntity(${PLUGIN_NAME}${ENTITY_NAME}& s, CFG_VAL config) {
    ADD {
        /* 
         * TODO
         * Add ATTRIBUTE macros for ${ENTITY_NAME} attributes
         */
    }
}
EOF
fi

cat << EOF >> ${PLUGIN_NAME}Configurator.cpp

void ${PLUGIN_NAME}Configurator::printConfiguratorConfig(LOG_LEVEL ll) {
    /*
     * TODO
     * Log attributes here for debug reasons or delete this method if there are
     * not attributes to log.
     */
    LOG_VAR(ll) << "  NumSpacesAsIndention: " << 2;
}
EOF
