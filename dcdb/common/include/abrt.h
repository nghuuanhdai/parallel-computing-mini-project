//================================================================================
// Name        : abrt.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Generic function for handling application crashes with backtrace.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
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
 * @file abrt.h
 *
 * @brief Generic function for handling application crashes with backtrace.
 *
 * @ingroup common
 */

#include <stdio.h>
#include <execinfo.h>

#ifndef ABRT_H
#define ABRT_H

#define STACKTRACE_DEPTH 20

typedef enum {
  SIGNAL,
  INTERR
} AbrtSrc;

/* Abort handler that prints out a stack trace */
static inline void abrt(int exitcode, AbrtSrc source)
{
  void* fcn_ptr[STACKTRACE_DEPTH];
  size_t fcn_ptr_num_elems;

  fcn_ptr_num_elems = backtrace(fcn_ptr, STACKTRACE_DEPTH);

  fprintf(stderr, "Program abort due to %s. Backtrace:\n",
      source == SIGNAL ? "signal" :
      source == INTERR ? "internal error" :
      "unknown error"
      );

  backtrace_symbols_fd(fcn_ptr, fcn_ptr_num_elems, STDERR_FILENO);

  exit(exitcode);
}

#endif
