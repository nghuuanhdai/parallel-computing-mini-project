#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}SensorBase.h
//================================================================================
// Name        : ${PLUGIN_NAME}SensorBase.h
// Author      : ${AUTHOR}
// Contact     :
// Copyright   : 
// Description : Sensor base class for ${PLUGIN_NAME} plugin.
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

/**
 * @defgroup ${PLUGIN_NAME_LWC} ${PLUGIN_NAME} plugin
 * @ingroup  pusherplugins
 *
 * @brief Describe your plugin in one sentence
 */

#ifndef ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORBASE_H_
#define ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORBASE_H_

#include "sensorbase.h"
EOF

if [ "$enableEntities" = true ]
then
    echo "" >> ${PLUGIN_NAME}SensorBase.h
    echo "#include \"${PLUGIN_NAME}${ENTITY_NAME}.h\"" >> ${PLUGIN_NAME}SensorBase.h
fi

cat << EOF >> ${PLUGIN_NAME}SensorBase.h

/* 
 * TODO
 * Add plugin specific includes
 */

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup ${PLUGIN_NAME_LWC}
 */
class ${PLUGIN_NAME}SensorBase : public SensorBase {
public:
    ${PLUGIN_NAME}SensorBase(const std::string& name) :
        SensorBase(name) {
        /*
         * TODO
         * Initialize plugin specific attributes
         */
    }

    ${PLUGIN_NAME}SensorBase(const ${PLUGIN_NAME}SensorBase& other) :
        SensorBase(other) {
        /*
         * TODO
         * Copy construct plugin specific attributes
         */
    }

    virtual ~${PLUGIN_NAME}SensorBase() {
        /*
         * TODO
         * If necessary, deconstruct plugin specific attributes
         */
    }

    ${PLUGIN_NAME}SensorBase& operator=(const ${PLUGIN_NAME}SensorBase& other) {
        SensorBase::operator=(other);
        /*
         * TODO
         * Implement assignment operator for plugin specific attributes
         */
         
        return *this;
    }

    /*
     * TODO
     * Getters and Setters for plugin specific attributes
     */

    void printConfig(LOG_LEVEL ll, LOGGER& lg) {
        /*
         * TODO
         * Log attributes here for debug reasons
         */
        LOG_VAR(ll) << "     NumSpacesAsIndention: " << 5;
    }

protected:
    /*
     * TODO
     * Add plugin specific attributes here
     */

};

#endif /* ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORBASE_H_ */
EOF
