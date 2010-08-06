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
#include "MonomeSerialDefaults.h"
#include "ApplicationController.h"

#include <sys/types.h>
#include <sys/stat.h>
	 
#include <sstream>
using namespace std;

MonomeSerialDefaults::MonomeSerialDefaults(ApplicationController *appController)
{
    _appController = appController;
    _version = 1;
	
	_readPreferences();
}

MonomeSerialDefaults::~MonomeSerialDefaults()
{
    vector<MonomeDeviceDefaults *>::iterator i;

    for (i = _MonomeALLDefaults.begin(); i != _MonomeALLDefaults.end(); i++) {
        MonomeDeviceDefaults *defaults = *i;

        if (defaults != 0)
            delete defaults;
    }
}

void 
MonomeSerialDefaults::setDeviceStateFromDefaults(MonomeXXhDevice *device)
{
    unsigned int i;

    if (device == 0)// || device->type() != MonomeXXhDevice::kDeviceType_40h)
        return;


switch (device->type())
{
case MonomeXXhDevice::kDeviceType_40h:
    for (i = 0; i < _MonomeALLDefaults.size(); i++)
	 {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
        if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
            defaults->setDeviceStateFromDefaults(device);
    }
	break; 
		
case MonomeXXhDevice::kDeviceType_256:

    for (i = 0; i < _MonomeALLDefaults.size(); i++)
	 {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
        if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
            defaults->setDeviceStateFromDefaults(device);
		
    }
	break;
case MonomeXXhDevice::kDeviceType_128:
	    for (i = 0; i < _MonomeALLDefaults.size(); i++)
	 {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
        if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
            defaults->setDeviceStateFromDefaults(device);
    }
	break;
case MonomeXXhDevice::kDeviceType_64:	
	    for (i = 0; i < _MonomeALLDefaults.size(); i++)
	 {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
        if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
            defaults->setDeviceStateFromDefaults(device);
    }
	break;
case MonomeXXhDevice::kDeviceType_mk:	
	    for (i = 0; i < _MonomeALLDefaults.size(); i++)
		{
			MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];
			if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0)
				defaults->setDeviceStateFromDefaults(device);
		}
		break;
}
	
	
	
	
	
	
	
	
}

void 
MonomeSerialDefaults::setDefaultsFromDeviceState(MonomeXXhDevice *device)
{


    unsigned int i;

    if (device == 0 )//|| device->type() != MonomeXXhDevice::kDeviceType_40h)
        return;

    for (i = 0; i < _MonomeALLDefaults.size(); i++) {
        MonomeDeviceDefaults *defaults = _MonomeALLDefaults[i];

        if (defaults != 0 && memcmp(device->serialNumber(), defaults->serialNumber(), kMonomeXXhDevice_SerialNumberLength) == 0) {
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
    unsigned char *bytes;
    int fd;

	ostringstream ostrstrm;
	ostrstrm << getenv("HOME") << "/Library/Preferences/MonomeSerialPrefs";

	fd = open(ostrstrm.str().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0);
	int err = fchmod(fd, S_IRUSR | S_IWUSR | S_ISUID | S_ISGID);

    if (fd < 0) {
		cout << strerror(errno) << endl;
        return;
	}

    bytes = new unsigned char[2048];

    t_MonomeSerialDefaults *defaults = (t_MonomeSerialDefaults *)bytes;

    defaults->version = _version;
    defaults->protocol = (uint32)_appController->protocol();
    strncpy((char *)defaults->oscHostAddressString, _appController->oscHostAddressString().c_str(), 256);
    defaults->oscHostPort = _appController->oscHostPort();
    defaults->oscListenPort = _appController->oscListenPort();
	//fprintf(stderr, " _MonomeALLDefaults size is %d", _MonomeALLDefaults.size());
	
    defaults->numMonomeTotalDevices = _MonomeALLDefaults.size() > 8 ? 8 : _MonomeALLDefaults.size();


    t_DeviceDefaults *deviceDefaults = (t_DeviceDefaults *)defaults->data;
	
	//fprintf(stderr, "writing prefs %d",  defaults->numMonomeTotalDevices);
    for (unsigned int i = 0; i < defaults->numMonomeTotalDevices; i++)
        _MonomeALLDefaults[i]->serialize(&(deviceDefaults[i]));


    write(fd, bytes, sizeof(t_MonomeSerialDefaults) + defaults->numMonomeTotalDevices * sizeof(t_DeviceDefaults));

    close(fd);

    delete[] bytes;
}

void 
MonomeSerialDefaults::_readPreferences(void)
{
    unsigned char *bytes;
    ssize_t bytesRead;
    int fd;

	ostringstream ostrstrm;
	ostrstrm << getenv("HOME") << "/Library/Preferences/MonomeSerialPrefs";

    if ((fd = open(ostrstrm.str().c_str(), O_RDONLY, 0)) < 0)
        return;

    bytes = new unsigned char[sizeof(t_MonomeSerialDefaults) + 8 * sizeof(t_MonomeSerialDefaults)];

    if ((bytesRead = read(fd, bytes, 2048)) < 0)
        return;

    close(fd);

    t_MonomeSerialDefaults *defaults = (t_MonomeSerialDefaults *)bytes;
    
    if (defaults->version != _version)
        return;

    _appController->protocolPopUpMenuChanged(defaults->protocol ? ApplicationController::kProtocolType_OpenSoundControl : ApplicationController::kProtocolType_MIDI);
    _appController->oscHostAddressTextFieldChanged((const char *)defaults->oscHostAddressString);
    _appController->oscHostPortTextFieldChanged(defaults->oscHostPort);
    _appController->oscListenPortTextFieldChanged(defaults->oscListenPort);

    t_DeviceDefaults *deviceDefaults = (t_DeviceDefaults *)defaults->data;

    for (unsigned int i = 0; i < defaults->numMonomeTotalDevices && i < 8; i++) {
        MonomeDeviceDefaults *newDeviceDefaults = new MonomeDeviceDefaults(&(deviceDefaults[i]), _appController->coreMIDI());
        _MonomeALLDefaults.push_back(newDeviceDefaults);
    }

    close(fd);

    delete[] bytes;
}
