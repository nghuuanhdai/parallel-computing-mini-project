//================================================================================
// Name        : unitconv.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for conversion of units in libdcdb.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//================================================================================

/**
 * @file unitconv.h
 *
 * @ingroup libdcdb
 */

#include <cstdint>
#include <string>

#ifndef DCDB_UNITCONV_H
#define DCDB_UNITCONV_H

namespace DCDB {

/**
 * Enum type describing all units supported by the libdcdb's unit conversion scheme.
 */
typedef enum {
  /* Undefined */
  Unit_None,

  /* Base units */
  Unit_Meter,
  Unit_Second,
  Unit_Ampere,
  Unit_Kelvin,
  Unit_Watt,
  Unit_Volt,
  Unit_Hertz,
  Unit_Joules,
  Unit_WattHours,
  Unit_CubicMetersPerSecond,
  Unit_CubicMetersPerHour,
  Unit_Bar,
  Unit_Pascal,

  /* Others */
  Unit_Celsius,
  Unit_Fahrenheit,
  Unit_Percent,

  /* 1e3 */
  Unit_KiloHertz,
  Unit_KiloWatt,
  Unit_KiloJoules,
  Unit_KiloWattHours,

  /* 1e6 */
  Unit_MegaHertz,
  Unit_MegaWatt,
  Unit_MegaJoules,
  Unit_MegaWattHours,

  /* 1e9 */
  Unit_GigaHertz,

  /* 1e-1 */
  Unit_DeciCelsius,

  /* 1e-2 */
  Unit_CentiMeter,
  Unit_CentiCelsius,

  /* 1e-3 */
  Unit_MilliMeter,
  Unit_MilliSecond,
  Unit_MilliAmpere,
  Unit_MilliKelvin,
  Unit_MilliWatt,
  Unit_MilliJoules,
  Unit_MilliVolt,
  Unit_LitersPerSecond,
  Unit_LitersPerHour,
  Unit_MilliCelsius,
  Unit_MilliBar,

  /* 1e-6 */
  Unit_MicroMeter,
  Unit_MicroSecond,
  Unit_MicroAmpere,
  Unit_MicroKelvin,
  Unit_MicroWatt,
  Unit_MicroJoules,
  Unit_MicroVolt,
  Unit_MicroCelsius,
} Unit;

/**
 * This class provides an interface to the unit conversion support of libdcdb.
 */
class UnitConv
{
public:
  static Unit fromString(std::string unit);
  static std::string toString(Unit unit);
  static bool convert(int64_t& value, Unit from, Unit to);
  static bool convert(double& value, Unit from, Unit to);
  static bool convertToBaseUnit(double& value, Unit from);
};

} /* End of namespace DCDB */

#endif /* DCDB_UNITCONV_H */
