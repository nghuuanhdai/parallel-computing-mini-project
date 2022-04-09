#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}SensorGroup.cpp
//================================================================================
// Name        : ${PLUGIN_NAME}SensorGroup.cpp
// Author      : ${AUTHOR}
// Contact     :
// Copyright   : 
// Description : Source file for ${PLUGIN_NAME} sensor group class.
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

#include "${PLUGIN_NAME}SensorGroup.h"

#include "timestamp.h"

${PLUGIN_NAME}SensorGroup::${PLUGIN_NAME}SensorGroup(const std::string& name) :
EOF

if [ "$enableEntities" = true ]
then
  echo "    SensorGroupTemplateEntity(name) {" >> ${PLUGIN_NAME}SensorGroup.cpp
else
  echo "    SensorGroupTemplate(name) {" >> ${PLUGIN_NAME}SensorGroup.cpp
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.cpp
    /* 
     * TODO
     * Construct attributes
     */
}

${PLUGIN_NAME}SensorGroup::${PLUGIN_NAME}SensorGroup(const ${PLUGIN_NAME}SensorGroup& other) :
EOF

if [ "$enableEntities" = true ]
then
  echo "    SensorGroupTemplateEntity(other) {" >> ${PLUGIN_NAME}SensorGroup.cpp
else
  echo "    SensorGroupTemplate(other) {" >> ${PLUGIN_NAME}SensorGroup.cpp
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.cpp
    /* 
     * TODO
     * Copy construct attributes
     */
}

${PLUGIN_NAME}SensorGroup::~${PLUGIN_NAME}SensorGroup() {
    /* 
     * TODO
     * Tear down attributes
     */
}

${PLUGIN_NAME}SensorGroup& ${PLUGIN_NAME}SensorGroup::operator=(const ${PLUGIN_NAME}SensorGroup& other) {
    SensorGroupTemplate::operator=(other);
    /* 
     * TODO
     * Implement assignment operator
     */

    return *this;
}

void ${PLUGIN_NAME}SensorGroup::execOnInit() {
    /* 
     * TODO
     * Implement one time initialization logic for this group here
     * (e.g. allocate memory for buffer) or remove this method if not
     * required.
     */
}

bool ${PLUGIN_NAME}SensorGroup::execOnStart() {
    /* 
     * TODO
     * Implement logic before the group starts polling here
     * (e.g. open a file descriptor) or remove this method if not required.
     */
    return true;
}

void ${PLUGIN_NAME}SensorGroup::execOnStop() {
    /* 
     * TODO
     * Implement logic when the group stops polling here
     * (e.g. close a file descriptor) or remove this method if not required.
     */
}

void ${PLUGIN_NAME}SensorGroup::read() {
    reading_t reading;
    reading.timestamp = getTimestamp();

    try {
        for(auto s : _sensors) {
            reading.value = /* 
                             * TODO
                             * Read a value for every sensor affiliated with this group and store
                             * it with the appropriate sensor.
EOF
  
if [ "$enableEntities" = true ]
then
  echo "                             * Make use of _${ENTITY_NAME_LWC} as required." >> ${PLUGIN_NAME}SensorGroup.cpp
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.cpp
                             */ 0;
            s->storeReading(reading);
#ifdef DEBUG
            LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
        }
    } catch (const std::exception& e) {
        LOG(error) << "Sensorgroup" << _groupName << " could not read value: " << e.what();
    }
}

void ${PLUGIN_NAME}SensorGroup::printGroupConfig(LOG_LEVEL ll) {
    /*
     * TODO
     * Log attributes here for debug reasons
EOF
  
if [ "$enableEntities" = true ]
then
  echo "     * Printing the name of associated _entity may be a good idea." >> ${PLUGIN_NAME}SensorGroup.cpp
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.cpp
     */

    LOG_VAR(ll) << "            NumSpacesAsIndention: " << 12;
}
EOF
