//================================================================================
// Name        : dcdbendian.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Generic functions for handling endianess in DCDB.
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
 * @file dcdbendian.h
 *
 * @brief Generic functions for handling endianess in DCDB.
 *
 * @ingroup common
 */

#ifndef DCDBENDIAN_H
#define DCDBENDIAN_H

#include <boost/predef/other/endian.h>
#include <stdint.h>

/* Check if we can use any optimized byte swap implementation */
#if defined(__llvm__) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__ICC)
#   define OPTIMIZED_BSWAP_16(x)    __builtin_bswap16(x)
#   define OPTIMIZED_BSWAP_32(x)    __builtin_bswap32(x)
#   define OPTIMIZED_BSWAP_64(x)    __builtin_bswap64(x)
#elif defined(_MSC_VER) && !defined(_DEBUG)
#   define OPTIMIZED_BSWAP_16(x)    _byteswap_ushort(x)
#   define OPTIMIZED_BSWAP_32(x)    _byteswap_ulong(x)
#   define OPTIMIZED_BSWAP_64(x)    _byteswap_uint64(x)
#endif

class Endian
{
public:
    static inline uint16_t swap(uint16_t x) {
#if defined OPTIMIZED_BSWAP_16
        return OPTIMIZED_BSWAP_16(x);
#else
        return (x>>8) | (x<<8);
#endif
    }

    static inline uint32_t swap(uint32_t x) {
#if defined OPTIMIZED_BSWAP_32
        return OPTIMIZED_BSWAP_32(x);
#else
        return ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |
                (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));
#endif
    }
    
    static inline uint64_t swap(uint64_t x) {
#if defined OPTIMIZED_BSWAP_64
        return OPTIMIZED_BSWAP_64(x);
#else
        return (((uint64_t)swap32((uint32_t)x))<<32) |
                (swap32((uint32_t)(x>>32)));
#endif
    }
    
    static inline uint16_t hostToBE(uint16_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return x;
#elif BOOST_ENDIAN_LITTLE_BYTE
        return swap(x);
#else
#   error Endianess of host is undefined!
#endif
    }
    
    static inline uint32_t hostToBE(uint32_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return x;
#elif BOOST_ENDIAN_LITTLE_BYTE
        return swap(x);
#else
#   error Endianess of host is undefined!
#endif
    }
    
    static inline uint64_t hostToBE(uint64_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return x;
#elif BOOST_ENDIAN_LITTLE_BYTE
        return swap(x);
#else
#   error Endianess of host is undefined!
#endif
    }
    
    static inline uint16_t hostToLE(uint16_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return swap(x);
#elif BOOST_ENDIAN_LITTLE_BYTE
        return x;
#else
#   error Endianess of host is undefined!
#endif
    }

    static inline uint32_t hostToLE(uint32_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return swap(x);
#elif BOOST_ENDIAN_LITTLE_BYTE
        return x;
#else
#   error Endianess of host is undefined!
#endif
    }

    static inline uint64_t hostToLE(uint64_t x) {
#if BOOST_ENDIAN_BIG_BYTE
        return swap(x);
#elif BOOST_ENDIAN_LITTLE_BYTE
        return x;
#else
#   error Endianess of host is undefined!
#endif
    }
    
    static inline uint16_t BEToHost(uint16_t x) { return hostToBE(x); }
    static inline uint32_t BEToHost(uint32_t x) { return hostToBE(x); }
    static inline uint64_t BEToHost(uint64_t x) { return hostToBE(x); }
    static inline uint16_t LEToHost(uint16_t x) { return hostToLE(x); }
    static inline uint32_t LEToHost(uint32_t x) { return hostToLE(x); }
    static inline uint64_t LEToHost(uint64_t x) { return hostToLE(x); }
};

#endif  /* DCDBENDIAN_H */
