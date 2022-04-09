//================================================================================
// Name        : PluginManager.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class bundling all logic related to dcdb-pusher plugins.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

#ifndef DCDBPUSHER_PLUGINMANAGER_H_
#define DCDBPUSHER_PLUGINMANAGER_H_

#include <boost/asio.hpp>
#include <string>
#include <vector>

#include "logging.h"

#include "includes/ConfiguratorInterface.h"

//TODO extract common code with AnalyticsManager in common base class
/**
 * Struct of values required to load analyzer dynamic libraries.
 *
 * @ingroup pusher
 */
typedef struct {
	std::string            id;
	void *                 DL;
	ConfiguratorInterface *configurator;
	create_t *             create;
	destroy_t *            destroy;
} pusherPlugin_t;

using pusherPluginStorage_t = std::vector<pusherPlugin_t>;

/**
 * @brief This class is responsible of managing plugins for dcdbpusher.
 *
 * @ingroup pusher
 */
class PluginManager {

      public:
	/**
     * @brief Constructor.
     *
     * @param io IO service to initialize the plugin's groups with.
     * @param pluginSettings Use this plugin settings as default when loading
     *                       a new plugin.
     */
	PluginManager(boost::asio::io_context &io, const pluginSettings_t &pluginSettings);

	/**
     * @brief Destructor.
     */
	virtual ~PluginManager();

	/**
     * @brief Load a new plugin.
     *
     * @details Searches and loads a shared library file for the plugin as well
     *          as its corresponding configuration file. The library file name
     *          is constructed from the plugin name according to
     *          libdcdbplugin_NAME.so (or .dylib for Apple). After the shared
     *          library is loaded the plugin reads in its configuration. After
     *          loading the plugin still has to be initialized (initPlugin())
     *          before it can be started (startPlugin()).
     *
     * @param name           Identifying name (ID) of the new plugin.
     * @param pluginPath     Path where the plugin shared library is located. If
     *                       not specified we will search the default library
     *                       directories (usr/lib and friends).
     * @param config         Path to plugin configuration file (including config
     *                       file name). If not specified we will search in the
     *                       current directory for "NAME.conf".
     *
     * @return True on success, false otherwise. In the latter case the plugin
     *         is not loaded.
     */
	bool loadPlugin(const std::string &name,
			const std::string &pluginPath = "",
			const std::string &config = "");

	// Undocumented: if no plugin name is specified all plugins are unloaded.
	/**
     * @brief Unload a plugin.
     *
     * @details Stops the specified plugin and removes it completely from the
     *          manager. To use the plugin again it has to be loaded first.
     *
     * @param id Identifying name of the plugin.
     */
	void unloadPlugin(const std::string &id = "");

	// Undocumented: if no plugin name is specified all plugins are initialized.
	/**
     * @brief Initialize a plugin.
     *
     * @details Initializes a plugin so it can be started. A plugin has to be
     *          be initialized only once after it was (re)loaded.
     *
     * @param id Identifying name of the plugin.
     *
     * @return True on success, false if the plugin could not be found.
     */
	bool initPlugin(const std::string &id = "");

	// Undocumented: if no plugin name is specified all plugins are started.
	/**
     * @brief Start a plugin.
     *
     * @details Start execution of a previously initialized plugin or resume
     *          execution of a stopped plugin. An unitialized plugin must not be
     *          started!
     *
     * @param id Identifying name of the plugin.
     *
     * @return True on success, false if the plugin could not be found.
     */
	bool startPlugin(const std::string &id = "");

	// Undocumented: if no plugin name is specified all plugins are stopped.
	/**
     * @brief Stop a plugin.
     *
     * @details Halts execution of the plugin. Can be continued with start().
     *
     * @param id Identifying name of the plugin.
     *
     * @return True on success, false if the plugin could not be found.
     */
	bool stopPlugin(const std::string &id = "");

	/**
     * @brief Reload the plugin's configuration.
     *
     * @details Clears the plugin internal configuration (deleting all of its
     *          groups and sensors). Reads in the plugin configuration again.
     *          At the end the plugin is in the same state as after it was
     *          loaded (i.e. it is not initialized yet). The plugin
     *          configuration file is expected to be the same (name and
     *          location) as when the plugin was first loaded.
     *
     * @param id Identifying name of the plugin.
     *
     * @return True on success, false otherwise. In the latter case the plugin
     *         is in an undefined state and should not be started. Failure is
     *         most likely caused by an invalid new configuration file. Try to
     *         reload the plugin again with a valid configuration.
     */
	bool reloadPluginConfig(const std::string &id);

	/**
     * @brief Get all currently loaded plugins.
     *
     * @return Storage of currently loaded plugins.
     */
	pusherPluginStorage_t &getPlugins() {
		return _plugins;
	}

	/**
     * @brief   Sets the internal path to be used when loading plugins by default
     * 
     * @param p The string to be used as path 
     */
	void setCfgFilePath(const std::string &p) { _cfgFilePath = p; }

      private:
	/**
     * @brief Utility method to check if the MQTT topics of the given plugin are
     *        valid.
     *
     * @param p Plugin.
     *
     * @return True if all topics are valid, false otherwise.
     */
	bool checkTopics(const pusherPlugin_t &p);

	/**
     * @brief Utility method to remove all MQTT topics associated to a plugin
     *        from the used set.
     *
     * @param p Plugin.
     */
	void removeTopics(const pusherPlugin_t &p);

	pusherPluginStorage_t    _plugins;        ///< Storage to hold all loaded plugins
	pluginSettings_t         _pluginSettings; ///< Default plugin settings to use when loading a new plugin
	std::string              _cfgFilePath;
	boost::asio::io_context& _io;
	logger_t                 lg;
};

#endif /* DCDBPUSHER_PLUGINMANAGER_H_ */
