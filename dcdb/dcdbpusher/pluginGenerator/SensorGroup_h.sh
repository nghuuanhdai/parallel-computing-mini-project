#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}SensorGroup.h
//================================================================================
// Name        : ${PLUGIN_NAME}SensorGroup.h
// Author      : ${AUTHOR}
// Contact     :
// Copyright   : 
// Description : Header file for ${PLUGIN_NAME} sensor group class.
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

#ifndef ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORGROUP_H_
#define ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORGROUP_H_

EOF

if [ "$enableEntities" = true ]
then
  echo "#include \"../../includes/SensorGroupTemplateEntity.h\"" >> ${PLUGIN_NAME}SensorGroup.h
else
  echo "#include \"../../includes/SensorGroupTemplate.h\"" >> ${PLUGIN_NAME}SensorGroup.h
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.h
#include "${PLUGIN_NAME}SensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup ${PLUGIN_NAME_LWC}
 */
EOF

if [ "$enableEntities" = true ]
then
  echo "class ${PLUGIN_NAME}SensorGroup : public SensorGroupTemplateEntity<${PLUGIN_NAME}SensorBase, ${PLUGIN_NAME}${ENTITY_NAME}> {" >> ${PLUGIN_NAME}SensorGroup.h
else
  echo "class ${PLUGIN_NAME}SensorGroup : public SensorGroupTemplate<${PLUGIN_NAME}SensorBase> {" >> ${PLUGIN_NAME}SensorGroup.h
fi

cat << EOF >> ${PLUGIN_NAME}SensorGroup.h
public:
    ${PLUGIN_NAME}SensorGroup(const std::string& name);
    ${PLUGIN_NAME}SensorGroup(const ${PLUGIN_NAME}SensorGroup& other);
    virtual ~${PLUGIN_NAME}SensorGroup();
    ${PLUGIN_NAME}SensorGroup& operator=(const ${PLUGIN_NAME}SensorGroup& other);

    void execOnInit()  final override;
    bool execOnStart() final override;
    void execOnStop()  final override;

    /* 
     * TODO
     * Add getter and setters for group attributes if required
     */
     
    void printGroupConfig(LOG_LEVEL ll) final override;

private:
    void read() final override;

    /* 
     * TODO
     * Add group internal attributes
     */
};

#endif /* ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}SENSORGROUP_H_ */
EOF
