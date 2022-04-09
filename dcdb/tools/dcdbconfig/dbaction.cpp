//================================================================================
// Name        : dbaction.cpp
// Author      : Axel Auweter
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

#include "dbaction.h"

/*
 * Print the help for the SENSOR command
 */
void DBAction::printHelp(int argc, char* argv[])
{
  /*            01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
  std::cout << "DB command help" << std::endl << std::endl;
  std::cout << "The DB command has the following options:" << std::endl;
  std::cout << "   INSERT <sid> <time> <value> - Insert test data into the data store" << std::endl;
  std::cout << "   FUZZYTRUNC <time>           - Truncate data that is older than <time>" << std::endl;
  std::cout << "   INIT                        - Initialize" << std::endl;
}

/*
 * Execute any of the DB commands
 */
int DBAction::executeCommand(int argc, char* argv[], int argvidx, const char* hostname)
{
  /* Independent from the command, we need to connect to the server */
  connection = new DCDB::Connection();
  connection->setHostname(hostname);
  if (!connection->connect()) {
      std::cerr << "Cannot connect to Cassandra database." << std::endl;
      return EXIT_FAILURE;
  }

  /* Check what we need to do (argv[argvidx] contains "DB") */
  argvidx++;
  if (argvidx >= argc) {
      std::cout << "The DB command needs at least two parameters." << std::endl;
      std::cout << "Run with 'HELP DB' to see the list of possible DB commands." << std::endl;
      goto executeCommandError;
  }

  if (strcasecmp(argv[argvidx], "INSERT") == 0) {
      /* INSERT needs two more parameters */
       if (argvidx+3 >= argc) {
           std::cout << "INSERT needs three more parameters!" << std::endl;
           goto executeCommandError;
       }
       doInsert(argv[argvidx+1], argv[argvidx+2], argv[argvidx+3]);
   } else if (strcasecmp(argv[argvidx], "FUZZYTRUNC") == 0) {
      /* FUZZYTRUNC needs one more parameter */
      if (argvidx+1 >= argc) {
          std::cout << "FUZZYTRUNC needs one more parameter!" << std::endl;
          goto executeCommandError;
      }
      doFuzzyTrunc(argv[argvidx+1]);
  } else if (strcasecmp(argv[argvidx], "INIT") == 0) {
      doInitSchema();
  } else {
      std::cout << "Invalid DB command: " << argv[argvidx] << std::endl;
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

/*
 * Insert a single sensor reading into the database
 */
void DBAction::doInsert(std::string sidstr, std::string timestr, std::string valuestr)
{
  DCDB::SensorDataStore ds(connection);
  DCDB::SensorId sid;
  DCDB::TimeStamp ts;
  int64_t value;

  if (!sid.mqttTopicConvert(sidstr)) {
      std::cout << "Invalid SID: " << sidstr << std::endl;
      return;
  }

  try {
      ts = DCDB::TimeStamp(timestr);
  }
  catch (std::exception& e) {
      std::cout << "Wrong time format." << std::endl;
      return;
  }

  try {
      value = boost::lexical_cast<int64_t>(valuestr);
  }
  catch (std::exception& e) {
      std::cout << "Wrong value format." << std::endl;
      return;
  }

  ds.insert(sid, ts.getRaw(), value);
}

/*
 * Fuzzy delete sensor data older than timestr
 * The goal of this is to kill entire cassandra rows, so we get the weekstamp of timestr,
 * subtract 1 and delete everything prior to that.
 */
void DBAction::doFuzzyTrunc(std::string timestr)
{
  DCDB::SensorDataStore ds(connection);
  DCDB::TimeStamp ts;

  try {
      ts = DCDB::TimeStamp(timestr);
  }
  catch (std::exception& e) {
      std::cout << "Wrong time format." << std::endl;
      return;
  }

  ds.truncBeforeWeek(ts.getWeekstamp());
}

void DBAction::doInitSchema()
{
  connection->initSchema();
}
