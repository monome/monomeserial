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
#ifndef __MonomeALLDefaults_h__
#define __MonomeALLDefaults_h__

#include "Types.h"
#include "MonomeXXhDevice.h"

#include <fstream>
#include <string>

const int NUM_ADCS = 4;
const int NUM_ENCODERS = 2;

class MonomeDeviceDefaults //defaults for any device type
{
public:
    MonomeDeviceDefaults(MonomeXXhDevice *device, CCoreMIDI *coreMIDI);
	MonomeDeviceDefaults(std::ifstream& stream, CCoreMIDI *coreMIDI);

    const char *serialNumber(void) const;

    void setDeviceStateFromDefaults(MonomeXXhDevice *device);
    void setDefaultsFromDeviceState(MonomeXXhDevice *device);

	void serialize(std::ofstream& stream);
	void deserialize(std::ifstream& stream);

private:
    string _getMIDIInputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpointRef);
    string _getMIDIOutputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpointRef);

    CCoreMIDIEndpointRef _getMIDIEnpointRefForInputDeviceWithName(const string& name);
    CCoreMIDIEndpointRef _getMIDIEnpointRefForOutputDeviceWithName(const string& name);

private:
    CCoreMIDI *_coreMIDI;

    MonomeXXhDevice::CableOrientation _orientation;

    char _serialNumber[kMonomeXXhDevice_SerialNumberLength];

    string _oscAddressPatternPrefix;
	unsigned int _oscHostPort;
	unsigned int _oscListenPort;
	std::string _oscHostAddress;

    unsigned int _oscStartColumn;
    unsigned int _oscStartRow;

    unsigned int _oscAdcOffset;
    unsigned int _oscEncOffset;

    bool _adcState[4];
    bool _encState[2];

    string _midiInputDeviceName;
    string _midiOutputDeviceName;
    unsigned char _midiInputChannel;
    unsigned char _midiOutputChannel;
};

#endif // __MonomeALLDefaults_h__
