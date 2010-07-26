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


#ifndef __MonomeSerialDefaults_h__
#define __MonomeSerialDefaults_h__

#include "serial/MonomeDeviceDefaults.h"

#include <vector>
#include <string>

using namespace std;

class ApplicationController;

const int PREFS_VERSION = 2;

const int PREFS_MAX_SAVED_DEVICES = 8;

class MonomeSerialDefaults
{
public:
    MonomeSerialDefaults(ApplicationController *appController);
    ~MonomeSerialDefaults();
    
    void setDeviceStateFromDefaults(MonomeXXhDevice *device);
    void setDefaultsFromDeviceState(MonomeXXhDevice *device);
    void writePreferences(void);

private:
    void _readPreferences(void);

private:
    ApplicationController *_appController;
    unsigned char _version;
	wstring _fileLocation;
    vector<MonomeDeviceDefaults *> _MonomeALLDefaults;
};
    
#endif // __MonomeSerialDefaults_h__
