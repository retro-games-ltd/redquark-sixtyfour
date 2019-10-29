/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef VICE_TYPES_H
#define VICE_TYPES_H

#include "vice.h"
#include <stdint.h>

// Portable type declarations
typedef uint8_t   BYTE;         // 8
typedef  int8_t   SIGNED_CHAR;  // 8

typedef uint16_t   WORD; // 16
typedef  int16_t  SWORD; // 16

typedef uint32_t  DWORD; // 32
typedef  int32_t SDWORD; // 32

typedef DWORD CLOCK;
/* Maximum value of a CLOCK.  */
#define CLOCK_MAX (~((CLOCK)0))

#define vice_ptr_to_int(x) ((int)(long)(x))
#define vice_ptr_to_uint(x) ((unsigned int)(unsigned long)(x))
#define int_to_void_ptr(x) ((void *)(long)(x))
#define uint_to_void_ptr(x) ((void *)(unsigned long)(x))

#endif
