//
//  LenovoXCC.cpp
//  dcdb
//
//  Created by Ott, Michael on 21.08.19.
//  Copyright © 2019 LRZ. All rights reserved.
//

#include "LenovoXCC.h"
#include "timestamp.h"
#include <cstring>
#include <iostream>

LenovoXCC::LenovoXCC(IPMIHost *host) {
	_host = host;
	_handle = 0;
	_generalDrift = 0;
	_datastoreDrift = 0;
}

LenovoXCC::~LenovoXCC() {}

int LenovoXCC::getDatastorePower(std::vector<reading_t> &readings) {
	if (_datastoreDrift || getDatastoreDrift() == 0) {
		if (openDatastore() == 0) {
			uint64_t ts1, ts2;
			uint8_t buf[256];
			int i = 0;

			if ((ts1 = readDatastoreTimestamp()) > 0) {
				ts2 = 0;
				readings.resize(3000);
				while (ts1 != ts2) {
					i = 0;
					for (int offset=16; offset < 6016; offset += 200) {
						int len = readDatastoreRange(offset, 200, buf, sizeof(buf));
						if (len >= 0) {
							uint16_t  n = (buf[5] | buf[6] << 8) / 2;
							uint16_t *buf16 = (uint16_t *)&buf[7];
							for (int j=0; j<n; j++) {
								readings[i].timestamp = ts1 + _datastoreDrift + MS_TO_NS(i * 10);
								readings[i].value = buf16[j];
								i++;
							}
						}
					}
					// Check whether the timestamp changed during readDatastorePower() and re-read if it did
					ts2 = readDatastoreTimestamp();
				}
			}
			closeDatastore();
			return i != 3000;
		}
	}
	return 1;
}

int LenovoXCC::getSingleEnergy(reading_t &reading) {
	if (_generalDrift || getGeneralDrift() == 0) {
		uint64_t ts, energy;
		if (readSingleEnergyRaw(ts, energy) == 0) {
			reading.timestamp = ts + _generalDrift;
			reading.value = energy;
			return 0;
		}
	}
	return 1;
}

int LenovoXCC::getBulkPower(std::vector<reading_t> &readings) {
	if (_generalDrift || getGeneralDrift() == 0) {
		uint8_t buf[256];
		uint8_t getBulkCmd[] = {0x00, 0x3a, 0x32, 0x04, 0x00, 0x00, 0x00, 0x00};
		int     len = -1;

		try {
			len = _host->sendRawCmd(getBulkCmd, sizeof(getBulkCmd), buf, sizeof(buf));
		} catch (...) {
			throw;
		}
		
		if ((len == 208) && (buf[0] == 0x32) && (buf[1] == 0x00)) {
			// The timestamp is the timestamp of the first reading, the last reading is 100*10ms later
			uint64_t timeStamp = extractTimestamp(&buf[2]) + _generalDrift - MS_TO_NS(1000);

			readings.resize(100);
			uint16_t *buf16 = (uint16_t *)&buf[8];
			for (int i=0; i<100; i++) {
				readings[i].timestamp = timeStamp + MS_TO_NS(i * 10);
				readings[i].value = buf16[(99-i)]; // The order of readings is descending
			}
			return 0;
		}
	}
	
	return 1;
}

int LenovoXCC::getBulkEnergy(std::vector<reading_t> &readings) {
	if (_generalDrift || getGeneralDrift() == 0) {
		uint8_t buf[256];
		uint8_t getBulkCmd[] = {0x00, 0x3a, 0x32, 0x04, 0x01, 0x00, 0x00, 0x00};
		int     len = -1;
		try {
			len = _host->sendRawCmd(getBulkCmd, sizeof(getBulkCmd), buf, sizeof(buf));
		} catch (...) {
			throw;
		}
		
		if ((len == 212) && (buf[0] == 0x32) && (buf[1] == 0x00)) {
			// The timestamp is the timestamp of the first reading, the last reading is 101*10ms later
			uint64_t timeStamp = extractTimestamp(&buf[2]) + _generalDrift - MS_TO_NS(1010);
			
			uint32_t baseEnergy;
			memcpy(&baseEnergy, &buf[8], sizeof(baseEnergy));
			
			readings.resize(101); // This is really 101!
			readings[0].timestamp = timeStamp;
			readings[0].value = baseEnergy;
	
			uint16_t *buf16 = (uint16_t *)&buf[12];
			for (int i=1; i<101; i++) {
				readings[i].timestamp = timeStamp + MS_TO_NS(i * 10);
				readings[i].value = baseEnergy + buf16[i-1] ;
			}
			return 0;
		}
	}
	
	return 1;
}

uint64_t LenovoXCC::extractTimestamp(const uint8_t *buf) {
	uint64_t timestamp = 0;
	uint32_t ts1;
	uint16_t ts2;
	memcpy(&ts1, &buf[0], sizeof(ts1));
	memcpy(&ts2, &buf[4], sizeof(ts2));
	timestamp = S_TO_NS(ts1);
	if (ts2 < 1000) {
		timestamp += MS_TO_NS(ts2);
	}
	return timestamp;
}

int LenovoXCC::readSingleEnergyRaw(uint64_t &timeStamp, uint64_t &energy) {
	uint8_t buf[256];
	uint8_t getSingleEnergyCmd[] = {0x00, 0x3a, 0x32, 0x04, 0x02, 0x00, 0x00, 0x00};
	int     len = -1;
	try {
		len = _host->sendRawCmd(getSingleEnergyCmd, sizeof(getSingleEnergyCmd), buf, sizeof(buf));
	} catch (...) {
		throw;
	}

	if ((len == 16) && (buf[0] == 0x32) && (buf[1] == 0x00)) {
		timeStamp = extractTimestamp(&buf[10]);

		uint32_t joules;
		uint16_t mJoules;
		memcpy(&joules, &buf[4], sizeof(joules));
		memcpy(&mJoules, &buf[8], sizeof(mJoules));
		energy = ((uint64_t)joules) * 1000;
		if (mJoules < 1000) {
			energy += mJoules;
		}
		return 0;
	}

	return -1;
}

int LenovoXCC::openDatastore() {
	if (_handle) {
		closeDatastore();
	}
	
	uint8_t buf[256];
	uint8_t getHandleCmd[] = {0x00, 0x2e, 0x90, 0x66, 0x4a, 0x00, 0x01, 0x01, 0x01, 0xF0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x70, 0x77, 0x72, 0x5f, 0x6d, 0x65, 0x74, 0x65, 0x72};
	int     len = -1;
	try {
		len = _host->sendRawCmd(getHandleCmd, sizeof(getHandleCmd), buf, sizeof(buf));
	} catch (...) {
		throw;
	}

	if ((len == 9) && (buf[0] == 0x90) && (buf[1] == 0x00)) {
		memcpy(&_handle, &buf[5], sizeof(_handle));
		return 0;
	} else {
		return -1;
	}
}

int LenovoXCC::closeDatastore() {
	if (!_handle) {
		return 2;
	}

	uint8_t buf[256];
	uint8_t closeHandleCmd[] = {0x00, 0x2e, 0x90, 0x66, 0x4a, 0x00, 0x05, 0xff, 0xff, 0xff, 0xff};
	memcpy(&closeHandleCmd[7], &_handle, sizeof(_handle));

	int len = -1;
	try {
		len = _host->sendRawCmd(closeHandleCmd, sizeof(closeHandleCmd), buf, sizeof(buf));
	} catch (...) {
		throw;
	}

	if ((len == 5) && (buf[0] == 0x90) && (buf[1] == 0x00)) {
		_handle = 0;
		return 0;
	} else {
		return -1;
	}
}
int LenovoXCC::readDatastoreRange(uint32_t offset, uint16_t count, uint8_t *buf, uint16_t bufLen) {
	uint8_t getDataCmd[] = {0x00, 0x2e, 0x90, 0x66, 0x4a, 0x00, 0x02, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};
	memcpy(&getDataCmd[7], &_handle, sizeof(_handle));
	memcpy(&getDataCmd[11], &offset, sizeof(offset));
	memcpy(&getDataCmd[15], &count, sizeof(count));

	int len = -1;
	try {
		len = _host->sendRawCmd(getDataCmd, sizeof(getDataCmd), buf, bufLen);
	} catch (...) {
		throw;
	}

	if ((len >= 2) && (buf[0] == 0x90) && (buf[1] == 0x00)) {
		return len;
	} else {
		return -1;
	}
}

uint64_t LenovoXCC::readDatastoreTimestamp() {
	uint8_t buf[256];
	int     len = -1;
	try {
		len = readDatastoreRange(0, 16, buf, sizeof(buf));
	} catch (...) {
		throw;
	}

	if ((len == 23) && (buf[0] == 0x90) && (buf[1] == 0x00)){
		uint64_t startedIndex;
		memcpy(&startedIndex, &buf[15], sizeof(startedIndex));
		return extractTimestamp(&buf[7]) + MS_TO_NS(startedIndex * 10);
	} else {
		return 0;
	}
}

int LenovoXCC::getDatastoreDrift() {
	if ((_handle != 0) || (openDatastore() == 0)) {
		uint64_t dsTs1, dsTs2, sysTs1, sysTs2;
		dsTs1 = dsTs2 = readDatastoreTimestamp();
		// Loop until the timestamp changes so we can be sure it's not outdated
		do {
			usleep(5000);
			sysTs1 = getTimestamp();
			dsTs2 = readDatastoreTimestamp();
			sysTs2 = getTimestamp();
		} while (dsTs1 == dsTs2);
		// The timestamp is the timestamp of the first reading, the last reading is 30s later
		_datastoreDrift = sysTs1 - (sysTs2 - sysTs1) / 2 - dsTs2 - S_TO_NS(30);
		closeDatastore();
#ifdef DEBUG
		LOGGER lg;
		LOG(debug) << " Datastore drift: " << prettyPrintTimestamp(_datastoreDrift) << " (" << (sysTs2 - sysTs1) << ")" << std::endl;
#endif
		return 0;
	}
	return 1;
}

int LenovoXCC::getGeneralDrift() {
	uint64_t xccTs, dummy;

	// Do a dummy read to make sure we have the IPMI connection open in order to minimize latency
	readSingleEnergyRaw(xccTs, dummy);
		
	uint64_t sysTs1 = getTimestamp();
	if (readSingleEnergyRaw(xccTs, dummy) == 0) {
		uint64_t sysTs2 = getTimestamp();
		_generalDrift = sysTs1 - (sysTs2 - sysTs1) / 2 - xccTs;
#ifdef DEBUG
		LOGGER lg;
		LOG(debug) << "General drift:   " << prettyPrintTimestamp(_generalDrift) << " (" << (sysTs2 - sysTs1) << ")" << std::endl;
#endif
		return 0;
	}
	
	return 1;
}
