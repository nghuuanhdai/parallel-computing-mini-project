//================================================================================
// Name        : sensornavigator.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Provide simple representation of a sensor hierarchy.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#ifndef PROJECT_SENSORNAVIGATOR_H
#define PROJECT_SENSORNAVIGATOR_H

#include <vector>
#include <unordered_map>
#include <set>
#include <boost/regex.hpp>
#include <limits.h>
#include <algorithm>
#include "mqttchecker.h"

using namespace std;

/**
 * @brief Provides a simple tree-like in-memory representation of a sensor
 *        hierarchy.
 *
 * @details A "sensor tree" can be built from either a Cassandra-like table in
 *          the format used by DCDB, or from a vector of* sensor names. In this
 *          second case, the list of regular expressions defining the sensor
 *          hierarchy must be supplied as well.
 *
 * @ingroup common
 */
class SensorNavigator {

public:

    //Identifies the root supernode
    static const char *rootKey;
    //Template identifier
    static const char *templateKey;
    //Separator for tree levels used in auto mode
    static const char pathSeparator;

    //Internal structure to store nodes
    struct Node {
        //Depth of this node in the sensor tree
        int depth;
        //Set of children nodes (e.g. cpus in a node or nodes in a rack)
        set<string> children;
        //Set of sensors contained within this node
        set<string> sensors;
        //Name of the parent for this node
        string parent;
        //MQTT topic used for sensors of this node (if any)
        string topic; };

    /**
    * @brief            Class constructor
    */
    SensorNavigator() {
        _hierarchy = NULL;
        _sensorTree = NULL;
        _treeDepth = -1;
        _usingTopics = false;
        _filter = "";
    }

    /**
    * @brief            Class destructor
    */
    ~SensorNavigator() {
        clearTree();
    }

    /**
    * @brief            Returns true if a sensor tree has been generated, false otherwise
    **/
    bool                treeExists() { return _sensorTree; }

    /**
    * @brief        	Creates a sensor tree.
    *
    *					In order to infer the hierarchy of sensors and build a tree, we start from a list of sensors,
    *					and a list of regular expressions which express the successive hierarchical levels in the
    *					sensor organization. For example, for sensor name mpp3.rack01.chassis02.node03.cpu01.temp, the
    *					dots define the different levels in the architecture, and the corresponding list of regexes
    *					would be ["mpp3.", "rack\d{2}.", "chassis\d{2}.", "node\d{2}.", "cpu\d{2}"]. The last part
    *					of the sensor name is supposed to constitute a "leaf" in the tree. If the input vector is
    *					empty, the tree is built automatically by interpreting each "/" in the sensor name as a tree
    *					level separator. The functionality implemented here requires careful naming of the sensors
    *					by developers.
    *
    * @param hierarchy	The vector of regular expression strings that define the hierarchy
    * @param sensors    The vector of sensor names that must be organized in a hierarchical tree
    * @param topics     The vector of topics corresponding to the names specified in the sensors vector
    *
    **/
    void                buildTree(const vector<string> *hierarchy, const vector<string> *sensors, const vector<string> *topics=NULL);

    /**
    * @brief        	Creates a sensor tree.
    *
    *					Overloaded version of the method by the same name. Takes as input a string containing a
    *					space-separated list of regular expressions defining the sensor hierarchy. If the string is
    *					empty, the tree is built automatically by interpreting each "/" in the sensor name as a tree
    *					level separator.
    *
    * @param hierarchy	String of comma-separated expressions that define the hierarchy
    * @param sensors    The vector of sensor names that must be organized in a hierarchical tree
    * @param topics     The vector of topics corresponding to the names specified in the sensors vector
    * @param delimiter  Sequence of characters used to split the sensor hierarchy string in tokens
    *
    **/
    void                buildTree(const string& hierarchy, const vector<string> *sensors, const vector<string> *topics=NULL, const string& delimiter=" ");

    /**
    * @brief        	Creates a sensor tree starting from a Cassandra-style table.
    *
    *					This version of buildTree does not employ a user-specified sensor hierarchy, but simply
    *					reconstruct the tree structure specified in the input Cassandra-style table. The table is
    *					supposed to have a map structure, in which each record represents a node in the tree (key), and
    *					has an associated list of node children (value). Children that are not present in the table
    *					keyspace are supposed to be sensors and not nodes in the tree.
    *
    * @param table      The Cassandra-like table from which a sensor tree must be built
    * @param root       Identifier for the "root" node in the table from which to start
    * @param ignore     Regular expression matching nodes in the tree that must be ignored
    *
    **/
    void                buildCassandraTree(const map<string, vector<string> > *table, const string& root="$ROOT$", const string& ignore="");


    /**
    * @brief            Clears the internal sensor tree (if any) and releases all related memory
    **/
    void                clearTree();

    /**
    * @brief            Indicates whether the sensor tree is complete or not
    *
    *                   A sensor tree is considered complete if all hierarchical levels specified during the build
     *                  phase were found, and therefore the tree depth matches the number of hierarchical expressions.
    *
    * @return           True if the tree is complete, False otherwise
    */
    bool                isTreeComplete() { return _hierarchy && _treeDepth == (int)_hierarchy->size() - 1; }

    /**
    * @return           The total depth of the internal sensor tree
    */
    int                 getTreeDepth() { return _treeDepth; }

    /**
    * @return           The total size (nodes + sensors) of the internal sensor tree
    */
    int                 getTreeSize() { return treeExists() ? _sensorTree->size() : -1; }

    /**
    * @brief           Returns the internal sensor tree, or a subtree
    *
    *                  Developers should usually not need to access the internal sensor tree structure; however,
    *                  this functionality is supplied as a "just in case" measure. Optionally, a starting root
    *                  node can be specified, and the algorithm will return the related subtree. Please note that
    *                  sensor nodes cannot act as root of the subtree.
    *
    * @param node      root node of the desired subtree
    * @param depth     maximum depth of the desired subtree
    * @return          The internal map used to store the sensor tree (or subtree)
    */
    unordered_map<string, Node>  *getSubTree(const string& node="__root__", int depth=INT_MAX);

    /**
    * @brief           Returns all nodes in the sensor tree at a certain depth
    *
    *                  If the recursive option is set to False, the method will return the set of nodes strictly at
    *                  level D. If True, all nodes at level D will be returned, plus all nodes at deeper levels.
    *
    * @param depth     The depth level at which nodes must be retrieved
    * @param recursive Enabled recursive node retrieval
    * @return          A set of node names
    */
    set<string>      *getNodes(int depth=-1, bool recursive=true);

    /**
    * @brief           Returns all children of an input node in the sensor tree
    *
    *                  If the recursive option is set to False, the method will return the set of nodes that are
    *                  children of the input node. If True, all nodes in the subtree whose root is the input node
    *                  will be returned instead. Note that the starting node cannot be a sensor node, but must be
    *                  a hierarchical node, e.g. "mpp3.r01.c02.", and not "mpp3.r01.c02.energy".
    *
    * @param node      The starting node from which children must be retrieved
    * @param recursive Enabled recursive node retrieval
    * @return          A set of node names
    */
    set<string>      *getNodes(const string& node="__root__", bool recursive=true);

    /**
    * @brief           Returns all sensors in the sensor tree at a certain depth
    *
    *                  If the recursive option is set to False, the method will return the set of sensors strictly at
    *                  level D. If True, all sensors at level D will be returned, plus all sensors at deeper levels.
    *
    * @param depth     The depth level at which sensors must be retrieved
    * @param recursive Enabled recursive sensor retrieval
    * @return          A set of sensor names
    */
    set<string>      *getSensors(int depth=-1, bool recursive=true);

    /**
    * @brief           Returns all sensors belonging to the input node in the sensor tree
    *
    *                  If the recursive option is set to False, the method will return the set of sensors that
    *                  belong to the input node. If True, all sensors in nodes that belong to the subtree whose root
    *                  is the input node will be returned instead. Note that the starting node cannot be a sensor node,
    *                  but must be a hierarchical node, e.g. "mpp3.r01.c02.", and not "mpp3.r01.c02.energy".
    *
    * @param node      The starting node from which sensors must be retrieved
    * @param recursive Enabled recursive sensor retrieval
    * @return          A set of sensor names
    */
    set<string>      *getSensors(const string& node="__root__", bool recursive=true);

    /**
    * @brief            Determines whether a node in the tree is a sensor
    *
    *                   A node in the sensor tree represents a sensor if it has no children, and no attached sensors
    *                   itself. In this case, the "leaf" node is considered to be a sensor. Otherwise, it is considered
    *                   to be a hierarchical node.
    *
    * @param node       Node name to look up in the tree
    * @return           True if the node is a sensor, false otherwise
    */
    bool                isSensorNode(const string& node);

    /**
    * @brief            Returns the depth of a node in the sensor tree
    *
    *                   The input node can both be a sensor or a hierarchical node.
    *
    * @param node       Node name to look up in the tree
    * @return           Depth of the node in the tree
    */
    int                 getNodeDepth(const string& node);

    /**
    * @brief            Returns the MQTT topic of a node in the sensor tree
    *
    *                   The input node can both be a sensor or a hierarchical node. In the case of hierarchical nodes,
     *                  the MQTT topic will be partial, and consist in the part that characterizes the node we are in.
    *
    * @param node       Node name to look up in the tree
    * @return           Topic of the node
    */
    string              getNodeTopic(const string& node);

    /**
    * @brief            Check for the existence of a node in the sensor tree
    *
    * @param node       Node name to look up in the tree
    * @return           True if the node exists, False otherwise
    */
    bool                nodeExists(const string& node);

    /**
    * @brief            Check for the existence of a sensor in the sensor tree
    *
    * @param node       Sensor name to look up in the tree
    * @return           True if the sensor exists, False otherwise
    */
    bool                sensorExists(const string& node);

    /**
    * @brief           Navigate to another point of the sensor tree
    *
    *                  Starting from an input node of the sensor tree, this method will navigate to a different node
    *                  (or set of nodes) depending on the direction parameter. There are two use cases:
    *
    *                  - direction < 0: we go upwards in the tree, getting the ancestors of the starting node. The
    *                    returned result is the ancestor of grade |direction|;
    *                  - direction > 0: we go deeper in the tree, getting all children of the starting node. The
    *                    returned result consists in the set of descendants of grade |direction|;
    *
    *                  An input of direction=0 leads to error. Input nodes can both be sensor or hierarchical nodes.
    *
    * @param node      The starting node for navigation in the tree
    * @param direction Defines where navigation should be heading
    * @return          The set of destination nodes on the given navigation path
    */
    set<string>         *navigate(const string& node="__root__", int direction=1);

    /**
    * @brief            Builds a MQTT topic for a new sensors associated with a given node
    *
    *                   This method builds MQTT topics for new sensors associated to specific nodes in a coherent way.
    *                   Specifically, the method will use the MQTT prefix associated to the input "node", and the
    *                   MQTT suffix given as input. All the middle characters left unused are set to "F", until the
    *                   target input length is reached.
    *
    * @param node       Name of the node to which the sensor belongs
    * @param suffix     Suffix of the new sensor
    * @param len        Fixed length of the final MQTT topic
    * @return           A full MQTT topic for the specified sensor
    */
    string              buildTopicForNode(const string& node, const string& suffix, int len=28);
    
    /**
     * @brief           Sets an internal regex which is used to filter out sensors
     * 
     *                  The filter is applied upon building the sensor tree (buildTree), and all sensor names not 
     *                  containing a partial match with the filter are eliminated. The filter is NOT applied when
     *                  using the buildCassandraTree method.
     * 
     * @param f         The filter regex to be used
     */
    void                setFilter(string f) { _filter = f; }

protected:

    void                _addSensor(const string& name, const string& topic);
    void                _addAutoSensor(const string& name, const string& topic);
    void                _addCassandraSensor(const string& name, const map<string, vector<string> > *table, int depth, boost::regex ignore);
    bool                _isSensorNode(const Node& node) { return node.sensors.empty() && node.children.empty(); }

    //Recursive internal algorithms used for their public counterparts
    void                _getSubTree(const string& node, unordered_map<string, Node> *m, int depth);
    void                _getNodes(const string& node, set<string> *vec, int depth);
    void                _getSensors(const string& node, set<string> *vec, int depth);

    //If True, topics are different from sensor names
    bool                   _usingTopics;
    //Depth of the sensor tree
    int                    _treeDepth;
    //Internal map used to store the sensor tree
    unordered_map<string, Node>      *_sensorTree;
    //Vector of regular expression objects that define the sensor hierarchy
    vector<boost::regex>   *_hierarchy;
    boost::cmatch          _match;
    //Internal regex to filter out sensor names
    std::string          _filter;

};

#endif //PROJECT_SENSORNAVIGATOR_H
