//================================================================================
// Name        : MariaDB.cpp
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

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/types.h> // uid_t
#include "../../../common/include/timestamp.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "MariaDB.h"
#include "MariaDB.h"

/**************SQLResult****************/

class SQLResult {
private:
        MYSQL_RES * _result;
public:
        SQLResult(MYSQL * mysql){
                _result= mysql_store_result(mysql);
        }
        ~SQLResult(){
                mysql_free_result(_result);
        }

        MYSQL_RES * get(){
                return _result;
        }

        MYSQL_ROW fetch_row(){
                return mysql_fetch_row(_result);
        }
};

/**************JobCache****************/

const std::string DELIMITER = "|";
std::string createIdJobCache(const std::string uid, int number_of_nodes, const std::string &job_id_string){
	std::stringstream id;
	id << job_id_string << DELIMITER << uid << DELIMITER << number_of_nodes;
	return id.str();
}

void JobCache::addJobToCache(const std::string uid, int number_of_nodes, const std::string &job_id_string, const std::string & job_id_db){
	//remove one element before inserting (the last condition (_jobCache.size() > JOB_CACHE_MAX_SIZE) shouldn't really happen...
	if(_jobCacheMap.size() == JOB_CACHE_MAX_SIZE || _jobCacheMap.size() > JOB_CACHE_MAX_SIZE){
		using MyPairType = std::pair<std::string, Job_info_t>;
		auto smallest = std::min_element(_jobCacheMap.begin(), _jobCacheMap.end(),
				[](const MyPairType& l, const MyPairType& r) -> bool {return l.second.last_seen_timestamp < r.second.last_seen_timestamp;});
		_jobCacheMap.erase(smallest);
	}
	Job_info_t ji;
	ji.job_id_db = job_id_db;
	ji.last_seen_timestamp = getTimestamp();
	_jobCacheMap[createIdJobCache(uid, number_of_nodes, job_id_string)] = ji;
}

Job_info_t * JobCache::find(const std::string uid, int number_of_nodes, const std::string &job_id_string){
	auto found = _jobCacheMap.find(createIdJobCache(uid, number_of_nodes, job_id_string));
	if(found != _jobCacheMap.end()){ //found
		found->second.last_seen_timestamp = getTimestamp();
		return &(found->second);
	}
	return nullptr;
}

std::string JobCache::cache2String(){
	std::stringstream out;
	out << "JobCache Size=" << _jobCacheMap.size() << std::endl;
	for(auto & kv: _jobCacheMap){
		out << "\tJobId=" << kv.first << std::endl;
		out << "\t\tjob_id_db=" << kv.second.job_id_db << std::endl;
		out << "\t\tlast_seen_timestamp=" << kv.second.last_seen_timestamp << std::endl;
	}
	return out.str();
}

/**************MariaDB****************/

MariaDB * MariaDB::instance = nullptr;
std::mutex MariaDB::mut;
std::once_flag MariaDB::init_once;

void MariaDB::print_error(std::string add_comment){
	if(_initialized){
    		LOG(error) << "Error(" << mysql_errno(_mysql) << ") " << add_comment <<" [" << mysql_sqlstate(_mysql) << "] \""<< mysql_error(_mysql) << "\"" ;
		if(mysql_errno(_mysql) == 2006){
			mysql_close(_mysql);
			_initialized=false;
		}
	} else {
		LOG(error) << "MySQL connection not initialized, will try to initialize on the next measurement...";
        }
}

MariaDB::MariaDB(): _mysql(NULL), _rotation(EVERY_MONTH), _every_x_days(0), _end_aggregate_timestamp(0), _initialized(false) {
}

void MariaDB::initialize(){
	instance = new MariaDB();
}

MariaDB * MariaDB::getInstance(){
	std::call_once(init_once, &MariaDB::initialize);
	return instance;
}

bool MariaDB::initializeConnection(const std::string & host, const std::string & user,
	const std::string & password, const std::string & database_name,
	Rotation_t rotation, int port, unsigned int every_x_days) {
	std::lock_guard<std::mutex>  lock(mut);
	if (!_initialized) {
		_mysql = mysql_init(NULL);
		if(!mysql_real_connect(_mysql, host.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, NULL, 0)){
			print_error();
			return false;
		}
		LOG(debug) << "Successfully connected to mariadb";
		_initialized = true;
	}
	return true;
}

bool MariaDB::finalizeConnection(){
	std::lock_guard<std::mutex>  lock(mut);
	if(_initialized){
		mysql_close(_mysql);
		LOG(debug) << "Closed mariadb";
		_initialized = false;
	}
	return true;
}


MariaDB::~MariaDB(){

}

bool MariaDB::getDBJobID(const std::string & job_id_string, std::string& job_db_id, const std::string & user, int number_nodes, int batch_domain) {
	std::lock_guard<std::mutex>  lock(mut);
	Job_info_t *job_info=_jobCache.find(user, number_nodes, job_id_string);
	if(job_info){ //found
		job_db_id = job_info->job_id_db;
		return true; //job found
	}

	std::stringstream build_query;
	build_query	<< "SELECT job_id, job_id_string FROM Accounting WHERE job_id_string='" << job_id_string << "' AND user='" << user << "'";
	build_query << " AND nodes=" << number_nodes << " AND batch_domain=" << batch_domain;
	auto query = build_query.str();
	LOG(debug)<< query;
	if (_initialized && mysql_real_query(_mysql, query.c_str(), query.size())) {
		print_error();
		return false;
	}

	SQLResult result(_mysql);
	if (result.get()) {
		MYSQL_ROW row;
		while ((row = result.fetch_row())) {
			if (row[0]) {
				job_db_id = row[0];
				std::string job_id_string = std::string(row[1]);
				_jobCache.addJobToCache(user, number_nodes, job_id_string, job_db_id);
				LOG(debug) << _jobCache.cache2String();
				return true; //found
			}
		}
	}
	return false;
}

bool MariaDB::getCurrentSuffixAggregateTable(std::string & suffix){
    if(_end_aggregate_timestamp){
	auto now_uts = getTimestamp();
       	if(now_uts < _end_aggregate_timestamp) { //suffix found, don't do anything
       	    suffix = _current_table_suffix;
            return true;
       	}
    }
    auto right_now = boost::posix_time::second_clock::local_time();
    auto date_time =  boost::posix_time::to_iso_extended_string(right_now);
    std::replace( date_time.begin(), date_time.end(), 'T', ' ');

    std::stringstream build_query;
    build_query << "SELECT suffix, UNIX_TIMESTAMP(end_timestamp) FROM SuffixToAggregateTable WHERE begin_timestamp <= \'";
    build_query << date_time << "\' AND end_timestamp > \'" << date_time << "\'";
    auto query = build_query.str();
    LOG(debug) << query;

    if(_initialized && mysql_real_query(_mysql, query.c_str(), query.size())){
    	print_error();
    	return false;
    }
    SQLResult result(_mysql);
    if(result.get()){
    	MYSQL_ROW row;
       	while((row = result.fetch_row())){
       		if(row[0] && row[1]){
       			suffix = std::string(row[0]);
                _current_table_suffix = suffix;
                std::string row1(row[1]);
                _end_aggregate_timestamp = std::stoull(row1) * 1e9;
                return true;
            } else {
            	return false;
            }
        }
    }
    return false;
}


bool MariaDB::insertIntoJob(const std::string& job_id_string, const std::string& uid, std::string & job_id_db, const std::string & suffix, int number_nodes, int batch_domain){
	std::lock_guard<std::mutex>  lock(mut);
	//maybe another thread did this for us
	Job_info_t *job_info=_jobCache.find(uid,number_nodes, job_id_string);
	if(job_info){
		job_id_db = job_info->job_id_db;
		return true;
	}

	//Also check that job was not inserted by another collector (shouldn't really happen but "sicher ist sicher"
	std::stringstream build_query;
	build_query	<< "SELECT job_id, job_id_string FROM Accounting WHERE job_id_string ='";
	build_query << job_id_string << "' AND user='" << uid <<"' AND nodes=" << number_nodes << " AND batch_domain=" << batch_domain;
	auto select_query = build_query.str();
	LOG(debug) << select_query;

	if (_initialized && mysql_real_query(_mysql, select_query.c_str(), select_query.size())) {
		print_error();
		return false;
	}

	bool job_found_in_db = false;
	SQLResult result(_mysql);
	if (result.get()) {
		MYSQL_ROW row;
		while ((row = result.fetch_row())) {
			if (row[0]) {
				job_id_db = row[0];
				_jobCache.addJobToCache(uid, number_nodes, job_id_string, job_id_db);
				LOG(debug) << _jobCache.cache2String();
				job_found_in_db=true;
			}
		}
	}

	if(!job_found_in_db) {
		std::stringstream build_insert;
		build_insert << "INSERT IGNORE INTO Accounting (job_id_string, user, nodes, aggregate_first_suffix, aggregate_last_suffix, batch_domain, perfdata_available) VALUES (\'" << job_id_string << "\',\'";
		build_insert << uid << "\',\'";
		build_insert << number_nodes << "\',\'";
		build_insert << suffix << "\',\'" << suffix << "\',\'";
		build_insert << batch_domain << "\',\'1\')"; //After batch domain, perfdata_available=1 
		std::string query = build_insert.str();
		LOG(debug)<< query;

		if (_initialized && mysql_real_query(_mysql, query.c_str(), query.size())) {
			print_error();
			return false;
		}
		job_id_db = std::to_string(mysql_insert_id(_mysql));
                //insert into job cache here?
		_jobCache.addJobToCache(uid, number_nodes, job_id_string, job_id_db);
		LOG(debug) << _jobCache.cache2String();
	}
	return true;
}


bool MariaDB::insertInAggregateTable(const std::string& suffix, Aggregate_info_t & agg, std::string & slurm_job_id){
	std::lock_guard<std::mutex>  lock(mut);
        std::stringstream build_insert;
        build_insert << "INSERT INTO Aggregate_" << suffix << " VALUES ( FROM_UNIXTIME(\'" << agg.timestamp;
        build_insert << "\'), \'" << agg.job_id_db;
        build_insert << "\', \'" << agg.property_type_id;
        build_insert << "\', \'" << agg.num_of_observations;
        build_insert << "\', \'" << agg.average;
        for(auto quant: agg.quantiles){
                build_insert << "\', \'" << quant;
        }
        build_insert << "\', \'" << agg.severity_average << "\')";
        std::string query = build_insert.str();
        LOG(debug) << query;

        if(_initialized && mysql_real_query(_mysql, query.c_str(), query.size())){
                std::stringstream build_job_id;
                build_job_id << "SLURM_JOB_ID=" << slurm_job_id;
                print_error(build_job_id.str());
                return false;
        }
        return true;
}

bool MariaDB::createNewAggregate(std::string& new_suffix){
    std::string select = "SELECT suffix, end_timestamp FROM SuffixToAggregateTable ORDER BY end_timestamp DESC LIMIT 1";
    if (_initialized && mysql_real_query(_mysql, select.c_str(), select.size())){
        print_error();
    }
    std::string last_suffix = "0";
    std::string end_timestamp = "";
    SQLResult result(_mysql);
    if(result.get()){
        MYSQL_ROW row;
        while ((row = result.fetch_row())){
                if(row[0]){
                        last_suffix = std::string(row[0]);
                }
                if(row[1]){
                        end_timestamp = std::string(row[1]);
                }
        }
    }
    if(end_timestamp.size() == 0){
        boost::gregorian::date today = boost::gregorian::day_clock::local_day();
        auto today_iso = to_iso_extended_string(today);
        end_timestamp = today_iso + " 00:00:00";
    }
    int new_suff = std::stoi(last_suffix) + 1;	

    std::string new_begin_timestamp, new_end_timestamp;
    getNewDates(end_timestamp, new_begin_timestamp, new_end_timestamp);

    std::stringstream build_insert;
    build_insert << "INSERT INTO SuffixToAggregateTable VALUES(\'" << new_suff;
    build_insert << "\', \'" << new_begin_timestamp << "\', \'" << new_end_timestamp << "\')";
    auto query = build_insert.str();
    LOG(debug) << query;

    if (_initialized && mysql_real_query(_mysql, query.c_str(), query.size())){
        print_error();
        return false;
    }
    std::stringstream build_create;
    build_create << "CREATE TABLE Aggregate_" << new_suff << " LIKE Aggregate";
    auto query2 = build_create.str();
    LOG(debug) << query2;

    if(_initialized && mysql_real_query(_mysql, query2.c_str(), query2.size() )){
        if(mysql_errno(_mysql) == 1050){
        	return true; //table exists!
        }
        print_error();
        return false;
    }
    new_suffix = std::to_string(new_suff);
    _current_table_suffix = new_suffix; 
    return true;
}


bool MariaDB::updateJobsLastSuffix(const std::string & job_id_string, const std::string & user, int number_nodes, const std::string& job_id_db, std::string & suffix){
	std::lock_guard<std::mutex> lock(mut);
	Job_info_t* job_info = _jobCache.find(user, number_nodes, job_id_string);
	if(job_info){ //found
		if(job_info->job_current_table_suffix.empty() || job_info->job_current_table_suffix != suffix){
			job_info->job_current_table_suffix = suffix; //set new suffix
			//must update;
		} else {
			//no need to update in databse
			return true;
		}
	} //not found must update

	std::stringstream build_update;
	build_update << "UPDATE Accounting SET aggregate_last_suffix=\'" << suffix << "\' WHERE job_id=" << job_id_db;
	auto query = build_update.str();
	LOG(debug)<< query;

	if (mysql_real_query(_mysql, query.c_str(), query.size())) {
		print_error();
		return false;
	}
	return true;
}


bool MariaDB::getTableSuffix(std::string & table_suffix){
	std::lock_guard<std::mutex> lock(mut);
	if (!getCurrentSuffixAggregateTable(table_suffix) && !createNewAggregate(table_suffix)) {
		return false;
	}
	return true;
}

void MariaDB::getNewDates(const std::string& last_end_timestamp, std::string & begin_timestamp, std::string & end_timestamp){
	begin_timestamp = last_end_timestamp;
	std::tm tm = { };
	std::stringstream ss(last_end_timestamp);
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

	boost::gregorian::date d = boost::gregorian::date_from_tm(tm);

	switch (_rotation) {
	case EVERY_YEAR:
		d += boost::gregorian::months(12);
		break;
	case EVERY_MONTH:
		d += boost::gregorian::months(1);
		break;
	case EVERY_XDAYS:
		d += boost::gregorian::days(_every_x_days);
		break;
	default:
		d += boost::gregorian::months(1);
		break;
	};

	boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
	boost::posix_time::ptime end_ts(d);
	boost::posix_time::time_duration diff = end_ts - epoch;

	_end_aggregate_timestamp = diff.total_seconds() * 1e9;
	end_timestamp = to_iso_extended_string(d) + " 00:00:00";
	LOG(debug) << "_end_aggregate_timestamp =" << _end_aggregate_timestamp;
	LOG(debug) << "end_timestamp =" << end_timestamp;
}


