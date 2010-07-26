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


#ifndef __AsynchronousSerialDeviceReader_h__
#define __AsynchronousSerialDeviceReader_h__

#include "SerialDevice.h"
#include <vector>
using namespace std;

typedef int (*AsynchronousSerialDeviceReaderCallback)(SerialDevice *, char *data, size_t len, void *userData);

class AsynchronousSerialDeviceReader
{
private:
	typedef struct {
		SerialDevice *device;
		size_t packetSize;
		AsynchronousSerialDeviceReaderCallback callback;
		void *userData;
	} SerialDeviceContext;

	enum { DEVICE_WAIT_TIMEOUT = 1000 };

public:
    AsynchronousSerialDeviceReader(size_t bufferSize = 1024);
    ~AsynchronousSerialDeviceReader(void);

    void addSerialDevice(SerialDevice *device, size_t packetSize, AsynchronousSerialDeviceReaderCallback callback, void *userData);
	void removeSerialDevice(SerialDevice *device);
	void startReading(void);
	void stopReading(void);

private:
	void _read(void);

private:
    vector<SerialDeviceContext> _deviceContexts;
	char *_serial_rx_in, *_serial_rx_out, *_serial_rx_buf;
    size_t _serial_rx_buf_size;

	HANDLE _readerThread;
    CRITICAL_SECTION _deviceContextsLock;
	bool _terminateThread;

    class AsynchronousSerialDeviceReaderLock 
	{
    public:
        AsynchronousSerialDeviceReaderLock(AsynchronousSerialDeviceReader *reader) 
		{ 
			EnterCriticalSection(_lock = &(reader->_deviceContextsLock)); 
		}
        ~AsynchronousSerialDeviceReaderLock() { LeaveCriticalSection(_lock); }

    private:
        CRITICAL_SECTION *_lock;
    };

    friend class AsynchronousSerialDeviceReaderLock;
	friend void _AsynchronousSerialDeviceReaderCallbackWrapper(void *userData);

};

#endif // __AsynchronousSerialDeviceReader_h__
