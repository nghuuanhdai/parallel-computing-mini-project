//================================================================================
// Name        : dcdbglobals.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal global settings for libdcdb
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
 * @brief This file contains some global definitions and names
 * used by the libdcdb library.
 */

#ifndef DCDB_GLOBALS_H
#define DCDB_GLOBALS_H

/* Legend:
 *   CED = Caliper Event Data
 *   CF  = Column Family
 *   JD  = Job Data
 */

#define KEYSPACE_NAME        "dcdb"
#define CF_SENSORDATA        "sensordata"
#define SENSORDATA_GC_GRACE_SECONDS "600"
#define SENSORDATA_COMPACTION "{'class' : 'TimeWindowCompactionStrategy', 'compaction_window_unit' : 'DAYS', 'compaction_window_size' : 1 }"
#define PAGING_SIZE 10000
#define JOB_PAGING_SIZE 100

#define CONFIG_KEYSPACE_NAME KEYSPACE_NAME "_config"
#define CF_PUBLISHEDSENSORS  "publishedsensors"
#define CF_MONITORINGMETADATA "monitoringmetadata"
#define CF_VIRTUALSENSORS    "virtualsensors"

#define CF_PROPERTY_PSWRITETIME "pswritetime"

#define CED_KEYSPACE_NAME KEYSPACE_NAME "_calievtdata"
#define CF_CALIEVTDATA "calievtdata"

#define JD_KEYSPACE_NAME KEYSPACE_NAME "_jobdata"
#define CF_JOBDATA  "jobdata"
#define CF_JOBDATAVIEW "jobdata_bytime"

#endif /* DCDB_GLOBALS_H */
