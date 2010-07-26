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


#include "../stdafx.h"
#include "FTD2xx.h"
#include "SerialDevice.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <io.h>
#include <time.h>
#include <sstream>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

using namespace std;

// internal data/functions to emulate strerror() (doesn't work properly unless you use FileDescriptors)
static string FtStatusNames[] = {
    "FT_OK",
    "FT_INVALID_HANDLE",
    "FT_DEVICE_NOT_FOUND",
    "FT_DEVICE_NOT_OPENED",
    "FT_IO_ERROR",
    "FT_INSUFFICIENT_RESOURCES",
    "FT_INVALID_PARAMETER",
    "FT_INVALID_BAUD_RATE",

    "FT_DEVICE_NOT_OPENED_FOR_ERASE",
    "FT_DEVICE_NOT_OPENED_FOR_WRITE",
    "FT_FAILED_TO_WRITE_DEVICE",
    "FT_EEPROM_READ_FAILED",
    "FT_EEPROM_WRITE_FAILED",
    "FT_EEPROM_ERASE_FAILED",
	"FT_EEPROM_NOT_PRESENT",
	"FT_EEPROM_NOT_PROGRAMMED",
	"FT_INVALID_ARGS",
	"FT_NOT_SUPPORTED",
	"FT_OTHER_ERROR",
	"FT_DEVICE_LIST_NOT_READY"
};

const string& getFtStatusCodeString(FT_STATUS status)
{
	return FtStatusNames[(int)status];
}

#define kSerialDeviceErrReturn  (-1)
#define kSerialDeviceInvalidHandle  (FT_INVALID_HANDLE)

SerialDeviceException::SerialDeviceException(const char *message, int errno_val)
{
    ostringstream ostrstrm;
	// replace strerror() with getFtStatusCodeString()
    ostrstrm << message << " - (" << errno_val << ") " << getFtStatusCodeString(errno_val);
    
    _errno_val = errno_val;
    _message = ostrstrm.str();
}

SerialDeviceException::SerialDeviceException(const string& message, int errno_val)
{
    ostringstream ostrstrm;
    ostrstrm << message << " - (" << errno_val << ") " << getFtStatusCodeString(errno_val);
    
    _errno_val = errno_val;
    _message = ostrstrm.str();
}    

SerialDevice::SerialDevice(const string& serialNumber)
{
	ostringstream ostrstrm("");
	bool opened = false;

	_serial = serialNumber;
    _unexpectedDeviceRemovalFlag = false;
	FT_STATUS ftStatus;

	do {
		ftStatus = FT_OpenEx((void*)serialNumber.c_str(),FT_OPEN_BY_SERIAL_NUMBER, &_fileHandle);

		if (ftStatus != FT_OK) {
			ostrstrm << "Error opening serial device " << serialNumber << endl;
			break;
		}
		opened = true;

		ftStatus = FT_SetBaudRate(_fileHandle, FT_BAUD_115200);
		if (ftStatus != FT_OK) {
			ostrstrm << "Error setting BaudRate for serial device " << serialNumber << endl;
			break;
		}

		// Flush input and output buffers.  We do this because we may have garbage data
		// if anything data has come in between when we opened the serial port and when we
		// set the termios attributes
		flush();

		return;

	} while (0);

	if (opened) {
		FT_Close(_fileHandle);
	}

	throw SerialDeviceException(ostrstrm.str(), ftStatus);
}

SerialDevice::~SerialDevice()
{
    if (!_unexpectedDeviceRemovalFlag) {
		FT_Close(_fileHandle);
	}
}

unsigned long 
SerialDevice::write(char *data, unsigned int len)
{
	unsigned long BytesWritten;

	FT_STATUS ftStatus = FT_Write(_fileHandle, data, len, &BytesWritten);
	if (ftStatus != FT_OK) {
		//throw SerialDeviceException("Error in FTDI API : FT_Write", ftStatus);
		return 0;
	}

	return BytesWritten;
}

unsigned long 
SerialDevice::read(char *buffer, size_t len)
{
	unsigned long BytesReceived;

	FT_STATUS ftStatus = FT_Read(_fileHandle, buffer, len, &BytesReceived);
	if (ftStatus != FT_OK) {
		//throw SerialDeviceException("Error in FTDI API : FT_Read", ftStatus);
		return 0;
	}

	return BytesReceived;
}

void SerialDevice::flush(void)
{
	FT_STATUS ftStatus = FT_Purge(_fileHandle, FT_PURGE_RX | FT_PURGE_TX);
	if (ftStatus != FT_OK) {
		//throw SerialDeviceException("Error in FTDI API : FT_Purge, with params FT_PURGE_RX | FT_PURGE_TX", ftStatus);
		return; // no throw for now
	}
}

unsigned long
SerialDevice::hasBytes(void)
{
	unsigned long EventDWord, RxBytes, TxBytes;

	FT_STATUS ftStatus = FT_GetStatus(_fileHandle, &RxBytes, &TxBytes, &EventDWord);
	if (ftStatus != FT_OK) {
		return 0;
	}
	return RxBytes;
}