//================================================================================
// Name        : dcdbquery.cpp
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Main file of the dcdbquery command line utility
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

/* C++ standard headers */
#include <iostream>
#include <stdexcept>

/* C standard headers */
#include <cstdlib>
#include <cstring>
#include <unistd.h>

/* Custom headers */
#include "dcdb/timestamp.h"
#include "dcdb/version.h"
#include "version.h"
#include "query.h"

void usage(void)
{
  if (isatty(fileno(stdin))) {
      /*            0---------1---------2---------3---------4---------5---------6---------7--------- */
      std::cout << "Usage:" << std::endl;
      std::cout << "  dcdbquery [-h <host>] [-r] [-l] [-u] <Sensor 1> [<Sensor 2> ...] [<Start> <End>]" << std::endl;
      std::cout << "  dcdbquery [-h <host>] [-r] [-l] [-u] -j <jobId> <Sensor 1> [<Sensor 2> ...]" << std::endl;
      std::cout << std::endl;
      std::cout << "Parameters:" << std::endl;
      std::cout << "  <jobId>       a job to query sensors for" << std::endl;
      std::cout << "  <Sensor n>    a sensor name" << std::endl;
      std::cout << "  <Start>       start of time series" << std::endl;
      std::cout << "  <End>         end of time series" << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -h<host>      Cassandra host                         [default: " << DEFAULT_CASSANDRAHOST << ":" << DEFAULT_CASSANDRAPORT << "]" << endl;
      std::cout << "  -r            Report timestamps in numerical format" << std::endl;
      std::cout << "  -l            Report times in local time (not UTC) [default]" << std::endl;
      std::cout << "  -u            Report times in UTC time" << std::endl;
  }
  else {
      std::cout << "Invalid request." << std::endl;
  }
  exit(EXIT_SUCCESS);
}

int main(int argc, char * const argv[])
{
  std::cout << "dcdbquery " << VERSION << " (libdcdb " << DCDB::Version::getVersion() << ")" << std::endl << std::endl;

  /* Check if run from command line */
  int argcReal;
  char **argvReal;
  if (isatty(fileno(stdin))) {
      argcReal = argc;
      argvReal = (char**)argv;
  }
  else {
      /* Check if we are a CGI program */
      if (!getenv("QUERY_STRING")) {
          std::cout << "No terminal and no QUERY_STRING environment variable." << std::endl;
          std::cout << "Exiting." << std::endl;
          exit(EXIT_FAILURE);
      }

      /* Print content type */
      std::cout << "Content-type: text/plain" << std::endl << std::endl;

      /* Create argcReal and argvReal */
      argcReal = 1;
      argvReal = (char**)malloc(sizeof(char*));
      argvReal[0] = strdup("dcdbquery"); /* dummy to make consistent with command line invocation */

      char *token = strtok(getenv("QUERY_STRING"), "&");
      while (token) {
          argcReal++;
          argvReal = (char**)realloc((void*)argvReal, argcReal*sizeof(char*));
          /* FIXME: should check if argvReal is NULL */
          argvReal[argcReal-1] = token;
          token = strtok(NULL, "&");
      }
  }

  /* Check command line arguments */
  if (argcReal <= 1) {
      usage();
  }

  /* Allocate our lovely helper class */
  DCDBQuery* myQuery;
  myQuery = new DCDBQuery();

  /* Get the options */
  int ret;
  const char *host = getenv("DCDB_HOSTNAME");
  if (!host) {
      host = "localhost";
  }
  std::string jobId;

  while ((ret=getopt(argcReal, argvReal, "+h:rluj:"))!=-1) {
      switch(ret) {
          case 'h':
              host = optarg;
              break;
          case 'r':
              myQuery->setRawOutputEnabled(true);
              break;
          case 'l':
              myQuery->setLocalTimeEnabled(true);
              break;
	  case 'u':
	      myQuery->setLocalTimeEnabled(false);
	      break;
	  case 'j':
	      jobId = optarg;
	      break;
          default:
              usage();
              exit(EXIT_FAILURE);
      }
  }

  /* Try to create TimeStamp objects from the arguments */
  DCDB::TimeStamp start, end;
  if (jobId.size() == 0) {
      if (argcReal > 2) {
	  try {
	      bool local = myQuery->getLocalTimeEnabled();
	      start = DCDB::TimeStamp(argvReal[argcReal-2], local);
	      if(std::string(argvReal[argcReal-2]) != std::string(argvReal[argcReal-1])) {
		  end = DCDB::TimeStamp(argvReal[argcReal - 1], local);
	      } else {
		  end = start;
	      }
	      argcReal-= 2;
	  } catch (std::exception& e) {
	      /* The last two parameters could not be parsed as timestamps, let's assume they are sensors and do a fuzzy querry */
	      start = end = DCDB::TimeStamp();
	  }
	  
	  /* Ensure start < end */
	  if(start > end) {
	      std::cout << "Start time must be earlier than end time." << std::endl;
	      exit(EXIT_FAILURE);
	  }
      }
  }

  /* Build a list of sensornames */
  std::list<std::string> sensors;
  for (int arg = optind; arg < argcReal; arg++) {
      sensors.push_back(argvReal[arg]);
  }

  if (myQuery->connect(host) == 0) {
      if (jobId.size() == 0) {
	  myQuery->doQuery(sensors, start, end);
      } else {
	  myQuery->dojobQuery(sensors, jobId);
      }
      myQuery->disconnect();
  }

  delete myQuery;

  return 0;
}
