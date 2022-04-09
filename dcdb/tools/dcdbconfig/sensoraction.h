//================================================================================
// Name        : sensoraction.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header for modifications on the DCDB sensor configuration
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
#include "cassandra.h"

#include "useraction.h"
#include "timestamp.h"

#ifndef SENSORACTION_H
#define SENSORACTION_H

class SensorAction : public UserAction
{
public:
  void printHelp(int argc, char* argv[]);
  int executeCommand(int argc, char* argv[], int argvidx, const char* hostname);

  SensorAction() {};
  virtual ~SensorAction() {};

protected:
  DCDB::Connection* connection;
  void doPublishSensor(const char* publicName, const char* sensorPattern);
  void doVCreateSensor(const char* publicName, const char* expression, const char* vSensorId, const char* tZero, const char* interval);
  void doList();
  void doListPublicSensors();
  void doShow(const char* publicName);
  void doScalingfactor(const char* publicName, const char* factor);
  void doUnit(const char* publicName, const char* unit);
  void doSensorProperty(const char* publicName, const char* cmd);
  void doExpression(const char* publicName, const char* expression);
  void doTZero(const char* publicName, const char* tZero);
  void doInterval(const char* publicName, const char *interval);
  void doTTL(const char* publicName, const char *ttl);
  void doOperations(const char* publicName, const char *operations);
  void doClearOperations(const char* publicName);
  void doClearOperationsByWildcard(const char* wildcard);
  void doUnPublishSensor(const char* publicName);
  void doUnPublishSensorsByWildcard(const char* wildcard);
};

#endif
