//================================================================================
// Name        : QueryEngine.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class that grants query access to local and remote sensors.
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

#ifndef PROJECT_QUERYENGINE_H
#define PROJECT_QUERYENGINE_H

#include "sensornavigator.h"
#include "sensorbase.h"
#include "metadatastore.h"
#include <atomic>

using namespace std;

struct qeJobData {
    std::string domainId;
    std::string jobId;
    std::string userId;
    uint64_t    startTime;
    uint64_t    endTime;
    std::list<std::string> nodes;
};

//Typedef for the callback used to retrieve sensors
typedef bool (*QueryEngineCallback)(const string&, const uint64_t, const uint64_t, vector<reading_t>&, const bool, const uint64_t);
//Typedef for the callback used to retrieve sensors
typedef bool (*QueryEngineGroupCallback)(const vector<string>&, const uint64_t, const uint64_t, vector<reading_t>&, const bool, const uint64_t);
//Typedef for the job retrieval callback
typedef bool (*QueryEngineJobCallback)(const string&, const uint64_t, const uint64_t, vector<qeJobData>&, const bool, const bool, const string&);
//Typedef for the metadata retrieval callback
typedef bool (*QueryEngineMetadataCallback)(const string&, SensorMetadata&);

/**
 * @brief Class that grants query access to local and remote sensors
 *
 * @details This class provides an abstraction layer to where the data analytics
 *          framework is executed: access interface to sensor data is the same
 *          on dcdbpusher and collectagent.
 *          This class is implemented according to the Singleton design pattern.
 *
 * @ingroup analytics
 */
class QueryEngine {

public:

    /**
    * @brief            Returns an instance to a QueryEngine object
    *
    *                   The QueryEngine class is implemented as a singleton: therefore, all entities calling the
    *                   getInstance method will share the same instance of the QueryEngine class.
    *
    * @returns          Reference to a QueryEngine object
    */
    static QueryEngine& getInstance() {
        static QueryEngine q;
        return q;
    }

    /**
    * @brief            Set SensorNavigator object
    *
    *                   This method sets the internal SensorNavigator object that can be used by other entities to
    *                   navigate the current sensor structure.
    *
    * @param navi       Pointer to a SensorNavigator object
    */
    void  setNavigator(shared_ptr<SensorNavigator> navi)         { _navigator = navi; }
    
    /**
     * @brief           Set internal map of sensors
     * 
     *                  In certain query callback implementations, a sensor map structure is used to make access to
     *                  sensor data more efficient. In this case, this method is to be used to set and expose such
     *                  structure correctly.
     * 
     * @param sMap      Pointer to a sensor map structure
     */
    void  setSensorMap(shared_ptr<map<string, SBasePtr>> sMap)    { _sensorMap = sMap; }

    /**
    * @brief            Set the current sensor hierarchy
    *
    *                   This method sets the internal string used to build the sensor tree in the Sensor Navigator.
    *                   This is used for convenience, so that access to the global settings is not necessary.
    *
    * @param hierarchy  String containing a sensor hierarchy
    */
    void  setSensorHierarchy(const string& hierarchy)            { _sensorHierarchy = hierarchy; }
    
    /**
     * @brief           Set the current sensor filter
     * 
     *                  This method sets the internal filter string used to discard sensors when building a 
     *                  SensorNavigator object.
     * 
     * @param filter    String containing the new filter 
     */
    void  setFilter(const string& filter)                        { _filter = filter; }

    /**
     * @brief           Set the current job filter
     * 
     *                  This method sets the internal filter string used by job operators to identify
     *                  the set of jobs for which they are responsible, based on their nodelist.
     * 
     * @param filter    String containing the new job filter 
     */
    void  setJobFilter(const string& jfilter)                        { _jobFilter = jfilter; }

    /**
     * @brief           Set the current job ID filter
     * 
     *                  This method sets the internal filter string used by job operators to identify
     *                  the set of jobs for which they are responsible, based on their ID. All jobs whose ID does not
     *                  match this filter are excluded.
     * 
     * @param jidfilter    String containing the new job ID filter 
     */
    void  setJobIDFilter(const string& jidfilter)                        { _jobIdFilter = jidfilter; }

    /**
     * @brief           Set the current job match string
     * 
     *                  The job match string is used to check which jobs must be processed by job operators.
     *                  For each node in the nodelist of a job, its hostname is filtered through the job filter.
     *                  If the mode of the filtered hostnames corresponds with the job match string, the job
     *                  is assigned to the job operator.
     * 
     * @param match    String containing the new job match string 
     */
    void  setJobMatch(const string& jMatch)                        { _jobMatch = jMatch; }

    /**
     * @brief           Set the current job domain ID
     * 
     *                  This method sets the internal domain ID to be used to query jobs. Jobs operators will be
     *                  able to work only on jobs belonging to this specific domain.
     * 
     * @param jDomain   String containing the new job domain ID
     */
    void  setJobDomainId(const string& jDomain)                        { _jobDomainId = jDomain; }

    /**
    * @brief            Sets the internal callback to retrieve sensor data
    *
    *                   This method sets the internal callback that will be used by the QueryEngine to retrieve sensor
    *                   data and thus implement an abstraction layer. The callback must store all values for the input
    *                   sensor name, in the given time range, into the "buffer" vector. If such vector has not been
    *                   supplied (NULL), the callback must allocate and return a new one.
    *
    * @param cb         Pointer to a function of type QueryEngineCallback
    */
    void  setQueryCallback(QueryEngineCallback cb)                      { _callback = cb; }

    /**
    * @brief            Sets the internal callback to retrieve sensor data in groups
    *
    *                   This method sets the internal callback that will be used by the QueryEngine to retrieve sensor
    *                   data and thus implement an abstraction layer. The callback must store all values for the input
    *                   vector of sensor names, in the given time range, into the "buffer" vector. If such vector has 
    *                   not been supplied (NULL), the callback must allocate and return a new one.
    *
    * @param cb         Pointer to a function of type QueryEngineCallback
    */
    void  setGroupQueryCallback(QueryEngineGroupCallback cb)                      { _gCallback = cb; }

    /**
    * @brief            Sets the internal callback to retrieve job data
    *
    *                   This method sets the internal callback that will be used by the QueryEngine to retrieve job
    *                   data and thus implement an abstraction layer. Behavior of the callback must be identical to
    *                   that specified in setQueryCallback.
    *
    * @param jcb        Pointer to a function of type QueryEngineJobCallback
    */
    void  setJobQueryCallback(QueryEngineJobCallback jcb)               { _jCallback = jcb; }

    /**
    * @brief            Sets the internal callback to retrieve sensor metadata data
    *
    *                   This method sets the internal callback that will be used by the QueryEngine to retrieve sensor
    *                   metadata and thus implement an abstraction layer. Behavior of the callback must be identical to
    *                   that specified in setQueryCallback.
    *
    * @param mcb        Pointer to a function of type QueryEngineMetadataCallback
    */
    void  setMetadataQueryCallback(QueryEngineMetadataCallback mcb)     { _mCallback = mcb; }

    /**
    * @brief            Returns the internal SensorNavigator object
    *
    * @return           Pointer to a SensorNavigator object
    */
    const shared_ptr<SensorNavigator> getNavigator()             { return _navigator; }
    
    /**
     * @brief           Returns the internal sensor map data structure
     * 
     * @return          Pointer to a sensor map 
     */
    const shared_ptr<map<string, SBasePtr>> getSensorMap()       { return _sensorMap; }

    /**
    * @brief            Returns the current sensor hierarchy
    *
    * @return           String containing the current sensor hierarchy
    */
    const string&  getSensorHierarchy()                          { return _sensorHierarchy; }

    /**
    * @brief            Returns the current sensor filter
    *
    * @return           String containing the current sensor filter
    */
    const string&  getFilter()                                   { return _filter; }

    /**
    * @brief            Returns the current job filter
    *
    * @return           String containing the current job filter
    */
    const string&  getJobFilter()                                   { return _jobFilter; }

    /**
    * @brief            Returns the current job ID filter
    *
    * @return           String containing the current job ID filter
    */
    const string&  getJobIdFilter()                                   { return _jobIdFilter; }

    /**
    * @brief            Returns the current job match string
    *
    * @return           String containing the current job match
    */
    const string&  getJobMatch()                                   { return _jobMatch; }

    /**
    * @brief            Returns the current job domain ID string
    *
    * @return           String containing the current job domain ID
    */
    const string&  getJobDomainId()                                { return _jobDomainId; }

    /**
    * @brief            Perform a sensor query
    *
    *                   This method allows to retrieve readings for a certain sensor in a given time range. The input
    *                   "buffer" vector allows to re-use memory over successive readings. Note that in order to use
    *                   this method, a callback must have been set through the setQueryCallback method. If not, this
    *                   method will throw an exception.
    *                   
    *                   The "rel" argument governs how the search is performed in local sensor caches: if set to true,
    *                   startTs and endTs indicate relative offsets against the most recent reading, and the returned
    *                   vector is a view of the cache whose range is computed statically in O(1), and therefore the
    *                   underlying data may be slightly unaligned depending on the sampling rate. If rel is set to
    *                   false, startTs and endTs are interpreted as absolute timestamps, and the cache view is 
    *                   determined by performing binary search with O(log(n)) complexity, thus resulting in a accurate 
    *                   time range. Relative and absolute mode have different data guarantees:
    *                   
    *                  - Relative: returned data is guaranteed to not be stale, but it can extend outside of the
    *                      queried range by a factor proportional to sensor's sampling rate.
    *                  - Absolute: returned data is guaranteed to not be stale and to be strictly within the queried
    *                      time range.
    *
    * @param name       Name of the sensor to be queried
    * @param startTs    Start timestamp (in nanoseconds) of the time range for the query
    * @param endTs      End timestamp (in nanoseconds) of the time range for the query. Must be >= startTs
    * @param buffer     Reference to a vector in which readings must be stored.
    * @param rel        If true, the input timestamps are considered to be relative offset against "now"
    * @param tol        Tolerance (in ns) for returned timestamps. Does not affect Cassandra range queries. 
    * @return           True if successful, false otherwise
    */
    bool querySensor(const string& name, const uint64_t startTs, const uint64_t endTs, vector<reading_t>& buffer, const bool rel=true, const uint64_t tol=3600000000000) {
        if(!_callback)
            throw runtime_error("Query Engine: callback not set!");
        if((startTs > endTs && !rel) || (startTs < endTs && rel))
            throw invalid_argument("Query Engine: invalid time range!");
        return _callback(name, startTs, endTs, buffer, rel, tol);
    }

    /**
    * @brief            Perform a sensor query
    *
    *                   This is an overloaded version of the querySensor() method. It accepts a vector of sensor names
    *                   instead of a single sensor. These will be queried collectively, and the result is returned.
    *                   
    * @return           True if successful, false otherwise
    */
    bool querySensor(const vector<string>& names, const uint64_t startTs, const uint64_t endTs, vector<reading_t>& buffer, const bool rel=true, const uint64_t tol=3600000000000) {
        if(!_gCallback)
            throw runtime_error("Query Engine: callback not set!");
        if((startTs > endTs && !rel) || (startTs < endTs && rel))
            throw invalid_argument("Query Engine: invalid time range!");
        return _gCallback(names, startTs, endTs, buffer, rel, tol);
    }

    /**
    * @brief            Perform a job query
    *
    *                   This method allows to retrieve data for jobs running in a given time range. The input
    *                   "buffer" vector allows to re-use memory over successive readings. Note that in order to use
    *                   this method, a callback must have been set through the setJobQueryCallback method. If not, this
    *                   method will throw an exception.
    *                   
    *                   The "rel" argument governs how the search is performed in local sensor caches: if set to true,
    *                   startTs and endTs indicate relative offsets against the most recent reading, and the returned
    *                   vector is a view of the cache whose range is computed statically in O(1), and therefore the
    *                   underlying data may be slightly unaligned depending on the sampling rate. If rel is set to
    *                   false, startTs and endTs are interpreted as absolute timestamps, and the cache view is 
    *                   determined by performing binary search with O(log(n)) complexity, thus resulting in a accurate 
    *                   time range. This parameter does not affect the query method when using the Cassandra datastore.
    *
    * @param jobId      ID of the job to be retrieved (only if range=false)
    * @param startTs    Start timestamp (in nanoseconds) of the time range for the query (only if range=true)
    * @param endTs      End timestamp (in nanoseconds) of the time range for the query. (only if range=true)
    * @param buffer     Reference to a vector in which job info must be stored.
    * @param rel        If true, the input timestamps are considered to be relative offset against "now"
    * @param range      If true, the jobId parameter is ignored, and all jobs in the given time range are returned 
    * @param domainId   Job domain ID to be used for the query 
    * @return           True if successful, false otherwise
    */
    bool queryJob(const string& jobId, const uint64_t startTs, const uint64_t endTs, vector<qeJobData>& buffer, const bool rel=true, const bool range=false, const string& domainId="default") {
        if(!_jCallback)
            throw runtime_error("Query Engine: job callback not set!");
        if((startTs > endTs && !rel) || (startTs < endTs && rel))
            throw invalid_argument("Query Engine: invalid time range!");
        return _jCallback(jobId, startTs, endTs, buffer, rel, range, domainId);
    }

    /**
    * @brief            Perform a sensor metadata query
    *
    *                   This method allows to retrieve the metadata of available sensors. The input "buffer" object
    *                   allows to re-use memory over successive readings. Note that in order to use this method, a 
    *                   callback must have been set through the setMetadataQueryCallback method. If not, this
    *                   method will throw an exception.
    *
    * @param name       Name of the sensor to be queried
    * @param buffer     SensorMetadata object in which to store the result
    * @return           True if successful, false otherwise
    */
    bool queryMetadata(const string& name, SensorMetadata& buffer) {
        if(!_mCallback)
            throw runtime_error("Query Engine: sensor metadata callback not set!");
        return _mCallback(name, buffer);
    }

    /**
     * @brief           Locks access to the QueryEngine
     * 
     *                  Once this method returns, the invoking thread can safely update all internal data structures
     *                  of the QueryEngine. the unlock() method must be called afterwards.
     */
    void lock() {
        // Locking out new threads from the QueryEngine callbacks
        updating.store(true);
        // Waiting until all previous threads have finished using the QueryEngine
        while(access.load()>0) {}
    }
    
    /**
     * @brief           Unlocks access to the QueryEngine
     * 
     *                  Must be called after a lock() call.
     */
    void unlock() {
        access.store(0);
        updating.store(false);
    }
    
    //Internal atomic flags used for utility purposes
    atomic<bool> updating;
    atomic<int>  access;

private:

    /**
    * @brief            Private class constructor
    */
    QueryEngine() {
        _navigator = NULL;
        _sensorMap = NULL;
        _callback = NULL;
        _jCallback = NULL;
        _mCallback = NULL;
        updating.store(false);
        access.store(0);
    }

    /**
    * @brief            Copy constructor is not available
    */
    QueryEngine(QueryEngine const&)     = delete;

    /**
    * @brief            Assignment operator is not available
    */
    void operator=(QueryEngine const&)  = delete;

    // Internal pointer to a SensorNavigator
    shared_ptr<SensorNavigator> _navigator;
    // Internal pointer to a sensor map used in certain query callback implementations
    shared_ptr<map<string, SBasePtr>> _sensorMap;
    // Callback used to retrieve sensor data
    QueryEngineCallback _callback;
    // Callback used to retrieve sensor data in groups
    QueryEngineGroupCallback _gCallback;
    // Callback used to retrieve job data
    QueryEngineJobCallback _jCallback;
    // Callback used to retrieve metadata
    QueryEngineMetadataCallback _mCallback;
    // String storing the current sensor hierarchy, used for convenience
    string _sensorHierarchy;
    // String storing the filter to be used when building a sensor navigator
    string _filter;
    // String storing the job filter to be used by job operators
    string _jobFilter;
    // String storing the matching string resulting from the job filter for a job to be processed
    string _jobMatch;
    // String storing the job ID filter to be used by job operators
    string _jobIdFilter;
    // String containing the job domain ID to be queried by job operators
    string _jobDomainId;
};

#endif //PROJECT_QUERYENGINE_H
