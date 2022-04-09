//
// Created by Netti, Alessio on 07.03.19.
//

#ifndef PROJECT_PLUGINSETTINGS_H
#define PROJECT_PLUGINSETTINGS_H

typedef struct {
    std::string sensorPattern;
    std::string mqttPrefix;
    std::string	tempdir;
    unsigned int cacheInterval;
} pluginSettings_t;

static bool to_bool(const std::string& s) { return s=="true" || s=="on"; }

#endif //PROJECT_PLUGINSETTINGS_H
