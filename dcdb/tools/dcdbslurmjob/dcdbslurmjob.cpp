//================================================================================
// Name        : dcdbslurmjob.cpp
// Author      : Michael Ott, Micha Mueller
// Copyright   : Leibniz Supercomputing Centre
// Description : Main file of the dcdbslurmjob command line utility
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

#include "../../common/include/globalconfiguration.h"
#include "timestamp.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdlib>
#include <dcdb/connection.h>
#include <dcdb/jobdatastore.h>
#include <iostream>
#include <mosquitto.h>
#include "dcdb/version.h"
#include "version.h"
#include "fcntl.h"
#include "unistd.h"

#define SLURM_JOBSTEP_SEP "."

int msgId = -1;
bool done = false;

DCDB::Connection *  dcdbConn = nullptr;
DCDB::JobDataStore *myJobDataStore = nullptr;
struct mosquitto *  myMosq = nullptr;
int timeout = 10;
int qos = 1;

void publishCallback(struct mosquitto *mosq, void *obj, int mid) {
	if(msgId != -1 && mid == msgId)
		done = true;
}

// Re-opens STDIN, STDOUT or STDERR to /dev/null if they are closed.
// Prevents libuv from making dcdbslurmjob crash when using Cassandra.
void fixFileDescriptors() {
    std::string stdArr[3] = {"STDIN", "STDOUT", "STDERR"};
    int fd = -1;
    for(int idx=0; idx<=2; idx++) {
        if(fcntl(idx, F_GETFD) < 0) {
            std::cerr << "Warning: detected closed " << stdArr[idx] << " channel. Fixing..." << std::endl;
            if((fd=open("/dev/null", O_RDWR)) < 0 || dup2(fd, idx) < 0) {
                std::cerr << "Error: cannot re-open " << stdArr[idx] << " channel." << std::endl;
            }
        }
    }
}

/*
 * Print usage information
 */
void usage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  dcdbslurmjob [-b<host>] [-t<timestamp>] [-n<nodelist>] [-d<domainid>] [-j<jobid>] [-i<userid>] start|stop" << std::endl;
    std::cout << "  dcdbslurmjob [-c<host>] [-u<username>] [-p<password>] [-t<timestamp>] [-n<nodelist>] [-j<jobid>] [-i<userid>] [-s<pattern>] start|stop" << std::endl;
    std::cout << "  dcdbslurmjob -h" << std::endl;
    std::cout << std::endl;

    std::cout << "Options:" << std::endl;
    std::cout << "  -b<hosts>     List of MQTT brokers           [default: localhost:1883]" << std::endl;
    std::cout << "  -q<qos>       MQTT QoS to use                [default: 1]" << std::endl;
    std::cout << "  -o<timeout>   MQTT timeout in seconds        [default: 10]" << std::endl;
    std::cout << "  -c<hosts>     List of Cassandra hosts        [default: none]" << std::endl;
    std::cout << "  -u<username>  Cassandra username             [default: none]" << std::endl;
    std::cout << "  -p<password>  Cassandra password             [default: none]" << std::endl;
    std::cout << "  -t<timestamp> Timestamp value                [default: now]" << std::endl;
    std::cout << "  -n<nodelist>  Comma-separated nodelist       [default: SLURM_JOB_NODELIST]" << std::endl;
    std::cout << "  -d<domainid>  Job domain id                  [default: default]" << std::endl;
    std::cout << "  -j<jobid>     String job id                  [default: SLURM_JOB_ID var]" << std::endl;
    std::cout << "  -i<userid>    Numerical user id              [default: SLURM_JOB_USER var]" << std::endl;
    std::cout << "  -s<pattern>   Nodelist substitution pattern  [default: none]" << std::endl;
    std::cout << "  -m<pattern>   Maximum job length in h        [default: none]" << std::endl;
    std::cout << "  -f            Force job insert/update        [default: no]" << std::endl;
    std::cout << std::endl;
    std::cout << "  -h            This help page" << std::endl;
    std::cout << std::endl;
    std::cout << "Options -b and -c|u|p are mutual exclusive! If both are specified, the latter takes precedence. By default MQTT broker is specified." << std::endl;
}

std::string getEnv(const char* var) {
    char* str = std::getenv(var);
    if (str != NULL) {
	    return std::string(str);
    } else {
	    return std::string("");
    }
}

void splitNodeList(const std::string& str, DCDB::NodeList& nl)
{
    nl.clear();
    std::string s1 = str;
    boost::regex r1("([^,[]+)(\\[[0-9,-]+\\])?(,|$)", boost::regex::extended);
    boost::smatch m1;
    while (boost::regex_search(s1, m1, r1)) {
		std::string hostBase = m1[1].str();
		
		if (m1[2].str().size() == 0) {
			nl.push_back(hostBase);
		} else {
			std::string s2 = m1[2].str();
			boost::regex r2("([0-9]+)-?([0-9]+)?(,|\\])", boost::regex::extended);
			boost::smatch m2;
			while (boost::regex_search(s2, m2, r2)) {
				if (m2[2] == "") {
					nl.push_back(hostBase + m2[1].str());
				} else {
					int start = atoi(m2[1].str().c_str());
					int stop = atoi(m2[2].str().c_str());
					for (int i=start; i<=stop; i++) {
						std::stringstream ss;
						ss << std::setw(m2[2].str().length()) << std::setfill('0') << i;
						nl.push_back(hostBase + ss.str());
					}
				}
				s2 = m2.suffix().str();
			}
		}
		s1 = m1.suffix().str();
    }
}

void convertNodeList(DCDB::NodeList& nl, std::string substitution) {
    //check if input has sed format of "s/.../.../" for substitution
    boost::regex  checkSubstitute("s([^\\\\]{1})([\\S|\\s]*)\\1([\\S|\\s]*)\\1");
    boost::smatch matchResults;
    
    if (regex_match(substitution, matchResults, checkSubstitute)) {
		//input has substitute format
		boost::regex re = (boost::regex(matchResults[2].str(), boost::regex_constants::extended));
		std::string fmt = matchResults[3].str();
		for (auto &n: nl) {
			 n = boost::regex_replace(n, re, fmt);
			 //std::cout << n <<" => " << mqtt << std::endl;
		}
    }
}

void splitHostList(const std::string& str, std::vector<std::string>& hl, char delim = ',')
{
    hl.clear();
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
		hl.push_back(token);
    }
}

void pickRandomHost(std::vector<std::string>& hl, std::string& host, int& port, bool erase = false) {
    srand (time(NULL));
    int n = rand() % hl.size();
    host = parseNetworkHost(hl[n]);
    port = atoi(parseNetworkPort(hl[n]).c_str());
    if (erase) {
		hl.erase(hl.begin()+n);
    }
}

int insertJob(DCDB::JobData job, bool start, bool cassandra) {
    // Cassandra-based job insertion
    if (cassandra) {
        if (start) {
            if(myJobDataStore->insertJob(job) != DCDB::JD_OK) {
                std::cerr << "Job data insert for job " << job.jobId << " failed!" << std::endl;
                return 1;
            }
        }
        else {
            DCDB::JobData jobStart;
            if (myJobDataStore->getJobById(jobStart, job.jobId, job.domainId) != DCDB::JD_OK) {
                std::cerr << "Could not retrieve job " << job.jobId << " to be updated!" << std::endl;
                return 1;
            }
            if (myJobDataStore->updateEndtime(jobStart.jobId, jobStart.startTime, job.endTime, job.domainId) != DCDB::JD_OK) {
                std::cerr << "Could not update end time of job " << job.jobId << "!" << std::endl;
                return 1;
            }
        }
    // MQTT-based job insertion
    } else {
        // Create job data string in JSON format
        std::string                 payload = "";
        std::string                 topic = "/DCDB_JOBDATA/"; // Do not change or keep in sync with simplemqttservermessage.h
        boost::property_tree::ptree config;
        std::ostringstream          output;
        config.clear();
        config.push_back(boost::property_tree::ptree::value_type("domainid", boost::property_tree::ptree(job.domainId)));
        config.push_back(boost::property_tree::ptree::value_type("jobid", boost::property_tree::ptree(job.jobId)));
        config.push_back(boost::property_tree::ptree::value_type("userid", boost::property_tree::ptree(job.userId)));
        config.push_back(boost::property_tree::ptree::value_type("starttime", boost::property_tree::ptree(std::to_string(job.startTime.getRaw()))));
        config.push_back(boost::property_tree::ptree::value_type("endtime", boost::property_tree::ptree(std::to_string(job.endTime.getRaw()))));
        boost::property_tree::ptree nodes;
        for (const auto &n : job.nodes) {
            nodes.push_back(boost::property_tree::ptree::value_type("", boost::property_tree::ptree(n)));
        }
        
        config.push_back(boost::property_tree::ptree::value_type("nodes", nodes));
        boost::property_tree::write_json(output, config, true);
        payload = output.str();
        //std::cout << "Payload:\n" << payload << std::endl;
        mosquitto_publish_callback_set(myMosq, publishCallback);
        uint64_t startTs = getTimestamp();
        msgId = -1;
        done = false;
        int ret = MOSQ_ERR_UNKNOWN;
        
        // Message sent to CollectAgent is independent of start/stop. We send the
        // same JSON in either case. CA does job insert or update depending
        // on job endtime value.
        if ((ret = mosquitto_publish(myMosq, &msgId, topic.c_str(), payload.length(), payload.c_str(), qos, false)) != MOSQ_ERR_SUCCESS) {
            std::cerr << "Could not publish data for job " << job.jobId << " via MQTT: " << mosquitto_strerror(ret) << std::endl;
            return 1;
        }
        do {
            if ((ret = mosquitto_loop(myMosq, -1, 1)) != MOSQ_ERR_SUCCESS) {
                std::cerr << "Error in mosquitto_loop for job " << job.jobId << ": " << mosquitto_strerror(ret) << std::endl;
                return 1;
            }
        } while(!done && getTimestamp() - startTs < (uint64_t)S_TO_NS(timeout));
    }
    return true;
}


/**
 * Retrieves Slurm job data from environment variables and sends it to either a
 * CollectAgent or a Cassandra database. Job data can also be passed as command
 * line options.
 */
int main(int argc, char** argv) {
    std::cout << "dcdbslurmjob " << VERSION << std::endl << std::endl;

    bool cassandra = false;
    
    std::vector<std::string> hostList;
    std::string host = "", cassandraUser = "", cassandraPassword = "";
    int port;
    std::string nodelist="", pnodelist="", jobId="", userId="", stepId="", packId="", taskId="";
    std::string domainId = JOB_DEFAULT_DOMAIN;
    std::string substitution="";
    int maxJobLength = -1;
    bool force = false;
    uint64_t ts=0;
    
    // Defining options
    const char *opts = "b:q:o:c:u:p:n:t:d:j:i:s:m:fh";

    char ret;
    while ((ret = getopt(argc, argv, opts))!=-1) {
        switch (ret)
        {
            case 'h':
                usage();
                return 0;
            default:
                break;
        }
    }
    
    if (argc < 2) {
        std::cerr << "At least one argument is required: start or stop" << std::endl;
        return 1;
    } else if(!boost::iequals(argv[argc-1], "start") && !boost::iequals(argv[argc-1], "stop")) {
        std::cerr << "Unsupported action: must either be start or stop" << std::endl;
        return 1;
    }
    
    optind = 1;
    while ((ret=getopt(argc, argv, opts))!=-1) {
        switch(ret) {
		case 'b': {
			cassandra = false;
			splitHostList(optarg, hostList);
			break;
		}
		case 'q':
			qos = atoi(optarg);
			break;
		case 'o':
			timeout = atoi(optarg);
			break;
		case 'c':
			cassandra = true;
			splitHostList(optarg, hostList);
			break;
		case 'u':
			cassandra = true;
			cassandraUser = optarg;
			break;
		case 'p': {
			cassandra = true;
			cassandraPassword = optarg;
			// What does this do? Mask the password?
			size_t pwdLen = strlen(optarg);
			memset(optarg, 'x', (pwdLen >= 3) ? 3 : pwdLen);
			if (pwdLen > 3) {
				memset(optarg + 3, 0, pwdLen - 3);
			}
			break;
		}
	    case 'n':
			nodelist = optarg;
			break;
		case 't':
			ts = std::stoull(optarg);
			break;
        case 'd':
            domainId = optarg;
            break;
		case 'j':
			jobId = optarg;
			break;
		case 'i':
			userId = optarg;
			break;
	    case 's':
			substitution = optarg;
			if (substitution == "SNG") {
				substitution = "s%([fi][0-9]{2})(r[0-9]{2})(c[0-9]{2})(s[0-9]{2})%/sng/\\1/\\2/\\3/\\4%";
				maxJobLength = 48;
			} else if (substitution == "DEEPEST") {
				substitution = "s%dp-(cn|dam|esb)([0-9]{2})%/deepest/\\1/s\\2%";
				maxJobLength = 20;
			}
			break;
	    case 'm':
			maxJobLength = std::stoull(optarg);
			break;
	    case 'f':
			force = true;
			break;
		case 'h':
		default:
			usage();
			return 1;
        }
    }

    // Check whether we are started by slurmd and are the first node in the nodelist
    std::string slurmNodename = getEnv("SLURMD_NODENAME");
    std::string slurmNodelist = getEnv("SLURM_JOB_NODELIST");
    if (slurmNodelist == "") {
	slurmNodelist = getEnv("SLURM_NODELIST");
    }
    DCDB::NodeList nl;
    splitNodeList(slurmNodelist, nl);
    if (!force && (slurmNodename.size() > 0)) {
	if (slurmNodename != nl.front()) {
	    std::cout << "Running in slurmd context but not the first node in nodelist. Exiting." << std::endl;
	    return 0;
	} else {
	    std::cout << "Running in slurmd context and are first in nodelist." << std::endl;
	}
    }
    
    if (hostList.size() == 0) {
		hostList.push_back("localhost");
    }

    // Initialize transport
    if (cassandra) {
        fixFileDescriptors();
	    // Allocate and initialize connection to Cassandra.
	    pickRandomHost(hostList, host, port);
	    if (port == 0) {
			port = 9042;
	    }
	
	    dcdbConn = new DCDB::Connection(host, port, cassandraUser, cassandraPassword);
	    if (!dcdbConn->connect()) {
		    std::cerr << "Cannot connect to Cassandra server " << host << ":" << port << std::endl;
		    return 1;
	    }
	    std::cout << "Connected to Cassandra server " << host << ":" << port << std::endl;
	    myJobDataStore = new DCDB::JobDataStore(dcdbConn);
    } else {
	    // Initialize Mosquitto library and connect to broker
	    char hostname[256];

	    if (gethostname(hostname, 255) != 0) {
		    std::cerr << "Cannot get hostname!" << std::endl;
		    return 1;
	    }
	    hostname[255] = '\0';
	    mosquitto_lib_init();
	    myMosq = mosquitto_new(hostname, false, NULL);
	    if (!myMosq) {
		    perror(NULL);
		    return 1;
	    }

	    int ret = MOSQ_ERR_UNKNOWN;
	    do {
		    pickRandomHost(hostList, host, port, true);
		    if (port == 0) {
			    port = 1883;
		    }
		    
		    if ((ret = mosquitto_connect(myMosq, host.c_str(), port, 1000)) != MOSQ_ERR_SUCCESS) {
			    std::cerr << "Could not connect to MQTT broker " << host << ":" << port << " (" << mosquitto_strerror(ret) << ")" <<std::endl;
		    } else {
			    std::cout << "Connected to MQTT broker " << host << ":" << port << ", using QoS " << qos << std::endl;
			    break;
		    }
	    } while (hostList.size() > 0);
	    
	    if (ret != MOSQ_ERR_SUCCESS) {
		    std::cout << "No more MQTT brokers left, aborting" << std::endl;
		    return 1;
	    }
    }

    // Collect job data
    bool start = boost::iequals(argv[argc-1], "start");
    bool isPackLeader=false;
    DCDB::JobData jd;
    int retCode = 0;

    // Fetching timestamp
    if(ts==0) {
        ts = getTimestamp();
    }
    
    // Fetching job ID
    if(jobId=="") {
        // Is this a job array?
        if((jobId=getEnv("SLURM_ARRAY_JOB_ID")) != "" && (taskId = getEnv("SLURM_ARRAY_TASK_ID")) != "") {
            jobId = jobId + "_" + taskId;
        // Is this a job pack? Packs and arrays cannot be combined in SLURM
        } else if ((jobId=getEnv("SLURM_PACK_JOB_ID")) != "" && (packId = getEnv("SLURM_PACK_JOB_OFFSET")) != "") {
            isPackLeader = packId=="0";
            jobId = jobId + "+" + packId;
            // In this case, packId contains the job ID of the whole pack
            packId = getEnv("SLURM_PACK_JOB_ID");
        // Is this an ordinary job?
        } else {
            jobId = getEnv("SLURM_JOB_ID");
            if (jobId == "") {
                jobId = getEnv("SLURM_JOBID");
            }
        }
        
		// Is this a step within a job/pack/array?
		stepId = getEnv("SLURM_STEP_ID");
		if (stepId=="") {
			stepId = getEnv("SLURM_STEPID");
		}
		if (stepId!="" && stepId!="0" && jobId!="")
			jobId = jobId + SLURM_JOBSTEP_SEP + stepId;
	}

	// Fetching user ID
    if(userId=="") {
        userId = getEnv("SLURM_JOB_USER");
        if(userId=="") {
            userId = getEnv("USER");
        }
    }

    DCDB::NodeList pnl;
    if (start) {
	    // Check whether a nodelist was provided as command line argument.
	    // Otherwise we have populated nl above already.
	    if(nodelist.size() > 0) {
		splitNodeList(nodelist, nl);
	    } else {
		nodelist = slurmNodelist;
	    }
	    convertNodeList(nl, substitution);
            // Getting the whole pack's node list, if necessary
            if(isPackLeader) {
                pnodelist = getEnv("SLURM_PACK_JOB_NODELIST");
            }
				
		std::cout << "DOMAINID = " << domainId << std::endl;
		std::cout << "JOBID    = " << jobId << std::endl;
        std::cout << "USER     = " << userId << std::endl;
        std::cout << "START    = " << ts << std::endl;
        std::cout << "NODELIST = " << nodelist << std::endl;
		std::cout << "SUBST    = " << substitution << std::endl;
		if (maxJobLength >= 0) {
			std::cout << "JOBLEN   = " << maxJobLength << std::endl;
		}
		std::cout << "NODES    =";
		for (auto &n: nl) {
			std::cout << " " << n;
		}
		std::cout << std::endl;
		
		// Only for job pack leaders that are starting up
		if(isPackLeader) {
            splitNodeList(pnodelist, pnl);
            convertNodeList(pnl, substitution);
            std::cout << "PACK     =";
            for (auto &n: pnl) {
                std::cout << " " << n;
            }
            std::cout << std::endl;
		}
		
        try {
            jd.domainId  = domainId;
            jd.jobId     = jobId;
            jd.userId    = userId;
            jd.startTime = DCDB::TimeStamp(ts);
            jd.endTime   = (maxJobLength >= 0) ? DCDB::TimeStamp((uint64_t) (ts + S_TO_NS((uint64_t)maxJobLength * 3600ull) + 1)) : DCDB::TimeStamp((uint64_t)0);
            jd.nodes     = nl;
        } catch(const std::invalid_argument& e) {
			std::cerr << "Invalid input format!" << std::endl;
			retCode = 1;
			goto exit;
		}
		
    } else if (boost::iequals(argv[argc-1], "stop")) {
        std::cout << "DOMAINID = " << domainId << std::endl;
        std::cout << "JOBID    = " << jobId << std::endl;
        std::cout << "STOP     = " << ts << std::endl;
        
        try {
            jd.domainId = domainId;
			jd.jobId = jobId;
			jd.endTime = DCDB::TimeStamp(ts);
		} catch (const std::invalid_argument &e) {
			std::cerr << "Invalid input format!" << std::endl;
			retCode = 1;
			goto exit;
		}
    }

    retCode = insertJob(jd, start, cassandra);
    if(isPackLeader) {
        if(start) {
            jd.nodes = pnl;
        }
        jd.jobId = packId;
        retCode = insertJob(jd, start, cassandra);
    }
    
exit:
	if (cassandra) {
		delete myJobDataStore;
		dcdbConn->disconnect();
		delete dcdbConn;
	} else {
		mosquitto_disconnect(myMosq);
		mosquitto_destroy(myMosq);
		mosquitto_lib_cleanup();
	}
	return retCode;
}
