//================================================================================
// Name        : jobaction.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation for performing actions on the DCDB Database
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

#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>

#include "jobaction.h"

/*
 * Print the help for the SENSOR command
 */
void JobAction::printHelp(int argc, char* argv[])
{
    /*            01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    std::cout << "JOB command help" << std::endl << std::endl;
    std::cout << "The JOB command has the following options:" << std::endl;
    std::cout << "   SHOW <jobid> <domainid>     - Shows information for a certain <jobid>" << std::endl;
    std::cout << "   LIST <domainid>             - Lists all job IDs stored in the database" << std::endl;
    std::cout << "   RUNNING <domainid>          - Lists all currently running jobs" << std::endl;
    std::cout << "   PENDING <domainid>          - Lists all jobs that have not yet started" << std::endl;
    std::cout << "   FINISHED <domainid>         - Lists all jobs that have already terminated" << std::endl;
    std::cout << std::endl;
    std::cout << "The <domainid> argument is optional and defines the job domain to query." << std::endl;
}

/*
 * Execute any of the Job commands
 */
int JobAction::executeCommand(int argc, char* argv[], int argvidx, const char* hostname)
{
    /* Independent from the command, we need to connect to the server */
    connection = new DCDB::Connection();
    connection->setHostname(hostname);
    if (!connection->connect()) {
        std::cerr << "Cannot connect to Cassandra database." << std::endl;
        return EXIT_FAILURE;
    }

    /* Check what we need to do (argv[argvidx] contains "JOB") */
    argvidx++;
    if (argvidx >= argc) {
        std::cout << "The JOB command needs at least two parameters." << std::endl;
        std::cout << "Run with 'HELP JOB' to see the list of possible JOB commands." << std::endl;
        goto executeCommandError;
    }

    if (strcasecmp(argv[argvidx], "SHOW") == 0) {
        /* SHOW needs two more parameters */
        if (argvidx+1 >= argc) {
            std::cout << "SHOW needs one more parameter!" << std::endl;
            goto executeCommandError;
        }
        std::string domainId = argvidx+2 >= argc ? JOB_DEFAULT_DOMAIN : argv[argvidx+2];
        doShow(argv[argvidx+1], domainId);
    }
    else if (strcasecmp(argv[argvidx], "LIST") == 0) {
        std::string domainId = argvidx+1 >= argc ? JOB_DEFAULT_DOMAIN : argv[argvidx+1];
        doList(domainId);
    }
    else if (strcasecmp(argv[argvidx], "RUNNING") == 0) {
        std::string domainId = argvidx+1 >= argc ? JOB_DEFAULT_DOMAIN : argv[argvidx+1];
        doRunning(domainId);
    }
    else if (strcasecmp(argv[argvidx], "PENDING") == 0) {
        std::string domainId = argvidx+1 >= argc ? JOB_DEFAULT_DOMAIN : argv[argvidx+1];
        doPending(domainId);
    }
    else if (strcasecmp(argv[argvidx], "FINISHED") == 0) {
        std::string domainId = argvidx+1 >= argc ? JOB_DEFAULT_DOMAIN : argv[argvidx+1];
        doFinished(domainId);
    }
    else {
        std::cout << "Invalid JOB command: " << argv[argvidx] << std::endl;
        goto executeCommandError;
    }

    /* Clean up */
    connection->disconnect();
    delete connection;
    return EXIT_SUCCESS;

    executeCommandError:
    connection->disconnect();
    delete connection;
    return EXIT_FAILURE;
}

void JobAction::doShow(std::string jobId, std::string domainId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::JobData jobData;
    DCDB::JDError err = jobDataStore.getJobById(jobData, jobId, domainId);
    std::list<std::string>::iterator nIt;
    
    switch (err) {
        case DCDB::JD_OK:
        case DCDB::JD_PARSINGERROR:
            if(err == DCDB::JD_PARSINGERROR) {
                std::cout << "Parsing error. Some fields may not be populated." << std::endl;
            }
            jobData.startTime.convertToLocal();
            jobData.endTime.convertToLocal();
            std::cout << "Domain ID:  " << jobData.domainId << std::endl;
            std::cout << "Job ID:     " << jobId << std::endl;
            std::cout << "User ID:    " << jobData.userId << std::endl;
            std::cout << "Start Time: " << jobData.startTime.getString() << " (" << jobData.startTime.getRaw() << ")" << std::endl;
            std::cout << "End Time:   " << jobData.endTime.getString() << " (" << jobData.endTime.getRaw() << ")" << std::endl;
            std::cout << "Node List:  ";
            nIt = jobData.nodes.begin();
            if(nIt != jobData.nodes.end()) {
                std::cout << *nIt;
                ++nIt;
                while(nIt != jobData.nodes.end()) {
                    std::cout << ", " << *nIt;
                    ++nIt;
                }
            }
            std::cout << std::endl;
            break;
        case DCDB::JD_JOBKEYNOTFOUND:
            std::cout << "Job key " << jobId << " with domain ID " << domainId << " not found." << std::endl;
            break;
        case DCDB::JD_JOBIDNOTFOUND:
            std::cout << "Job ID " << jobId << " with domain ID " << domainId << " not found." << std::endl;
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void JobAction::printList(std::list<DCDB::JobData>& jobList) {
    std::cout << "Domain ID, Job ID, User ID, Start Time, End Time, #Nodes" << std::endl;
    for(const auto &j : jobList) {
	std::cout << j.domainId << "," << j.jobId << "," << j.userId << "," << j.startTime.getRaw() << "," << j.endTime.getRaw() << "," << j.nodes.size() << std::endl;
    }
    std::cout << std::endl;
}

void JobAction::doList(std::string domainId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::TimeStamp tsEnd((uint64_t)LLONG_MAX);
    DCDB::TimeStamp tsStart((uint64_t)0);
    std::list<DCDB::JobData> jobList;
    DCDB::JDError err = jobDataStore.getJobsInIntervalIncl(jobList, tsStart, tsEnd, domainId);
    switch (err) {
        case DCDB::JD_OK:
        case DCDB::JD_PARSINGERROR:
            if(err == DCDB::JD_PARSINGERROR) {
                std::cout << "Parsing error. Some fields may not be populated." << std::endl;
            }
	        printList(jobList);
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void JobAction::doPending(std::string domainId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::TimeStamp tsEnd;
    DCDB::TimeStamp tsStart(tsEnd.getRaw() - JOB_ACTION_OFFSET);
    std::list<DCDB::JobData> jobList;
    DCDB::JDError err = jobDataStore.getJobsInIntervalPending(jobList, tsStart, tsEnd, domainId);
    switch (err) {
        case DCDB::JD_OK:
        case DCDB::JD_PARSINGERROR:
            if(err == DCDB::JD_PARSINGERROR) {
                std::cout << "Parsing error. Some fields may not be populated." << std::endl;
            }
            printList(jobList);
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void JobAction::doRunning(std::string domainId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::TimeStamp tsEnd;
    DCDB::TimeStamp tsStart(tsEnd.getRaw() - JOB_ACTION_OFFSET);
    std::list<DCDB::JobData> jobList;
    DCDB::JDError err = jobDataStore.getJobsInIntervalRunning(jobList, tsStart, tsEnd, domainId);
    switch (err) {
        case DCDB::JD_OK:
        case DCDB::JD_PARSINGERROR:
            if(err == DCDB::JD_PARSINGERROR) {
                std::cout << "Parsing error. Some fields may not be populated." << std::endl;
            }
            printList(jobList);
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void JobAction::doFinished(std::string domainId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::TimeStamp tsEnd;
    DCDB::TimeStamp tsStart((uint64_t)0);
    std::list<DCDB::JobData> jobList;
    DCDB::JDError err = jobDataStore.getJobsInIntervalFinished(jobList, tsStart, tsEnd, domainId);
    switch (err) {
        case DCDB::JD_OK:
        case DCDB::JD_PARSINGERROR:
            if(err == DCDB::JD_PARSINGERROR) {
                std::cout << "Parsing error. Some fields may not be populated." << std::endl;
            }
            printList(jobList);
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}
