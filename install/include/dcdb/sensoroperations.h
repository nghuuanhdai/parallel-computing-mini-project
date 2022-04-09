//================================================================================
// Name        : sensoroperations.h
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for sensor operations.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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
 * @file sensoroperations.h
 *
 * @ingroup libdcdb
 */

#include <stdio.h>
#include <cstdint>
#include <cinttypes>
#include <cstdlib>
#include "unitconv.h"

#ifndef sensoroperations_h
#define sensoroperations_h

namespace DCDB {
    
    typedef enum {
        DCDB_OP_SUCCESS,
        DCDB_OP_OVERFLOW,
        DCDB_OP_DIVISION_BY_ZERO,
        DCDB_OP_UNKNOWN,
    } DCDB_OP_RESULT;

    
    DCDB_OP_RESULT safeAdd(int64_t lh, int64_t rh, int64_t* result);
    DCDB_OP_RESULT safeMult(int64_t lh, int64_t rh, int64_t* result);
    DCDB_OP_RESULT safeMult(double lh, double rh, int64_t* result);
    DCDB_OP_RESULT scale(int64_t* result, double scalingFactor, double baseScalingFactor);
    DCDB_OP_RESULT delta(int64_t lh, int64_t rh, int64_t* result);
    DCDB_OP_RESULT delta(uint64_t lh, uint64_t rh, int64_t* result);
    DCDB_OP_RESULT derivative(int64_t lhx, int64_t rhx, uint64_t lht, uint64_t rht, int64_t* result, DCDB::Unit unit = DCDB::Unit_None);
    DCDB_OP_RESULT integral(int64_t x, uint64_t lht, uint64_t rht, int64_t* result, DCDB::Unit unit = DCDB::Unit_None);
    DCDB_OP_RESULT rate(int64_t x, uint64_t lht, uint64_t rht, int64_t* result);

} /* End of namespace DCDB */

#endif /* sensoroperations_h */
