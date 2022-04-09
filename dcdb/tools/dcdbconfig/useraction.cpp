//================================================================================
// Name        : useraction.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Factory for classes depending on the user chosen action
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

#include "useraction.h"
#include "sensoraction.h"
#include "dbaction.h"
#include "jobaction.h"

#include <cstring>
#include <memory>

/*
 * This function acts as a class factory to return the
 * appropriate class handling a given type of action
 * (e.g. "sensors")
 */
std::shared_ptr<UserAction> UserActionFactory::getAction(const char *actionStr)
{
  UserAction *action = nullptr;

  if (strcasecmp(actionStr, "sensor") == 0) {
      action = new SensorAction();
  }
  else if (strcasecmp(actionStr, "db") == 0) {
      action = new DBAction();
  }
  else if (strcasecmp(actionStr, "job") == 0) {
      action = new JobAction();
  }

  if (action != nullptr) {
      return std::shared_ptr<UserAction>(action);
  }
  else {
      return nullptr;
  }
}
