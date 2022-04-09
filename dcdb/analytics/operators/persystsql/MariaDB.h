//================================================================================
// Name        : MariaDB.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features to use Units in Operators.
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

#ifndef ANALYTICS_OPERATORS_PERSYSTSQL_MARIADB_H_
#define ANALYTICS_OPERATORS_PERSYSTSQL_MARIADB_H_

#include "mariadb/mysql.h"
#include "../../../common/include/logging.h"
#include <vector>
#include <string>
#include <map>
#include <mutex>

struct Aggregate_info_t {
	std::string job_id_db;
	unsigned int timestamp;
	unsigned int property_type_id;
	unsigned int num_of_observations;
	float average;
	std::vector<float> quantiles;
	float severity_average;
};

struct Job_info_t {
		std::string job_id_db;
		unsigned long long last_seen_timestamp;
		std::string job_current_table_suffix;
};

class JobCache {
private:
	std::map<std::string, Job_info_t> _jobCacheMap; //< Job id string to job data
	const std::size_t JOB_CACHE_MAX_SIZE = 1000;
public:
	/**
         * Adds one job to cache, if cache is full, the job last seen will be removed
	 * so the new added job can feet in. 
	 */
	void addJobToCache(const std::string uid, int number_of_nodes, const std::string &job_id_string, const std::string & job_id_db);
	
	/**
	 * Find job in cache
	 */
	Job_info_t * find(const std::string uid, int number_of_nodes, const std::string &job_id_string);

	/**
	 * Get entire cache as string for debugging
	 */
	std::string cache2String();
};

class MariaDB {

public:
	enum Rotation_t {
		EVERY_YEAR, EVERY_MONTH, EVERY_XDAYS //number of days must be provided
	};

protected:

	MariaDB();
	virtual ~MariaDB();
	static void initialize();

	MYSQL *_mysql;
	Rotation_t _rotation;
	unsigned int _every_x_days; //ignored except when EVERY_XDAYS is chosen
	unsigned long long _end_aggregate_timestamp;
	std::string _current_table_suffix;
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

	static MariaDB * instance;
	static std::mutex mut;
	static std::once_flag init_once;
	bool _initialized;
	JobCache _jobCache;

	/** print error.
	 * Prints the mysql error message. If connection is gone (Error 2006) then we also close the connection.
	 * Please check with isInitialized() to initialize it again.
         * String is NOT passed as reference to avoid having to pass always an empty string when calling member
         * function.
	 */
	void print_error(std::string slurm_job_id="");

	bool getCurrentSuffixAggregateTable(std::string & new_suffix);
	bool createNewAggregate(std::string& new_suffix);
	void getNewDates(const std::string& last_end_timestamp,	std::string & begin_timestamp, std::string & end_timestamp);

public:

	/**
	 * Connect to database.
	 */
	bool initializeConnection(const std::string & host,	const std::string & user, const std::string & password,
			const std::string & database_name, Rotation_t rotation, int port = 3306, unsigned int every_x_days = 0);

	bool isInitialized(){
		return _initialized;
	}

	/**
	 * Disconnect
	 */
	bool finalizeConnection();

	/**
	 * Check if job_id (db) exist. If map empty it doesn't exist/job not found is not yet on accounting.
	 * @param job_id_strings	job id strings including array jobid.
	 * @param job_id_map    job_id_string to job_id (db) map
	 */
	bool getDBJobID(const std::string & job_id_string, std::string& job_db_id, const std::string & user, int number_nodes, int batch_domain);

	/**
	 * Insert job in the accounting table.
	 */
	bool insertIntoJob(const std::string& job_id_string, const std::string& uid, std::string & job_id_db, const std::string & suffix, int number_nodes, int batch_domain);

	/**
	 * Insert performance data into the aggregate table (Aggregate_<suffix>)
	 */
	bool insertInAggregateTable(const std::string& suffix, Aggregate_info_t & agg_info, std::string & slurm_job_id);

	/**
	 * Update the last suffix in the Accounting table
	 * @param job_id_map    job_id_string to job_id (db) map
	 * @param suffix		Aggregate table suffix
	 */
	bool updateJobsLastSuffix(const std::string & job_id_string, const std::string & user, int number_nodes, const std::string& job_id_db, std::string & suffix);

	/**
	 * Get the next or the current table suffix
	 */
	bool getTableSuffix(std::string & table_suffix);

	/**
	 * Singleton object. Get here your instance!
	 */
	static MariaDB * getInstance();
};


#endif /* ANALYTICS_OPERATORS_PERSYSTSQL_MARIADB_H_ */
