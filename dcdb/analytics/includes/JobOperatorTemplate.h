//================================================================================
// Name        : JobOperatorTemplate.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features needed by Operators.
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

#ifndef PROJECT_JOBOPERATORTEMPLATE_H
#define PROJECT_JOBOPERATORTEMPLATE_H

#include "OperatorTemplate.h"

/**
 * @brief Template that implements features needed by Job Operators and
 *        complying to OperatorInterface.
 *
 * @details This template is derived from OperatorTemplate, and is adjusted to
 *          simplify job-related computations.
 *
 * @ingroup operator
 */
template <typename S>
class JobOperatorTemplate : virtual public OperatorTemplate<S> {
    // The template shall only be instantiated for classes which derive from SensorBase
    static_assert(is_base_of<SensorBase, S>::value, "S must derive from SensorBase!");

protected:
    
    // For readability
    using S_Ptr = shared_ptr<S>;
    using U_Ptr = shared_ptr< UnitTemplate<S> >;

public:
    
    /**
    * @brief            Class constructor
    *
    * @param name       Name of the operator
    */
    JobOperatorTemplate(const string name) :
            OperatorTemplate<S>(name) {
        
        _unitAccess.store(false);
        this->_dynamic = true;
        this->_jobFilterStr = QueryEngine::getInstance().getJobFilter();
        this->_jobMatchStr = QueryEngine::getInstance().getJobMatch();
        this->_jobIdFilterStr = QueryEngine::getInstance().getJobIdFilter();
        this->_jobDomainId = QueryEngine::getInstance().getJobDomainId();
        this->_jobFilter = boost::regex(this->_jobFilterStr);
        this->_jobIdFilter = boost::regex(this->_jobIdFilterStr);
    }

    /**
    * @brief            Copy constructor
    *
    */
    JobOperatorTemplate(const JobOperatorTemplate& other) :
            OperatorTemplate<S>(other) {
        
        _unitAccess.store(false);
        this->_dynamic = true;
        this->_jobFilterStr = QueryEngine::getInstance().getJobFilter();
        this->_jobMatchStr = QueryEngine::getInstance().getJobMatch();
        this->_jobIdFilterStr = QueryEngine::getInstance().getJobIdFilter();
        this->_jobDomainId = QueryEngine::getInstance().getJobDomainId();
        this->_jobFilter = boost::regex(this->_jobFilterStr);
        this->_jobIdFilter = boost::regex(this->_jobIdFilterStr);
    }

    /**
    * @brief            Assignment operator
    *
    */
    JobOperatorTemplate& operator=(const JobOperatorTemplate& other) {
        OperatorTemplate<S>::operator=(other);
        this->_dynamic = true;
        this->_jobFilterStr = QueryEngine::getInstance().getJobFilter();
        this->_jobMatchStr = QueryEngine::getInstance().getJobMatch();
        this->_jobIdFilterStr = QueryEngine::getInstance().getJobIdFilter();
        this->_jobDomainId = QueryEngine::getInstance().getJobDomainId();
        this->_jobFilter = boost::regex(this->_jobFilterStr);
        this->_jobIdFilter = boost::regex(this->_jobIdFilterStr);
        return *this;
    }
            
    /**
    * @brief            Class destructor
    */
    virtual ~JobOperatorTemplate() {}
    
    /**
    * @brief              Returns the units of this operator
    *
    *                     The units returned by this method are of the UnitInterface type. The actual units, in their
    *                     derived type, are used internally. This type of operator employs dynamic units that are
    *                     generated at runtime: as such, an internal unit lock is acquired upon calling this method,
    *                     and must later be released through the releaseUnits() method.
    *
    * @return             The vector of UnitInterface objects of this operator
    */
    virtual vector<UnitPtr>& getUnits() override	{
        // Spinlock to regulate access to units - normally innocuous
        while(_unitAccess.exchange(true)) {}
        return this->_baseUnits;
    }
    
    /**
     * @brief             Releases the access lock to units
     * 
     *                    This method must be called anytime operations on units are performed through getUnits().
     */
    virtual void releaseUnits() override {
        _unitAccess.store(false);
    }
    
    /**
    * @brief              Performs an on-demand compute task
    *
    *                     Unlike the protected computeAsync and compute methods, computeOnDemand allows to interactively
    *                     perform data analytics queries on the operator, which must have the _streaming attribute set
    *                     to false. A unit is generated on the fly, corresponding to the input node given as input,
    *                     and results are returned in the form of a map.
    *
    * @param node         Unit name for which the query must be performed
    * @return             a map<string, reading_t> containing the output of the query
    */
    virtual map<string, reading_t> computeOnDemand(const string& node="__root__") override {
        map<string, reading_t> outMap;
        if( !this->_streaming ) {
            try {
                // Getting exclusive access to the operator
                while( this->_onDemandLock.exchange(true) ) {}
                std::string jobId = MQTTChecker::topicToJob(node);
                _jobDataVec.clear();
                if(this->_queryEngine.queryJob(jobId, 0, 0, _jobDataVec, true, false, _jobDomainId) && !_jobDataVec.empty()) {
                    U_Ptr jobUnit = jobDataToUnit(_jobDataVec[0]);
                    if(!jobUnit)
                        throw std::runtime_error("Job " + node + " not in the domain of operator " + this->_name + "!");
                    this->compute(jobUnit, _jobDataVec[0]);
                    this->retrieveAndFlush(outMap, jobUnit);
                } else
                    throw std::runtime_error("Operator " + this->_name + ": cannot retrieve job data!");
            } catch(const exception& e) {
                this->_onDemandLock.store(false);
                throw;
            }
            this->_onDemandLock.store(false);
        } else if( this->_keepRunning ) {
            bool found = false;
            //Spinning explicitly as we need to iterate on the derived Unit objects
            while(_unitAccess.exchange(true)) {}
            for(const auto& u : this->_units)
                if(u->getName() == node) {
                    found = true;
                    this->retrieveAndFlush(outMap, u, false);
                }
            releaseUnits();

            if(!found)
                throw std::domain_error("Job " + node + " does not belong to the domain of " + this->_name + "!");
        } else
            throw std::runtime_error("Operator " + this->_name + ": not available for on-demand query!");
        return outMap;
    }
    
protected:
    
    using OperatorTemplate<S>::compute;
    
    /**
    * @brief              Data analytics (job) computation logic
    *
    *                     This method contains the actual logic used by the analyzed, and is automatically called by
    *                     the computeAsync method. This variant of the compute() method defined in OperatorTemplate also
    *                     includes a job data structure in its list of arguments, and is specialized for job operators.
    *
    * @param unit         Shared pointer to unit to be processed
    * @param jobData      Job data structure 
    */
    virtual void compute(U_Ptr unit, qeJobData& jobData) = 0;
    
    /**
     * @brief           This method encapsulates all logic to generate and manage job units
     * 
     *                  The algorithm implemented in this method is very similar to that used in computeOnDemand in
     *                  OperatorTemplate, and it is used to manage job units both in on-demand and streaming mode. The
     *                  internal unit cache is used to store recent job units. Moreover, the job data returned by the
     *                  QueryEngine is converted to a format compatible with the UnitGenerator.
     * 
     * @param jobData   a qeJobData struct containing job information
     * @return          A shared pointer to a job unit object
     */
    virtual U_Ptr jobDataToUnit(qeJobData& jobData) {
        string jobTopic = MQTTChecker::jobToTopic(jobData.jobId);
        U_Ptr jobUnit = nullptr;
        if(!this->_unitCache)
            throw std::runtime_error("Initialization error in operator " + this->_name + "!");

        if (this->_unitCache->count(jobTopic)) {
            jobUnit = this->_unitCache->at(jobTopic);
            if(!this->_streaming)
                LOG(debug) << "Operator " << this->_name << ": cache hit for unit " << jobTopic << ".";
            
        } else {
            if (!this->_unitCache->count(SensorNavigator::templateKey))
                throw std::runtime_error("No template unit in operator " + this->_name + "!");
            if(!this->_streaming)
                LOG(debug) << "Operator " << this->_name << ": cache miss for unit " << jobTopic << ".";
            if(!this->filterJob(jobData))
                return nullptr;
            U_Ptr uTemplate = this->_unitCache->at(SensorNavigator::templateKey);
            shared_ptr<SensorNavigator> navi = this->_queryEngine.getNavigator();
            UnitGenerator<S> unitGen(navi);
            // The job unit is generated as a hierarchical unit
            jobUnit = unitGen.generateFromTemplate(uTemplate, jobTopic, jobData.nodes, this->_mqttPart, this->_enforceTopics, this->_relaxed);
            // Initializing sensors if necessary
            jobUnit->init(this->_interval, this->_queueSize);
            this->addToUnitCache(jobUnit);
        }
        return jobUnit;
    }
    
    /**
     * @brief             Tests the job against the internal filter
     * 
     *                    This method is used to filter out jobs for which this operator is not responsible. By default,
     *                    the operator checks the first node in the nodelist of the job, and if its hostname matches
     *                    with the internal job filter regex, the job is accepted. This method can be overridden to
     *                    implement more complex filtering policies.
     * 
     * @param jobData     a qeJobData struct containing job information
     * @return            True if the job should be processed, false otherwise
     */
    virtual bool filterJob(qeJobData& jobData) {
        // Job with no nodes - a unit cannot be built
        if(jobData.nodes.empty())
            return false;
        // Filtering and formatting the node list
        for(auto& nodeName : jobData.nodes)
            nodeName = MQTTChecker::formatTopic(nodeName) + std::string(1, MQTT_SEP);
        
        // First, we apply the job ID filter, if configured
        if(_jobIdFilterStr!="" && !boost::regex_search(jobData.jobId.c_str(), _match, _jobIdFilter))
            return false;
        
        // No filter was set - every job is accepted
        if(_jobFilterStr=="" || _jobMatchStr=="")
            return true;
        
        // Counting the different matches to the job filter - e.g., different racks, islands, etc.
        std::map<std::string, uint64_t> matchCtr;
        for(const auto& nodeName : jobData.nodes) {
            if(boost::regex_search(nodeName.c_str(), _match, _jobFilter)) {
                ++matchCtr[_match.str(0)];
            }
        }
        
        // Computing the actual mode - the filtered node name acts as a tie breaker
        std::pair<std::string, uint64_t> mode = {"", 0};
        for(const auto& kv : matchCtr) {
            if (kv.second > mode.second || (kv.second == mode.second && kv.first > mode.first)) {
                mode = kv;
            }
        }
        
        // If the mode corresponds to the job match string, the check is successful.
        return mode.first == _jobMatchStr;
    }
    
    /**
    * @brief              Performs a compute task
    *
    *                     This method is tasked with scheduling the next compute task, and invoking the internal
    *                     compute() method, which encapsulates the real logic of the operator. The compute method
    *                     is automatically called over units as required by the operator's configuration.
    *                     
    *                     In the case of job operators, this method will also automatically retrieve the list of jobs
    *                     that were running in the last interval. One unit for each of them is instantiated (or 
    *                     retrieved from the local unit cache, if available) and then the compute phase starts.
    *
    */
    virtual void computeAsync() override {
        try {
            _jobDataVec.clear();
            uint64_t queryTsEnd = !this->_scheduledTime ? getTimestamp() : this->_scheduledTime;
            uint64_t queryTsStart = queryTsEnd - (this->_interval * 1000000);
            if(this->_queryEngine.queryJob("", queryTsStart, queryTsEnd, _jobDataVec, false, true, _jobDomainId)) {
                _tempUnits.clear();
                // Producing units from the job data, discarding invalid jobs in the process
                for(auto& job : _jobDataVec) {
                    try {
                        _tempUnits.push_back(jobDataToUnit(job));
                    } catch(const invalid_argument& e2) { 
                        LOG(debug) << e2.what(); 
                        _tempUnits.push_back(nullptr); 
                        continue; }
                }
                
                // Performing actual computation on each unit
                for(size_t idx=0; idx<_tempUnits.size(); idx++) {
                    if (_tempUnits[idx]) {
                        try {
                            this->compute(_tempUnits[idx], _jobDataVec[idx]);
                        } catch(const exception& e) {
                            LOG(error) << e.what();
                            continue;
                        }
                    }
                }
                // Acquiring the spinlock to refresh the exposed units
                while(_unitAccess.exchange(true)) {}
                this->clearUnits();
                for(const auto& ju : _tempUnits)
                    if(ju)
                        this->addUnit(ju);
                _unitAccess.store(false);
                _tempUnits.clear();
            }
            else
                LOG(debug) << "Operator " + this->_name + ": cannot retrieve job data!";
        } catch(const exception& e) {
            LOG(error) << "Operator " + this->_name + ": internal error " + e.what() + " during computation!";
            _unitAccess.store(false);
        }

        if (this->_timer && this->_keepRunning) {
            this->_scheduledTime = this->nextReadingTime();
            this->_timer->expires_at(timestamp2ptime(this->_scheduledTime));
            this->_pendingTasks++;
            this->_timer->async_wait(bind(&JobOperatorTemplate::computeAsync, this));
        }
        this->_pendingTasks--;
    }
    
    // Vector of recently-modified units
    vector<U_Ptr> _tempUnits;
    // Spinlock used to regulate access to the internal units map, for "visualization" purposes
    atomic<bool> _unitAccess;
    // Vector of job data structures used to retrieve job data at runtime
    vector<qeJobData> _jobDataVec;
    // Regex object used to filter out jobs
    string _jobFilterStr;
    string _jobMatchStr;
    boost::regex _jobFilter;
    boost::cmatch _match;
    // Filters for jobs based on their IDs
    string _jobIdFilterStr;
    boost::regex _jobIdFilter;
    // Job domain ID to be used
    string _jobDomainId;
    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
    

};

#endif //PROJECT_JOBOPERATORTEMPLATE_H
