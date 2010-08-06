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

#ifdef WIN32
#include "stdafx.h"
#endif

#include "MonomeSerialDefaults.h"
#include "ApplicationController.h"
	 
#include <stdexcept>

#include <sstream>
#include <shlobj.h>
#include <fstream>
#include <string>

#define PREFS_FOLDER  L"\\MonomeSerial"
#define PREFS_FILE    L"\\MonomeSerialPreferences.txt"

using namespace std;

const string PROTOCOL_MIDI_STRING = "MIDI";
const string PROTOCOL_OSC_STRING = "OSC";

const string& getProtocolString(ApplicationController::ProtocolType protocolType)
{
	switch(protocolType) {
		case ApplicationController::kProtocolType_MIDI:
			return PROTOCOL_MIDI_STRING;
		case ApplicationController::kProtocolType_OpenSoundControl:
		default:
			return PROTOCOL_OSC_STRING;
	}
}


ApplicationController::ProtocolType getProtocolValue(const string& protocolStr)
{
	if (protocolStr == PROTOCOL_MIDI_STRING) {
		return ApplicationController::kProtocolType_MIDI;
	}
	else {
		return ApplicationController::kProtocolType_OpenSoundControl;
	}
}

MonomeSerialDefaults::MonomeSerialDefaults(ApplicationController *appController)
{
	wchar_t AppDir[255];
	wstring dir;
	ITEMIDLIST* pidl;
	HRESULT hRes = SHGetSpecialFolderLocation( NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE , &pidl );
	if (hRes == NOERROR) 
	{
		SHGetPathFromIDList( pidl, AppDir );
	}

	wostringstream ostrstrm;
	ostrstrm << AppDir << PREFS_FOLDER;
	dir = ostrstrm.str();

	// create prefs directory if it doesn't exist
	if (!CreateDirectory(dir.c_str(), NULL)) {
		int err = GetLastError();

		if (err == ERROR_ALREADY_EXISTS) {
			//do nothing
		}
		else if (err == ERROR_PATH_NOT_FOUND) {
			Alert(L"There was an error finding your ApplicationData preferences folder.  Please contact monome support at www.monome.org");
			return;
		}
	}
	
	ostrstrm.str(L"");  // reset stream to empty string
	ostrstrm << dir << PREFS_FILE;
	_fileLocation = ostrstrm.str();

    _appController = appController;
    _version = PREFS_VERSION;
	
	_readPreferences();
}

MonomeSerialDefaults::~MonomeSerialDefaults()
{
    vector<MonomeDeviceDefaults *>::iterator i;

    for (i = _MonomeALLDefaults.begin(); i != _MonomeALLDefaults.end(); i++) {
        MonomeDeviceDefaults *defaults = *i;

		if (defaults != 0) {
            delete defaults;
			defaults = 0;
		}
    }
}

void 
MonomeSerialDefaults::setDeviceStateFromDefaults(MonomeXXhDevice *device)
{
    unsigned int i;

    if (device == 0)// || device->type() != MonomeXXhDevice::kDeviceType_40h)
        return;

	if (_MonomeALLDefaults.size() == 0)
		return;

	switch (device->type())
	{
	case MonomeXXhDevice::kDeviceType_40h:
		for (i = 0; i < _MonomeALLDefaults.size(); i++)
		{
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0) {
				defaults->setDeviceStateFromDefaults(device);
			}
		}
		break; 
			
	case MonomeXXhDevice::kDeviceType_256:
		for (i = 0; i < _MonomeALLDefaults.size(); i++)
		{
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
				defaults->setDeviceStateFromDefaults(device);
		}
		break;
	case MonomeXXhDevice::kDeviceType_128:
		 for (i = 0; i < _MonomeALLDefaults.size(); i++)
		 {
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
				defaults->setDeviceStateFromDefaults(device);
		}
		break;
	case MonomeXXhDevice::kDeviceType_64:	
		 for (i = 0; i < _MonomeALLDefaults.size(); i++)
		 {
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
				defaults->setDeviceStateFromDefaults(device);
		}
		break;
	case MonomeXXhDevice::kDeviceType_mk:	
		 for (i = 0; i < _MonomeALLDefaults.size(); i++)
		 {
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
				defaults->setDeviceStateFromDefaults(device);
		}
		break;
	}
}

void 
MonomeSerialDefaults::setDefaultsFromDeviceState(MonomeXXhDevice *device)
{
    unsigned int i, len;

    if (device == 0)
        return;

	len = _MonomeALLDefaults.size();

    for (i = 0; i < len; i++) {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];

        if (defaults != 0 && strncmp(device->serialNumber().c_str(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0) {
            defaults->setDefaultsFromDeviceState(device);
            return;
        }
    }

    MonomeDeviceDefaults *defaults = new MonomeDeviceDefaults(device, _appController->coreMIDI());
    _MonomeALLDefaults.push_back(defaults);
}

void
MonomeSerialDefaults::writePreferences(void)
{
	ofstream stream(_fileLocation.c_str());

	stream << "version : " << (int)_version << endl
		<< "protocol : " << getProtocolString( _appController->protocol() ) << endl;

	int len = _MonomeALLDefaults.size();
	int count = 0;
	for(int i = 0; i < len && i < PREFS_MAX_SAVED_DEVICES; i++) {
		MonomeDeviceDefaults *dev = _MonomeALLDefaults[i];
		if (!dev) continue;

		stream << "device : " 
			<< count++ 
			<< endl;
		dev->serialize(stream);
	}

	stream.close();
}

void 
MonomeSerialDefaults::_readPreferences(void)
{
	ifstream stream(_fileLocation.c_str());
	if (stream.fail()) {
		return;
	}
	
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
			else if (val == 0) {
				break;
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
				secondpart += val;
			}
		}

		if (firstpart == "version") {
			this->_version = atoi(secondpart.c_str());
		}
		else if (firstpart == "protocol") {
			_appController->protocolPopUpMenuChanged(
					getProtocolValue(secondpart)
				);
		}
		else if (firstpart == "device") {
			while(!stream.eof()) {
				MonomeDeviceDefaults *dev = new MonomeDeviceDefaults(stream, _appController->coreMIDI());
				this->_MonomeALLDefaults.push_back(dev);
			}
		}
	}
}

