//================================================================================
// Name        : jobdatastore_internal.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal interface for inserting and querying DCDB job data.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//================================================================================

/*
 * @file
 * @brief This file contains the internal functions of the
 *        Job Data Store which are provided by the
 *        JobDataStoreImpl class.
 */

#ifndef DCDB_JOBDATASTORE_INTERNAL_H
#define DCDB_JOBDATASTORE_INTERNAL_H

#include <list>
#include <unordered_set>
#include <string>

#include "dcdb/jobdatastore.h"
#include "dcdb/connection.h"
#include "dcdb/timestamp.h"

namespace DCDB {

  /**
   * @brief The JobDataStoreImpl class contains all protected
   *        functions belonging to JobDataStore which are
   *        hidden from the user of the libdcdb library.
   */
  class JobDataStoreImpl {
  protected:
    Connection* connection; /**< The Connection object that does the low-level stuff for us. */
    CassSession* session;   /**< The CassSession object given by the connection. */
    const CassPrepared* preparedInsert; /**< The prepared statement for fast insertions. */

    /**
     * @brief Prepare for insertions.
     * @param ttl   A TTL that will be set for newly inserted values. Set to 0 to insert without TTL.
     */
    void prepareInsert(uint64_t ttl);

  public:
    /* See jobdatastore.h for documentation */

    JDError insertJob(JobData& jdata);
    JDError updateJob(JobData& jdata);
    JDError updateEndtime(JobId jobId, TimeStamp startTs, TimeStamp endTime, std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError updateStartTime(JobId jobId, TimeStamp startTs, TimeStamp newStartTs, std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError deleteJob(JobId jid, TimeStamp startTs, std::string domainId=JOB_DEFAULT_DOMAIN);

    JDError getJobByPrimaryKey(JobData& job, JobId jid, TimeStamp startTs, std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getJobById(JobData& job, JobId jid, std::string domainId=JOB_DEFAULT_DOMAIN);
    
    JDError getJobsInIntervalExcl(std::list<JobData>& jobs,
                                  TimeStamp intervalStart,
                                  TimeStamp intervalEnd,
                                  std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getJobsInIntervalIncl(std::list<JobData>& jobs,
                                  TimeStamp intervalStart,
                                  TimeStamp intervalEnd,
                                  std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getJobsInIntervalRunning(std::list<JobData>& jobs,
                                  TimeStamp intervalStart,
                                  TimeStamp intervalEnd,
                                  std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getJobsInIntervalFinished(std::list<JobData>& jobs,
                                  TimeStamp intervalStart,
                                  TimeStamp intervalEnd,
                                  std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getJobsInIntervalPending(std::list<JobData>& jobs,
                                  TimeStamp intervalStart,
                                  TimeStamp intervalEnd,
                                  std::string domainId=JOB_DEFAULT_DOMAIN);
    JDError getNodeList(NodeList& nodes, JobId jid, TimeStamp startTs);

    JobDataStoreImpl(Connection* conn);
    virtual ~JobDataStoreImpl();
  
private:
      
    // Private utility method to avoid code duplication
    JDError parseJobs(CassIterator* rowIt, std::list<JobData>& jobs, std::unordered_set<JobId>* jobIds, uint64_t filterWriteTime=0);
    // Private utility method to run statements (and avoid duplication)
    JDError runStatement(CassStatement* statement, std::list<JobData>& jobs, std::unordered_set<JobId>* jobIds, uint64_t filterWriteTime=0);
  };

} /* End of namespace DCDB */

#endif /* DCDB_JOBDATASTORE_INTERNAL_H */
