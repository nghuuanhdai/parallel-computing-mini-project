//================================================================================
// Name        : sensoroperations.cpp
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for sensor operations.
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

#include <limits>
#include <cmath>
#include "dcdb/sensoroperations.h"
#include "timestamp.h"

namespace DCDB {
    
void doubleToFraction(int64_t* num, int64_t* den, double number) {
    
    *den = 1;
    while((std::abs(number) - std::abs(floor(number)))) {
        number *= 10;
        *den *= 10;
    }
    *num = number;
}

/* Safe implementation of addition between 64-bit integers */
DCDB_OP_RESULT safeAdd(int64_t lh, int64_t rh, int64_t* result) {
    
    if ( (lh < 0.0) == (rh < 0.0)
        && std::abs(lh) > std::numeric_limits<int64_t>::max() - std::abs(rh) )
        return DCDB_OP_OVERFLOW;
    
    *result = (int64_t) lh + rh;
    
    return DCDB_OP_SUCCESS;
}

/* Safe multiplication for 64-bit integers */
DCDB_OP_RESULT safeMult(int64_t lh, int64_t rh, int64_t* result) {
    
    if (lh > 0 && rh > 0 && lh > std::numeric_limits<int64_t>::max() / rh) {
        *result = std::numeric_limits<int64_t>::max();
        return DCDB_OP_OVERFLOW;
    }
    if (lh < 0 && rh > 0 && lh < std::numeric_limits<int64_t>::min() / rh) {
        *result = std::numeric_limits<int64_t>::min();
        return DCDB_OP_OVERFLOW;
    }
    if (lh > 0 && rh < 0 && rh < std::numeric_limits<int64_t>::min() / lh) {
        *result = std::numeric_limits<int64_t>::min();
        return DCDB_OP_OVERFLOW;
    }
    if (lh < 0 && rh < 0 && (lh <= std::numeric_limits<int64_t>::min()
                             || rh <= std::numeric_limits<int64_t>::min()
                             || -lh > std::numeric_limits<int64_t>::max() / -rh)) {
        *result = std::numeric_limits<int64_t>::max();
        return DCDB_OP_OVERFLOW;
    }
    *result = lh*rh;
    
    return DCDB_OP_SUCCESS;
}
    
/* Scale function for double  */
DCDB_OP_RESULT scale(int64_t* result, double scalingFactor, double baseScalingFactor) {
    
    double factor = scalingFactor * baseScalingFactor;
    int64_t product = *result;
    
    if(std::abs(factor) - std::abs(floor(factor))) {
        
        int64_t num = 0;
        int64_t denom = 0;
        
        doubleToFraction(&num, &denom, factor);
        
        DCDB_OP_RESULT multResult = safeMult(num, *result, &product);
        
        if(multResult != DCDB_OP_SUCCESS) {
            *result = product;
            return multResult;
        }
        
        *result = product / denom;
        return DCDB_OP_SUCCESS;
    }
        
    return safeMult((int64_t) factor, product, result);
};

/* Safe delta function */
DCDB_OP_RESULT delta(int64_t lh, int64_t rh, int64_t* result) {
    
    //TODO: Need to address overflow for monotonic sensor data, e.g., energy reaching max value.
    //Maybe need to add another field to sensorconfig ("monotonic")?
    //Or need to pass the maximum value detectable by the sensor?
    
    *result = lh - rh;
    
    return DCDB_OP_SUCCESS;
};

DCDB_OP_RESULT delta(uint64_t lh, uint64_t rh, int64_t* result) {
	
	//TODO: Need to address overflow for monotonic sensor data, e.g., energy reaching max value.
	//Maybe need to add another field to sensorconfig ("monotonic")?
	//Or need to pass the maximum value detectable by the sensor?
	
	*result = lh - rh;
	
	return DCDB_OP_SUCCESS;
};

/* Safe implementation of a derivative */
DCDB_OP_RESULT derivative(int64_t lhx, int64_t rhx, uint64_t lht, uint64_t rht, int64_t* result, DCDB::Unit unit) {
    
    int64_t dx = 0;
    uint64_t dt = 0;
    
    DCDB_OP_RESULT deltaResult = delta(lhx, rhx, &dx);
    
    if(deltaResult != DCDB_OP_SUCCESS) {
        *result = dx;
        return deltaResult;
    }
    
    dt = (int64_t) lht - rht;
    
    if(dt == 0)
        return DCDB_OP_DIVISION_BY_ZERO; //Presumably it's always != 0...
    
    uint64_t tsDivisor = 1;
    switch (unit) {
	case DCDB::Unit_Joules:
	case DCDB::Unit_KiloJoules:
	case DCDB::Unit_MegaJoules:
	    tsDivisor = 1000000000ll;
	    break;
	case DCDB::Unit_WattHours:
	case DCDB::Unit_KiloWattHours:
	case DCDB::Unit_MegaWattHours:
	    tsDivisor = 1000000000ll * 3600;
	    break;
	default:
	    break;
    }
    
    if (tsDivisor == 1) {
	*result = dx / dt;
    } else {
	*result = dx / ((double) dt / tsDivisor);
    }
    
    return DCDB_OP_SUCCESS;
};

/* Safe implementation of an integral */
DCDB_OP_RESULT integral(int64_t x, uint64_t lht, uint64_t rht, int64_t* result, DCDB::Unit unit) {
    int64_t dt = (int64_t) lht - rht;
    
    uint64_t tsDivisor = 1;
    switch (unit) {
	case DCDB::Unit_Watt:
	case DCDB::Unit_KiloWatt:
	case DCDB::Unit_MegaWatt:
	    tsDivisor = 1000000000ll;
	    break;
	default:
	    break;
    }

    if (tsDivisor == 1) {
	return safeMult(x, dt, result);
    } else {
	if (dt > 1000000000ll) {
	    dt/= 1000000;
	    tsDivisor/= 1000000;
	} else if (dt > 1000000ll) {
	    dt/= 1000;
	    tsDivisor/= 1000;
	}
	DCDB_OP_RESULT rc = safeMult(x, dt, result);
	*result = *result / tsDivisor;
	return rc;
    }
};

DCDB_OP_RESULT rate(int64_t x, uint64_t lht, uint64_t rht, int64_t* result) {
    int64_t dt = NS_TO_S(((int64_t) lht - rht));

    *result = x / dt;
    return DCDB_OP_SUCCESS;
};

} /* end DCDB namespace */
