//================================================================================
// Name        : metadatastore.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : A meta-data store for sensors
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
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

#ifndef PROJECT_METADATASTORE_H
#define PROJECT_METADATASTORE_H

#include <set>
#include <unordered_map>
#include <atomic>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include "globalconfiguration.h"

using namespace std;

//TODO: evaluate if necessary to change TTL type to int64 with default -1 value
class SensorMetadata {
    
public:

    typedef enum { 
        IS_OPERATION_SET = 1,
        IS_VIRTUAL_SET = 2, 
        INTEGRABLE_SET = 4,
        MONOTONIC_SET = 8,
        PUBLICNAME_SET = 16,
        PATTERN_SET = 32,
        UNIT_SET = 64,
        SCALE_SET = 128,
        TTL_SET = 256,
        INTERVAL_SET = 512,
        OPERATIONS_SET = 1024,
	DELTA_SET = 2048
    } MetadataMask;
    
    SensorMetadata() :
        isOperation(false),
        isVirtual(false),
        integrable(false),
        monotonic(false),
        publicName(""),
        pattern(""),
        unit(""),
        scale(1),
        ttl(0),
        interval(0),
        operations(),
        delta(false),
        setMask(0) {}

    SensorMetadata(const SensorMetadata& other) {
        SensorMetadata();
        setMask = other.setMask;
        isOperation = other.isOperation;
        isVirtual = other.isVirtual;
        integrable = other.integrable;
        monotonic = other.monotonic;
        publicName = other.publicName;
        pattern = other.pattern;
        unit = other.unit;
        scale = other.scale;
        ttl = other.ttl;
        interval = other.interval;
        operations = other.operations;
	delta = other.delta;
    }

    SensorMetadata& operator=(const SensorMetadata& other) {
        setMask = other.setMask;
        isOperation = other.isOperation;
        isVirtual = other.isVirtual;
        integrable = other.integrable;
        monotonic = other.monotonic;
        publicName = other.publicName;
        pattern = other.pattern;
        unit = other.unit;
        scale = other.scale;
        ttl = other.ttl;
        interval = other.interval;
        operations = other.operations;
	delta = other.delta;
        return *this;
    }
    
    /**
     * @brief               Parses a JSON string and stores the content in this object.
     * 
     *                      If parsing fails, a InvalidArgument exception is thrown.
     * 
     * @param payload       JSON-encoded string containing metadata information
     */
    void parseJSON(const string& payload) {
        boost::property_tree::iptree config;
        std::istringstream str(payload);
        boost::property_tree::read_json(str, config);
        parsePTREE(config);
    }

    /**
     * @brief               Parses a CSV string and stores the content in this object.
     * 
     *                      If parsing fails, a InvalidArgument exception is thrown.
     * 
     * @param payload       CSV-encoded string containing metadata information
     */
    void parseCSV(const string& payload) {
        uint64_t fieldCtr = 0, oldPos = 0, newPos = 0;
        std::string buf = "";
        while(oldPos < payload.length() && (newPos = payload.find(",", oldPos)) != std::string::npos) {
            if((newPos - oldPos) > 0) {
                buf = payload.substr(oldPos, newPos - oldPos);
                switch (fieldCtr) {
                    case 0 :
                        setScale(stod(buf));
                        break;
                    case 1 :
                        setIsOperation(to_bool(buf));
                        break;
                    case 2 :
                        setIsVirtual(to_bool(buf));
                        break;
                    case 3 :
                        setMonotonic(to_bool(buf));
                        break;
                    case 4 :
                        setIntegrable(to_bool(buf));
                        break;
                    case 5 :
                        setUnit(buf);
                        break;
                    case 6 :
                        setPublicName(buf);
                        break;
                    case 7 :
                        setPattern(buf);
                        break;
                    case 8 :
                        setInterval(stoull(buf) * 1000000);
                        break;
                    case 9 :
                        setTTL(stoull(buf) * 1000000);
                        break;
                    case 10 :
                        setOperations(_parseOperations(buf, ','));
                        oldPos = payload.length();
                        break;
		    case 11 :
			setDelta(to_bool(buf));
			break;
                }
            }
            fieldCtr++;
            oldPos = newPos + 1;
        }
        if(fieldCtr < 11) {
            throw std::invalid_argument("Wrong number of fields in CSV entry!");
        }
    }

    /**
     * @brief               Parses a PTREE INFO block and stores the content in this object.
     *
     *                      If parsing fails, a InvalidArgument exception is thrown.
     *                      
     * @param config        PTREE block containing metadata
     */
    void parsePTREE(boost::property_tree::iptree& config) {
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config) {
            if (boost::iequals(val.first, "monotonic")) {
                setMonotonic(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "isVirtual")) {
                setIsVirtual(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "isOperation")) {
                setIsOperation(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "integrable")) {
                setIntegrable(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "unit")) {
                setUnit(val.second.data());
            } else if (boost::iequals(val.first, "publicName")) {
                setPublicName(val.second.data());
            } else if (boost::iequals(val.first, "pattern")) {
                setPattern(val.second.data());
            } else if (boost::iequals(val.first, "scale")) {
                setScale(stod(val.second.data()));
            } else if (boost::iequals(val.first, "interval")) {
                setInterval(stoull(val.second.data()) * 1000000);
            } else if (boost::iequals(val.first, "ttl")) {
                setTTL(stoull(val.second.data()) * 1000000);
            } else if (boost::iequals(val.first, "operations")) {
                setOperations(val.second.data());
	    } else if (boost::iequals(val.first, "delta")) {
		setDelta(to_bool(val.second.data()));
            }
        }
    }
    
    /**
     * @brief               Converts the content of this object into JSON format.      
     * 
     * @return              String containing the JSON representation of this object
     */
    string getJSON() const {
        boost::property_tree::ptree config;
        std::ostringstream output;
        _dumpPTREE(config);
        boost::property_tree::write_json(output, config, true);
        return output.str();
    }

    /**
     * @brief               Converts the content of this object into CSV format.      
     * 
     * @return              String containing the CSV representation of this object
     */
    string getCSV() const {
        std::string buf = "";
        if(setMask & SCALE_SET) {
            std::ostringstream scaleStream;
            scaleStream << scale;
            buf += scaleStream.str() + ",";
        } else {
            buf += ",";
        }
        buf += (setMask & IS_OPERATION_SET) ? bool_to_str(isOperation) + "," : ",";
        buf += (setMask & IS_VIRTUAL_SET) ? bool_to_str(isVirtual) + "," : ",";
        buf += (setMask & MONOTONIC_SET) ? bool_to_str(monotonic) + "," : ",";
        buf += (setMask & INTEGRABLE_SET) ? bool_to_str(integrable) + "," : ",";
        buf += (setMask & UNIT_SET) ? unit + "," : ",";
        buf += (setMask & PUBLICNAME_SET) ? publicName + "," : ",";
        buf += (setMask & PATTERN_SET) ? pattern + "," : ",";
        buf += (setMask & INTERVAL_SET) ? to_string(interval / 1000000) + "," : ",";
        buf += (setMask & TTL_SET) ? to_string(ttl / 1000000) + "," : ",";
        buf += (setMask & OPERATIONS_SET) ? _dumpOperations(',') + "," : ",";
	buf += (setMask & DELTA_SET) ? bool_to_str(delta) + "," : ",";
        return buf;
    }

    /**
     * @brief               Returns a sensorMetadata_t object from the internal map, converted into PTREE format.     
     * 
     * @return              A PTREE object representing this SensorMetadata object
     */
    boost::property_tree::ptree getPTREE() const {
        boost::property_tree::ptree config;
        _dumpPTREE(config);
        return config;
    }
    
    const bool isValid()          const               { return setMask & (PUBLICNAME_SET | PATTERN_SET); }            
    
    //Getters and setters
    const bool* getIsOperation()  const               { return (setMask & IS_OPERATION_SET) ? &isOperation : nullptr; }
    const bool* getIsVirtual()    const               { return (setMask & IS_VIRTUAL_SET) ? &isVirtual : nullptr; }
    const bool* getIntegrable()   const               { return (setMask & INTEGRABLE_SET) ? &integrable : nullptr; }
    const bool* getMonotonic()    const               { return (setMask & MONOTONIC_SET) ? &monotonic : nullptr; }
    const string* getPublicName() const               { return (setMask & PUBLICNAME_SET) ? &publicName : nullptr; }
    const string* getPattern()    const               { return (setMask & PATTERN_SET) ? &pattern : nullptr; }
    const string* getUnit()       const               { return (setMask & UNIT_SET) ? &unit : nullptr; }
    const double* getScale()      const               { return (setMask & SCALE_SET) ? &scale : nullptr; }
    const uint64_t* getTTL()      const               { return (setMask & TTL_SET) ? &ttl : nullptr; }
    const uint64_t* getInterval() const               { return (setMask & INTERVAL_SET) ? &interval : nullptr; }
    const set<string>* getOperations() const          { return (setMask & OPERATIONS_SET) ? &operations : nullptr; }
    const string getOperationsString() const          { return (setMask & OPERATIONS_SET) ? _dumpOperations() : ""; }
    const bool* getDelta()        const               { return (setMask & DELTA_SET) ? &delta : nullptr; }

    void setIsOperation(bool o)                 { isOperation = o; setMask = setMask | IS_OPERATION_SET; }
    void setIsVirtual(bool v)                   { isVirtual = v; setMask = setMask | IS_VIRTUAL_SET; }
    void setIntegrable(bool i)                  { integrable = i; setMask = setMask | INTEGRABLE_SET; }
    void setMonotonic(bool m)                   { monotonic = m; setMask = setMask | MONOTONIC_SET; }
    void setPublicName(string p)                { publicName = p; setMask = setMask | PUBLICNAME_SET; }
    void setPattern(string p)                   { pattern = p; setMask = setMask | PATTERN_SET; }
    void setUnit(string u)                      { unit = u; setMask = setMask | UNIT_SET; }
    void setScale(double s)                     { scale = s; setMask = setMask | SCALE_SET; }
    void setTTL(uint64_t t)                     { ttl = t; setMask = setMask | TTL_SET; }
    void setInterval(uint64_t i)                { interval = i; setMask = setMask | INTERVAL_SET; }
    void setOperations(const string& o)         { setOperations(_parseOperations(o)); }
    void clearOperations()                      { operations.clear(); setMask = setMask & ~OPERATIONS_SET; }
    void setDelta(bool d)                       { delta = d; setMask = setMask | DELTA_SET; }

    // Merges a set of operations with the local one
    void setOperations(const set<string>& o) { 
        if(setMask & OPERATIONS_SET)
            operations.insert(o.begin(), o.end());
        else
            operations = set<string>(o);
        setMask = setMask | OPERATIONS_SET;
    }

    // Adds a single operation. Requires the publicName field to be set.
    // Here the operation is assumed to be the full sensor name, from which the actual operation name is extracted.
    bool addOperation(const string& opName) {
        if ((setMask & PUBLICNAME_SET) && publicName.length()>0 && opName.length()>publicName.length() 
        && !opName.compare(0, publicName.length(), publicName)) {
            setOperations(opName.substr(publicName.length()));
            return true;
        }
        else
            return false;
    }
    
protected:

    // Parses a operations string and sanitizes it from excess whitespace
    set<string> _parseOperations(const string& str, const char sep=',') {
        set<string> v;
        
        // We split the string into the comma-separated tokens
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, sep)) {
            if(!token.empty()) {
                boost::algorithm::trim(token);
                v.insert(token);
            }
        }
        return v;
    }
    
    string _dumpOperations(const char sep=',') const {
        string out="";
        // We re-write the vector into a string, this time properly formatted
        string sepStr = string(1,sep);
        if(setMask & OPERATIONS_SET) {
            for (const auto &el : operations)
                out += el + sepStr;
            if (!out.empty() && out.back() == sep)
                out.erase(out.size() - 1, 1);
        }
        return out;
    }
    
    // Dumps the contents of "s" in "config"
    void _dumpPTREE(boost::property_tree::ptree& config) const {
        config.clear();
        if(setMask & SCALE_SET) {
            std::ostringstream scaleStream;
            scaleStream << scale;
            config.push_back(boost::property_tree::ptree::value_type("scale", boost::property_tree::ptree(scaleStream.str())));
        }
        if(setMask & IS_OPERATION_SET)
            config.push_back(boost::property_tree::ptree::value_type("isOperation", boost::property_tree::ptree(bool_to_str(isOperation))));
        if(setMask & IS_VIRTUAL_SET)
            config.push_back(boost::property_tree::ptree::value_type("isVirtual", boost::property_tree::ptree(bool_to_str(isVirtual))));
        if(setMask & MONOTONIC_SET)
            config.push_back(boost::property_tree::ptree::value_type("monotonic", boost::property_tree::ptree(bool_to_str(monotonic))));
        if(setMask & INTEGRABLE_SET)
            config.push_back(boost::property_tree::ptree::value_type("integrable", boost::property_tree::ptree(bool_to_str(integrable))));
        if(setMask & UNIT_SET)
            config.push_back(boost::property_tree::ptree::value_type("unit", boost::property_tree::ptree(unit)));
        if(setMask & PUBLICNAME_SET)
            config.push_back(boost::property_tree::ptree::value_type("publicName", boost::property_tree::ptree(publicName)));
        if(setMask & PATTERN_SET)
            config.push_back(boost::property_tree::ptree::value_type("pattern", boost::property_tree::ptree(pattern)));
        if(setMask & INTERVAL_SET)
            config.push_back(boost::property_tree::ptree::value_type("interval", boost::property_tree::ptree(to_string(interval / 1000000))));
        if(setMask & TTL_SET)
            config.push_back(boost::property_tree::ptree::value_type("ttl", boost::property_tree::ptree(to_string(ttl / 1000000))));
        if(setMask & OPERATIONS_SET)
            config.push_back(boost::property_tree::ptree::value_type("operations", boost::property_tree::ptree(_dumpOperations())));
	if(setMask & DELTA_SET)
	    config.push_back(boost::property_tree::ptree::value_type("delta", boost::property_tree::ptree(bool_to_str(delta))));

    }

    // Protected class members
    bool                isOperation;
    bool                isVirtual;
    bool                integrable;
    bool                monotonic;
    string              publicName;
    string              pattern;
    string              unit;
    double              scale;
    uint64_t            ttl;
    uint64_t            interval;
    set<string>         operations;
    bool                delta;
    uint64_t            setMask;
};

// ---------------------------------------------------------------------------
// --------------------------- METADATASTORE CLASS --------------------------- 
// ---------------------------------------------------------------------------

class MetadataStore {
    
public:
    
    /**
     * @brief               Public class constructor.
     */
    MetadataStore() {
        _updating.store(false);
        _access.store(0);
    }
    
    /**
     * @brief               Class destructor.
     */
    ~MetadataStore() {}
    
    /**
     * @brief               Clears the internal metadata map.
     */
    void clear() { 
        _metadata.clear(); 
    }
    
    /**
     * @brief               Returns the internal metadata map.
     * 
     * @return              A string to Metadata_t map
     */
    const unordered_map<string, SensorMetadata>& getMap() { 
        return _metadata; 
    }
    
    /**
     * @brief               Waits for internal updates to finish.
     */
    const void wait() {
        while(_updating.load()) {}
        ++_access;
    }

    /**
     * @brief               Reduces the internal reading counter.
     */
    const void release() {
        --_access;
    }
    
    /**
     * @brief               Stores a sensorMetadata_t object in the internal map.
     * 
     *                      If the input key already exists in the map, the entry is overwritten.
     * 
     * @param key           Sensor key under which metadata must be stored
     * @param s             Object containing sensor metadata
     * @return              True if "key" is unique, False if there was a collision
     */
    bool store(const string& key, const SensorMetadata& s) {
        // Spinlock to update the metadata store
        while(_updating.exchange(true)) {}
        while(_access.load()>0) {}
        bool overwritten = !_metadata.count(key);
        _metadata[key] = s;
        _updating.store(false);
        return overwritten;
    }
    
    /**
     * @brief               Stores a sensorMetadata_t object in the internal map, after parsing it from a JSON string.
     * 
     *                      If the input key already exists in the map, the entry is overwritten. If parsing fails,
     *                      a InvalidArgument exception is thrown.
     * 
     * @param key           Sensor key under which metadata must be stored
     * @param payload       JSON-encoded string containing metadata information
     * @return              True if "key" is unique, False if there was a collision
     */
    bool storeFromJSON(const string& key, const string& payload) {
        SensorMetadata metadata;
        metadata.parseJSON(payload);
        return store(key, metadata);
    }
    
    /**
     * @brief               Stores a sensorMetadata_t object in the internal map, after parsing it from a INFO block.
     *
     *                      If the input key already exists in the map, the entry is overwritten. If parsing fails,
     *                      a InvalidArgument exception is thrown.
     *                      
     * @param key           Sensor key under which metadata must be stored
     * @param config        PTREE block containing metadata
     * @return              True if "key" is unique, False if there was a collision
     */
    bool storeFromPTREE(const string& key, boost::property_tree::iptree& config) {
        SensorMetadata metadata;
        metadata.parsePTREE(config);
        return store(key, metadata);
    }
    
    /**
     * @brief               Returns a sensorMetadata_t object from the internal map. 
     * 
     *                      If the input key does not exist in the map, a InvalidArgument exception is thrown.
     * 
     * @param key           Sensor key to be queried
     * @return              A reference to a sensorMetadata_t object
     */
    SensorMetadata get(const string& key) {
        wait();
        auto it = _metadata.find(key);
        if(it==_metadata.end()) {
            release();
            throw invalid_argument("MetadataStore: key " + key + " does not exist!");
        } else {
            SensorMetadata sm = it->second;
            release();
            return sm;
        }
    }

    /**
     * @brief               Returns the TTL of a sensorMetadata_t object from the internal map. 
     * 
     *                      If the input key does not exist in the map, the value -1 is returned. This method exists
     *                      to boost (slightly) look-up performance in the CollectAgent, which requires TTL values
     *                      when performing database inserts.
     * 
     * @param key           Sensor key to be queried
     * @return              TTL value in seconds
     */
    int64_t getTTL(const string& key) {
        int64_t ttl = 0;
        wait();
        auto it = _metadata.find(key);
        ttl = it==_metadata.end() || !it->second.getTTL() ? -1 : *it->second.getTTL()/1000000000;
        release();
        return ttl;
    }
    
    /**
     * @brief               Returns a sensorMetadata_t object from the internal map, converted into JSON format.      
     * 
     *                      If the input key does not exist in the map, a InvalidArgument exception is thrown.
     * 
     * @param key           Sensor key to be queried
     * @return              String containing the JSON representation of the sensorMetadata_t object
     */
    string getJSON(const string& key) {
        return this->get(key).getJSON();
    }
    
    /**
     * @brief               Returns a sensorMetadata_t object from the internal map, converted into PTREE format.     
     * 
     *                      If the input key does not exist in the map, a InvalidArgument exception is thrown.
     * 
     * @param key           Sensor key to be queried
     * @return              A PTREE object representing the sensorMetadata_t object
     */
     boost::property_tree::ptree getPTREE(const string& key) {
        return this->get(key).getPTREE();
    }
    
protected:
    
    unordered_map<string, SensorMetadata> _metadata;
    atomic<bool> _updating;
    atomic<int>  _access;
    
};


#endif //PROJECT_METADATASTORE_H
