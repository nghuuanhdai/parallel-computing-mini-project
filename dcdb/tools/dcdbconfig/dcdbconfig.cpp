//================================================================================
// Name        : dcdbconfig.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Main file of the dcdbconfig command line utility
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
#include <cstring>

#include <unistd.h>

#include "dcdb/version.h"
#include "version.h"
#include "cassandra.h"
#include "useraction.h"



void usage(int argc, char* argv[])
{
  std::cout << "Usage: " << argv[0] << " [-h host] <command> [<arguments> ... ]" << std::endl << std::endl;
  std::cout << "Valid commands are: " << std::endl;
  std::cout << "    HELP <command name> - print help for given command" << std::endl;
  std::cout << "    DB                  - perform low-level database functions" << std::endl;
  std::cout << "    SENSOR              - list and configure sensors" << std::endl;
  std::cout << "    JOB                 - list and show job information" << std::endl;
}

int main(int argc, char* argv[])
{
  std::cout << "dcdbconfig " << VERSION << " (libdcdb " << DCDB::Version::getVersion() << ")" << std::endl << std::endl;

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
      argvReal[0] = strdup("dcdbconfig"); /* dummy to make consistent with command line invocation */

      char *token = strtok(getenv("QUERY_STRING"), "&");
      while (token) {
          argcReal++;
          argvReal = (char**)realloc((void*)argvReal, argcReal*sizeof(char*));
          /* FIXME: should check if argvReal is NULL */
          argvReal[argcReal-1] = token;
          token = strtok(NULL, "&");
      }
  }

  /* Check command line parameters */
  if (argcReal < 2) {
      usage(argcReal, argvReal);
      exit(EXIT_FAILURE);
  }

  int ret;
  const char *host = getenv("DCDB_HOSTNAME");
  if (!host) {
      host = "localhost";
  }

  while ((ret=getopt(argcReal, argvReal, "+h:"))!=-1) {
      switch(ret) {
          case 'h':
              host = optarg;
              break;
          default:
              usage(argcReal, argvReal);
              exit(EXIT_FAILURE);
      }
  }

  if (optind >= argcReal) {
      std::cout << "Missing command!" << std::endl;
      usage(argcReal, argvReal);
      exit(EXIT_FAILURE);
  }

  /* Process user command */
  if (strcasecmp(argvReal[optind], "help") == 0) {
      /* Help is special: either we do general usage or we trigger the class factory and run the printHelp() function */
      if (optind + 1 >= argcReal) {
          usage (argcReal, argvReal);
      }
      else {
          auto action = UserActionFactory::getAction(argvReal[optind+1]);
          if (action) {
              action->printHelp(argcReal, argvReal);
          }
          else {
              std::cout << "Cannot provide help for unknown command: " << argvReal[optind+1] << std::endl;
              exit(EXIT_FAILURE);
          }
      }
  }
  else {
      /* If the command is not help, we try to instantiate the respective class through the factory and process the command */
      auto action = UserActionFactory::getAction(argvReal[optind]);
      if (action) {
          return action->executeCommand(argcReal, argvReal, optind, host);
      }
      else {
          std::cout << "Unknwon command: " << argvReal[1] << std::endl;
          usage(argcReal, argvReal);
          exit(EXIT_FAILURE);
      }
  }

  /* Shouldn't fall through here */
  return 0;
}
