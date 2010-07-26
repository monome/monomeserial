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
#include "AsynchronousSerialDeviceReader.h"
#include "Message.h"
#include "Message256.h"
#include <sstream>
#include <stdexcept>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

using std::runtime_error;

void _AsynchronousSerialDeviceReaderCallbackWrapper(void *userData)
{
	AsynchronousSerialDeviceReader* reader = static_cast<AsynchronousSerialDeviceReader*>(userData);
	reader->_read();
}

AsynchronousSerialDeviceReader::AsynchronousSerialDeviceReader(size_t bufferSize)
{
	_serial_rx_in = _serial_rx_out = _serial_rx_buf = 0;

	_serial_rx_buf_size = bufferSize;
    _serial_rx_buf = new char[_serial_rx_buf_size];
    _serial_rx_in = _serial_rx_out = _serial_rx_buf;

	_readerThread = INVALID_HANDLE_VALUE;
	_terminateThread = false;
    InitializeCriticalSection(&_deviceContextsLock);
}

AsynchronousSerialDeviceReader::~AsynchronousSerialDeviceReader()
{
	stopReading();

	DeleteCriticalSection(&_deviceContextsLock);

	if (_serial_rx_buf)
	{
        delete[] _serial_rx_buf;
		_serial_rx_buf = 0;
	}
}

void 
AsynchronousSerialDeviceReader::addSerialDevice(SerialDevice *device, 
                                                size_t packetSize, 
                                                AsynchronousSerialDeviceReaderCallback callback,
                                                void *userData)
{
	if (device == 0 || packetSize == 0 || callback == 0)
        return;

	AsynchronousSerialDeviceReaderLock(this);
    vector<SerialDeviceContext>::iterator i;

    for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
		SerialDeviceContext context = *i;
		if (context.device == device) {
			context.packetSize = packetSize;
            context.callback = callback;
			context.userData = userData;

			return;
		}
    }

	SerialDeviceContext context = {device, packetSize, callback, userData};
	_deviceContexts.push_back(context);
}

void 
AsynchronousSerialDeviceReader::removeSerialDevice(SerialDevice *device)
{
	if (device == 0)
        return;

    AsynchronousSerialDeviceReaderLock(this);
    vector<SerialDeviceContext>::iterator i;

    for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
		if ((*i).device == device) {
			_deviceContexts.erase(i);
            break;
        }
    }
}

void 
AsynchronousSerialDeviceReader::startReading(void)
{
	if (_readerThread != INVALID_HANDLE_VALUE) {
        return;
	}

	_readerThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)_AsynchronousSerialDeviceReaderCallbackWrapper, this, 0, NULL);
	if (_readerThread == INVALID_HANDLE_VALUE) {
		throw runtime_error("Failed to create device reader thread.");
	}
}

void 
AsynchronousSerialDeviceReader::stopReading(void)
{
    if (_readerThread == INVALID_HANDLE_VALUE)
        return;

    _terminateThread = true;
	DWORD timeout = (int)((float)DEVICE_WAIT_TIMEOUT * 1.1f);

	if (WaitForSingleObject(_readerThread, timeout) == ERROR_TIMEOUT) {
		throw runtime_error("Failed to end device read thread");
	}
	
	CloseHandle(_readerThread);
	_readerThread = INVALID_HANDLE_VALUE;
}

void
AsynchronousSerialDeviceReader::_read()
{
	vector<SerialDeviceContext>::iterator i;
	DWORD EventMask = FT_EVENT_RXCHAR;
	DWORD eventResult = 0;
	int counter = 0;
	size_t numDevices = 0;
	int deviceNum = 0;

	while (!_terminateThread) {
		do {
			numDevices = _deviceContexts.size();
			Sleep(100);
		}
		while (numDevices == 0);

		EnterCriticalSection(&_deviceContextsLock); 
//		{
			HANDLE *waitObjects = new HANDLE[numDevices];

			counter = 0;
			for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
				SerialDeviceContext &context = *i;

				if (context.device && !context.device->unexpectedDeviceRemovalFlag()) {
					HANDLE eventObj = CreateEvent(NULL, FALSE, FALSE, NULL);
					if (eventObj == NULL || eventObj == INVALID_HANDLE_VALUE) {
						//throw runtime_error("Error setting Event Notification for SerialDevice");
#ifdef DEBUG_PRINT
						cout << "Error creating event (call CreateEvent function) for SerialDevice " << i->device->serialNumber();
						if (eventObj == NULL) {
							cout << ".  Last error = " << GetLastError();
						}
						cout << endl;
#endif
						continue;
					}
					
					if (FT_SetEventNotification(context.device->fileHandle(), EventMask, eventObj) != FT_OK) {
#ifdef DEBUG_PRINT
						cout << "Error calling FT_SetEventNotification() for SerialDevice " << i->device->serialNumber() << ".  Last error = " << GetLastError() << endl;
#endif
						continue;				
					}
					else {
						waitObjects[counter++] = eventObj;
					}
				}
			}
//		}
		LeaveCriticalSection(&_deviceContextsLock);

		while (true) {
			if (_terminateThread || (numDevices != _deviceContexts.size())) {
				break;
			}

			eventResult = WaitForMultipleObjects(numDevices, waitObjects, false, DEVICE_WAIT_TIMEOUT);
			if (eventResult == WAIT_FAILED) {
#ifdef DEBUG_PRINT
				cout << "WaitForMultipleObjects() call returned WAIT_FAILED.  Last error = " << GetLastError() << endl;
#endif	
			}
			else if (eventResult >= WAIT_ABANDONED_0) {
				deviceNum = eventResult - WAIT_ABANDONED_0;
//#ifdef DEBUG_PRINT  // Don't print this, since we will get a WAIT_ABANDONED every second that a device is not pressed
//				cout << "WaitForMultipleObjects() call returned WAIT_ABANDONED (for device : " << deviceNum << ")." << endl;
//#endif	
				continue;
			}
			else if ((eventResult == WAIT_TIMEOUT)) {
#ifdef DEBUG_PRINT
				cout << "WaitForMultipleObjects() call returned WAIT_TIMEOUT." << endl;
#endif	
				continue;
			}
			else {
				EnterCriticalSection(&_deviceContextsLock); 
//				{
					for (int i = eventResult - WAIT_OBJECT_0; i < numDevices; i++) {
						// get first matching device and loop from there.  we will skip a device if !device->hasBytes().
						if (_deviceContexts.size() == 0) {
							break;
						}
						SerialDeviceContext &context = _deviceContexts[i];
						SerialDevice *device = context.device;

						DWORD bytesToRead, bytesRead;
						bytesToRead = device->hasBytes();

						if (bytesToRead && (bytesRead = device->read(_serial_rx_in, bytesToRead)) > 0) {
							_serial_rx_in += bytesRead;

							if (_serial_rx_in >= _serial_rx_buf + _serial_rx_buf_size)
								_serial_rx_in -= _serial_rx_buf_size;

							while (_serial_rx_out != _serial_rx_in) {
								if (_serial_rx_in >= _serial_rx_out && _serial_rx_in - _serial_rx_out < context.packetSize)
									break;

								if (context.callback != 0 && 
									context.callback(device, _serial_rx_out, context.packetSize, context.userData) != 0) {
									_serial_rx_out = _serial_rx_in = _serial_rx_buf;

									//device->flush();
				                    
									break;
								}

								_serial_rx_out += context.packetSize;
								if (_serial_rx_out >= _serial_rx_buf + _serial_rx_buf_size)
									_serial_rx_out -= _serial_rx_buf_size;
							}
						}
					}
//				}
				LeaveCriticalSection(&_deviceContextsLock);

				if (_deviceContexts.size() != numDevices) {
					break;
				}
			}
		}

		for (int j = 0; j < _deviceContexts.size(); j++) {
			HANDLE _handle = waitObjects[j];
			if (_handle != INVALID_HANDLE_VALUE) {
				CloseHandle(waitObjects[j]);
			}
		}

		if (waitObjects) {
			delete[] waitObjects;
			waitObjects = 0;
		}
	}

	/*if (_serial_rx_buf) {
		delete[] _serial_rx_buf;
		_serial_rx_buf = 0;
	}*/
}
