//================================================================================
// Name        : jobaction.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Headers for performing actions on the DCDB Database
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


#include <dcdb/connection.h>
#include <dcdb/sensordatastore.h>
#include <dcdb/jobdatastore.h>
#include <dcdb/timestamp.h>

#include <string>
#include <limits.h>

#include "useraction.h"

#ifndef JOBACTION_H
#define JOBACTION_H

#define JOB_ACTION_OFFSET 10000000000

class JobAction : public UserAction
{
public:
    void printHelp(int argc, char* argv[]);
    int executeCommand(int argc, char* argv[], int argvidx, const char* hostname);

    JobAction() {};
    virtual ~JobAction() {};

protected:
    DCDB::Connection* connection;

    void doShow(std::string jobId, std::string domainId);
    void doList(std::string domainId);
    void doRunning(std::string domainId);
    void doFinished(std::string domainId);
    void doPending(std::string domainId);

private:
    void printList(std::list<DCDB::JobData>& jobList);
};

#endif
