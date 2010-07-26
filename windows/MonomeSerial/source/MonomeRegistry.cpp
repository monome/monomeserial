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

// this file is created for Windows only by Daniel Battaglia

#include "stdafx.h"
#include "MonomeRegistry.h"
#include <list>

using namespace std;

bool GetActiveDevices(map<wstring, wstring> *devices)
{
	if (devices == 0)
		return false;

	GetAllDevices(devices);
	map<wstring, wstring> myDevices;
	
	if (devices->size() == 0)
		return false;

	map<wstring, wstring>::iterator i;
	for (i = devices->begin(); i != devices->end(); i++) {
		if (isDeviceActive(i->second)) {
			myDevices[i->first] = i->second;
		}
	}

	devices->swap(myDevices);

	return myDevices.size();
}

bool GetAllDevices(map<wstring, wstring> *devices)
{
	if (devices == 0 || devices->size() != 0)
		throw L"devices map object must be initialized and empty";

	DWORD ulOptions = 0;
	HKEY hKeyCurrent;
	REGSAM desiredAccess = KEY_READ;
	LONG result;

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEVICE_LOCATION, 0, KEY_READ, &hKeyCurrent);
	if (result != ERROR_SUCCESS) {
		devices = 0;
		return false;
	}		

	DWORD dwIndex = 0;
	WCHAR lpName[MAX_KEY];
	DWORD size;
	list<wstring> keys;

	while(true) {
		size = MAX_KEY;
		result = RegEnumKeyEx(hKeyCurrent, dwIndex++, lpName, &size, 0, 0, 0, 0);
		if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {
			keys.push_back(lpName);
			continue;
		}
		else if (result == ERROR_NO_MORE_ITEMS) {
			break;
		}
		else {
			break;
		}
	}

	if (keys.size() == 0)
		return false;

	RegCloseKey(hKeyCurrent);

	wstring *current;
	wstring serial;
	list<wstring>::iterator it;
	list<wstring>::size_type pos;

	for (it = keys.begin(); it != keys.end(); it++) {
		current = &(*it);

		if ((pos = current->find(L"m40h")) != wstring::npos || 
			(pos = current->find(L"m256")) != wstring::npos || 
			(pos = current->find(L"m128")) != wstring::npos || 
			(pos = current->find(L"m64"))  != wstring::npos )
		{
			serial = current->substr(pos, 8);

			DWORD ulOptions = 0;
			HKEY hKeyCurrent;
			REGSAM desiredAccess = KEY_READ;
			LONG result;

			wstring location = DEVICE_LOCATION;
			location += L"\\"; 
			location += *current;
			location += COMPORT_SUBLOCATION;

			result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, location.c_str(), 0, KEY_READ, &hKeyCurrent);
			if (result != ERROR_SUCCESS) {
				return false;
			}
			
			// set outside of loop, obviously
			dwIndex = 0;

			DWORD dwType, dwTypeQuery;
			DWORD dwDataSize;
			WCHAR valName[MAX_VAL];
			BYTE data[MAX_VAL];

			while(true) {
				size = MAX_KEY;
				dwDataSize = 0, dwType = 0, dwTypeQuery = 0;
				memset(valName, 0, MAX_VAL);
				memset(data, 0, MAX_VAL);

				result = ::RegEnumValueW(hKeyCurrent, dwIndex++, valName, &size, 0, &dwType, 0, 0);
				if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {
					if (dwType == REG_SZ && memcmp(valName, L"PortName", 9) == 0) {
						do {
							result = RegQueryValueEx(hKeyCurrent, valName, 0, &dwTypeQuery, data, &dwDataSize);
							if (memcmp(data, L"COM", 3) == 0) {
								wstring temp;
								for (int i = 0; i < dwDataSize; i++) {
									if (data[i] != 0) { // take care of wierd char to wchar_t conversion
										temp += (WCHAR)data[i];
									}
								}
								(*devices)[serial] = temp;
								break;
							}
						}
						while (result == ERROR_MORE_DATA);
					}
					continue;
				}
				else if (result == ERROR_NO_MORE_ITEMS) {
					break;
				}
				else {
					break;
				}
			}

			RegCloseKey(hKeyCurrent);
		}
	}

	return true;
}

bool isDeviceActive(wstring &comPort)
{
	DWORD ulOptions = 0;
	HKEY hKeyCurrent;
	REGSAM desiredAccess = KEY_READ;
	LONG result;

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, OPENPORTS_LOCATION, 0, KEY_READ, &hKeyCurrent);
	if (result != ERROR_SUCCESS) {
		return false;
	}		
	
	// set outside of loop, obviously
	DWORD dwIndex = 0;

	DWORD dwType, dwTypeQuery, size;
	DWORD dwDataSize;
	WCHAR valName[MAX_VAL];
	BYTE data[MAX_VAL];

	while(true) {
		size = MAX_KEY;
		dwDataSize = 0, dwType = 0, dwTypeQuery = 0;
		memset(valName, 0, MAX_VAL);
		memset(data, 0, MAX_VAL);

		result = ::RegEnumValueW(hKeyCurrent, dwIndex++, valName, &size, 0, &dwType, 0, 0);
		if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {
			if (dwType == REG_SZ && memcmp(valName, L"\\Device\\VCP", 11) == 0) {
				do {
					result = RegQueryValueEx(hKeyCurrent, valName, 0, &dwTypeQuery, data, &dwDataSize);
					wstring temp;
					for (int i = 0; i < dwDataSize; i++) {
						if (data[i] != 0) { // take care of wierd char to wchar_t conversion
							temp += (WCHAR)data[i];
						}
					}
					if (temp == comPort) {
						RegCloseKey(hKeyCurrent);
						return true;
					}
				}
				while (result == ERROR_MORE_DATA);
			}
			continue;
		}
		else if (result == ERROR_NO_MORE_ITEMS) {
			break;
		}
		else {
			break;
		}
	}

	RegCloseKey(hKeyCurrent);
	return false;
}
