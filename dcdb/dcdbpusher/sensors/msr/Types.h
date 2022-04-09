//================================================================================
// Name        : Types.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Type definitions for MSR plugin.
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
 * @file
 * @ingroup msr
 */

#ifndef SRC_SENSORS_MSR_TYPES_H_
#define SRC_SENSORS_MSR_TYPES_H_

/*
        MSR addreses from
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Appendix A "PERFORMANCE-MONITORING EVENTS"
*/

#define INST_RETIRED_ANY_ADDR (0x309)
#define CPU_CLK_UNHALTED_THREAD_ADDR (0x30A)
#define CPU_CLK_UNHALTED_REF_ADDR (0x30B)
#define IA32_CR_PERF_GLOBAL_CTRL (0x38F)
#define IA32_CR_FIXED_CTR_CTRL (0x38D)

/* \brief Fixed Event Control Register format

        According to
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Figure 30-7. Layout of
        IA32_FIXED_CTR_CTRL MSR Supporting Architectural Performance Monitoring Version 3
*/
struct FixedEventControlRegister {
	union {
		struct
		{
			// CTR0
			uint64_t os0 : 1;
			uint64_t usr0 : 1;
			uint64_t any_thread0 : 1;
			uint64_t enable_pmi0 : 1;
			// CTR1
			uint64_t os1 : 1;
			uint64_t usr1 : 1;
			uint64_t any_thread1 : 1;
			uint64_t enable_pmi1 : 1;
			// CTR2
			uint64_t os2 : 1;
			uint64_t usr2 : 1;
			uint64_t any_thread2 : 1;
			uint64_t enable_pmi2 : 1;

			uint64_t reserved1 : 52;
		} fields;
		uint64_t value;
	};
};

#endif /* SRC_SENSORS_MSR_TYPES_H_ */
