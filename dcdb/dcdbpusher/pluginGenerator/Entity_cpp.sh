#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}${ENTITY_NAME}.cpp
//================================================================================
// Name        : ${PLUGIN_NAME}${ENTITY_NAME}.cpp
// Author      : ${AUTHOR}
// Contact     :
// Copyright   :
// Description : Source file for ${PLUGIN_NAME}${ENTITY_NAME} class.
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

#include "${PLUGIN_NAME}${ENTITY_NAME}.h"

${PLUGIN_NAME}${ENTITY_NAME}::${PLUGIN_NAME}${ENTITY_NAME}(const std::string& name) :
    EntityInterface(name) {
    /* 
     * TODO
     * Construct attributes
     */
}

${PLUGIN_NAME}${ENTITY_NAME}::${PLUGIN_NAME}${ENTITY_NAME}(const ${PLUGIN_NAME}${ENTITY_NAME}& other) :
    EntityInterface(other) {
    /* 
     * TODO
     * Copy construct attributes
     */
}

${PLUGIN_NAME}${ENTITY_NAME}::~${PLUGIN_NAME}${ENTITY_NAME}() {
    /* 
     * TODO
     * Tear down attributes
     */
}

${PLUGIN_NAME}${ENTITY_NAME}& ${PLUGIN_NAME}${ENTITY_NAME}::operator=(const ${PLUGIN_NAME}${ENTITY_NAME}& other) {
    EntityInterface::operator=(other);
    /* 
     * TODO
     * Implement assignment operator here
     */

    return *this;
}

/* 
 * TODO
 * Implement own methods
 */

void ${PLUGIN_NAME}${ENTITY_NAME}::execOnInit() {
    /*
     * TODO
     * Implement logic to initialize the entity here or remove method if not required. 
     * Will be called exactly once at startup.
     */
}

void ${PLUGIN_NAME}${ENTITY_NAME}::printEntityConfig(LOG_LEVEL ll) {
    /*
     * TODO
     * Log attributes here for debug reasons
     */
    LOG_VAR(ll) << eInd << "Att: " << 3;
}
EOF
