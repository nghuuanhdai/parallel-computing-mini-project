//================================================================================
// Name        : jobdatastore.cpp
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for inserting and querying DCDB job data.
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
 * @file
 * @brief This file contains actual implementations of the jobdatastore
 * interface. The interface itself forwards all functions to the internal
 * JobDataStoreImpl. The real logic is implemented in the JobDataStoreImpl.
 */

#include <cinttypes>
#include <list>
#include <string>

#include "cassandra.h"

#include "dcdb/jobdatastore.h"
#include "jobdatastore_internal.h"
#include "dcdb/connection.h"
#include "dcdbglobals.h"

using namespace DCDB;

/**
 * @details
 * Since we want high-performance inserts, we prepare the insert CQL query in
 * advance and only bind it on the actual insert.
 */
void JobDataStoreImpl::prepareInsert(uint64_t ttl) {
  CassError rc = CASS_OK;
  CassFuture* future = NULL;
  const char* query;

  /* Free the old prepared if necessary. */
  if (preparedInsert) {
    cass_prepared_free(preparedInsert);
  }

  char *queryBuf = NULL;
  if (ttl == 0) {
    query = "INSERT INTO " JD_KEYSPACE_NAME "." CF_JOBDATA
        " (domain, jid, uid, start_ts, end_ts, nodes) VALUES (?, ?, ?, ?, ?, ?);";
  }
  else {
    queryBuf = (char*)malloc(256);
    snprintf(queryBuf, 256, "INSERT INTO " JD_KEYSPACE_NAME "." CF_JOBDATA
        " (domain, jid, uid, start_ts, end_ts, nodes) VALUES (?, ?, ?, ?, ?, ?) "
        "USING TTL %" PRIu64 " ;", ttl);
    query = queryBuf;
  }

  future = cass_session_prepare(session, query);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
  } else {
    preparedInsert = cass_future_get_prepared(future);
  }

  cass_future_free(future);
  if (queryBuf) {
    free(queryBuf);
  }
}

/**
 * @details
 * Extract all data from the JobData object and push it into the data store.
 */
JDError JobDataStoreImpl::insertJob(JobData& jdata) {
  JDError error = JD_UNKNOWNERROR;

  /* Insert into Cassandra */
  CassError rc = CASS_OK;
  CassStatement* statement = NULL;
  CassFuture *future = NULL;

  statement = cass_prepared_bind(preparedInsert);

  cass_statement_bind_string_by_name(statement, "domain", jdata.domainId.empty() ? JOB_DEFAULT_DOMAIN : jdata.domainId.c_str());
  cass_statement_bind_string_by_name(statement, "jid", jdata.jobId.c_str());
  cass_statement_bind_string_by_name(statement, "uid", jdata.userId.c_str());
  cass_statement_bind_int64_by_name(statement, "start_ts", jdata.startTime.getRaw());
  cass_statement_bind_int64_by_name(statement, "end_ts", jdata.endTime.getRaw());

  /* Copy the string node list to a varchar set */
  CassCollection* set = cass_collection_new(CASS_COLLECTION_TYPE_SET,
                                            jdata.nodes.size());
  for (auto& s : jdata.nodes) {
    cass_collection_append_string(set, s.c_str());
  }

  cass_statement_bind_collection_by_name(statement, "nodes", set);

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Clean up in the meantime */
  cass_collection_free(set);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;
  }

  cass_future_free(future);
  cass_statement_free(statement);
  return error;
}

/**
 * @details
 * Update the job with matching JobId and StartTs in the data store with the
 * values provided by the given JobData object. If no such job exists in the
 * data store yet, it is inserted.
 * The JobData object is expected to be complete. Partial
 * updates of only selected fields are not supported. Instead, one has to
 * retrieve the other JobData information via getJobById() or
 * getJobByPrimaryKey() first and complete its JobData object for the update.
 */
JDError JobDataStoreImpl::updateJob(JobData& jdata) {
  JDError error = JD_UNKNOWNERROR;

  /* Update entry in Cassandra (actually upserts) */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "UPDATE " JD_KEYSPACE_NAME "." CF_JOBDATA
      " SET uid = ?, end_ts = ?, nodes = ? WHERE domain = ? AND jid = ? AND start_ts = ? ;";

  statement = cass_statement_new(query, 6);

  cass_statement_bind_string(statement, 3, jdata.domainId.c_str());
  cass_statement_bind_string(statement, 4, jdata.jobId.c_str());
  cass_statement_bind_int64(statement, 5, jdata.startTime.getRaw());

  cass_statement_bind_string(statement, 0, jdata.userId.c_str());
  cass_statement_bind_int64(statement, 1, jdata.endTime.getRaw());
  
  /* Copy the string node list to a varchar set */
  CassCollection* set = cass_collection_new(CASS_COLLECTION_TYPE_SET, jdata.nodes.size());
  for (auto& s : jdata.nodes) {
    cass_collection_append_string(set, s.c_str());
  }

  cass_statement_bind_collection(statement, 2, set);

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Clean up in the meantime */
  cass_collection_free(set);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * Update the job with matching JobId and StartTs in the data store with the
 * provided end time. If no such job exists in the data store yet, it is
 * inserted.
 */
JDError JobDataStoreImpl::updateEndtime(JobId jobId, TimeStamp startTs, TimeStamp endTime, std::string domainId) {
  /* Check if the input for the primary key is valid and reasonable */
  if (startTs.getRaw() == 0) {
    return JD_BADPARAMS;
  }

  JDError error = JD_UNKNOWNERROR;

  /* Update entry in Cassandra (actually upserts) */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "UPDATE " JD_KEYSPACE_NAME "." CF_JOBDATA
      " SET end_ts = ? WHERE domain = ? AND jid = ? AND start_ts = ?;";

  statement = cass_statement_new(query, 4);
  
  cass_statement_bind_string(statement, 1, domainId.c_str());
  cass_statement_bind_string(statement, 2, jobId.c_str());
  cass_statement_bind_int64(statement, 3, startTs.getRaw());
  cass_statement_bind_int64(statement, 0, endTime.getRaw());

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * Update the job with matching JobId and StartTs in the data store with the
 * provided new start time. If no such job exists in the data store yet, it is
 * inserted - otherwise, a delete must be performed on the previous entry beforehand.
 */
JDError JobDataStoreImpl::updateStartTime(JobId jobId, TimeStamp startTs, TimeStamp newStartTs, std::string domainId) {
    JDError error = JD_UNKNOWNERROR;
    JobData jd;
    
    getJobByPrimaryKey(jd, jobId, startTs, domainId);
    deleteJob(jobId, startTs, domainId);

    jd.jobId = jobId;
    jd.domainId = domainId;
    jd.startTime = newStartTs;
    
    return insertJob(jd);
}

/**
 * @details
 * Delete the entry with matching JobId and start TimeStamp from the data store.
 */
JDError JobDataStoreImpl::deleteJob(JobId jid, TimeStamp startTs, std::string domainId)  {
  JDError error = JD_UNKNOWNERROR;

  /* Remove entry from Cassandra */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "DELETE FROM " JD_KEYSPACE_NAME "." CF_JOBDATA
      " WHERE domain = ? AND jid = ? AND start_ts = ?;";

  statement = cass_statement_new(query, 3);

  cass_statement_bind_string(statement, 0, domainId.c_str());
  cass_statement_bind_string(statement, 1, jid.c_str());
  cass_statement_bind_int64(statement, 2, startTs.getRaw());

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * Find the entry in the data store with matching JobId and start_ts and store
 * the corresponding values in the JobData object.
 */
JDError JobDataStoreImpl::getJobByPrimaryKey(JobData& job, JobId jid, TimeStamp startTs, std::string domainId) {
  JDError error = JD_UNKNOWNERROR;

  /* Select entry from Cassandra */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATA
      " WHERE domain = ? AND jid = ? AND start_ts = ?;";

  statement = cass_statement_new(query, 3);

  cass_statement_bind_string(statement, 0, domainId.c_str());
  cass_statement_bind_string(statement, 1, jid.c_str());
  cass_statement_bind_int64(statement, 2, startTs.getRaw());

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;

    const CassResult* cresult = cass_future_get_result(future);
    size_t rowCnt = cass_result_row_count(cresult);

    /* Check if the returned data is reasonable */
    if (rowCnt == 0) {
      error = JD_JOBKEYNOTFOUND;
    } else {
      /* Retrieve data from result */
      const CassRow* row = cass_result_first_row(cresult);

      cass_int64_t startTs, endTs;
      const char *domId, *jobId, *userId;
      size_t domainId_len, jobId_len, userId_len;
      /* domain, jid and start_ts are always set. Other values should be checked */
      cass_value_get_string(cass_row_get_column_by_name(row, "domain"), &domId, &domainId_len);
      cass_value_get_string(cass_row_get_column_by_name(row, "jid"), &jobId, &jobId_len);
      cass_value_get_int64(cass_row_get_column_by_name(row, "start_ts"), &startTs);
      if (cass_value_get_string(cass_row_get_column_by_name(row, "uid"), &userId, &userId_len) != CASS_OK) {
        userId = "";
        userId_len = 0;
        error = JD_PARSINGERROR;
      }
      if (cass_value_get_int64(cass_row_get_column_by_name(row, "end_ts"), &endTs) != CASS_OK) {
        endTs = 0;
        error = JD_PARSINGERROR;
      }

      /* Copy the data in the JobData object */
      job.domainId = (DomainId) std::string(domId, domainId_len);
      job.jobId = (JobId) std::string(jobId, jobId_len);
      job.userId = (UserId) std::string(userId, userId_len);
      job.startTime = (uint64_t) startTs;
      job.endTime = (uint64_t) endTs;

      /* Do not forget about the nodes... */
      const char* nodeStr;
      size_t nodeStr_len;

      const CassValue* set = cass_row_get_column_by_name(row, "nodes");
      CassIterator *setIt = nullptr;
      
      if(set && (setIt = cass_iterator_from_collection(set))) {
        while (cass_iterator_next(setIt)) {
            cass_value_get_string(cass_iterator_get_value(setIt), &nodeStr, &nodeStr_len);
            job.nodes.emplace_back(nodeStr, nodeStr_len);
        }
    
        cass_iterator_free(setIt);
      } else { 
        error = JD_PARSINGERROR;
      }
    }

    cass_result_free(cresult);
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * Find the entry in the data store with matching JobId and highest start_ts
 * value (= most recent job) and store the
 * corresponding values in the JobData object.
 */
JDError JobDataStoreImpl::getJobById(JobData& job, JobId jid, std::string domainId) {
  JDError error = JD_UNKNOWNERROR;

  /* Select entry from Cassandra */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATA
      " WHERE domain = ? AND jid = ? ORDER BY jid DESC, start_ts DESC LIMIT 1;";

  statement = cass_statement_new(query, 2);

  cass_statement_bind_string(statement, 0, domainId.c_str());
  cass_statement_bind_string(statement, 1, jid.c_str());

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;

    const CassResult* cresult = cass_future_get_result(future);
    size_t rowCnt = cass_result_row_count(cresult);

    /* Check if the returned data is reasonable */
    if (rowCnt == 0) {
      error = JD_JOBIDNOTFOUND;
    } else {
      /* Retrieve data from result */
      const CassRow* row = cass_result_first_row(cresult);
      
      cass_int64_t startTs, endTs;
      const char *domId, *jobId, *userId;
      size_t domainId_len, jobId_len, userId_len;

      /* domain, jid and start_ts are always set. Other values should be checked */
      cass_value_get_string(cass_row_get_column_by_name(row, "domain"), &domId, &domainId_len);
      cass_value_get_string(cass_row_get_column_by_name(row, "jid"), &jobId, &jobId_len);
      cass_value_get_int64(cass_row_get_column_by_name(row, "start_ts"), &startTs);
      if (cass_value_get_string(cass_row_get_column_by_name(row, "uid"), &userId, &userId_len) != CASS_OK) {
        userId = "";
        userId_len = 0;
        error = JD_PARSINGERROR;
      }
      if (cass_value_get_int64(cass_row_get_column_by_name(row, "end_ts"), &endTs) != CASS_OK) {
        endTs = 0;
        error = JD_PARSINGERROR;
      }

      /* Copy the data in the JobData object */
      job.domainId = (DomainId) std::string(domId, domainId_len);
      job.jobId = (JobId) std::string(jobId, jobId_len);
      job.userId = (UserId) std::string(userId, userId_len);
      job.startTime = (uint64_t) startTs;
      job.endTime = (uint64_t) endTs;

      /* Do not forget about the nodes... */
      const char* nodeStr;
      size_t nodeStr_len;

      const CassValue* set = cass_row_get_column_by_name(row, "nodes");
      CassIterator *setIt = nullptr;
      
      if(set && (setIt = cass_iterator_from_collection(set))) {
        while (cass_iterator_next(setIt)) {
            cass_value_get_string(cass_iterator_get_value(setIt),
                                  &nodeStr, &nodeStr_len);
            job.nodes.emplace_back(nodeStr, nodeStr_len);
        }
    
        cass_iterator_free(setIt);
      } else { 
        error = JD_PARSINGERROR;
      }
    }

    cass_result_free(cresult);
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * Find all entries in the data store whose start_ts AND end_ts lay within
 * the specified interval. Store the found entries in the JobData list.
 */
JDError JobDataStoreImpl::getJobsInIntervalExcl(std::list<JobData>& jobs,
                                                TimeStamp intervalStart,
                                                TimeStamp intervalEnd,
                                                std::string domainId) {
  /* Check if the input is valid and reasonable */
  if (intervalEnd.getRaw() == 0) {
    return JD_BADPARAMS;
  }
  if (intervalStart >= intervalEnd) {
    return JD_BADPARAMS;
  }

  JDError error = JD_UNKNOWNERROR;
  CassStatement* statement = nullptr;

  /* Select entries from Cassandra */
  const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
      " WHERE domain = ? AND end_ts <= ? AND start_ts >= ? ALLOW FILTERING;";

  statement = cass_statement_new(query, 3);
  cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

  cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, intervalEnd.getRaw());
  cass_statement_bind_int64(statement, 2, intervalStart.getRaw());
  error = runStatement(statement, jobs, NULL);
  cass_statement_free(statement);
  
  return error;
}

/**
 * @details
 * Find all entries in the data store whose start_ts OR end_ts lays within
 * the specified interval. Store the found entries in the JobData list.
 * Cassandra only supports AND conditions in its query language. Therefore
 * we cannot SELECT directly the required jobs. Instead we have to do two
 * selects and manually deduplicate the results.
 */
JDError JobDataStoreImpl::getJobsInIntervalIncl(std::list<JobData>& jobs,
                                                TimeStamp intervalStart,
                                                TimeStamp intervalEnd,
                                                std::string domainId) {
  /* Check if the input is valid and reasonable */
  if (intervalEnd.getRaw() == 0) {
    return JD_BADPARAMS;
  }
  if (intervalStart >= intervalEnd) {
    return JD_BADPARAMS;
  }

  JDError error = JD_UNKNOWNERROR;
  JDError error2 = JD_UNKNOWNERROR;
  std::unordered_set<JobId> jobIds;
  CassStatement* statement = nullptr;
  
  /* +++ First SELECT +++ */
  /* Select entries from Cassandra where start_ts lays within the interval */
  const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
      " WHERE domain = ? AND start_ts >= ? AND start_ts <= ? ALLOW FILTERING;";

  statement = cass_statement_new(query, 3);
  cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

  cass_statement_bind_string(statement, 0, domainId.c_str());
  cass_statement_bind_int64(statement, 1, intervalStart.getRaw());
  cass_statement_bind_int64(statement, 2, intervalEnd.getRaw());
  error = runStatement(statement, jobs, &jobIds);
  cass_statement_free(statement);
  
  /* +++ Second SELECT +++ */
  /* Select entries from Cassandra where end_ts lays within the interval */
  query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
      " WHERE domain = ? AND end_ts >= ? AND end_ts <= ? ALLOW FILTERING;";

  statement = cass_statement_new(query, 3);
  cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

  cass_statement_bind_string(statement, 0, domainId.c_str());
  cass_statement_bind_int64(statement, 1, intervalStart.getRaw());
  cass_statement_bind_int64(statement, 2, intervalEnd.getRaw());
  error2 = runStatement(statement, jobs, &jobIds);
  cass_statement_free(statement);
  
  if(error2 != JD_OK)
      error = error2;
  
  return error;
}

/**
 * @details
 * Find all entries in the data store corresponding to jobs that were running 
 * in the queried time interval, i.e., their start time is less than the queried 
 * intervalEnd, and their end time is 0 or greater than startInterval.
 */
JDError JobDataStoreImpl::getJobsInIntervalRunning(std::list<JobData>& jobs,
                                                   TimeStamp intervalStart,
                                                   TimeStamp intervalEnd,
                                                   std::string domainId) {
    /* Check if the input is valid and reasonable */
    if (intervalEnd.getRaw() == 0) {
        return JD_BADPARAMS;
    }
    if (intervalStart >= intervalEnd) {
        return JD_BADPARAMS;
    }

    JDError error = JD_UNKNOWNERROR;
    JDError error2 = JD_UNKNOWNERROR;
    std::unordered_set<JobId> jobIds;
    CassStatement* statement = nullptr;

    /* +++ First SELECT +++ */
    /* Select entries from Cassandra where start_ts lays within the interval */
    const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
    " WHERE domain = ? AND end_ts = ? AND start_ts < ? AND start_ts > ?;";

    statement = cass_statement_new(query, 4);
    cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

    cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, (int64_t)0);
    cass_statement_bind_int64(statement, 2, intervalEnd.getRaw());
    cass_statement_bind_int64(statement, 3, (int64_t)0);
    error = runStatement(statement, jobs, &jobIds);
    cass_statement_free(statement);

    /* +++ Second SELECT +++ */
    /* Select entries from Cassandra where end_ts lays within the interval */
    query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
    " WHERE domain = ? AND end_ts > ? AND start_ts < ? AND start_ts > ? ALLOW FILTERING;";

    statement = cass_statement_new(query, 4);
    cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

    cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, intervalStart.getRaw());
    cass_statement_bind_int64(statement, 2, intervalEnd.getRaw());
    cass_statement_bind_int64(statement, 3, (int64_t)0);
    error2 = runStatement(statement, jobs, &jobIds);
    cass_statement_free(statement);

    if(error2 != JD_OK)
        error = error2;

    return error;
}

/**
 * @details
 * Find all entries in the data store corresponding to jobs that have terminated 
 * in the queried time interval, i.e., their end time is within the queried interval.
 */
JDError JobDataStoreImpl::getJobsInIntervalFinished(std::list<JobData>& jobs,
                                                   TimeStamp intervalStart,
                                                   TimeStamp intervalEnd,
                                                   std::string domainId) {
    /* Check if the input is valid and reasonable */
    if (intervalEnd.getRaw() == 0) {
        return JD_BADPARAMS;
    }
    if (intervalStart >= intervalEnd) {
        return JD_BADPARAMS;
    }
    
    JDError error = JD_UNKNOWNERROR;
    CassStatement* statement = nullptr;

    /* Select entries from Cassandra where end_ts lays within the interval */
    const char* query = "SELECT * FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
    " WHERE domain = ? AND end_ts > ? AND end_ts < ? AND start_ts > ? ALLOW FILTERING;";

    statement = cass_statement_new(query, 4);
    cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

    cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, intervalStart.getRaw());
    cass_statement_bind_int64(statement, 2, intervalEnd.getRaw());
    cass_statement_bind_int64(statement, 3, (int64_t)0);
    error = runStatement(statement, jobs, NULL);
    cass_statement_free(statement);
    
    return error;
}

/**
 * @details
 * Find all entries in the data store corresponding to jobs that are in pending state
 * in the queried time interval, i.e., their start time is 0 or higher than the end of the interval.
 */
JDError JobDataStoreImpl::getJobsInIntervalPending(std::list<JobData>& jobs,
                                                    TimeStamp intervalStart,
                                                    TimeStamp intervalEnd,
                                                    std::string domainId) {
    /* Check if the input is valid and reasonable */
    if (intervalEnd.getRaw() == 0) {
        return JD_BADPARAMS;
    }
    if (intervalStart >= intervalEnd) {
        return JD_BADPARAMS;
    }

    JDError error = JD_UNKNOWNERROR;
    JDError error2 = JD_UNKNOWNERROR;
    std::unordered_set<JobId> jobIds;
    CassStatement* statement = nullptr;
    
    // First query: we pick the jobs whose start time is greater than the interval's start (and that were inserted
    // into the table before the interval's end)
    const char* query = "SELECT domain,writetime(uid),jid,start_ts,end_ts,nodes,uid FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
    " WHERE domain = ? AND start_ts > ? ALLOW FILTERING;";

    statement = cass_statement_new(query, 2);
    cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

    cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, intervalStart.getRaw());
    error = runStatement(statement, jobs, &jobIds, intervalEnd.getRaw());
    cass_statement_free(statement);

    // Second query: we pick the jobs whose start time is undefined (and that were inserted
    // into the table before the interval's end)
    query = "SELECT domain,writetime(uid),jid,start_ts,end_ts,nodes,uid FROM " JD_KEYSPACE_NAME "." CF_JOBDATAVIEW
    " WHERE domain = ? AND start_ts = ? ALLOW FILTERING;";

    statement = cass_statement_new(query, 2);
    cass_statement_set_paging_size(statement, JOB_PAGING_SIZE);

    cass_statement_bind_string(statement, 0, domainId.c_str());
    cass_statement_bind_int64(statement, 1, (int64_t)0);
    error2 = runStatement(statement, jobs, &jobIds, intervalEnd.getRaw());
    cass_statement_free(statement);

    if(error2 != JD_OK)
        error = error2;
    
    return error;
}

JDError JobDataStoreImpl::runStatement(CassStatement* statement, std::list<JobData>& jobs, std::unordered_set<JobId>* jobIds, uint64_t filterWriteTime) {
    JDError error = JD_UNKNOWNERROR;
    CassError rc = CASS_OK;
    CassFuture* future = nullptr;
    
    if(!statement)
        return error;
    
    bool morePages = false;
    do {
        /* All parameters bound. Now execute the statement asynchronously */
        future = cass_session_execute(session, statement);

        /* Wait for the statement to finish */
        cass_future_wait(future);

        rc = cass_future_error_code(future);
        if (rc != CASS_OK) {
            connection->printError(future);
            error = JD_UNKNOWNERROR;
            morePages = false;
        } else {
            error = JD_OK;
            
            /* Retrieve data from result */
            const CassResult* cresult = cass_future_get_result(future);
            CassIterator* rowIt = cass_iterator_from_result(cresult);
            
            error = parseJobs(rowIt, jobs, jobIds, filterWriteTime);
            
            if((morePages = cass_result_has_more_pages(cresult)))
                cass_statement_set_paging_state(statement, cresult);

            cass_iterator_free(rowIt);
            cass_result_free(cresult);
        }

        cass_future_free(future);

    }
    while(morePages);

    return error;
}

JDError JobDataStoreImpl::parseJobs(CassIterator* rowIt, std::list<JobData>& jobs, std::unordered_set<JobId>* jobIds, uint64_t filterWriteTime) {
    JDError error = JD_OK;
    cass_int64_t startTs, endTs;
    cass_int64_t writeTs;
    const char *domId, *jobId, *userId;
    size_t domainId_len, jobId_len, userId_len;
    JobData job;
    while (cass_iterator_next(rowIt)) {
        const CassRow *row = cass_iterator_get_row(rowIt);
        
        if(filterWriteTime>0) {
            // Skipping a job if it does not respect the write time filter
            if(cass_value_get_int64(cass_row_get_column_by_name(row, "writetime(uid)"), &writeTs)!=CASS_OK || (uint64_t)writeTs*1000>filterWriteTime) {
                continue;
            }
        }
        
        /* domain, jid and start_ts should always be set. Other values should be checked */
        cass_value_get_string(cass_row_get_column_by_name(row, "domain"), &domId, &domainId_len);
        cass_value_get_string(cass_row_get_column_by_name(row, "jid"), &jobId, &jobId_len);
        cass_value_get_int64(cass_row_get_column_by_name(row, "start_ts"), &startTs);
        
        if (cass_value_get_string(cass_row_get_column_by_name(row, "uid"), &userId, &userId_len) != CASS_OK) {
            userId = "";
            userId_len = 0;
            error = JD_PARSINGERROR;
        }
        if (cass_value_get_int64(cass_row_get_column_by_name(row, "end_ts"), &endTs) != CASS_OK) {
            endTs = 0;
            error = JD_PARSINGERROR;
        }

        /* Copy the data into job object */
        job.jobId = (JobId) std::string(jobId, jobId_len);
        /* Set-based deduplication */
        if (jobIds==nullptr || jobIds->insert(job.jobId).second) {
            job.domainId = (DomainId) std::string(domId, domainId_len);
            job.userId = (UserId) std::string(userId, userId_len);
            job.startTime = (uint64_t) startTs;
            job.endTime = (uint64_t) endTs;

            /* Do not forget about the nodes... */
            const char *nodeStr;
            size_t nodeStr_len;

            const CassValue *set = cass_row_get_column_by_name(row, "nodes");
            CassIterator *setIt = nullptr;
            
            if(set && (setIt = cass_iterator_from_collection(set))) {
                while (cass_iterator_next(setIt)) {
                    cass_value_get_string(cass_iterator_get_value(setIt), &nodeStr, &nodeStr_len);
                    job.nodes.emplace_back(nodeStr, nodeStr_len);
                }
                
                cass_iterator_free(setIt);
            } else {
                error = JD_PARSINGERROR;
            }
            
            //TODO job.nodes list deep copied?
            jobs.push_back(job);
            job.nodes.clear();
        }
    }
    return error;
}

/**
 * @details
 * Find the entry in the data store with matching JobId and highest start_ts
 * value (= most recent job) and store the
 * corresponding nodes in the NodeList.
 */
JDError JobDataStoreImpl::getNodeList(NodeList& nodes, JobId jid, TimeStamp startTs) {
  JDError error = JD_UNKNOWNERROR;

  /* Select entry from Cassandra */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "SELECT nodes FROM " JD_KEYSPACE_NAME "." CF_JOBDATA
      " WHERE jid = ? ORDER BY start_ts LIMIT 1;";

  statement = cass_statement_new(query, 1);

  cass_statement_bind_string(statement, 0, jid.c_str());

  /* All parameters bound. Now execute the statement asynchronously */
  future = cass_session_execute(session, statement);

  /* Wait for the statement to finish */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    error = JD_UNKNOWNERROR;
  } else {
    error = JD_OK;

    const CassResult* cresult = cass_future_get_result(future);
    size_t rowCnt = cass_result_row_count(cresult);

    /* Check if the returned data is reasonable */
    if (rowCnt == 0) {
      error = JD_JOBIDNOTFOUND;
    } else {
      /* Retrieve data from result */
      const CassRow* row = cass_result_first_row(cresult);

      /* Copy the nodes in the NodeList */
      const char* nodeStr;
      size_t nodeStr_len;

      const CassValue* set = cass_row_get_column_by_name(row, "nodes");
      CassIterator *setIt = nullptr;
      
      if(set && (setIt = cass_iterator_from_collection(set))) {
          while (cass_iterator_next(setIt)) {
              cass_value_get_string(cass_iterator_get_value(setIt), &nodeStr, &nodeStr_len);
              nodes.emplace_back(nodeStr, nodeStr_len);
          }

          cass_iterator_free(setIt);
      } else {
          error = JD_PARSINGERROR;
      }
    }

    cass_result_free(cresult);
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return error;
}

/**
 * @details
 * This constructor sets the internal connection variable to
 * the externally provided Connection object and also
 * retrieves the CassSession pointer of the connection.
 */
JobDataStoreImpl::JobDataStoreImpl(Connection* conn) {
  connection = conn;
  session = connection->getSessionHandle();

  preparedInsert = nullptr;
  prepareInsert(0);
}

/**
 * @details
 * The destructor just resets the internal pointers. Deletion of the pointers
 * (except preparedInsert) is not our responsibility.
 */
JobDataStoreImpl::~JobDataStoreImpl() {
  connection = nullptr;
  session = nullptr;
  if (preparedInsert) {
      cass_prepared_free(preparedInsert);
  }
}

/* ########################################################################## */

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::insertJob(JobData& jdata) {
  return impl->insertJob(jdata);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::updateJob(JobData& jdata) {
  return impl->updateJob(jdata);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::updateEndtime(JobId jobId, TimeStamp startTs, TimeStamp endTime, std::string domainId) {
  return impl->updateEndtime(jobId, startTs, endTime, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::updateStartTime(JobId jobId, TimeStamp startTs, TimeStamp newStartTs, std::string domainId) {
    return impl->updateStartTime(jobId, startTs, newStartTs, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::deleteJob(JobId jid, TimeStamp startTs, std::string domainId) {
  return impl->deleteJob(jid, startTs, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobByPrimaryKey(JobData& job, JobId jid, TimeStamp startTs, std::string domainId) {
  return impl->getJobByPrimaryKey(job, jid, startTs, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobById(JobData& job, JobId jid, std::string domainId) {
  return impl->getJobById(job, jid, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobsInIntervalExcl(std::list<JobData>& jobs,
                                            TimeStamp intervalStart,
                                            TimeStamp intervalEnd,
                                            std::string domainId) {
  return impl->getJobsInIntervalExcl(jobs, intervalStart, intervalEnd, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobsInIntervalIncl(std::list<JobData>& jobs,
                                            TimeStamp intervalStart,
                                            TimeStamp intervalEnd,
                                            std::string domainId) {
  return impl->getJobsInIntervalIncl(jobs, intervalStart, intervalEnd, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobsInIntervalRunning(std::list<JobData>& jobs,
                                               TimeStamp intervalStart,
                                               TimeStamp intervalEnd,
                                               std::string domainId) {
    return impl->getJobsInIntervalRunning(jobs, intervalStart, intervalEnd, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobsInIntervalFinished(std::list<JobData>& jobs,
                                               TimeStamp intervalStart,
                                               TimeStamp intervalEnd,
                                               std::string domainId) {
    return impl->getJobsInIntervalFinished(jobs, intervalStart, intervalEnd, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getJobsInIntervalPending(std::list<JobData>& jobs,
                                               TimeStamp intervalStart,
                                               TimeStamp intervalEnd,
                                               std::string domainId) {
    return impl->getJobsInIntervalPending(jobs, intervalStart, intervalEnd, domainId);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the JobDataStoreImpl class.
 */
JDError JobDataStore::getNodeList(NodeList& nodes, JobId jid,
                                  TimeStamp startTs) {
  return impl->getNodeList(nodes, jid, startTs);
}

/**
 * @details
 * This constructor allocates the implementation class which
 * holds the actual implementation of the class functionality.
 */
JobDataStore::JobDataStore(Connection* conn) {
  impl = new JobDataStoreImpl(conn);
}

/**
 * @details
 * The JobDataStore destructor deallocates the
 * JobDataStoreImpl and CassandraBackend objects.
 */
JobDataStore::~JobDataStore() {
  /* Clean up... */
  if (impl) {
    delete impl;
  }
}
