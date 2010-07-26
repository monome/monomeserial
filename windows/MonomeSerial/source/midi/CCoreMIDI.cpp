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
/***********
* Current code by Daniel Battaglia and Steve Duda.
* Released under the original GPL
************/


#include "../stdafx.h"
#include "CCoreMIDI.h"

#ifdef DEBUG_PRINT
#include <iostream>
#endif

CCoreMIDI::CCoreMIDI(CCoreMIDIReceiveCallback receiveCallback, void* owner)
{
	_receiveCallback = receiveCallback;
	_owner = owner;
}

CCoreMIDI::~CCoreMIDI()
{
	list<CCoreMIDIEndpoint *>::iterator i;
	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); ++i) {
		CCoreMIDISend* send = dynamic_cast<CCoreMIDISend*>(*i);
		delete send;
	}
	_myOutputDevices.clear();


	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); ++i) {
		CCoreMIDIReceive* recv = dynamic_cast<CCoreMIDIReceive*>(*i);
		delete recv;
	}
	_myInputDevices.clear();
}


CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForInputDevice(const string &deviceName)
{
	list<CCoreMIDIEndpoint *>::iterator i;
	CCoreMIDIEndpoint* currentDevice = NULL;
	CCoreMIDIEndpointRef ref = NULL;

	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); i++) {
		currentDevice = static_cast<CCoreMIDIEndpoint*>(*i);
		if (currentDevice->getDeviceName() == deviceName) {
			ref = static_cast<CCoreMIDIEndpointRef>(currentDevice);
		}
	}

	return ref;
}

CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForInputDevice(int deviceId)
{
	list<CCoreMIDIEndpoint *>::iterator i;
	CCoreMIDIEndpoint* currentDevice = NULL;
	CCoreMIDIEndpointRef ref = NULL;

	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); i++) {
		currentDevice = static_cast<CCoreMIDIEndpoint*>(*i);
		if (currentDevice->getDeviceID() == deviceId) {
			ref = static_cast<CCoreMIDIEndpointRef>(currentDevice);
		}
	}

	return ref;
}

CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForOutputDevice(const string &deviceName)
{
	list<CCoreMIDIEndpoint *>::iterator i;
	CCoreMIDIEndpoint* currentDevice = NULL;
	CCoreMIDIEndpointRef ref = NULL;

	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		currentDevice = static_cast<CCoreMIDIEndpoint*>(*i);
		if (currentDevice->getDeviceName() == deviceName) {
			ref = static_cast<CCoreMIDIEndpointRef>(currentDevice);
		}
	}

	return ref;
}

CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForOutputDevice(int deviceId)
{
	list<CCoreMIDIEndpoint *>::iterator i;
	CCoreMIDIEndpoint* currentDevice = NULL;
	CCoreMIDIEndpointRef ref = NULL;

	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		currentDevice = static_cast<CCoreMIDIEndpoint*>(*i);
		if (currentDevice->getDeviceID() == deviceId) {
			ref = static_cast<CCoreMIDIEndpointRef>(currentDevice);
		}
	}

	return ref;
}

const string
CCoreMIDI::getInputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpoint)
{
	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); i++) {
		if (*i == endpoint) {
			return (static_cast<CCoreMIDIEndpoint*>(endpoint))->getDeviceName();
		}
	}

	return string();
}

int 
CCoreMIDI::getInputDeviceIDForEndpointRef(CCoreMIDIEndpointRef endpoint)
{
	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); i++) {
		if (*i == endpoint) {
			return (static_cast<CCoreMIDIEndpoint*>(endpoint))->getDeviceID();
		}
	}

	return CCoreMIDI::invalidDeviceID;
}

const string
CCoreMIDI::getOutputDeviceNameForEndpointRef(CCoreMIDIEndpointRef endpoint)
{
	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		if (*i == endpoint) {
			return (static_cast<CCoreMIDIEndpoint*>(endpoint))->getDeviceName();
		}
	}

	return string();
}

int 
CCoreMIDI::getOutputDeviceIDForEndpointRef(CCoreMIDIEndpointRef endpoint)
{
	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		if (*i == endpoint) {
			return (static_cast<CCoreMIDIEndpoint*>(endpoint))->getDeviceID();
		}
	}

	return CCoreMIDI::invalidDeviceID;
}


CCoreMIDIEndpointRef
CCoreMIDI::createInputDevice(const string& destinationName)
{
	CCoreMIDIEndpoint *ref;
	if ((ref = static_cast<CCoreMIDIEndpoint*>(this->getEndpointRefForInputDevice(destinationName))) != NULL)
		return ref;

	const list<string> devices = CCoreMIDI::getMidiInputDeviceSources();
	list<string>::const_iterator i;
	bool found = false;
	for (i = devices.begin(); i != devices.end(); i++) {
		if (*i == destinationName) {
			found = true;
			break;
		}
	}

	if (!found) {
		return NULL;
	}

	try {
		ref = new CCoreMIDIReceive(destinationName, this->_receiveCallback, this->_owner);
	}
	catch (CMIDIInException &ex) {
		AfxMessageBox(L"An error has occured while attempting to create a new MIDI In Connection.");
		return 0;
	}

	_myInputDevices.push_back(ref);

	return ref;
}

CCoreMIDIEndpointRef
CCoreMIDI::createInputDevice(int deviceID)
{

	CCoreMIDIEndpoint *ref;
	if ((ref = static_cast<CCoreMIDIEndpoint*>(this->getEndpointRefForInputDevice(deviceID))) != NULL) {
		return ref;
	}

	if (deviceID < 0 || deviceID > CCoreMIDI::getNumberOfInputDevices()) {
		return NULL;
	}

	try {
		ref = new CCoreMIDIReceive(deviceID, this->_receiveCallback, this->_owner);
	}
	catch (CMIDIInException &ex) {
		AfxMessageBox(L"An error has occured while attempting to create a new MIDI In Connection.");
		return 0;
	}

#ifdef DEBUG_PRINT
	cout << "\n...Created input device!..." << endl;
#endif

	_myInputDevices.push_back(ref);

	return ref;
}

CCoreMIDIEndpointRef
CCoreMIDI::createOutputDevice(const string& sourceName)
{
	CCoreMIDIEndpoint *ref;
	if ((ref = static_cast<CCoreMIDIEndpoint*>(this->getEndpointRefForOutputDevice(sourceName))) != NULL) {
		return ref;
	}

	const list<string> devices = CCoreMIDI::getMidiOutputDeviceSources();
	list<string>::const_iterator i;
	bool found = false;
	for (i = devices.begin(); i != devices.end(); i++) {
		if (*i == sourceName) {
			found = true;
			break;
		}
	}

	if (!found) {
		return NULL;
	}


	try {
		CCoreMIDISend* snd = new CCoreMIDISend(sourceName);
		ref = static_cast<CCoreMIDIEndpoint*>(snd);
	}
	catch (midi::CMIDIOutException &ex) {
		AfxMessageBox(L"An error has occured while attempting to create a new MIDI Out Connection.");
		return 0;
	}

	if (ref == 0) {
		return 0;
	}

	_myOutputDevices.push_back(ref);

	return ref;
}

CCoreMIDIEndpointRef
CCoreMIDI::createOutputDevice(int deviceID)
{
	CCoreMIDIEndpoint *ref;
	if ((ref = static_cast<CCoreMIDIEndpoint*>(this->getEndpointRefForOutputDevice(deviceID))) != NULL)
		return ref;

	if (deviceID < 0 || deviceID > CCoreMIDI::getNumberOfOutputDevices()) {
		return NULL;
	}

	try {
		ref = new CCoreMIDISend(deviceID);
	}
	catch (midi::CMIDIOutException &ex) {
		AfxMessageBox(L"An error has occured while attempting to create a new MIDI Out Connection.");
		return 0;
	}

	_myOutputDevices.push_back(ref);

	return ref;
}

void
CCoreMIDI::closeInputDevice(CCoreMIDIEndpointRef destinationRef)
{
	if (destinationRef == NULL) {
		return;
	}

	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myInputDevices.begin(); i != _myInputDevices.end(); i++) {
		CCoreMIDIEndpoint *ep = *i;
		if (ep == destinationRef) {
			_myInputDevices.remove(ep);
			delete ep;
			break;
		}
	}
}

void 
CCoreMIDI::closeOutputDevice(CCoreMIDIEndpointRef sourceRef)
{
	if (sourceRef == NULL) {
		return;
	}

	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		CCoreMIDIEndpoint *ep = *i;
		if (ep == sourceRef) {
			_myOutputDevices.remove(ep);
			delete ep;
			break;
		}
	}
}

void 
CCoreMIDI::sendShort(CCoreMIDIEndpointRef endpointRef, char midiStatusByte, char midiDataByte1, char midiDataByte2)
{
	if (endpointRef == 0) {
		return;
	}

	list<CCoreMIDIEndpoint*>::iterator i;
	for (i = _myOutputDevices.begin(); i != _myOutputDevices.end(); i++) {
		if (endpointRef == *i) {
			CCoreMIDISend *midiSend = dynamic_cast<CCoreMIDISend*>((CCoreMIDIEndpoint*)endpointRef);
			
			if (midiSend != NULL) {
				midiSend->sendMessage(midiStatusByte, midiDataByte1, midiDataByte2);
			}
		}
	}
}