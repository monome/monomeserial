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
#include "MonomeDeviceDefaults.h"

MonomeDeviceDefaults::MonomeDeviceDefaults(MonomeXXhDevice *device, CCoreMIDI *coreMIDI)
{
    if (device == 0 || coreMIDI == 0)
        throw -1; // this could be a real exceptions someday.

// removed  device->type() != MonomeXXhDevice::kDeviceType_40h || 
    _coreMIDI = coreMIDI;
    memcpy(_serialNumber, device->serialNumber(), kMonomeXXhDevice_SerialNumberLength);

    _orientation = (MonomeXXhDevice::CableOrientation) MonomeXXhDevice::kCableOrientation_Left;//device->cableOrientation();
	if (device->type() ==  MonomeXXhDevice::kDeviceType_128) _orientation = MonomeXXhDevice::kCableOrientation_Top;
	
	if (device->type() ==  MonomeXXhDevice::kDeviceType_128) 
	
    _oscAddressPatternPrefix = device->oscAddressPatternPrefix();
    _oscStartColumn = device->oscStartColumn();
    _oscStartRow = device->oscStartRow();

    _oscAdcOffset = device->oscAdcOffset();
    _oscEncOffset = device->oscEncOffset();

    for (unsigned int i = 0; i < 4; i++)
        _adcState[i] = device->adcEnableState(i);

    for (unsigned int i = 0; i < 2; i++)
        _encState[i] = device->encEnableState(i);

    _midiInputDeviceName = _getMIDIInputDeviceNameForEndpointRef(device->MIDIInputDevice());
    _midiOutputDeviceName = _getMIDIOutputDeviceNameForEndpointRef(device->MIDIOutputDevice());
    _midiInputChannel = device->MIDIInputChannel();
    _midiOutputChannel = device->MIDIOutputChannel();
	
		
}

MonomeDeviceDefaults::MonomeDeviceDefaults(t_DeviceDefaults *defaults, CCoreMIDI *coreMIDI)
{
    _coreMIDI = coreMIDI;

    switch (defaults->cableOrientation) {
    case 1:
        _orientation = MonomeXXhDevice::kCableOrientation_Top;
        break;
      
    case 2:
        _orientation = MonomeXXhDevice::kCableOrientation_Right;
        break;
      
    case 3:
        _orientation = MonomeXXhDevice::kCableOrientation_Bottom;
        break;
      
    default:
        _orientation = MonomeXXhDevice::kCableOrientation_Left;
        break;
		//if (MonomeXXhDevice::type()==kDeviceType_128)
	
    }      

    memcpy(_serialNumber, defaults->serialNumber, kMonomeXXhDevice_SerialNumberLength);

    _oscAddressPatternPrefix = string((const char *)defaults->oscAddressPatternPrefix);
    _oscStartColumn = defaults->oscStartColumn;
    _oscStartRow = defaults->oscStartRow;
    _oscAdcOffset = defaults->oscAdcOffset;
    _oscEncOffset = defaults->oscEncOffset;

    for (unsigned int i = 0; i < 4; i++)
        _adcState[i] = defaults->adcEnableState[i];

    for (unsigned int i = 0; i < 2; i++)
        _encState[i] = defaults->encEnableState[i];

    _midiInputDeviceName = string((const char *)defaults->midiInputDeviceName);
    _midiOutputDeviceName = string((const char *)defaults->midiOutputDeviceName);
    _midiInputChannel = defaults->midiInputChannel;
    _midiOutputChannel = defaults->midiOutputChannel;
}

const char *
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
	//fprintf(stderr, "setdevicestatefromdefaults orientation is %i", _orientation);
    device->setOscAddressPatternPrefix(_oscAddressPatternPrefix);
    device->setOscStartColumn(_oscStartColumn);
    device->setOscStartRow(_oscStartRow);
    device->setOscAdcOffset(_oscAdcOffset);
    device->setOscEncOffset(_oscEncOffset);

    for (unsigned int i = 0; i < 4; i++) 
        device->oscAdcEnableStateChangeEvent(i, _adcState[i]);

    for (unsigned int i = 0; i < 2; i++)
        device->oscEncEnableStateChangeEvent(i, _encState[i]);

    device->setMIDIInputDevice(_getMIDIEnpointRefForInputDeviceWithName(_midiInputDeviceName));
    device->setMIDIOutputDevice(_getMIDIEnpointRefForOutputDeviceWithName(_midiOutputDeviceName));
    device->setMIDIInputChannel(_midiInputChannel);
    device->setMIDIOutputChannel(_midiOutputChannel);
}

void 
MonomeDeviceDefaults::setDefaultsFromDeviceState(MonomeXXhDevice *device)
{
    if (device == 0)// || device->type() != MonomeXXhDevice::kDeviceType_40h)
        return;

    memcpy(_serialNumber, device->serialNumber(), kMonomeXXhDevice_SerialNumberLength);

    _orientation = device->cableOrientation();

    _oscAddressPatternPrefix = device->oscAddressPatternPrefix();
    _oscStartColumn = device->oscStartColumn();
    _oscStartRow = device->oscStartRow();

    _oscAdcOffset = device->oscAdcOffset();
    _oscEncOffset = device->oscEncOffset();

    for (unsigned int i = 0; i < 4; i++)
        _adcState[i] = device->adcEnableState(i);

    for (unsigned int i = 0; i < 2; i++)
        _encState[i] = device->encEnableState(i);

    _midiInputDeviceName = _getMIDIInputDeviceNameForEndpointRef(device->MIDIInputDevice());
    _midiOutputDeviceName = _getMIDIOutputDeviceNameForEndpointRef(device->MIDIOutputDevice());
    _midiInputChannel = device->MIDIInputChannel();
    _midiOutputChannel = device->MIDIOutputChannel();
}

void 
MonomeDeviceDefaults::serialize(t_DeviceDefaults *defaults)
{
    memcpy(defaults->serialNumber, _serialNumber, kMonomeXXhDevice_SerialNumberLength);
    defaults->cableOrientation = _orientation;
	
    strncpy((char *)defaults->oscAddressPatternPrefix, _oscAddressPatternPrefix.c_str(), 256);
    defaults->oscStartColumn = _oscStartColumn;
    defaults->oscStartRow = _oscStartRow;
    defaults->oscAdcOffset = _oscAdcOffset;
    defaults->oscEncOffset = _oscEncOffset;

    for (unsigned int i = 0; i < 4; i++)
        defaults->adcEnableState[i] = _adcState[i];

    for (unsigned int i = 0; i < 2; i++)
        defaults->encEnableState[i] = _encState[i];

    strncpy((char *)defaults->midiInputDeviceName, _midiInputDeviceName.c_str(), 256);
    strncpy((char *)defaults->midiOutputDeviceName, _midiOutputDeviceName.c_str(), 256);
    
    defaults->midiInputChannel = _midiInputChannel;
    defaults->midiOutputChannel = _midiOutputChannel;
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
        if (name == _coreMIDI->getInputDeviceName(i))
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

