//================================================================================
// Name        : jobdatastore.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for inserting and querying DCDB job data.
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


/**
 * @file jobdatastore.h
 * @brief This file contains parts of the public API for the libdcdb library.
 * It contains the class definition of the JobDataStore class, that handles
 * database operations for job data.
 *
 * @ingroup libdcdb
 */

#ifndef DCDB_JOBDATASTORE_H
#define DCDB_JOBDATASTORE_H

#include <cstdint>
#include <list>
#include <string>

#include "cassandra.h"
#include "connection.h"
#include "timestamp.h"

// Default domain ID for the jobs if none is specified
#define JOB_DEFAULT_DOMAIN "default"

namespace DCDB {

  /* Forward-declaration of the implementation-internal classes */
  class JobDataStoreImpl;

  using DomainId = std::string;
  using JobId = std::string;
  using UserId = std::string;
  using NodeList = std::list<std::string>;

  /**
   * @brief This struct is a container for the information DCDB keeps about
   * SLURM jobs. Both jobId and startTime are required to uniquely identify a
   * job.
   */
  struct JobData {
    DomainId domainId;  /**< Domain of the job (e.g., system, partition) **/
    JobId jobId;        /**< SLURM job id of the job. */
    UserId userId;      /**< Id of the user who submitted the job. */
    TimeStamp startTime;/**< Time when the job started (started != submitted)
                             in ns since Unix epoch. */
    TimeStamp endTime;  /**< Time when the job finished in ns since Unix
                             epoch. */
    NodeList nodes;     /**< List of nodes the job occupied. */
    /* extend as required */
  };

  typedef enum {
    JD_OK,            /**< Everything went fine. */
    JD_JOBKEYNOTFOUND,/**< Not job with matching primary key was found */
    JD_JOBIDNOTFOUND, /**< The given JobId was not found in the data store. */
    JD_BADPARAMS,     /**< The provided parameters are ill-formed. Either
                           because they are erroneous or incomplete. */
    JD_PARSINGERROR,  /**< Data retrieved from the data store could not be
                           parsed and a default value was returned instead.
                           Use results with care and on own risk. */
    JD_UNKNOWNERROR   /**< An unknown error occurred. */
  } JDError;

  /**
   * @brief %JobDataStore is the class of the libdcdb library
   * to write and read job data.
   */
  class JobDataStore {
    private:
      JobDataStoreImpl* impl;

    public:
      
      /**
       * @brief This function inserts a single job into the database.
       *
       * @param jdata   Reference to a JobData object filled with all the
       *                information about the job. At least jobId and startTime
       *                have to be filled in as they form the primary key.
       *                Other JobData values may be left out and can be updated
       *                later with updateJob().
       * @return        See JDError
       */
      JDError insertJob(JobData& jdata);

      /**
       * @brief Update a job.
       *
       * @details Updates the job in the database whose primary key matches
       *          jdata. If no entry is found a new one is created (upsert).
       *          Updates all values of the JobData struct.
       *
       * @param jdata   Reference to a JobData object filled with all the
       *                information about the job.
       * @return        See JDError.
       */
      JDError updateJob(JobData& jdata);

      /**
       * @brief Update the end time of the job with matching primary key.
       *
       * @param jobId       JobId of the job to be updated. Makes up the
       *                    primary key together with startTime.
       * @param startTime   Start time of the job. Part of the primary key.
       * @param endTime     New endTime to be inserted.
       * @param domainId    Domain ID of the job (optional).
       *
       * return             See JDError
       */
      JDError updateEndtime(JobId jobId, TimeStamp startTs, TimeStamp endTime, std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Update the start time of the job with matching primary key.
       * 
       * @details           Since the start time is part of the primary key, the job has 
       *                    to be deleted and then re-inserted into the table again.
       *
       * @param jobId       JobId of the job to be updated. Makes up the
       *                    primary key together with startTime.
       * @param startTime   Start time of the job. Part of the primary key.
       * @param newStartTs  New start time to be inserted.
       * @param domainId    Domain ID of the job (optional).
       *
       * return             See JDError
       */
      JDError updateStartTime(JobId jobId, TimeStamp startTs, TimeStamp newStartTs, std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Deletes a job from the job data list.
       *
       * @param jid         JobId. Makes up the primary key together with startTs.
       * @param startTs     Start timestamp of the job. Part of the primary key.
       * @param domainId    Domain ID of the job (optional).
       * @return            See JDError.
       */
      JDError deleteJob(JobId jid, TimeStamp startTs, std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve a job by its primary key.
       *
       * @param job         Reference to a JobData object that will be populated
       *                    with the job data.
       * @param jid         Id of the job to be retrieved. Makes up the primary key
       *                    together with startTs.
       * @param startTs     Start time of the job. Part of the primary key.
       * @param domainId    Domain ID of the job (optional).
       * @return            See JDError.
       */
      JDError getJobByPrimaryKey(JobData& job, JobId jid, TimeStamp startTs, std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve the most recent job with jid.
       *
       * @param job         Reference to a JobData object that will be populated with
       *                    the job data.
       * @param jid         Id of the job whose information should be retrieved. If
       *                    multiple jobs with the same jid are present the most
       *                    recent one is returned.
       * @param domainId    Domain ID of the job (optional).
       * @return            See JDError.
       */
      JDError getJobById(JobData& job, JobId jid, std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve an exclusive list of jobs which were run in the given
       *        time interval.
       *
       * @details EXCLUSIVE version; only jobs whose start AND end time lay
       *          within the interval are returned. See also
       *          getJobsInIntervalIncl().
       *
       * @param jobs            Reference to a list of JobData that will be
       *                        populated with the jobs.
       * @param intervalStart   Start time of the interval.
       * @param intervalEnd     End time of the interval.
       * @param domainId    Domain ID of the job (optional).
       * @return                See JDError.
       */
      JDError getJobsInIntervalExcl(std::list<JobData>& jobs,
                                    TimeStamp intervalStart,
                                    TimeStamp intervalEnd,
                                    std::string domainId=JOB_DEFAULT_DOMAIN);
      
      
      /**
       * @brief Retrieve an inclusive list of jobs which were run in the given
       *        time interval.
       *
       * @details INCLUSIVE version; all jobs whose start OR end time lays
       *          within the interval are returned. See also
       *          getJobsInIntervalExcl().
       *
       * @param jobs            Reference to a list of JobData that will be
       *                        populated with the jobs.
       * @param intervalStart   Start time of the interval.
       * @param intervalEnd     End time of the interval.
       * @param domainId    Domain ID of the job (optional).
       * @return                See JDError.
       */
      JDError getJobsInIntervalIncl(std::list<JobData>& jobs,
                                    TimeStamp intervalStart,
                                    TimeStamp intervalEnd,
                                    std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve a list of jobs which were run in the given time interval.
       *
       * @details Find all entries in the data store corresponding to jobs that were running in 
       *        the queried time interval, i.e., their start time is less than the queried intervalEnd, 
       *        and their end time is 0 or greater than startInterval.
       *
       * @param jobs            Reference to a list of JobData that will be
       *                        populated with the jobs.
       * @param intervalStart   Start time of the interval.
       * @param intervalEnd     End time of the interval.
       * @param domainId    Domain ID of the job (optional).
       * @return                See JDError.
       */
      JDError getJobsInIntervalRunning(std::list<JobData>& jobs,
                                       TimeStamp intervalStart,
                                       TimeStamp intervalEnd,
                                       std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve a list of jobs which terminated in the given time interval.
       *
       * @details Find all entries in the data store corresponding to jobs that terminated in 
       *        the queried time interval, i.e., their end time is within the queried interval.
       *
       * @param jobs            Reference to a list of JobData that will be
       *                        populated with the jobs.
       * @param intervalStart   Start time of the interval.
       * @param intervalEnd     End time of the interval.
       * @param domainId    Domain ID of the job (optional).
       * @return                See JDError.
       */
      JDError getJobsInIntervalFinished(std::list<JobData>& jobs,
                                        TimeStamp intervalStart,
                                        TimeStamp intervalEnd,
                                        std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve an inclusive list of jobs which were in pending state in the given
       *        time interval.
       *
       * @details Find all entries in the data store corresponding to jobs that were in pending state in 
       *        the queried time interval, i.e., their start time is greater than the queried intervalStart, 
       *        and the writing time of the corresponding record in the DB is lower than intervalEnd.
       *
       * @param jobs            Reference to a list of JobData that will be
       *                        populated with the jobs.
       * @param intervalStart   Start time of the interval.
       * @param intervalEnd     End time of the interval.
       * @param domainId    Domain ID of the job (optional).
       * @return                See JDError.
       */
      JDError getJobsInIntervalPending(std::list<JobData>& jobs,
                                       TimeStamp intervalStart,
                                       TimeStamp intervalEnd,
                                       std::string domainId=JOB_DEFAULT_DOMAIN);

      /**
       * @brief Retrieve the list of nodes which were used by a job.
       *
       * @param nodes   Reference to a NodeList which will be populated with
       *                the nodes.
       * @param jid     Id of the job whose nodes should be retrieved.
       * @param startTs Start timestamp of the job to make up the full primary
       *                key.
       * @return        See JDError.
       */
      JDError getNodeList(NodeList& nodes, JobId jid, TimeStamp startTs);

      /**
       * @brief A shortcut constructor for a JobDataStore object that allows
       *        accessing the data store through a connection that is already
       *        established.
       * @param conn     The Connection object of an established connection to
       *                 Cassandra.
       */
      JobDataStore(Connection* conn);

      /**
       * @brief The standard destructor for a JobDatStore object.
       */
      virtual ~JobDataStore();
  };

} /* End of namespace DCDB */

#endif /* DCDB_JOBDATASTORE_H */
