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


#ifndef __CCoreMIDI_h__
#define __CCoreMIDI_h__

#include "CoreMIDITypes.h"

using namespace midi;
using namespace std;


class CCoreMIDI
{
public:
	CCoreMIDI(CCoreMIDIReceiveCallback receiveCallback, void* owner);
    ~CCoreMIDI();

public: // public interface
    CCoreMIDIEndpointRef getEndpointRefForInputDevice(const string &deviceName);
	CCoreMIDIEndpointRef getEndpointRefForInputDevice(int deviceId);
	CCoreMIDIEndpointRef getEndpointRefForOutputDevice(const string &deviceName);
    CCoreMIDIEndpointRef getEndpointRefForOutputDevice(int deviceId);

	const string getInputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpoint);
	int getInputDeviceIDForEndpointRef(CCoreMIDIEndpointRef endpoint);
	const string getOutputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpoint);
	int getOutputDeviceIDForEndpointRef(CCoreMIDIEndpointRef endpoint);

    CCoreMIDIEndpointRef createInputDevice(const string& destinationName);
	CCoreMIDIEndpointRef createInputDevice(int deviceID);
	CCoreMIDIEndpointRef createOutputDevice(const string& sourceName);
	CCoreMIDIEndpointRef createOutputDevice(int deviceID);

    void closeOutputDevice(CCoreMIDIEndpointRef destinationRef);
	void closeInputDevice(CCoreMIDIEndpointRef sourceRef);

	void sendShort(CCoreMIDIEndpointRef endpointRef, char midiStatusByte, char midiDataByte1, char midiDataByte2);

	void* getOwner()
	{
		return _owner;
	}

	CCoreMIDIReceiveCallback getCallback()
	{
		return _receiveCallback;
	}

#pragma region Static Interface (Inline Methods)

public: // public static interface

	static const int invalidDeviceID = -1;

	static int getNumberOfInputDevices(void) 
	{ 
		return CCoreMIDIReceive::getNumberOfInputDevices(); 
	}

	static int getNumberOfOutputDevices(void) 
	{ 
		return CCoreMIDISend::getNumberOfOutputDevices(); 
	}

    static string getInputDeviceName(int inputDevice) 
	{
		return CCoreMIDIReceive::getInputDeviceName(inputDevice);
	}

    static string getOutputDeviceName(int outputDevice)	
	{
		return CCoreMIDISend::getOutputDeviceName(outputDevice);
	}

	static int getMidiInputDeviceByName(const string &device) 
	{
		return CCoreMIDIReceive::getMidiInputDeviceByName(device);
	}

	static int getMidiOutputDeviceByName(const string &device) 
	{
		return CCoreMIDISend::getMidiOutputDeviceByName(device);
	}
	
	static const list<string> getMidiInputDeviceSources() 
	{
		return CCoreMIDIReceive::getMidiInputDeviceSources();
	}

	static const list<string> getMidiOutputDeviceSources() 
	{
		return CCoreMIDISend::getMidiOutputDeviceSources();
	}

#pragma endregion

private:
    list<CCoreMIDIEndpoint *> _myOutputDevices;
    list<CCoreMIDIEndpoint *> _myInputDevices;

	CCoreMIDIReceiveCallback _receiveCallback;
	void* _owner;
};




#endif // __CCoreMIDI_h__