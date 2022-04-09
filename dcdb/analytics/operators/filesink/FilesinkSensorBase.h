//================================================================================
// Name        : FilesinkSensorBase.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

/**
 * @defgroup filesink Filesink operator plugin.
 * @ingroup operator
 *
 * @brief Filesink operator plugin.
 */

#ifndef PROJECT_FILESINKSENSORBASE_H
#define PROJECT_FILESINKSENSORBASE_H

#include "sensorbase.h"
#include <fstream>

/**
 * @brief Sensor base for filesink plugin
 *
 * @ingroup filesink
 */
class FilesinkSensorBase : public SensorBase {

public:
    
    // Constructor and destructor
    FilesinkSensorBase(const std::string& name) : SensorBase(name) {
        _file.reset(nullptr);
        _adjusted = false;
        _path = "";
    }

    // Copy constructor
    FilesinkSensorBase(FilesinkSensorBase& other) : SensorBase(other) {
        _file.reset(nullptr);
        _adjusted = false;
        _path = other._path;
    }

    FilesinkSensorBase& operator=(const FilesinkSensorBase& other) {
        SensorBase::operator=(other);
        _file.reset(nullptr);
        _adjusted = false;
        _path = other._path;
        return *this;
    }

    virtual ~FilesinkSensorBase() {}

    void setPath(const std::string& path)			    { _path = path; }
    void setAdjusted(bool a)                            { _adjusted = a; }
    
    const std::string&   getPath()                      { return _path; }
    bool getAdjusted()                                  { return _adjusted; }
    bool isOpen()                                       { return _file && _file->is_open(); }

    bool openFile() {
        closeFile();
        _file.reset(new std::ofstream(_path));
        if (!_file->is_open()) {
            _file.reset(nullptr);
            return false;
        }
        return true;
    }

    bool closeFile() {
        if(_file && _file->is_open())
            _file->close();
        _file.reset(nullptr);
        return true;
    }

    bool writeFile(reading_t val) {
        if(_file && _file->is_open()) {
            try {
                _file->seekp(0, std::ios::beg);
                *_file << val.value << std::endl;
                return true;
            } catch(const std::exception &e) {
                _file->close();
                _file.reset(nullptr);
            }
        }
        return false;
    }
    
    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Path:        " << _path;
    }

protected:

    bool _adjusted;
    std::string _path;
    std::unique_ptr<std::ofstream> _file;
};

using FilesinkSBPtr = std::shared_ptr<FilesinkSensorBase>;

#endif //PROJECT_FILESINKSENSORBASE_H
