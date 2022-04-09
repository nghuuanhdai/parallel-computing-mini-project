//================================================================================
// Name        : sensoraction.cpp
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of actions on the DCDB sensor configuration
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

#include <cstdlib>
#include <cstring>
#include <cinttypes>

#include <dcdb/sensorconfig.h>
#include <dcdb/unitconv.h>
#include "cassandra.h"
#include "metadatastore.h"

#include "sensoraction.h"

/*
 * Print the help for the SENSOR command
 */
void SensorAction::printHelp(int argc, char* argv[])
{
  /*            01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
  std::cout << "SENSOR command help" << std::endl << std::endl;
  std::cout << "The SENSOR command has the following options:" << std::endl;
  std::cout << "   PUBLISH <public name> <pattern>          - Make a sensor publicly available under" << std::endl;
  std::cout << "                                              <public name> comprising of all internal" << std::endl;
  std::cout << "                                              sensors matching the given <pattern>." << std::endl;
  std::cout << "   VCREATE <public name> <expr> <vsensorid> <t0> <freq>" << std::endl;
  std::cout << "                                            - Create a virtual public sensor that is" << std::endl;
  std::cout << "                                              visible as <public name> and evaluates" << std::endl;
  std::cout << "                                              <expr> starting at time t0 every <freq>" << std::endl;
  std::cout << "                                              nanoseconds. Cached values are stored" << std::endl;
  std::cout << "                                              under the unique <vsensorid>." << std::endl;
  std::cout << "   LIST                                     - List all public sensors." << std::endl;
  std::cout << "   LISTPUBLIC                               - Same as LIST, includes patterns." << std::endl;
  std::cout << "   SHOW <public name>                       - Show details for a given sensor." << std::endl;
  std::cout << "   SCALINGFACTOR <public name> <fac>        - Set scaling factor to <fac>." << std::endl;
  std::cout << "   UNIT <public name> <unit>                - Set unit to <unit>." << std::endl;
  std::cout << "   SENSORPROPERTY <public name> [<sensor property>,<sensor property>,...]" << std::endl;
  std::cout << "                                            - Get/Set sensor properties. Valid properties: " << std::endl;
  std::cout << "                                              integrable, monotonic, delta" << std::endl;
  std::cout << "   EXPRESSION <public name> <expr>          - Change expression of virt sensor." << std::endl;
  std::cout << "   TZERO <public name> <t0>                 - Change t0 of virt sensor." << std::endl;
  std::cout << "   INTERVAL <public name> <inter>           - Change interval of a sensor." << std::endl;
  std::cout << "   TTL <public name> <ttl>                  - Change time to live of a sensor." << std::endl;
  std::cout << "   OPERATIONS <public name> <operation>,<operation>,..." << std::endl;
  std::cout << "                                            - Set operations for the sensor (e.g., avg, stddev,...)." << std::endl;
  std::cout << "   CLEAROPERATIONS <public name>            - Remove all existing operations for the sensor." << std::endl;
  std::cout << "   CLEAROPERATIONSW <wildcard>              - Remove operations from sensors using a wildcard." << std::endl;
  std::cout << "   UNPUBLISH <public name>                  - Unpublish a sensor." << std::endl;
  std::cout << "   UNPUBLISHW <wildcard>                    - Unpublish sensors using a wildcard." << std::endl;
}

/*
 * Execute any of the SENSOR commands
 */
int SensorAction::executeCommand(int argc, char* argv[], int argvidx, const char* hostname)
{
  /* Independent from the command, we need to connect to the server */
  connection = new DCDB::Connection();
  connection->setHostname(hostname);
  if (!connection->connect()) {
      std::cerr << "Cannot connect to Cassandra database." << std::endl;
      return EXIT_FAILURE;
  }

  /* Check what we need to do (argv[argvidx] contains "SENSOR") */
  argvidx++;
  if (argvidx >= argc) {
      std::cout << "The SENSOR command needs at least one parameter." << std::endl;
      std::cout << "Run with 'HELP SENSOR' to see the list of possible SENSOR commands." << std::endl;
      goto executeCommandError;
  }

  if (strcasecmp(argv[argvidx], "PUBLISH") == 0) {
      /* PUBLISH needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "PUBLISH needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doPublishSensor(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "VCREATE") == 0) {
      /* VCREATE needs four more parameters */
      if (argvidx+5 >= argc) {
          std::cout << "VCREATE needs five more parameters!" << std::endl;
          goto executeCommandError;
      }
      doVCreateSensor(argv[argvidx+1], argv[argvidx+2], argv[argvidx+3], argv[argvidx+4], argv[argvidx+5]);
  }
  else if (strcasecmp(argv[argvidx], "LIST") == 0) {
      doList();
  }
  else if (strcasecmp(argv[argvidx], "LISTPUBLIC") == 0) {
      doListPublicSensors();
  }
  else if (strcasecmp(argv[argvidx], "SHOW") == 0) {
      if (argvidx+1 >= argc) {
          std::cout << "SHOW needs a sensor name as parameter!" << std::endl;
          goto executeCommandError;
      }
      doShow(argv[argvidx+1]);
  }
  else if (strcasecmp(argv[argvidx], "SCALINGFACTOR") == 0) {
      /* SCALINGFACTOR needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "SCALINGFACTOR needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doScalingfactor(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "UNIT") == 0) {
      /* UNIT needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "UNIT needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doUnit(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "SENSORPROPERTY") == 0) {
      /* SENSORPROPERTY needs at least one parameter */
      if (argvidx+1 >= argc) {
          std::cout << "SENSORPROPERTY needs at least one more parameter!" << std::endl;
          goto executeCommandError;
      }
      char* cmd = nullptr;
      if (argvidx+2 >= argc) {
	  cmd = argv[argvidx+2];
      }

      doSensorProperty(argv[argvidx+1], cmd);
  }
  else if (strcasecmp(argv[argvidx], "EXPRESSION") == 0) {
      /* EXPRESSION needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "EXPRESSION needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doExpression(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "TZERO") == 0) {
      /* TZERO needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "TZERO needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doTZero(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "INTERVAL") == 0) {
      /* INTERVAL needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "INTERVAL needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doInterval(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "TTL") == 0) {
      /* TTL needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "TTL needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doTTL(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "OPERATIONS") == 0) {
      /* OPERATIONS needs two more parameters */
      if (argvidx+2 >= argc) {
          std::cout << "OPERATIONS needs two more parameters!" << std::endl;
          goto executeCommandError;
      }
      doOperations(argv[argvidx+1], argv[argvidx+2]);
  }
  else if (strcasecmp(argv[argvidx], "CLEAROPERATIONS") == 0) {
      /* CLEAROPERATIONS needs one more parameter */
      if (argvidx+1 >= argc) {
          std::cout << "CLEAROPERATIONS needs one more parameter!" << std::endl;
          goto executeCommandError;
      }
      doClearOperations(argv[argvidx+1]);
  }
  else if (strcasecmp(argv[argvidx], "CLEAROPERATIONSW") == 0) {
      /* CLEAROPERATIONSW needs one more parameter */
      if (argvidx+1 >= argc) {
          std::cout << "CLEAROPERATIONSW needs one more parameter!" << std::endl;
          goto executeCommandError;
      }
      doClearOperationsByWildcard(argv[argvidx+1]);
  }
  else if (strcasecmp(argv[argvidx], "UNPUBLISH") == 0) {
      /* UNPUBLISH needs one more parameter */
      if (argvidx + 1 >= argc) {
          std::cout << "UNPUBLISH needs a parameter!" << std::endl;
          goto executeCommandError;
      }
      doUnPublishSensor(argv[argvidx + 1]);
  }
  else if (strcasecmp(argv[argvidx], "UNPUBLISHW") == 0) {
      /* UNPUBLISHW needs one more parameter */
      if (argvidx+1 >= argc) {
          std::cout << "UNPUBLISHW needs a parameter!" << std::endl;
          goto executeCommandError;
      }
      doUnPublishSensorsByWildcard(argv[argvidx+1]);
  }
  else {
      std::cout << "Invalid SENSOR command: " << argv[argvidx] << std::endl;
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
 * Publish a sensor
 */
void SensorAction::doPublishSensor(const char* publicName, const char* sensorPattern)
{
  DCDB::SCError err;

  DCDB::SensorConfig sensorConfig(connection);
  err = sensorConfig.publishSensor(publicName, sensorPattern);

  switch(err) {
  case DCDB::SC_INVALIDPATTERN:
    std::cout << "Invalid sensor pattern: " << sensorPattern << std::endl;
    break;
  case DCDB::SC_INVALIDPUBLICNAME:
    std::cout << "Invalid sensor public name: " << publicName << std::endl;
    break;
  case DCDB::SC_INVALIDSESSION:
    std::cout << "Invalid dcdb session." << std::endl;
    break;
  default:
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  }
}

/*
 * Create a virtual sensor
 */
void SensorAction::doVCreateSensor(const char* publicName, const char* expression, const char* vSensorId, const char* tZero, const char* interval)
{
  /* Convert tZero to TimeStamp */
  DCDB::TimeStamp tz(tZero);

  /* Convert interval to int64_t - sadly, there's no real C++ way of doing this */
  int64_t freq;
  if (sscanf(interval, "%" SCNd64, &freq) != 1) {
      std::cout << interval << " is not a valid number." << std::endl;
      return;
  }

  DCDB::SCError err;

  DCDB::SensorConfig sensorConfig(connection);
  err = sensorConfig.publishVirtualSensor(publicName, expression, vSensorId, tz, freq);

  switch(err) {
  case DCDB::SC_INVALIDEXPRESSION:
    // We should get a proper error message in the exception handler.
    //std::cout << "Invalid expression: " << expression << std::endl;
    break;
  case DCDB::SC_INVALIDVSENSORID:
    std::cout << "Invalid vsensorid: " << vSensorId << std::endl;
    std::cout << "Valid vsensorids are provided as 128 bits hex values." << std::endl;
    std::cout << "You may separate hex characters with slash characters for readability." << std::endl;
    std::cout << "Example: /00000000/deadbeef/cafeaffe/0000/0001" << std::endl;
    break;
  case DCDB::SC_EXPRESSIONSELFREF:
    std::cout << "Invalid expression: A virtual sensor must not reference itself." << std::endl;
    break;
  case DCDB::SC_INVALIDPUBLICNAME:
    std::cout << "Invalid sensor public name: " << publicName << std::endl;
    break;
  case DCDB::SC_INVALIDSESSION:
    std::cout << "Invalid dcdb session." << std::endl;
    break;
  default:
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  }
}

/*
 * List all published sensors by name
 */
void SensorAction::doList()
{
  DCDB::SensorConfig sensorConfig(connection);
  std::list<std::string> publicSensors;

  sensorConfig.getPublicSensorNames(publicSensors);
  for (std::list<std::string>::iterator it=publicSensors.begin(); it != publicSensors.end(); it++) {
      std::cout << (*it) << std::endl;
  }
}

/*
 * List all published sensors with name and pattern
 */
void SensorAction::doListPublicSensors()
{
  DCDB::SensorConfig sensorConfig(connection);
  std::list<DCDB::PublicSensor> publicSensors;

  sensorConfig.getPublicSensorsVerbose(publicSensors);
  for (std::list<DCDB::PublicSensor>::iterator it=publicSensors.begin(); it != publicSensors.end(); it++) {
      if ((*it).is_virtual) {
          std::cout << "(v) " << (*it).name << " : " << (*it).expression << std::endl;
      }
      else {
          std::cout << "    " << (*it).name << " : " << (*it).pattern << std::endl;
      }
  }
}

/*
 * Show the details for a given sensor TODO:Change Integrable
 */
void SensorAction::doShow(const char* publicName)
{
  DCDB::SensorConfig sensorConfig(connection);
  DCDB::PublicSensor publicSensor;
  DCDB::SCError err = sensorConfig.getPublicSensorByName(publicSensor, publicName);

  SensorMetadata sm;
  sm.setOperations(publicSensor.operations);
  
  switch (err) {
  case DCDB::SC_OK:
    if (!publicSensor.is_virtual) {
    std::cout << "Details for public sensor " << publicSensor.name << ":" << std::endl;
    std::cout << "Pattern: " << publicSensor.pattern << std::endl;
    }
    else {
        std::cout << "Details for virtual sensor " << publicSensor.name << ":" << std::endl;
        std::cout << "Expression: " << publicSensor.expression << std::endl;
        std::cout << "vSensorId: " << publicSensor.v_sensorid << std::endl;
        DCDB::TimeStamp tz(publicSensor.t_zero);
        std::cout << "T-Zero: " << tz.getString() << " (" << tz.getRaw() << ")" << std::endl;
    }
    std::cout << "Unit: " << publicSensor.unit << std::endl;
    std::cout << "Scaling factor: " << publicSensor.scaling_factor << std::endl;
    std::cout << "Operations: " << sm.getOperationsString() << std::endl;
    std::cout << "Interval: " << publicSensor.interval << std::endl;
    std::cout << "TTL: " << publicSensor.ttl << std::endl;
    std::cout << "Sensor Properties: ";
    if((publicSensor.sensor_mask & INTEGRABLE) == INTEGRABLE)
        std::cout << "Integrable ";
    if((publicSensor.sensor_mask & MONOTONIC) == MONOTONIC)
        std::cout << "Monotonic ";
    std::cout << std::endl;
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set the scaling factor for a sensor
 */
void SensorAction::doScalingfactor(const char* publicName, const char* factor)
{
  double f;
  std::string fact(factor);
  try {
      f = std::stod(fact);
  }
  catch(std::exception& e) {
      std::cout << factor << " is not a number." << std::endl;
      return;
  }

  DCDB::SensorConfig sensorConfig(connection);
  DCDB::PublicSensor publicSensor;
  DCDB::SCError err = sensorConfig.getPublicSensorByName(publicSensor, publicName);

  switch (err) {
  case DCDB::SC_OK:
    sensorConfig.setSensorScalingFactor(publicName, f);
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set the unit for a sensor.
 */
void SensorAction::doUnit(const char* publicName, const char* unit)
{
  DCDB::SensorConfig sensorConfig(connection);
  DCDB::PublicSensor publicSensor;
  DCDB::SCError err = sensorConfig.getPublicSensorByName(publicSensor, publicName);

  switch (err) {
  case DCDB::SC_OK:
    if (DCDB::UnitConv::fromString(unit) != DCDB::Unit_None) {
      sensorConfig.setSensorUnit(publicName, unit);
      sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    } else {
      std::cout << "Unknown unit: " << unit << std::endl;
    }
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set or unset the integrable flag for a sensor
 */
void SensorAction::doSensorProperty(const char* publicName, const char* cmd)
{
  DCDB::SensorConfig sensorConfig(connection);
  DCDB::PublicSensor publicSensor;
  DCDB::SCError err = sensorConfig.getPublicSensorByName(publicSensor, publicName);

  switch (err) {
  case DCDB::SC_OK:
      {
          uint64_t mask = publicSensor.sensor_mask;
          
	  if (cmd != nullptr) {
	      if(strcasestr(cmd,"integrable")) {
		  if ((mask & INTEGRABLE) != INTEGRABLE) {
		      mask = mask | INTEGRABLE;
		  }
	      } else if(strcasestr(cmd, "monotonic")) {
		  if ((mask & MONOTONIC) != MONOTONIC) {
		      mask = mask | MONOTONIC;
		  }
	      } else if(strcasestr(cmd, "delta")) {
		  if ((mask & DELTA) != DELTA) {
		      mask = mask | DELTA;
		  }
	      } else {
		  std::cout << "Unknown option: " << cmd << std::endl;
		  std::cout << "Valid sensor properties are: INTEGRABLE, MONOTONIC, DELTA" << std::endl;
	      }
			
	      if(mask != publicSensor.sensor_mask) {
		  sensorConfig.setSensorMask(publicName, mask);
		  sensorConfig.setPublishedSensorsWritetime(getTimestamp());
	      }
	  }
      
	  std::cout << publicSensor.name << ":";
	  if ((mask & INTEGRABLE) == INTEGRABLE) {
	      std::cout << " integrable";
	  } else if ((mask & MONOTONIC) == MONOTONIC) {
	      std::cout << " monotonic";
	  } else if ((mask & DELTA) == DELTA) {
	      std::cout << " delta";
	  }
	  std::cout << std::endl;

          break;
      }
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set the expression for a virtual sensor
 */
void SensorAction::doExpression(const char* publicName, const char* expression)
{
  DCDB::SensorConfig sensorConfig(connection);
  DCDB::SCError err = sensorConfig.setVirtualSensorExpression(publicName, expression);

  switch (err) {
  case DCDB::SC_OK:
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  case DCDB::SC_INVALIDSESSION:
    std::cout << "Invalid session!" << std::endl;
    break;
  case DCDB::SC_WRONGTYPE:
    std::cout << "Sensor " << publicName << " is not virtual!" << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set the T0 for a virtual sensor
 */
void SensorAction::doTZero(const char* publicName, const char* tZero)
{
  /* Convert tZero to TimeStamp */
  DCDB::TimeStamp tz(tZero);

  DCDB::SensorConfig sensorConfig(connection);
  DCDB::SCError err = sensorConfig.setVirtualSensorTZero(publicName, tz);

  switch (err) {
  case DCDB::SC_OK:
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  case DCDB::SC_INVALIDSESSION:
    std::cout << "Invalid session!" << std::endl;
    break;
  case DCDB::SC_WRONGTYPE:
    std::cout << "Sensor " << publicName << " is not virtual!" << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

/*
 * Set the interval for a sensor
 */
void SensorAction::doInterval(const char* publicName, const char *interval)
{
  /* Convert interval to int64_t - sadly, there's no real C++ way of doing this */
  int64_t freq;
  if (sscanf(interval, "%" SCNd64, &freq) != 1) {
      std::cout << interval << " is not a valid number." << std::endl;
      return;
  }

  DCDB::SensorConfig sensorConfig(connection);
  DCDB::SCError err = sensorConfig.setSensorInterval(publicName, freq);

  switch (err) {
  case DCDB::SC_OK:
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
    break;
  case DCDB::SC_UNKNOWNSENSOR:
    std::cout << "Unknown sensor name: " << publicName << std::endl;
    break;
  case DCDB::SC_INVALIDSESSION:
    std::cout << "Invalid session!" << std::endl;
    break;
  default:
    std::cout << "Internal error." << std::endl;
  }
}

void SensorAction::doTTL(const char* publicName, const char *ttl) {
    /* Convert ttl to int64_t - sadly, there's no real C++ way of doing this */
    int64_t ttl_int;
    if (sscanf(ttl, "%" SCNd64, &ttl_int) != 1) {
        std::cout << ttl << " is not a valid number." << std::endl;
        return;
    }

    DCDB::SensorConfig sensorConfig(connection);
    DCDB::SCError err = sensorConfig.setTimeToLive(publicName, ttl_int);

    switch (err) {
        case DCDB::SC_OK:
            sensorConfig.setPublishedSensorsWritetime(getTimestamp());
            break;
        case DCDB::SC_UNKNOWNSENSOR:
            std::cout << "Unknown sensor name: " << publicName << std::endl;
            break;
        case DCDB::SC_INVALIDSESSION:
            std::cout << "Invalid session!" << std::endl;
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void SensorAction::doOperations(const char* publicName, const char *operations)
{
    SensorMetadata sm;
    sm.setOperations(std::string(operations));
    
    DCDB::SensorConfig sensorConfig(connection);
    DCDB::SCError err = sensorConfig.setOperations(publicName, *sm.getOperations());
    
    switch (err) {
        case DCDB::SC_OK:
            sensorConfig.setPublishedSensorsWritetime(getTimestamp());
            break;
        case DCDB::SC_UNKNOWNSENSOR:
            std::cout << "Unknown sensor name: " << publicName << std::endl;
            break;
        case DCDB::SC_INVALIDSESSION:
            std::cout << "Invalid session!" << std::endl;
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void SensorAction::doClearOperations(const char* publicName)
{
    DCDB::SensorConfig sensorConfig(connection);
    DCDB::SCError err = sensorConfig.clearOperations(publicName);

    switch (err) {
        case DCDB::SC_OK:
            sensorConfig.setPublishedSensorsWritetime(getTimestamp());
            break;
        case DCDB::SC_UNKNOWNSENSOR:
            std::cout << "Unknown sensor name: " << publicName << std::endl;
            break;
        case DCDB::SC_INVALIDSESSION:
            std::cout << "Invalid session!" << std::endl;
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

void SensorAction::doClearOperationsByWildcard(const char* wildcard)
{
    DCDB::SensorConfig sensorConfig(connection);
    DCDB::SCError err = sensorConfig.clearOperationsByWildcard(wildcard);

    switch (err) {
        case DCDB::SC_OK:
            sensorConfig.setPublishedSensorsWritetime(getTimestamp());
            break;
        case DCDB::SC_INVALIDSESSION:
            std::cout << "Invalid session!" << std::endl;
            break;
        default:
            std::cout << "Internal error." << std::endl;
    }
}

/*
 * Unpublish a sensor
 */
void SensorAction::doUnPublishSensor(const char* publicName)
{
  DCDB::SensorConfig sensorConfig(connection);
  sensorConfig.unPublishSensor(publicName);
  sensorConfig.setPublishedSensorsWritetime(getTimestamp());
}

void SensorAction::doUnPublishSensorsByWildcard(const char* wildcard)
{
    DCDB::SensorConfig sensorConfig(connection);
    sensorConfig.unPublishSensorsByWildcard(wildcard);
    sensorConfig.setPublishedSensorsWritetime(getTimestamp());
}
