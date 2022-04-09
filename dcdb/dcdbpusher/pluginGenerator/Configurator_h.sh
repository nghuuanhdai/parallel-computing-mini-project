#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME}Configurator.h
//================================================================================
// Name        : ${PLUGIN_NAME}Configurator.h
// Author      : ${AUTHOR}
// Contact     :
// Copyright   :
// Description : Header file for ${PLUGIN_NAME} plugin configurator class.
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

#ifndef ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}CONFIGURATOR_H_
#define ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}CONFIGURATOR_H_

EOF

if [ "$enableEntities" = true ]
then
  echo "#include \"../../includes/ConfiguratorTemplateEntity.h\"" >> ${PLUGIN_NAME}Configurator.h
else
  echo "#include \"../../includes/ConfiguratorTemplate.h\"" >> ${PLUGIN_NAME}Configurator.h
fi
echo "#include \"${PLUGIN_NAME}SensorGroup.h\"" >> ${PLUGIN_NAME}Configurator.h
  
if [ "$enableEntities" = true ]
then
  echo "#include \"${PLUGIN_NAME}${ENTITY_NAME}.h\"" >> ${PLUGIN_NAME}Configurator.h
fi

cat << EOF >> ${PLUGIN_NAME}Configurator.h

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup ${PLUGIN_NAME_LWC}
 */
EOF

if [ "$enableEntities" = true ]
then
  echo "class ${PLUGIN_NAME}Configurator : public ConfiguratorTemplateEntity<${PLUGIN_NAME}SensorBase, ${PLUGIN_NAME}SensorGroup, ${PLUGIN_NAME}${ENTITY_NAME}> {" >> ${PLUGIN_NAME}Configurator.h
else
  echo "class ${PLUGIN_NAME}Configurator : public ConfiguratorTemplate<${PLUGIN_NAME}SensorBase, ${PLUGIN_NAME}SensorGroup> {" >> ${PLUGIN_NAME}Configurator.h
fi

cat << EOF >> ${PLUGIN_NAME}Configurator.h

public:
    ${PLUGIN_NAME}Configurator();
    virtual ~${PLUGIN_NAME}Configurator();

protected:
    /* Overwritten from ConfiguratorTemplate */
    void sensorBase(${PLUGIN_NAME}SensorBase& s, CFG_VAL config) override;
    void sensorGroup(${PLUGIN_NAME}SensorGroup& s, CFG_VAL config) override;
EOF
  
if [ "$enableEntities" = true ]
then
  cat << EOF >> ${PLUGIN_NAME}Configurator.h
    void sensorEntity(${PLUGIN_NAME}${ENTITY_NAME}& s, CFG_VAL config) override;
EOF
fi

cat << EOF >> ${PLUGIN_NAME}Configurator.h

    //TODO implement if required
    //void global(CFG_VAL config) override;
    //void derivedSetGlobalSettings(const pluginSettings_t& pluginSettings) override;

    void printConfiguratorConfig(LOG_LEVEL ll) final override;
};

extern "C" ConfiguratorInterface* create() {
    return new ${PLUGIN_NAME}Configurator;
}

extern "C" void destroy(ConfiguratorInterface* c) {
    delete c;
}

#endif /* ${PLUGIN_NAME_UPC}_${PLUGIN_NAME_UPC}CONFIGURATOR_H_ */
EOF
