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


#ifndef __SerialDevice_h__
#define __SerialDevice_h__

#include <stdio.h>
#include <string>  
#include <io.h>
#include "FTD2XX.h"

#include "types.h"

using namespace std;

class SerialDeviceException
{
public:
    SerialDeviceException(const char *message, int errno_val);
    SerialDeviceException(const string& message, int errno_val);
    const string& getMessage(void) const { return _message; }

private:
    string _message;
    int _errno_val;
};

class SerialDevice
{
public:
    SerialDevice(const string& serialNumber);
    ~SerialDevice(void);
    
    unsigned long write(char *data, unsigned int len);
	unsigned long read(char *buffer, size_t len);
    void flush(void);

	// added by daniel b for Windows FTDI version.  returns the # of bytes in the receive queue.
	unsigned long hasBytes(void);

	const string& serialNumber(void) const;
    int fileDescriptor(void) const;
	HANDLE fileHandle(void) const;

    void setUnexpectedDeviceRemovalFlag(bool flag);
    bool unexpectedDeviceRemovalFlag(void);
    
private:
	string _serial;
    FT_HANDLE _fileHandle;
    bool _unexpectedDeviceRemovalFlag;
};

inline const string& 
SerialDevice::serialNumber(void) const
{
    return _serial;
}

inline FT_HANDLE 
SerialDevice::fileHandle(void) const
{
    return _fileHandle;
}

inline bool
SerialDevice::unexpectedDeviceRemovalFlag(void)
{
    return _unexpectedDeviceRemovalFlag;
}

inline void
SerialDevice::setUnexpectedDeviceRemovalFlag(bool flag)
{
    _unexpectedDeviceRemovalFlag = flag;
}

#endif // __SerialDevice_h__
