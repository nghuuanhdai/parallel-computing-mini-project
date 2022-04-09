//
//  LenovoXCC.hpp
//  dcdb
//
//  Created by Ott, Michael on 21.08.19.
//  Copyright © 2019 LRZ. All rights reserved.
//

#ifndef LenovoXCC_hpp
#define LenovoXCC_hpp

#include "IPMIHost.h"
#include "cacheentry.h"
#include <cstdint>

class LenovoXCC {

      public:
	LenovoXCC(IPMIHost *host);
	virtual ~LenovoXCC();

	int      getDatastorePower(std::vector<reading_t> &readings);
	int      getSingleEnergy(reading_t &reading);
	int      getBulkPower(std::vector<reading_t> &readings);
	int      getBulkEnergy(std::vector<reading_t> &readings);

	int      getGeneralDrift();
	int      getDatastoreDrift();

      private:
	uint64_t extractTimestamp(const uint8_t *buf);
	int      readSingleEnergyRaw(uint64_t &timeStamp, uint64_t &energy);
//	int      getGeneralDrift();
	int      openDatastore();
	int      closeDatastore();
	uint64_t readDatastoreTimestamp();
	int      readDatastoreRange(uint32_t offset, uint16_t count, uint8_t *buf, uint16_t bufLen);
//	int      getDatastoreDrift();

	IPMIHost *_host;
	uint32_t  _handle;
	int64_t   _generalDrift;
	int64_t   _datastoreDrift;

};
#endif /* LenovoXCC_hpp */
