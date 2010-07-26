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
#include "../stdafx.h"
#include "MonomeDeviceDefaults.h"
#include <list>
#include <stdexcept>
#include <fstream>

using namespace std;

const string ORIENTATION_UP = "up";
const string ORIENTATION_DOWN = "down";
const string ORIENTATION_LEFT = "left";
const string ORIENTATION_RIGHT = "right";

const string& getOrientationString(MonomeXXhDevice::CableOrientation orientation)
{
	switch(orientation) {
		case MonomeXXhDevice::kCableOrientation_Bottom:
			return ORIENTATION_DOWN;
		case MonomeXXhDevice::kCableOrientation_Right:
			return ORIENTATION_RIGHT;
		case MonomeXXhDevice::kCableOrientation_Top:
			return ORIENTATION_UP;
		case MonomeXXhDevice::kCableOrientation_Left:
		default:
			return ORIENTATION_LEFT;
	}
}

MonomeXXhDevice::CableOrientation getOrientationValue(const string& orientationStr)
{
	if (orientationStr == ORIENTATION_UP) {
		return MonomeXXhDevice::kCableOrientation_Top;
	}
	else if (orientationStr == ORIENTATION_DOWN) {
		return MonomeXXhDevice::kCableOrientation_Bottom;
	}
	else if (orientationStr == ORIENTATION_RIGHT) {
		return MonomeXXhDevice::kCableOrientation_Right;
	}
	else {
		return MonomeXXhDevice::kCableOrientation_Left;
	}
}


MonomeDeviceDefaults::MonomeDeviceDefaults(MonomeXXhDevice *device, CCoreMIDI *coreMIDI)
{
    if (device == 0 || coreMIDI == 0)
		return;

// removed  device->type() != MonomeXXhDevice::kDeviceType_40h || 

    _coreMIDI = coreMIDI;

    memcpy(_serialNumber, device->serialNumber().c_str(), kMonomeXXhDevice_SerialNumberLength);
	
    _orientation = device->cableOrientation();

    _oscAddressPatternPrefix = device->oscAddressPatternPrefix();
    _oscStartColumn = device->oscStartColumn();
    _oscStartRow = device->oscStartRow();

	_oscHostPort = device->OscHostPort();
	_oscListenPort = device->OscListenPort();
	_oscHostAddress = device->OscHostAddress();

    _oscAdcOffset = device->oscAdcOffset();
    _oscEncOffset = device->oscEncOffset();

	for (unsigned int i = 0; i < 4; i++) {
        _adcState[i] = device->adcEnableState(i);
	}

	for (unsigned int i = 0; i < 2; i++) {
        _encState[i] = device->encEnableState(i);
	}

    _midiInputDeviceName = _getMIDIInputDeviceNameForEndpointRef(device->MIDIInputDevice());
    _midiOutputDeviceName = _getMIDIOutputDeviceNameForEndpointRef(device->MIDIOutputDevice());
    _midiInputChannel = device->MIDIInputChannel();
    _midiOutputChannel = device->MIDIOutputChannel();
}

MonomeDeviceDefaults::MonomeDeviceDefaults(std::ifstream& stream, CCoreMIDI *coreMIDI)
{
	_coreMIDI = coreMIDI;

	memset(_serialNumber, 0, kMonomeXXhDevice_SerialNumberLength);
	
	_orientation = MonomeXXhDevice::kCableOrientation_Left;

    _oscAddressPatternPrefix = "/box";
    _oscStartColumn = 0;
    _oscStartRow = 0;

	_oscHostPort = 8000;
	_oscListenPort = 8080;
	_oscHostAddress = "127.0.0.1";

    _oscAdcOffset = 0;
    _oscEncOffset = 0;

	memset(_adcState, false, 4);
	memset(_encState, false, 2);

    _midiInputDeviceName = "";
    _midiOutputDeviceName = "";
    _midiInputChannel = 0;
    _midiOutputChannel = 0;

    deserialize(stream);
}

const char*
MonomeDeviceDefaults::serialNumber(void) const
{
    return _serialNumber;
}

void 
MonomeDeviceDefaults::setDeviceStateFromDefaults(MonomeXXhDevice *device)
{
    if (device == 0)
        return;

    device->setCableOrientation(_orientation);
    device->setOscAddressPatternPrefix(_oscAddressPatternPrefix);
    device->setOscStartColumn(_oscStartColumn);
    device->setOscStartRow(_oscStartRow);
    device->setOscAdcOffset(_oscAdcOffset);
    device->setOscEncOffset(_oscEncOffset);
	device->setOscHostPort(_oscHostPort);
	device->setOSCListenPort(_oscListenPort);
	device->setOscHostAddress(_oscHostAddress);

	for (unsigned int i = 0; i < NUM_ADCS; i++) {
        device->oscAdcEnableStateChangeEvent(i, _adcState[i]);
	}

	for (unsigned int i = 0; i < NUM_ENCODERS; i++) {
        device->oscEncEnableStateChangeEvent(i, _encState[i]);
	}

	device->setMIDIInputChannel(_midiInputChannel);
    device->setMIDIOutputChannel(_midiOutputChannel);

	// create the midi device if it doesn't exist and is valid
	CCoreMIDIEndpointRef inEndpoint, outEndpoint;
	inEndpoint = outEndpoint = 0;

	/* input device */
	try {
		const list<string> inDevices = CCoreMIDI::getMidiInputDeviceSources();
		list<string>::const_iterator i;
		if (device->MIDIInputDevice() == 0) {
			for (i = inDevices.begin(); i != inDevices.end(); i++) {
				if (*i == _midiInputDeviceName) {
					inEndpoint = _coreMIDI->createInputDevice(_midiInputDeviceName);
					break;
				}
			}
		}

		device->setMIDIInputDevice(inEndpoint);
	}
	catch(...) {
		device->setMIDIInputDevice(0);
	}

	/* output device */
	try {
		const list<string> outDevices = CCoreMIDI::getMidiOutputDeviceSources();
		list<string>::const_iterator i;
		if (device->MIDIOutputDevice() == 0) {
			for (i = outDevices.begin(); i != outDevices.end(); i++) {
				if (*i == _midiOutputDeviceName) {
					outEndpoint = _coreMIDI->createOutputDevice(_midiOutputDeviceName);
					break;
				}
			}
		}

		device->setMIDIOutputDevice(outEndpoint);
	}
	catch(...) {
		device->setMIDIOutputDevice(0);
	}
}

void 
MonomeDeviceDefaults::setDefaultsFromDeviceState(MonomeXXhDevice *device)
{
    if (device == 0)
        return;

    memcpy(_serialNumber, device->serialNumber().c_str(), kMonomeXXhDevice_SerialNumberLength);

    _orientation = device->cableOrientation();

    _oscAddressPatternPrefix = device->oscAddressPatternPrefix();
    _oscStartColumn = device->oscStartColumn();
    _oscStartRow = device->oscStartRow();

    _oscAdcOffset = device->oscAdcOffset();
    _oscEncOffset = device->oscEncOffset();

	_oscHostPort = device->OscHostPort();
	_oscListenPort = device->OscListenPort();
	_oscHostAddress = device->OscHostAddress();

	for (unsigned int i = 0; i < NUM_ADCS; i++) {
        _adcState[i] = device->adcEnableState(i);
	}

	for (unsigned int i = 0; i < NUM_ENCODERS; i++) {
        _encState[i] = device->encEnableState(i);
	}

    _midiInputDeviceName = _getMIDIInputDeviceNameForEndpointRef(device->MIDIInputDevice());
    _midiOutputDeviceName = _getMIDIOutputDeviceNameForEndpointRef(device->MIDIOutputDevice());
    _midiInputChannel = device->MIDIInputChannel();
    _midiOutputChannel = device->MIDIOutputChannel();
}


void 
MonomeDeviceDefaults::serialize(std::ofstream& stream) {
	stream << "serial : " << _serialNumber << endl
		<< "orientation : " << getOrientationString(_orientation) << endl
		<< "prefix : " << _oscAddressPatternPrefix << endl
		<< "hostport : " << _oscHostPort << endl
		<< "listenport : " <<  _oscListenPort << endl
		<< "hostaddress : " << _oscHostAddress << endl
		<< "startcol : " << _oscStartColumn << endl
		<< "startrow : " << _oscStartRow << endl
		<< "adcoffset : " << _oscAdcOffset << endl
		<< "encoffset : " << _oscEncOffset << endl
		<< "mididevnamein : " << _midiInputDeviceName << endl
		<< "mididevnameout : " << _midiOutputDeviceName << endl
		<< "midichanin : " << _midiInputChannel << endl
		<< "midichanout : " << _midiOutputChannel << endl
		<< "adcstates : ";

	for(int i = 0; i < NUM_ADCS; i++) {
		int state = _adcState[i] > 0 ? 1 : 0;
		stream << state << " ";
	}

	stream << endl
		<< "encstates : ";

	for(int i = 0; i < NUM_ENCODERS; i++) {
		int state = _encState[i] > 0 ? 1 : 0;
		stream << state << " ";
	}

	stream << endl;
}

void
MonomeDeviceDefaults::deserialize(std::ifstream& stream)
{
	const int BUFFER_SIZE = 256;

	char linebuffer[BUFFER_SIZE];

	while(!stream.eof()) {
		// get data
		memset(linebuffer, 0, BUFFER_SIZE);
		stream.getline(linebuffer, BUFFER_SIZE);

		// parse data
		string firstpart;
		string secondpart;
		bool onsecond = false;
		for(int i = 0; i < BUFFER_SIZE; i++) {
			char val = *(linebuffer + i);
			if (val == ' ') {
				continue;
			}

			if (!onsecond) {
				if (val == ':') {
					onsecond = true;
				}
				else {
					firstpart += val;
				}
			}
			else {
				if (val == 0) {
					break;
				}
				else {
					secondpart += val;
				}
			}
		}

		// set values
		if (firstpart == "device") {
			return;
		}
		else if (firstpart == "serial") {
			for(int i = 0; i < secondpart.size() && i < kMonomeXXhDevice_SerialNumberLength; i++) {
				this->_serialNumber[i] = secondpart[i];
			}
		}
		else if(firstpart == "orientation") {
			this->_orientation = getOrientationValue(secondpart);
		}
		else if(firstpart == "prefix") {
			this->_oscAddressPatternPrefix = secondpart;
		}
		else if(firstpart == "hostport") {
			this->_oscHostPort = atoi(secondpart.c_str());
		}
		else if(firstpart == "listenport") {
			this->_oscListenPort = atoi(secondpart.c_str());
		}
		else if(firstpart == "hostaddress") {
			this->_oscHostAddress = secondpart;
		}
		else if(firstpart == "startcol") {
			this->_oscStartColumn = atoi(secondpart.c_str());
		}
		else if(firstpart == "startrow") {
			this->_oscStartRow = atoi(secondpart.c_str());
		}
		else if(firstpart == "adcoffset") {
			this->_oscAdcOffset = atoi(secondpart.c_str());
		}
		else if(firstpart == "encoffset") {
			this->_oscEncOffset = atoi(secondpart.c_str());
		}
		else if(firstpart == "mididevnamein") {
			this->_midiInputDeviceName = secondpart;
		}
		else if(firstpart == "mididevnameout") {
			this->_midiOutputDeviceName = secondpart;
		}
		else if(firstpart == "midichanin") {
			this->_midiInputChannel = atoi(secondpart.c_str());
		}
		else if(firstpart == "midichanout") {
			this->_midiOutputChannel = atoi(secondpart.c_str());
		}
		else if(firstpart == "adcstates") {
			int count = 0;
			for(int i = 0; i < secondpart.length(); i++) {
				if (secondpart[i] == ' ') {
					continue;
				}
				else {
					this->_adcState[count++] = secondpart[i] != '0';
				}
			}
		}
		else if(firstpart == "encstates") {
			int count = 0;
			for(int i = 0; i < secondpart.length(); i++) {
				if (secondpart[i] == ' ') {
					continue;
				}
				else {
					this->_encState[count++] = secondpart[i] != '0';
				}
			}
		}
	}
}

string 
MonomeDeviceDefaults::_getMIDIInputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpointRef)
{
    unsigned int i;

    if (endpointRef == 0)
        return string();

    for (i = 0; i < _coreMIDI->getNumberOfInputDevices(); i++) { 
        CCoreMIDIEndpointRef e = _coreMIDI->getEndpointRefForInputDevice(i);

        if (e == endpointRef) 
            return _coreMIDI->getInputDeviceName(i);
    }

    return string();
}

string 
MonomeDeviceDefaults::_getMIDIOutputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpointRef)
{
    unsigned int i;

    if (endpointRef == 0)
        return string();

    for (i = 0; i < _coreMIDI->getNumberOfOutputDevices(); i++) { 
        CCoreMIDIEndpointRef e = _coreMIDI->getEndpointRefForOutputDevice(i);

        if (e == endpointRef) 
            return _coreMIDI->getOutputDeviceName(i);
    }

    return string();
}

CCoreMIDIEndpointRef 
MonomeDeviceDefaults::_getMIDIEnpointRefForInputDeviceWithName(const string& name)
{
    unsigned int i;

    for (i = 0; i < _coreMIDI->getNumberOfInputDevices(); i++) {
		if (name == CCoreMIDI::getInputDeviceName(i))
            return _coreMIDI->getEndpointRefForInputDevice(i);
    }

    return _coreMIDI->getEndpointRefForInputDevice(0);
}

CCoreMIDIEndpointRef 
MonomeDeviceDefaults::_getMIDIEnpointRefForOutputDeviceWithName(const string& name)
{
    unsigned int i;

    for (i = 0; i < _coreMIDI->getNumberOfOutputDevices(); i++) {
        if (name == _coreMIDI->getOutputDeviceName(i))
            return _coreMIDI->getEndpointRefForOutputDevice(i);
    }

    return _coreMIDI->getEndpointRefForOutputDevice(0);
}
