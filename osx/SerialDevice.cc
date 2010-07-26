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
#include "SerialDevice.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <sstream>
using namespace std;

#define kSerialDeviceErrReturn  (-1)

SerialDeviceException::SerialDeviceException(const char *message, int errno_val)
{
    ostringstream ostrstrm;
    ostrstrm << message << " - (" << errno_val << ") " << strerror(errno_val);
    
    _errno_val = errno_val;
    _message = ostrstrm.str();
}

SerialDeviceException::SerialDeviceException(const string& message, int errno_val)
{
    ostringstream ostrstrm;
    ostrstrm << message << " - (" << errno_val << ") " << strerror(errno_val);
    
    _errno_val = errno_val;
    _message = ostrstrm.str();
}    

SerialDevice::SerialDevice(const string& bsdFilePath)
{
    struct termios options;
	ostringstream ostrstrm;
    string errorMessage;

    _bsdFilePath = bsdFilePath;
    _fileDescriptor = kSerialDeviceErrReturn;
    _unexpectedDeviceRemovalFlag = false;

	do {
		// Open the serial port read/write, with no controlling terminal, 
		// and don't wait for a connection.
		// be non-blocking.

        sleep(2);  // this is a cheap hack.  I'm getting resource busy errors when I first try to open the device.
                   // in theory, IOKit is not supposed to notify me of a discovered device until the driver has been loaded.
                   // I suspect I'm getting early notification, and there's some loading/configuration going on when I 
                   // first try to open the device (I don't know, I'm not a kernel programmer).  hopefully waiting a bit
                   // will correct this problem.
#define NONBLOCKING
#ifdef NONBLOCKING
		_fileDescriptor = open(_bsdFilePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
#else
        _fileDescriptor = open(_bsdFilePath.c_str(), O_RDWR | O_NOCTTY);
#endif

        if (_fileDescriptor == kSerialDeviceErrReturn && errno == 16) {
			cout << "Error opening serial port " << _bsdFilePath << " - " << strerror(errno) << " (" << errno << ")." << endl;
            cout << "Waiting to try again..." << endl;
            sleep(2);
#ifndef NONBLOCKING           
            _fileDescriptor = open(_bsdFilePath.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
#else            
            _fileDescriptor = open(_bsdFilePath.c_str(), O_RDWR | O_NOCTTY);
#endif
        }

		if (_fileDescriptor == kSerialDeviceErrReturn) {
			ostrstrm << "Error opening serial port " << _bsdFilePath;
			errorMessage = ostrstrm.str();
            break;
		}

		// Note that open() follows POSIX semantics: multiple open() calls to 
		// the same file will succeed unless the TIOCEXCL ioctl is issued.
		// This will prevent additional opens except by root-owned processes.
		// See tty(4) ("man 4 tty") and ioctl(2) ("man 2 ioctl") for details.

		if (ioctl(_fileDescriptor, TIOCEXCL) == kSerialDeviceErrReturn) {
			ostrstrm << "Error setting TIOCEXCL on " << _bsdFilePath;
			errorMessage = ostrstrm.str();
            break;
		}

		// Get the current options and save them so we can restore the 
		// default settings later.
		
		if (tcgetattr(_fileDescriptor, &_originalTTYAttrs) == kSerialDeviceErrReturn) {	
			ostrstrm << "Error getting tty attributes from " << _bsdFilePath;
			errorMessage = ostrstrm.str();
            break;
		}
	
		// The serial port attributes such as timeouts and baud rate are set by 
		// modifying the termios structure and then calling tcsetattr to
		// cause the changes to take effect. Note that the
		// changes will not take effect without the tcsetattr() call.
		// See tcsetattr(4) ("man 4 tcsetattr") for details.

        memcpy(&options, &_originalTTYAttrs, sizeof(struct termios));

		// Set raw input (non-canonical) mode, with reads blocking until either 
		// a single character has been received or a one second timeout expires.
		// See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") 
		// for details.

        cfmakeraw(&options);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
		options.c_cc[VMIN] = 1;
		options.c_cc[VTIME] = 10;

		// The baud rate, word length, and handshake options can be set as follows:
        options.c_cflag |= (CS8);        // Use 7 bit words
        
		// Cause the new options to take effect immediately.
		if (tcsetattr(_fileDescriptor, TCSANOW, &options) == kSerialDeviceErrReturn) {
			ostrstrm << "Error setting tty attributes on " << _bsdFilePath;
			errorMessage = ostrstrm.str();
            break;
		}
		
		// Flush input and output buffers.  We do this because we may have garbage data
		// if anything data has come in between when we opened the serial port and when we
		// set the termios attributes
		
		tcflush(_fileDescriptor, TCIOFLUSH);

        return;

	} while (0);
	
    if (_fileDescriptor != kSerialDeviceErrReturn) 
        close(_fileDescriptor);
    
    throw SerialDeviceException(errorMessage, errno);
}

SerialDevice::~SerialDevice()
{
    if (!_unexpectedDeviceRemovalFlag) {
        if (_fileDescriptor != kSerialDeviceErrReturn) {
            if (tcdrain(_fileDescriptor) == kSerialDeviceErrReturn) 
                cout << "Error waiting for drain - " << strerror(errno) << " (" << errno << ")." << endl;

            // It is good practice to reset a serial port back to the state in
            // which you found it. This is why we saved the original termios struct
            // The constant TCSANOW (defined in termios.h) indicates that
            // the change should take effect immediately.
            
            if (tcsetattr(_fileDescriptor, TCSANOW, &_originalTTYAttrs) == kSerialDeviceErrReturn)
                cout << "Error resetting tty attributes - " << strerror(errno) << " (" << errno << ")." << endl;
            
            close(_fileDescriptor);
            _fileDescriptor = kSerialDeviceErrReturn;
        }
    }
}

int 
SerialDevice::write(char *data, unsigned int len)
{
    if (_fileDescriptor == kSerialDeviceErrReturn)
        return 0;

    return ::write(_fileDescriptor, data, len);
}

ssize_t 
SerialDevice::read(char *buffer, size_t len)
{
    if (_fileDescriptor == kSerialDeviceErrReturn)
        return -1;

    return ::read(_fileDescriptor, buffer, len);
}

void SerialDevice::flush(void)
{
    tcflush(_fileDescriptor, TCIOFLUSH);
}
