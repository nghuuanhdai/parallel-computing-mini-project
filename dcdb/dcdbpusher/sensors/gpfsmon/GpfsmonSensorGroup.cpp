//================================================================================
// Name        : GpfsmonSensorGroup.cpp
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Gpfsmon sensor group class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#include "GpfsmonSensorGroup.h"

#include "timestamp.h"
#include <algorithm>
#include <fstream>
#include <sys/stat.h>

GpfsmonSensorGroup::GpfsmonSensorGroup(const std::string &name)
    : SensorGroupTemplate(name) {
	if (!fileExists(TMP_GPFSMON))
		createTempFile();
	_data.resize(GPFS_METRIC::SIZE);
}

GpfsmonSensorGroup &GpfsmonSensorGroup::operator=(const GpfsmonSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	//no need to copy _data
	return *this;
}

GpfsmonSensorGroup::GpfsmonSensorGroup(const GpfsmonSensorGroup &other)
    : SensorGroupTemplate(other) {
	_data.resize(GPFS_METRIC::SIZE);
	if (!fileExists(TMP_GPFSMON))
		createTempFile();
}

GpfsmonSensorGroup::~GpfsmonSensorGroup() {
	_data.clear();
}

void GpfsmonSensorGroup::read() {
	ureading_t reading;
	reading.timestamp = getTimestamp();
        FILE * pf = nullptr;
	try {
		std::string toparse;
		pf = popen(_cmd_io.c_str(), "r");
		if (pf != nullptr) {
			char buf[BUFFER_SIZE];
			while (fgets(buf, BUFFER_SIZE, pf) != nullptr) {
				if (parseLine(std::string(buf))) {
					for (auto &s : _sensors) {
						reading.value = _data[s->getMetricType()];
#ifdef DEBUG
						LOG(debug) << _groupName << "::" << s->getName() << ": \"" << reading.value << "\"";
#endif
						s->storeReading(reading);
					}
				} else {
					LOG(error) << "Sensorgroup " << _groupName << " could not parse line" << buf;
					//assume there was a problem with the temp file
					if (!fileExists(TMP_GPFSMON))
						createTempFile();
				}
			}
                        pclose(pf);
                        pf = nullptr;
		} else {
		    LOG(error) << "Sensorgroup " << _groupName << " popen failed: " << strerror(errno);
		}
	} catch (const std::exception &e) {
		LOG(error) << "Sensorgroup " << _groupName << " could not read value: " << e.what();
	}
        if(pf != nullptr){
             pclose(pf);
        }
}

void GpfsmonSensorGroup::createTempFile() {
	std::ofstream gpfsmonFile;
	gpfsmonFile.open(TMP_GPFSMON);
	if (gpfsmonFile.is_open()) {
		gpfsmonFile << "io_s\n";
		gpfsmonFile.close();
	} else {
		LOG(error) << "Gpfsmon: unable to create temporary file " << TMP_GPFSMON << " for mmpmon: " << strerror(errno);
	}
}

void GpfsmonSensorGroup::execOnInit() {
	if (!fileExists(TMP_GPFSMON))
		createTempFile();
	_data.resize(GPFS_METRIC::SIZE);
}

bool GpfsmonSensorGroup::fileExists(const char *filename) {
	//https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
}

bool GpfsmonSensorGroup::parseLine(std::string &&toparse) {
	std::string::size_type bytereads_pos = toparse.find("_br_ ");
	std::string::size_type bytewrite_pos = toparse.find(" _bw_ ");

	if (bytereads_pos != std::string::npos && bytewrite_pos != std::string::npos) {
		_data[IOBYTESREAD] = std::stoull(toparse.substr(bytereads_pos + 5, bytewrite_pos - bytereads_pos));
	} else {
		return false;
	}
	std::string::size_type opens_pos = toparse.find(" _oc_ ");
	if (opens_pos != std::string::npos) {
		_data[IOBYTESWRITE] = std::stoull(toparse.substr(bytewrite_pos + 6, opens_pos - bytewrite_pos));
	} else {
		return false;
	}
	std::string::size_type closes_pos = toparse.find(" _cc_ ");
	if (closes_pos != std::string::npos) {
		_data[IOOPENS] = std::stoull(toparse.substr(opens_pos + 6, closes_pos - opens_pos));
	} else {
		return false;
	}
	std::string::size_type reads_pos = toparse.find(" _rdc_ ");
	if (reads_pos != std::string::npos) {
		_data[IOCLOSES] = std::stoull(toparse.substr(closes_pos + 6, reads_pos - closes_pos));
	} else {
		return false;
	}
	std::string::size_type writes_pos = toparse.find(" _wc_ ");
	if (writes_pos != std::string::npos) {
		_data[IOREADS] = std::stoull(toparse.substr(reads_pos + 7, writes_pos - reads_pos));
	} else {
		return false;
	}
	std::string::size_type dir_pos = toparse.find(" _dir_");
	if (dir_pos != std::string::npos) {
		_data[IOWRITES] = std::stoull(toparse.substr(writes_pos + 6, dir_pos - writes_pos));
	} else {
		return false;
	}
	return true;
}
