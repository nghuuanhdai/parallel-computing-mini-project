//================================================================================
// Name        : unitconv.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for conversion of units in libdcdb.
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

#include "dcdb/unitconv.h"

#include <list>

using namespace DCDB;

/*
  Undefined
  Unit_None,

  Base units
  Unit_Meter,
  Unit_Second,
  Unit_Ampere,
  Unit_Kelvin,
  Unit_Watt,
  Unit_Volt,

  Others
  Unit_Celsius,
  Unit_Fahrenheit,

  1e-2
  Unit_CentiMeter,
  Unit_CentiCelsius,

  1e-3
  Unit_MilliMeter,
  Unit_MilliSecond,
  Unit_MilliAmpere,
  Unit_MilliKelvin,
  Unit_MilliWatt,
  Unit_MilliVolt,
  Unit_MilliCelsius,

  1e-6
  Unit_MicroMeter,
  Unit_MicroSecond,
  Unit_MicroAmpere,
  Unit_MicroKelvin,
  Unit_MicroWatt,
  Unit_MicroVolt,
  Unit_MicroCelsius,
*/

typedef struct {
  Unit unit;
  std::string str;
  Unit baseUnit;
  int64_t baseConvFactor;   /* a baseConFactor of -1000 indicates that the value has to be divided by 1000 to convert to base, +1000 means to multiply with 1000 */
  double baseConvOffset;   /* Conversion always first multiplies/divides, then adds the offset */
} ConversionTableEntry;

/*
 * Note: since we are using negative offsets into this table to indicate reverse conversion,
 * the first entry (index 0) in the table must be an identity conversion.
 */
ConversionTableEntry conversionTable[] = {
   { Unit_None, "none", Unit_None, 1, 0},
   { Unit_Meter, "m", Unit_Meter, 1, 0 },
   { Unit_CentiMeter, "cm", Unit_Meter, -100, 0},
   { Unit_MilliMeter, "mm", Unit_Meter, -1000, 0 },
   { Unit_MicroMeter, "um", Unit_Meter, -1000000, 0 },
   { Unit_Second, "s", Unit_Second, 1, 0 },
   { Unit_MilliSecond, "ms", Unit_Second, -1000, 0 },
   { Unit_MicroSecond, "us", Unit_Second, -1000000, 0 },
   { Unit_Ampere, "A", Unit_Ampere, 1, 0 },
   { Unit_MilliAmpere, "mA", Unit_Ampere, -1000, 0 },
   { Unit_MicroAmpere, "uA", Unit_Ampere, -1000000, 0 },
   { Unit_Kelvin, "K", Unit_Kelvin, 1, 0 },
   { Unit_MilliKelvin, "mK", Unit_Kelvin, -1000, 0 },
   { Unit_MicroKelvin, "uK", Unit_Kelvin, -1000000, 0 },
   { Unit_Watt, "W", Unit_Watt, 1, 0 },
   { Unit_MilliWatt, "mW", Unit_Watt, -1000, 0 },
   { Unit_MicroWatt, "uW", Unit_Watt, -1000000, 0 },
   { Unit_KiloWatt, "kW", Unit_Watt, 1000, 0 },
   { Unit_MegaWatt, "MW", Unit_Watt, 1000000, 0 },
   { Unit_Volt, "V", Unit_Volt, 1, 0 },
   { Unit_MilliVolt, "mV", Unit_Volt, -1000, 0 },
   { Unit_MicroVolt, "uV", Unit_Volt, -1000000, 0 },
   { Unit_Celsius, "C", Unit_Celsius, 1, 0 },
   { Unit_DeciCelsius, "dC", Unit_Celsius, -10, 0 },
   { Unit_CentiCelsius, "cC", Unit_Celsius, -100, 0},
   { Unit_MilliCelsius, "mC", Unit_Celsius, -1000, 0 },
   { Unit_MicroCelsius, "uC", Unit_Celsius, -1000000, 0},
   { Unit_Hertz, "Hz", Unit_Hertz, 1, 0},
   { Unit_KiloHertz, "kHz", Unit_Hertz, 1000, 0},
   { Unit_MegaHertz,"MHz", Unit_Hertz, 1000000, 0},
   { Unit_Joules, "J", Unit_Joules, 1, 0},
   { Unit_MilliJoules, "mJ", Unit_Joules, -1000, 0},
   { Unit_MicroJoules, "uJ", Unit_Joules, -1000000, 0},
   { Unit_KiloJoules, "kJ", Unit_Joules, 1000, 0},
   { Unit_MegaJoules, "MJ", Unit_Joules, 1000000, 0},
   { Unit_WattHours, "Wh", Unit_WattHours, 1, 0},
   { Unit_KiloWattHours, "kWh", Unit_WattHours, 1000, 0},
   { Unit_MegaWattHours, "MWh", Unit_WattHours, 1000000, 0},
   { Unit_CubicMetersPerSecond, "m3/s", Unit_CubicMetersPerHour, 3600, 0},
   { Unit_CubicMetersPerHour, "m3/h", Unit_LitersPerHour, 1000, 0},
   { Unit_LitersPerSecond, "l/s", Unit_LitersPerHour, 3600, 0},
   { Unit_LitersPerHour, "l/h", Unit_LitersPerHour, 1, 0},
   { Unit_Bar, "Bar", Unit_Bar, 1, 0},
   { Unit_MilliBar, "mBar", Unit_Bar, -1000, 0},
   { Unit_Percent, "%", Unit_Percent, 1, 0},
   { Unit_Celsius, "C", Unit_MilliKelvin, 1000, 273150 },
   { Unit_Fahrenheit, "F", Unit_MilliKelvin, 555, 255116 },
   { Unit_WattHours, "Wh", Unit_Joules, 3600, 0},
   { Unit_Pascal, "Pa", Unit_Bar, -10000, 0},
};

#define ConversionTableSize ((int)(sizeof(conversionTable)/sizeof(ConversionTableEntry)))

Unit UnitConv::fromString(std::string unit) {
  /* Walk the table to find the string */
  for (int i=0; i<ConversionTableSize; i++) {
      if ((unit.length() == conversionTable[i].str.length()) && (conversionTable[i].str.compare(unit) == 0)) {
          return conversionTable[i].unit;
      }
  }

  /* No match --> return None */
  return Unit_None;
}

std::string UnitConv::toString(Unit unit) {
  /* Walk the table to find the unit */
  for (int i=0; i<ConversionTableSize; i++) {
      if (conversionTable[i].unit == unit) {
          return conversionTable[i].str;
      }
  }

  return std::string("");
}

static bool dfs(std::list<int>& chain, Unit in, Unit out) {
  /* If there is nothing to convert, we don't have to search */
  if (in == out) {
      return true;
  }

  /* If the search depth exceeds the number of entries in the conversion table, abort */
  if (chain.size() > ConversionTableSize) {
      return false;
  }

  for (int i=0; i<ConversionTableSize; i++) {
      if ((conversionTable[i].unit == in) && (conversionTable[i].unit != conversionTable[i].baseUnit)) {
          /* Make sure this is not just the reverse of the last operation */
          if ((chain.size() == 0) || (chain.back() != -i)) {
              /* If this conversion leads us to the result, we're done */
              chain.push_back(i);
              if (conversionTable[i].baseUnit == out) {
                  return true;
              }
              /* If this conversion can handle our in unit, search for possible follow-up conversions */
              if (dfs(chain, conversionTable[i].baseUnit, out)) {
                  return true;
              }
              else {
                  chain.pop_back();
              }
          }
      }
      if (conversionTable[i].baseUnit == in && (conversionTable[i].unit != conversionTable[i].baseUnit)) {
          if ((chain.size() == 0) || (chain.back() != i)) {
              chain.push_back(-i);
              if (conversionTable[i].unit == out) {
                  return true;
              }
              if (dfs(chain, conversionTable[i].unit, out)) {
                  return true;
              }
              else {
                  chain.pop_back();
              }
          }
      }
  }
  return false;
}

bool UnitConv::convert(int64_t& value, Unit in, Unit out) {
  /* Do a depth-first search to find a suitable conversion path */
  std::list<int> convChain;
  if (dfs(convChain, in, out))  {
      /* Convert */
      double factor = 1;
      double offset = 0;

      for (std::list<int>::iterator it = convChain.begin(); it != convChain.end(); it++) {
          double newFact = (*it) < 0 ? -conversionTable[-(*it)].baseConvFactor : conversionTable[*it].baseConvFactor;
          double  newOff = (*it) < 0 ? -conversionTable[-(*it)].baseConvOffset : conversionTable[*it].baseConvOffset;
          double absFactor = factor > 0 ? factor : -factor;
          double absNewFact = newFact > 0 ? newFact : -newFact;

          /* Do offset calculation */
          if (newFact > 0) {
              offset = (offset * newFact) + newOff;
          }
          else {
              offset = (offset + newOff) / (-newFact);
          }

          /* Do factor calculation */
          if (factor > 0 && newFact > 0) {
              factor = factor * newFact;
          }
          else if (factor < 0 && newFact < 0) {
              factor = -(factor * newFact);
          }
          else if (absFactor == absNewFact) {
              factor = 1;
          }
          else if (absFactor > absNewFact && factor > 0) {
              factor = (absFactor / absNewFact);
          }
          else if (absFactor < absNewFact && newFact > 0) {
              factor = (absNewFact / absFactor);
          }
          else if (absFactor > absNewFact && factor < 0) {
              factor = -(absFactor / absNewFact);
          }
          else if (absFactor < absNewFact && newFact < 0) {
              factor = -(absNewFact / absFactor);
          }
          else {
              return false;
          }
      }

      /* Calculate final result */
      if (factor > 0) {
          value = (value * factor) + (int64_t)offset;
      }
      else {
          value = (value / (-factor)) + (int64_t)offset;
      }

      return true;
  }

  return false;
}

bool UnitConv::convert(double& value, Unit in, Unit out) {
  /* Do a depth-first search to find a suitable conversion path */
  std::list<int> convChain;
  if (dfs(convChain, in, out))  {
      /* Convert */
      double factor = 1;
      double offset = 0;

      for (std::list<int>::iterator it = convChain.begin(); it != convChain.end(); it++) {
          double newFact = (*it) < 0 ? -conversionTable[-(*it)].baseConvFactor : conversionTable[*it].baseConvFactor;
          double  newOff = (*it) < 0 ? -conversionTable[-(*it)].baseConvOffset : conversionTable[*it].baseConvOffset;
          double absFactor = factor > 0 ? factor : -factor;
          double absNewFact = newFact > 0 ? newFact : -newFact;

          /* Do offset calculation */
          if (newFact > 0) {
              offset = (offset * newFact) + newOff;
          }
          else {
              offset = (offset + newOff) / (-newFact);
          }

          /* Do factor calculation */
          if (factor > 0 && newFact > 0) {
              factor = factor * newFact;
          }
          else if (factor < 0 && newFact < 0) {
              factor = -(factor * newFact);
          }
          else if (absFactor == absNewFact) {
              factor = 1;
          }
          else if (absFactor > absNewFact && factor > 0) {
              factor = (absFactor / absNewFact);
          }
          else if (absFactor < absNewFact && newFact > 0) {
              factor = (absNewFact / absFactor);
          }
          else if (absFactor > absNewFact && factor < 0) {
              factor = -(absFactor / absNewFact);
          }
          else if (absFactor < absNewFact && newFact < 0) {
              factor = -(absNewFact / absFactor);
          }
          else {
              return false;
          }
      }

      /* Calculate final result */
      if (factor > 0) {
          value = (value * factor) + (int64_t)offset;
      }
      else {
          value = (value / (-factor)) + (int64_t)offset;
      }

      return true;
  }

  return false;
}

bool UnitConv::convertToBaseUnit(double& value, Unit from) {
   Unit to = from;
   for (int i=0; i<ConversionTableSize; i++) {
      if (conversionTable[i].unit == from) {
	 to = conversionTable[i].baseUnit;
	 break;
      }
   }

   if (to != from) {
      return convert(value, from, to);
   } else {
      return true;
   }
}

