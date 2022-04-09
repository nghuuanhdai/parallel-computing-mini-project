//================================================================================
// Name        : c_api.cpp
// Author      : Axel Auweter, Daniele Tafani, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C API Implementation for libdcdb
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

#include <cstdio>
#include <iostream>
#include <list>

#include <pthread.h>

#include "dcdb/c_api.h"

#include "dcdb/sensorid.h"
#include "dcdb/timestamp.h"
#include "dcdb/connection.h"
#include "dcdb/sensorconfig.h"
#include "dcdb/sensordatastore.h"
#include "dcdb/jobdatastore.h"

using namespace DCDB;

Connection* connectToDatabase(const char* hostname, uint16_t port) {
  Connection* conn = new Connection(hostname, port);

  if (!conn->connect()) {
    delete conn;
    return NULL;
  }

  //dcdbConn->initSchema();

  return conn;
}

DCDB_C_RESULT disconnectFromDatabase(Connection* conn) {
  if (conn) {
    conn->disconnect();
    delete conn;
  }
  return DCDB_C_OK;
}

JobDataStore* constructJobDataStore(Connection* conn) {
  if (conn) {
    return new JobDataStore(conn);
  }
  return NULL;
}

DCDB_C_RESULT insertJobStart(JobDataStore* jds, JobId jid, UserId uid,
                             uint64_t startTs, const char ** nodes,
                             unsigned nodeSize) {
  if (!jds) {
    return DCDB_C_CONNERR;
  }

  JobData jdata;
  JDError ret;

  jdata.jobId = jid;
  jdata.userId = uid;
  jdata.startTime = startTs;
  jdata.endTime = (uint64_t) 0;
  for(unsigned i = 0; i < nodeSize; i++) {
    jdata.nodes.push_back(nodes[i]);
  }

  ret = jds->insertJob(jdata);

  if (ret == JD_OK) {
    return DCDB_C_OK;
  } else if (ret == JD_BADPARAMS) {
    return DCDB_C_BADPARAMS;
  }
  return DCDB_C_UNKNOWN;
}

DCDB_C_RESULT updateJobEnd(JobDataStore* jds, JobId jid, uint64_t endTs) {
  if (!jds) {
    return DCDB_C_CONNERR;
  }

  JobData jdata;
  JDError ret;

  ret = jds->getJobById(jdata, jid);

  if (ret == JD_UNKNOWNERROR || ret == JD_PARSINGERROR) {
    return DCDB_C_UNKNOWN;
  } else if (ret == JD_JOBIDNOTFOUND) {
    return DCDB_C_BADPARAMS;
  } else if (ret != JD_OK) {
    return DCDB_C_UNKNOWN;
  }

  if (jds->updateEndtime(jid, jdata.startTime, endTs) != JD_OK) {
    return DCDB_C_UNKNOWN;
  }

  return DCDB_C_OK;
}

DCDB_C_RESULT printJob(JobDataStore* jds, JobId jid) {
  if (!jds) {
    return DCDB_C_CONNERR;
  }

  JobData jdata;
  JDError ret;

  ret = jds->getJobById(jdata, jid);

  switch (ret) {
    case JD_OK:
      std::cout << "Successfully retrieved job:" << std::endl;
      std::cout << "  JobId:     " << jdata.jobId << std::endl;
      std::cout << "  UserId:    " << jdata.userId << std::endl;
      std::cout << "  StartTime: " << jdata.startTime.getString() << std::endl;
      std::cout << "  EndTime:   " << jdata.endTime.getString() << std::endl;
      std::cout << "  Nodes:     " << std::endl;
      for (const auto& n : jdata.nodes) {
        std::cout << "    " << n << std::endl;
      }
      break;
    case JD_JOBIDNOTFOUND:
      std::cout << "Could not retrieve job: JobId not found." << std::endl;
      break;
    case JD_PARSINGERROR:
      std::cout << "Could not retrieve job: Error while parsing result." <<
      std::endl;
      break;
    case JD_UNKNOWNERROR:
      std::cout << "Could not retrieve job: Unknown error." << std::endl;
      break;
    default:
      std::cout << "Fatal bug: this message should be unreachable!" <<
      std::endl;
      break;
  }

  return DCDB_C_OK;
}

DCDB_C_RESULT destructJobDataStore(JobDataStore* jds) {
  if (jds) {
    delete jds;
  }
  return DCDB_C_OK;
}
