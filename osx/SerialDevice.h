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
#ifndef __SerialDevice_h__
#define __SerialDevice_h__

#include <termios.h>
#include <stdio.h>
#include <string.h>

#include <string>
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
    SerialDevice(const string& bsdFilePath);
    ~SerialDevice(void);
    
    int write(char *data, unsigned int len);
    ssize_t read(char *buffer, size_t len);
    void flush(void);

    const string& bsdFilePath(void) const;
    int fileDescriptor(void) const;

    void setUnexpectedDeviceRemovalFlag(bool flag);
    bool unexpectedDeviceRemovalFlag(void);
    
private:
    string _bsdFilePath;
    int _fileDescriptor;
    struct termios _originalTTYAttrs;

    bool _unexpectedDeviceRemovalFlag;  // We set this flag if the device is unexpectedly removed so we don't call
                                        // tcdrain in the destructor with unpleasant consequences.
};

inline const string& 
SerialDevice::bsdFilePath(void) const
{
    return _bsdFilePath;
}

inline int 
SerialDevice::fileDescriptor(void) const
{
    return _fileDescriptor;
}

inline void
SerialDevice::setUnexpectedDeviceRemovalFlag(bool flag)
{
    _unexpectedDeviceRemovalFlag = flag;
}

inline bool
SerialDevice::unexpectedDeviceRemovalFlag(void)
{
    return _unexpectedDeviceRemovalFlag;
}


#endif // __SerialDevice_h__
