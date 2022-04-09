//================================================================================
// Name        : useraction.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header of factory for classes depending on the user chosen action
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


#include <memory>

#ifndef USERACTION_H
#define USERACTION_H

class UserAction
{
public:
  virtual ~UserAction() {}
  virtual void printHelp(int argc, char* argv[]) = 0;
  virtual int executeCommand(int argc, char* argv[], int argvidx, const char* hostname) = 0;
};

class UserActionFactory
{
public:
  static std::shared_ptr<UserAction> getAction(const char *actionStr);
};

#endif // USERACTION_H
