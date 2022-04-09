#!/bin/sh
# This file is part of DCDB (DataCenter DataBase)
# Copyright (C) 2019 Leibniz Supercomputing Centre

cat << EOF > ${PLUGIN_NAME_LWC}.conf
;comments in config files are indicated by a semicolon

global {
    mqttPrefix /${PLUGIN_NAME_LWC}Prefix

    ;add here other global attributes for your plugin
}

EOF
  
if [ "$enableEntities" = true ]
then
  cat << EOF >> ${PLUGIN_NAME_LWC}.conf
template_group def1 {
    ;define template groups by appending "template_"
    interval  1000
    minValues 3
    groupAtt  1234
    ;add other attributes your plugin requires for a sensor group

    ;define sensors belonging to the group below
    sensor temp1 {
        sensorAtt 5678
        ;add other attributes your plugin requires for a sensor
    }
}

${ENTITY_NAME_LWC} ent1 {
    entityAtt   1234

    single_sensor sens1 {
        ;if you want a group with only one sensor you can use a single_sensor

        default    temp1
        mqttsuffix /sens1
        ;add other attributes your plugin requires for a sensor
    }

    group g1 {
        interval 1000
        mqttpart /g1
        default  def1

        ;sensor temp1 is taken from def1 and does not need to be redefined
        sensor gSens {
            mqttsuffix /gSensTopic
        }
    }
}
EOF
else
  cat << EOF >> ${PLUGIN_NAME_LWC}.conf
template_group def1 {
    ;define template groups by appending "template_"
    interval  1000
    minValues 3
    groupAtt  1234
    ;add other attributes your plugin requires for a sensor group

    ;define sensors belonging to the group below
    sensor temp1 {
        sensorAtt 5678
        ;add other attributes your plugin requires for a sensor
    }
}

single_sensor sens1 {
    ;if you want a group with only one sensor you can use a single_sensor

    default    temp1
    mqttsuffix /sens1Suffix
    ;add other attributes your plugin requires for a sensor
}

group g1 {
    interval 1000
    mqttpart /group1 
    default  def1

    ;sensor temp1 is taken from def1 and does not need to be redefined
    sensor gSens {
        mqttsuffix /gSens01
    }
}
EOF
fi
