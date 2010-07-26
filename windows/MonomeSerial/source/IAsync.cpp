/*
 * MonomeSerial, a simple MIDI and OpenSoundControl routing utility for the monome 40h
 * Copyright (C) 2007 Joe Lake
 *
 *   - Additional OS X development : Copyright (C) 2007 Steve Duda
 *   - Windows  port / development : Copyright (C) 2007 Daniel Battaglia
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

// NOTE - for a POSIX/non MS-VisualStudio build, remove this line!
#include "stdafx.h"

#include "IAsync.h"


ILockable::ILockable(void)
{
#ifdef WIN32
	InitializeCriticalSection(&_lock);
#elif defined POSIX
	pthread_mutex_init(&_lock, NULL);
#endif
}

ILockable::~ILockable(void)
{
#ifdef WIN32
	DeleteCriticalSection(&_lock);
#elif defined POSIX
	pthread_mutex_destroy(&_lock);
#endif
}

void
ILockable::lockObject()
{
#ifdef WIN32
    EnterCriticalSection(_lock = &(ilock->_lock)); 
#elif defined POSIX
	pthread_mutex_lock(_lock = &(ilock->_lock));
#endif
}

void
~ILockable::unlockObject() 
{ 
#ifdef WIN32
    LeaveCriticalSection(_lock); 
#elif defined POSIX
	pthread_mutex_unlock(_lock);
#endif
}


