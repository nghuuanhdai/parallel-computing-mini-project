//================================================================================
// Name        : dcdbdaemon.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Generic function for daemonizing an application.
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
 * @file dcdbdaemon.h
 *
 * @brief Generic function for daemonizing and application.
 *
 * @ingroup common
 */

#ifndef DCDBDAEMON_H
#define DCDBDAEMON_H

#include <unistd.h>
#include <cstdlib>


static int dcdbdaemon()
{
#ifdef __linux__
  return daemon(1, 0);
#else
  switch(fork()) {
  case -1:
    return -1;
    break;
  case 0:
    break;
  default:
    exit(0);
  }

  if (setsid() == -1)
    return -1;

  return 0;
#endif

}

#endif /* DCDBDAEMON_H */
