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

#ifndef __ILOCKABLE_H__
#define __ILOCKABLE_H__

#include <cstddef>

// platform specifics

#if defined WIN32

#ifdef __AFXWIN_H__  // if using MFC, you cannot include <Windows.h> directly, must use <afx.h> instead
#include <afx.h>
#else
#include <Windows.h>
#endif
#define LOCK CRITICAL_SECTION
#define THREAD HANDLE
#define IMultithreadedCallback LPTHREAD_START_ROUTINE  // takes a void*, returns a DWORD
typedef void* (*IMultithreadedMethodCallback)(void*)


#elif defined POSIX

#include <pthread.h>
#define LOCK pthread_mutex_t
#define THREAD pthread_t
typedef void* (*IMultithreadedCallback)(void*)
#define IMultithreadedMethodCallback IMultithreadedCallback

#endif

// ------------------


class ILockable
{
public:
	ILockable(void);
	~ILockable(void);

protected:
	// call this to lock the deriving class object indefinately (until unlock is called)
	lockObject();
	// call this to unlock
	unlockObject();

	// create this object in a method to lock the deriving class for the duration of the calling scope
	class ILockableThreadLock
    {
    public:
        ILockableThreadLock(ILockable *ilock) 
        {
		#ifdef WIN32
            EnterCriticalSection(_lock = &(ilock->_lock)); 
		#elif defined POSIX
			pthread_mutex_lock(_lock = &(ilock->_lock));
		#endif
        }
        ~ILockableThreadLock() 
		{ 
		#ifdef WIN32
            LeaveCriticalSection(_lock); 
		#elif defined POSIX
			pthread_mutex_unlock(_lock);
		#endif
	
	private:
        LOCK *_lock;
    };

	friend class ILockableThreadLock;

private:
	LOCK _lock;
};

#endif