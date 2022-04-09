#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}${ENTITY_NAME}.h
//================================================================================
// Name        : ${PLUGIN_NAME}${ENTITY_NAME}.h
// Author      : ${AUTHOR}
// Contact     :
// Copyright   :
// Description : Header file for ${PLUGIN_NAME}${ENTITY_NAME} class.
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

#ifndef ${PLUGIN_NAME_UPC}${ENTITY_NAME_UPC}_H_
#define ${PLUGIN_NAME_UPC}${ENTITY_NAME_UPC}_H_

#include "../../includes/EntityInterface.h"

/**
 * @brief This class handles ${PLUGIN_NAME} ${ENTITY_NAME}.
 *
 * @ingroup ${PLUGIN_NAME_LWC}
 */
class ${PLUGIN_NAME}${ENTITY_NAME} : public EntityInterface {

public:
    ${PLUGIN_NAME}${ENTITY_NAME}(const std::string& name);
    ${PLUGIN_NAME}${ENTITY_NAME}(const ${PLUGIN_NAME}${ENTITY_NAME}& other);
    virtual ~${PLUGIN_NAME}${ENTITY_NAME}();
    ${PLUGIN_NAME}${ENTITY_NAME}& operator=(const ${PLUGIN_NAME}${ENTITY_NAME}& other);

    /* 
     * TODO
     * Add methods the entity provides to the SensorGroups.
     * Add getter/setter for the attributes as required.
     */

    void execOnInit() final override;

    void printEntityConfig(LOG_LEVEL ll) final override;

private:
    /* 
     * TODO
     * Add attributes required for your entity
     */
};

#endif /* ${PLUGIN_NAME_UPC}${ENTITY_NAME_UPC}_H_ */
EOF
