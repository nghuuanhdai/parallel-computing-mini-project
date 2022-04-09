//================================================================================
// Name        : dcdbunitconv.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Command line utility for testing the DCDB's unit conversion
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

/* C standard headers */
#include <cstdint>
#include <cinttypes>
#include <cstring>

/* Custom headers */
#include "dcdb/unitconv.h"
#include "dcdb/version.h"
#include "version.h"

void usage()
{
  std::cout
    << "Usage: dcdbunitconv [-f] <value> <from> <to>" << std::endl
    << "   where <value> is a integer value" << std::endl
    << "         <from>  is the source unit" << std::endl
    << "         <to>    is the target unit" << std::endl
    << "         -f      enables floating-point output" << std::endl;
}

int main(int argc, const char* argv[])
{
  std::cout << "dcdbunitconv " << VERSION << " (libdcdb " << DCDB::Version::getVersion() << ")" << std::endl << std::endl;

  /* Check command line */
  if (argc < 4) {
      usage();
      return 1;
  }

  /* Parse command line */
  int64_t ivalue;
  double fvalue;

  DCDB::Unit from, to;
  bool fp = false;
  int argshift = 0;

  if ((strlen(argv[1]) == 2) && (strcmp("-f", argv[1]) == 0)) {
      fp = true;
      if (argc < 5) {
          usage();
          return 1;
      }
      argshift = 1;
  }

  if (fp) {
      if (sscanf(argv[1+argshift], "%lf", &fvalue) != 1) {
          std::cout << "Cannot interpret " << argv[1+argshift] << std::endl;
          return 2;
      }
  }
  else {
      if (sscanf(argv[1+argshift], "%" SCNd64, &ivalue) != 1) {
          std::cout << "Cannot interpret " << argv[1+argshift] << std::endl;
          return 2;
      }
  }

  from = DCDB::UnitConv::fromString(argv[2+argshift]);
  if (from == DCDB::Unit_None) {
      std::cout << "No known unit: " << argv[2+argshift] << std::endl;
      return 3;
  }
  to   = DCDB::UnitConv::fromString(argv[3+argshift]);
  if (to == DCDB::Unit_None) {
      std::cout << "No known unit: " << argv[3+argshift] << std::endl;
      return 4;
  }

  /* Run conversion */
  if (fp) {
      if (DCDB::UnitConv::convert(fvalue, from, to)) {
          std::cout << fvalue << std::endl;
      }
      else {
          std::cout << "Cannot convert from " << DCDB::UnitConv::toString(from) << " to " << DCDB::UnitConv::toString(to) << std::endl;
          return 5;
      }
  }
  else {
      if (DCDB::UnitConv::convert(ivalue, from, to)) {
          std::cout << ivalue << std::endl;
      }
      else {
          std::cout << "Cannot convert from " << DCDB::UnitConv::toString(from) << " to " << DCDB::UnitConv::toString(to) << std::endl;
          return 5;
      }
  }

  return 0;
}
