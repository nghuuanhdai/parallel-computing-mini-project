/*
 * PluginDefinitions.h
 *
 *  Created on: 15.06.2018
 *      Author: Micha Mueller
 */

#ifndef PLUGINDEFINITIONS_H_
#define PLUGINDEFINITIONS_H_

/*
 * Simple header file to pull the pluginVector_t typedef out of Configuration.h
 * This way HttpsServer.h only needs to include PluginDefinitions.h and we can avoid
 * a dependency circle when including HttpsServer.h from Configuration.h
 */

#include <string>
#include <vector>

#include "ConfiguratorInterface.h"

//struct of values required for a dynamic library.
typedef struct {
	std::string            id;
	void *                 DL;
	ConfiguratorInterface *configurator;
	create_t *             create;
	destroy_t *            destroy;
} dl_t;

typedef std::vector<dl_t> pluginVector_t;

#endif /* PLUGINDEFINITIONS_H_ */
