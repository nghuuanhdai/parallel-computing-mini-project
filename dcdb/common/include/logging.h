//================================================================================
// Name        : logging.h
// Author      : Micha Mueller, Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Provide logging functionality.
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

#ifndef LOGGING_H_
#define LOGGING_H_

/**
 * @file logging.h
 *
 * @brief Provide logging functionality.
 *
 * @details Simple header file to bundle all includes required for logging with
 *          boost::log. Simplifies logging: one has to include only this single
 *          header file which additionally offers shortcut macros for convenient
 *          logging.
 *
 * @ingroup common
 */

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#define LOG_LEVEL boost::log::trivial::severity_level
#define LOGGER boost::log::sources::severity_logger<LOG_LEVEL>

//further abbreviate the boost shortcut-macro
//to use it, only a boost severity-logger named lg is required
#define LOG(sev) BOOST_LOG_SEV(lg, boost::log::trivial::sev)

//set of log macros for specific application parts
#define ServerLOG(sev)  LOG(sev) << "HttpsServer: "
#define RESTAPILOG(sev) LOG(sev) << "REST-API: "
#define LOGM(sev)       LOG(sev) << "Mosquitto: "

//another shortcut which can take the severity level as variable
#define LOG_VAR(var) BOOST_LOG_SEV(lg, var)

// Handy typedef to instantiate loggers
typedef boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_t;

/**
* @brief    Initialize the logging environment.
**/
void initLogging();

/**
* @brief        	Setup a command-line logger.
*
*                   Only prints timestamp (without date), severity and message to terminal.
*
* @return           BOOST command-line logger object
**/
auto setupCmdLogger() -> decltype(boost::log::add_console_log());


/**
* @brief        	Setup a file logger.
*
*                   Setup logger to file; we now should know where the writable tempdir is
*                   number logfiles ascending; rotate logfile every 10 MiB
*                   format:	LineID [Timestamp] ThreadID <severity>: Message
*
* @param  logPath   Path to which log files must be stored
* @param  logName   Prefix used to name log files
* @return           BOOST file logger object
**/
auto setupFileLogger(std::string logPath, std::string logName) -> decltype(boost::log::add_file_log(""));

/**
* @brief            Numeric verbosity level to boost::log severity level
*
* @param  logLevel	The numeric verbosity level
* @return		    The boost::log severity level
**/
boost::log::trivial::severity_level translateLogLevel(int logLevel);

#endif /* LOGGING_H_ */
