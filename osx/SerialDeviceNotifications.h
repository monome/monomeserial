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
#ifndef __SERIALDEVICENOTIFICATIONTHREAD_H__
#define __SERIALDEVICENOTIFICATIONTHREAD_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*SerialDeviceNotificationCallback)(const char *deviceFilePath, void *userData);

typedef struct _SerialDeviceNotificationContext {
    SerialDeviceNotificationCallback serialDeviceDiscoveredCallback;
    SerialDeviceNotificationCallback serialDeviceTerminatedCallback;
    void *userData;
} SerialDeviceNotificationContext;

void RegisterForSerialDeviceNotifications(SerialDeviceNotificationContext *context);

#ifdef __cplusplus
}
#endif

#endif
