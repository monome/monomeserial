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
#include "AsynchronousSerialDeviceReader.h"

void *_AsynchronousSerialDeviceReaderCallbackWrapper(void *userData)
{
    AsynchronousSerialDeviceReader *SELF = (AsynchronousSerialDeviceReader *)userData;
    SELF->_read();
	
	return NULL;
}

AsynchronousSerialDeviceReader::AsynchronousSerialDeviceReader(size_t bufferSize)
{
    _serial_rx_buf_size = bufferSize;
    _serial_rx_buf = new char[_serial_rx_buf_size];
    _serial_rx_in = _serial_rx_out = _serial_rx_buf;

	_pthread = 0;
    _terminate_pthread = false;
    pthread_mutex_init(&_deviceContextsLock, NULL);
}

AsynchronousSerialDeviceReader::~AsynchronousSerialDeviceReader()
{
    if (_pthread != 0) {
        _terminate_pthread = true;
        pthread_join(_pthread, NULL);
    }

    pthread_mutex_destroy(&_deviceContextsLock);

    if (_serial_rx_buf != 0)
        delete[] _serial_rx_buf;
}

void 
AsynchronousSerialDeviceReader::addSerialDevice(SerialDevice *device, 
                                                size_t packetSize, 
                                                AsynchronousSerialDeviceReaderCallback callback,
                                                void *userData)
{
    AsynchronousSerialDeviceReaderLock(this);
    vector<SerialDeviceContext>::iterator i;

    if (device == 0 || packetSize == 0 || callback == 0)
        return;

    for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
        if ((*i).device == device) {
            (*i).packetSize = packetSize;
            (*i).callback = callback;
            (*i).userData = userData;
            return;
        }
    }

    SerialDeviceContext context = { device, packetSize, callback, userData };
    _deviceContexts.push_back(context);
}

void 
AsynchronousSerialDeviceReader::removeSerialDevice(SerialDevice *device)
{
    AsynchronousSerialDeviceReaderLock(this);
    vector<SerialDeviceContext>::iterator i;

    if (device == 0)
        return;

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
    if (_pthread != 0)
        return;

    pthread_create(&_pthread, NULL, _AsynchronousSerialDeviceReaderCallbackWrapper, this);
}

void 
AsynchronousSerialDeviceReader::stopReading(void)
{
    if (_pthread == 0)
        return;

    _terminate_pthread = true;
    pthread_join(_pthread, NULL);
	
	_pthread = 0;
}

void 
AsynchronousSerialDeviceReader::_read(void)
{
    vector<SerialDeviceContext>::iterator i;
    int nfds;
    fd_set readfds;
    struct timeval timeout;
    size_t bytesToRead;
    ssize_t bytesRead;

    while (!_terminate_pthread) {
        pthread_mutex_lock(&_deviceContextsLock);

        nfds = 0;
        FD_ZERO(&readfds);

        for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
            SerialDeviceContext &context = *i;
            
            if (context.device && !context.device->unexpectedDeviceRemovalFlag()) {
                int fd = context.device->fileDescriptor();

                if (fd >= nfds)
                    nfds = fd + 1;
                
                FD_SET(fd, &readfds);
            }            
        }

        pthread_mutex_unlock(&_deviceContextsLock);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        nfds = select(nfds, &readfds, NULL, NULL, &timeout);
        
        if (_terminate_pthread)
            break;

        if (nfds > 0) {
            pthread_mutex_lock(&_deviceContextsLock);

            for (i = _deviceContexts.begin(); i != _deviceContexts.end(); i++) {
                SerialDeviceContext &context = *i;
                SerialDevice *device = context.device;

                if (device != 0 && !device->unexpectedDeviceRemovalFlag()) {
                    if (FD_ISSET(device->fileDescriptor(), &readfds)) {
                        if (_serial_rx_in >= _serial_rx_out)
                            bytesToRead = _serial_rx_buf + _serial_rx_buf_size - _serial_rx_in;
                        else 
                            bytesToRead = _serial_rx_out - _serial_rx_in;

                        if ((bytesRead = device->read(_serial_rx_in, bytesToRead)) > 0) {
                            _serial_rx_in += bytesRead;

                            if (_serial_rx_in >= _serial_rx_buf + _serial_rx_buf_size)
                                _serial_rx_in -= _serial_rx_buf_size;

                            while (_serial_rx_out != _serial_rx_in) {
                                if (_serial_rx_in >= _serial_rx_out && _serial_rx_in - _serial_rx_out < context.packetSize)
                                    break;

                                if (context.callback != 0 && 
                                    context.callback(device, _serial_rx_out, context.packetSize, context.userData) != 0) {
                                    _serial_rx_out = _serial_rx_in = _serial_rx_buf;

                                    device->flush();
                                    
                                    break;
                                }

                                _serial_rx_out += context.packetSize;
                                if (_serial_rx_out >= _serial_rx_buf + _serial_rx_buf_size)
                                    _serial_rx_out -= _serial_rx_buf_size;
                            }
                        }
                    }
                }
            }

            pthread_mutex_unlock(&_deviceContextsLock);
        }
    }
}
