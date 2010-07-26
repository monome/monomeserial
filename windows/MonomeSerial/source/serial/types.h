/*
 * MonomeSerial, a simple MIDI and OpenSoundControl routing utility for the monome 40h
 * Copyright (C) 2007 Joe Lake
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
//--------------------------------------------
/*
 * Current code by Daniel Battaglia and Steve Duda.
 * Released under the original GPL
 */
//--------------------------------------------


#ifndef __TYPES_H__
#define __TYPES_H__

//#include "StdAfx.h"
#include <afx.h>

#include <cstdlib>
#include <cstring>


#ifdef __cplusplus
extern "C"
{
#endif

typedef __int8				sint8;
typedef unsigned __int8     uint8;

typedef __int16				sint16;
typedef unsigned __int16	uint16;

typedef __int32				sint32;
typedef unsigned __int32	uint32;

typedef __int64				sint64;
typedef unsigned __int64	uint64;



typedef union {
    uint32 uLong;
    uint16 uInt[2];
    uint8  uChar[4];
} ULong;
#ifndef __cplusplus
typedef uint8 bool;
#endif

#ifndef TRUE
#define TRUE       (1)
#endif
#ifndef FALSE
#define FALSE      (0)
#endif
#ifndef true
#define true        TRUE
#endif
#ifndef false
#define false       FALSE
#endif

#ifdef __cplusplus
}
#endif

#endif
