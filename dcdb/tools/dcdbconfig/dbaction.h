//================================================================================
// Name        : dbaction.h
// Author      : Axel Auweter
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
#include <dcdb/timestamp.h>

#include <string>

#include "useraction.h"

#ifndef DBACTION_H
#define DBACTION_H

class DBAction : public UserAction
{
public:
  void printHelp(int argc, char* argv[]);
  int executeCommand(int argc, char* argv[], int argvidx, const char* hostname);

  DBAction() {};
  virtual ~DBAction() {};

protected:
  DCDB::Connection* connection;

  void doInsert(std::string sidstr, std::string timestr, std::string valuestr);
  void doFuzzyTrunc(std::string timestr);
  void doInitSchema();
};

#endif
