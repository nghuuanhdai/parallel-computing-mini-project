//================================================================================
// Name        : logging.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of logging functionality.
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

#include "logging.h"

void initLogging() {
    //Init logging environment
    boost::log::add_common_attributes();
}

//Setup a command line logger
//only print timestamp (without date), severity and message to terminal
auto setupCmdLogger() -> decltype(boost::log::add_console_log()) {
    auto logger = boost::log::add_console_log(std::clog,
          boost::log::keywords::format = (boost::log::expressions::stream
                  << "[" << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S") << "]"
                  << " <" << boost::log::trivial::severity << ">"
                  << ": " << boost::log::expressions::smessage),
          boost::log::keywords::filter = (boost::log::trivial::severity >= boost::log::trivial::info));
    return logger;
}

//Setup logger to file; we now should know where the writable tempdir is
//number logfiles ascending; rotate logfile every 10 MiB
//format:	LineID [Timestamp] ThreadID <severity>: Message
auto setupFileLogger(std::string logPath, std::string logName) -> decltype(boost::log::add_file_log("")) {
    auto logger = boost::log::add_file_log(
            boost::log::keywords::file_name = logPath + logName + "_%N.log",
            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
            boost::log::keywords::format = ( boost::log::expressions::stream
                    << boost::log::expressions::attr< unsigned int >("LineID")
                    << " [" << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d, %H:%M:%S") << "]"
                    << " " << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type >("ThreadID")
                    << " <" << boost::log::trivial::severity << ">"
                    << ": " << boost::log::expressions::smessage),
            boost::log::keywords::filter = (boost::log::trivial::severity >= boost::log::trivial::trace),
            boost::log::keywords::auto_flush = true);
    return logger;
}

boost::log::trivial::severity_level translateLogLevel(int logLevel) {
    switch (logLevel) {
        case 0:
            return boost::log::trivial::fatal;
        case 1:
            return boost::log::trivial::error;
        case 2:
            return boost::log::trivial::warning;
        case 3:
            return boost::log::trivial::info;
        case 4:
            return boost::log::trivial::debug;
        case 5:
            return boost::log::trivial::trace;
        default:
            return boost::log::trivial::info;
    }
}
