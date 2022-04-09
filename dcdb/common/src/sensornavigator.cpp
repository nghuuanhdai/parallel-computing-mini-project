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

#include "sensornavigator.h"

const char* SensorNavigator::rootKey = "__root__";
const char* SensorNavigator::templateKey = "__template__";
const char SensorNavigator::pathSeparator = '/';

void SensorNavigator::clearTree() {
    if(_sensorTree) {
        _sensorTree->clear();
        delete _sensorTree;
        _sensorTree = NULL;
    }
    if(_hierarchy) {
        _hierarchy->clear();
        delete _hierarchy;
        _hierarchy = NULL;
    }
    _treeDepth = -1;
    _usingTopics = false;
}

bool SensorNavigator::nodeExists(const string& node) {
    return _sensorTree && _sensorTree->count(node) && !isSensorNode(node);
}

bool SensorNavigator::sensorExists(const string& node) {
    return _sensorTree && _sensorTree->count(node) && isSensorNode(node);
}

string SensorNavigator::buildTopicForNode(const string& node, const string& suffix, int len) {
    if(!_sensorTree || !_sensorTree->count(node) || isSensorNode(node))
        throw domain_error("SensorNavigator: node not found in tree!");
    
    return MQTTChecker::formatTopic(getNodeTopic(node)) + MQTTChecker::formatTopic(suffix);
}

bool SensorNavigator::isSensorNode(const string& node) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    return _isSensorNode(_sensorTree->at(node));
}

int SensorNavigator::getNodeDepth(const string& node) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    return _sensorTree->at(node).depth;
}

string SensorNavigator::getNodeTopic(const string& node) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    return _usingTopics ? _sensorTree->at(node).topic : node;
}

void SensorNavigator::buildTree(const string& hierarchy, const vector<string> *sensors, const vector<string> *topics, const string& delimiter) {
    //if( hierarchy == "" )
    //    throw invalid_argument("SensorNavigator: cannot build sensor hierarchy!");

    vector<string> hierarchyVec;
    string hBuf = hierarchy;
    size_t pos;
    if(!hierarchy.empty())
        while( !hBuf.empty() ) {
            pos = hBuf.find(delimiter);
            hierarchyVec.push_back(hBuf.substr(0, pos));
            if(pos == std::string::npos)
                hBuf.clear();
            else
                hBuf.erase(0, pos + delimiter.length());
        }
    buildTree(&hierarchyVec, sensors, topics);
}

void SensorNavigator::buildTree(const vector<string> *hierarchy, const vector<string> *sensors, const vector<string> *topics) {
    if(sensors && sensors->size() > 0)
        clearTree();
    else
        throw invalid_argument("SensorNavigator: cannot build sensor hierarchy!");

    bool autoMode = false;
    boost::regex filterReg(_filter);
    if(!hierarchy || hierarchy->empty()) {
        _hierarchy = NULL;
        autoMode = true;
    }
    else {
        _hierarchy = new vector<boost::regex>();
        string s = "";
        //Each level in the hierarchy includes the regular expressions at the upper levels
        for (const auto &l: *hierarchy) {
            s += l;
            _hierarchy->push_back(boost::regex(s));
        }
        autoMode = false;
    }

    _usingTopics = topics != NULL;
    _sensorTree = new unordered_map<string, Node>();

    //We initialize the sensor tree by pushing the root supernode
    Node rootNode = {-1, set<string>(), set<string>(), "", ""};
    _sensorTree->insert(make_pair(rootKey, rootNode));
    //We iterate over all sensor names that were supplied and build up the tree
    for(int i=0; i < (int)sensors->size(); i++)
        if(_filter=="" || boost::regex_search(sensors->at(i).c_str(), _match, filterReg)) {
            if (autoMode)
                _addAutoSensor(sensors->at(i), topics ? topics->at(i) : "");
            else
                _addSensor(sensors->at(i), topics ? topics->at(i) : "");
        }
}

void SensorNavigator::buildCassandraTree(const map<string, vector<string> > *table, const string& root, const string& ignore) {
    if(table->size() > 0 && table->count(root))
        clearTree();
    else
        throw invalid_argument("SensorNavigator: cannot build sensor hierarchy!");

    _hierarchy = NULL;
    _usingTopics = false;
    _sensorTree = new unordered_map<string, Node>();

    boost::regex ignoreReg(ignore);

    //We initialize the sensor tree by pushing the root supernode
    Node rootNode = {-1, set<string>(), set<string>(), "", ""};
    _sensorTree->insert(make_pair(rootKey, rootNode));
    //We manually add the root's children to the tree, so as to map the Cassandra root key to the one we use
    for(const auto& s : table->at(root)) {
        if(!boost::regex_search(s.c_str(), _match, ignoreReg)) {
            Node newNode = {0, set<string>(), set<string>(), rootKey, ""};
            _sensorTree->insert(make_pair(s, newNode));
            if (table->count(s)) {
                _sensorTree->at(rootKey).children.insert(s);
                _addCassandraSensor(s, table, 1, ignoreReg);
            } else {
                //If the entry is a sensor it will share the same depth as its father, hence the -1 decrease
                _sensorTree->at(s).depth -= 1;
                _sensorTree->at(rootKey).sensors.insert(s);
            }
        }
    }
}

unordered_map<string, SensorNavigator::Node> *SensorNavigator::getSubTree(const string& node, int depth) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    if( depth < -1 )
        throw out_of_range("SensorNavigator: depth not valid for subtree query!");

    //We start by picking the start node, then we call the recursive routine
    unordered_map<string, SensorNavigator::Node> *m = new unordered_map<string, Node>();
    m->insert(make_pair(node,_sensorTree->at(node)));
    _getSubTree(node, m, depth);
    return m;
}

set<string> *SensorNavigator::getNodes(int depth, bool recursive) {
    if( depth < -1 || depth > _treeDepth )
        throw out_of_range("SensorNavigator: depth not valid for node query!");

    if(!_sensorTree)
        throw runtime_error("SensorNavigator: sensor tree not initialized!");

    //We pick all nodes in the tree whose depth is consistent with the input
    //Iterating over the node map is sufficient for this purpose
    set<string> *vec = new set<string>();
    for(const auto& n : *_sensorTree)
        if(!_isSensorNode(n.second) && (n.second.depth == depth || (recursive && n.second.depth >= depth)))
            vec->insert(n.first);

    return vec;
}

set<string> *SensorNavigator::getNodes(const string& node, bool recursive) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    if(isSensorNode(node))
        throw invalid_argument("SensorNavigator: input must be a node, not a sensor!");

    //We start by picking all children of the start node, then we proceed in the subtree (if recursive)
    set<string> *vec = new set<string>();
    _getNodes(node, vec, recursive ? 0 : 1);
    return vec;
}

set<string> *SensorNavigator::getSensors(int depth, bool recursive) {
    if( depth < -1 || depth > _treeDepth )
        throw out_of_range("SensorNavigator: depth not valid for sensor query!");

    if(!_sensorTree)
        throw runtime_error("SensorNavigator: sensor tree not initialized!");

    //Like in getNodes, we iterate over the node map and pick all sensors that are at least at the given input depth
    set<string> *vec = new set<string>();
    for(const auto& n : *_sensorTree)
        if(!_isSensorNode(n.second) && (n.second.depth == depth || (recursive && n.second.depth >= depth)))
            vec->insert(n.second.sensors.begin(), n.second.sensors.end());

    return vec;
}

set<string> *SensorNavigator::getSensors(const string& node, bool recursive) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    if(isSensorNode(node))
        throw invalid_argument("SensorNavigator: input must be a node, not a sensor!");

    //We start by picking all sensors of the start node, then we call the recursive routine
    set<string> *vec = new set<string>();
    _getSensors(node, vec, recursive ? 0 : 1);
    return vec;
}

set<string> *SensorNavigator::navigate(const string& node, int direction) {
    if(!_sensorTree || !_sensorTree->count(node))
        throw domain_error("SensorNavigator: node not found in tree!");

    set<string> *vec = new set<string>();
    if(direction == 0)
        vec->insert(node);
        //if direction is negative, we go up in the tree
    else if(direction < 0) {
        int ctr = -direction;
        string currNode = node;
        //We go up the sensor tree in iterative fashion
        while (ctr-- > 1 && currNode != rootKey) {
            currNode = _sensorTree->at(currNode).parent;
        }
        vec->insert(_sensorTree->at(currNode).parent);
    }
        //Else, we go down to the children
    else {
        _getNodes(node, vec, direction);
    }
    return vec;
}

void SensorNavigator::_getNodes(const string& node, set<string> *vec, int depth) {
    if(!_sensorTree || !_sensorTree->count(node))
        return;

    if(depth<=1)
        vec->insert(_sensorTree->at(node).children.begin(), _sensorTree->at(node).children.end());
    //We iterate over all children of the input node, and call the recursive routine over them
    //depth=1 implies that we are at the last level of the search; in this case, we do not continue
    //an input value of depth<1 implies that the search must proceed for the entire subtree
    if(depth!=1)
        for(const auto& n : _sensorTree->at(node).children)
            _getNodes(n, vec, depth-1);
}

void SensorNavigator::_getSensors(const string& node, set<string> *vec, int depth) {
    if(!_sensorTree || !_sensorTree->count(node))
        return;

    if(depth<=1)
        vec->insert(_sensorTree->at(node).sensors.begin(), _sensorTree->at(node).sensors.end());
    //We iterate over all children of the input node, and call the recursive routine over them
    //Works the same as _getNodes; here, depth does not really refer to the depth level in the tree (as sensors share
    //the depth level of the node they belong to) but rather to the search steps performed. A depth of 1 means that
    //only the sensors of the original node will be added, 2 means that the sensors of its children will be added, etc.
    if(depth!=1)
        for(const auto& n : _sensorTree->at(node).children)
            _getSensors(n, vec, depth-1);
}

void SensorNavigator::_getSubTree(const string& node, unordered_map<string, SensorNavigator::Node> *m, int depth) {
    if(!_sensorTree || !_sensorTree->count(node))
        return;

    for(const auto& s : _sensorTree->at(node).sensors)
        m->insert(make_pair(s,_sensorTree->at(s)));

    //We iterate over all children of the input node, and add them to the subtree
    if(_sensorTree->at(node).depth < depth)
        for(const auto& n : _sensorTree->at(node).children) {
            m->insert(make_pair(n,_sensorTree->at(n)));
            _getSubTree(n, m, depth);
        }
}

void SensorNavigator::_addSensor(const string& name, const string& topic) {
    string last=rootKey, prev="";

    int d=0;
    //We verify how deep in the hierarchy the input sensor is by progressively matching it with the hierarchical regexes
    for(d=0; d < (int)_hierarchy->size(); d++)
        //If we have a match with regex at level d, the sensor belongs to its subtree
        if( boost::regex_search(name.c_str(), _match, _hierarchy->at(d)) ) {
            //prev contains the name of the parent for the current node in the subtree
            prev = last;
            //last is the name of the node we are currently at; the sensor lies in this subtree
            last = _match.str(0);

            //If this node has never been instantiated, we create it now
            if( !_sensorTree->count(last) ) {
                Node newNode = {d, set<string>(), set<string>(), prev, topic};
                _sensorTree->insert(make_pair(last, newNode));
                //we add the current node to the list of children of its parent
                _sensorTree->at(prev).children.insert(last);
            }
            else if(_usingTopics) {
                // Intersecting the topics from the new sensor and the existing hierarchical node
                int pCtr = 0;
                Node& target = _sensorTree->at(last);
                while( pCtr < (int)topic.length() && pCtr < (int)target.topic.length() && target.topic[pCtr] == topic[pCtr])
                    pCtr++;
                target.topic = target.topic.substr(0, pCtr);
            }

            //Refreshing tree depth
            if( d > _treeDepth )
                _treeDepth = d;
        } else
            break;
    //Once out of the loop, we know to which node the sensor actually belongs
    _sensorTree->at(last).sensors.insert(name);
    //We add the sensor node corresponding to the current sensor
    Node sNode = {d-1, set<string>(), set<string>(), last, topic};
    _sensorTree->insert(make_pair(name, sNode));
}

void SensorNavigator::_addAutoSensor(const string& name, const string& topic) {
    string last=rootKey, prev="";
    //sepPos=0 implies that the first character is skipped when searching for separators
    size_t sepPos = 0;
    int d=0;

    if(name.empty() || name[name.size()-1] == pathSeparator) {
        clearTree();
        throw invalid_argument("SensorNavigator: sensor " + name + " does not describe a valid tree path!");
    }

    //We verify how deep in the hierarchy the input sensor is by searching for the appropriate path separators
    while((sepPos = name.find(pathSeparator, sepPos+1)) != std::string::npos) {
        //prev contains the name of the parent for the current node in the subtree
        prev = last;
        //last is the name of the node we are currently at; the sensor lies in this subtree
        last = name.substr(0, sepPos+1);

        //If this node has never been instantiated, we create it now
        if( !_sensorTree->count(last) ) {
            Node newNode = {d, set<string>(), set<string>(), prev, topic};
            _sensorTree->insert(make_pair(last, newNode));
            //we add the current node to the list of children of its parent
            _sensorTree->at(prev).children.insert(last);
        }
        else if(_usingTopics) {
            // Intersecting the topics from the new sensor and the existing hierarchical node
            int pCtr = 0;
            Node& target = _sensorTree->at(last);
            while( pCtr < (int)topic.length() && pCtr < (int)target.topic.length() && target.topic[pCtr] == topic[pCtr])
                pCtr++;
            target.topic = target.topic.substr(0, pCtr);
        }

        //Refreshing tree depth
        if( d > _treeDepth )
            _treeDepth = d;
        d++;
    }

    //Once out of the loop, we know to which node the sensor actually belongs
    _sensorTree->at(last).sensors.insert(name);
    //We add the sensor node corresponding to the current sensor
    Node sNode = {d-1, set<string>(), set<string>(), last, topic};
    _sensorTree->insert(make_pair(name, sNode));
}

void SensorNavigator::_addCassandraSensor(const string& name, const map<string, vector<string> > *table, int depth, boost::regex ignore) {
    for(const auto& s : table->at(name)) {
        if (!boost::regex_search(s.c_str(), _match, ignore)) {
            Node newNode = {depth, set<string>(), set<string>(), name, ""};
            if (!_sensorTree->count(s))
                _sensorTree->insert(make_pair(s, newNode));
            //else
            //    LOG(warning) << "Node " << s << " has multiple parents!";
            //If s is in the table, then it must be a group, otherwise it is a sensor
            if (table->count(s)) {
                _sensorTree->at(name).children.insert(s);
                _addCassandraSensor(s, table, depth + 1, ignore);
            } else {
                _sensorTree->at(s).depth -= 1;
                _sensorTree->at(name).sensors.insert(s);
            }
        }
    }
}
