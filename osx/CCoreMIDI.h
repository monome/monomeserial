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
#ifndef __CCoreMIDI_h__
#define __CCoreMIDI_h__

// central park history of parks library!

#include <CoreMIDI/CoreMIDI.h>

#include <list>
#include <string>
using namespace std;

typedef void *CCoreMIDIEndpointRef;
typedef void *CCoreMIDIPortRef;
typedef void (*CCoreMIDIReadProc)(const MIDIPacketList *packetList, void *userData, CCoreMIDIEndpointRef source);
typedef void (*CCoreMIDINotificationProc)(const MIDINotification *message, void *userData);

void _cCoreMIDINotificationProc(const MIDINotification *message, void *refCon);

class CCoreMIDI
{
public:
    CCoreMIDI(const string& clientName);
    ~CCoreMIDI();

    int getNumberOfInputDevices(void);
    int getNumberOfOutputDevices(void);

    string getInputDeviceName(int inputDevice);
    string getOutputDeviceName(int outputDevice);

    CCoreMIDIEndpointRef getEndpointRefForInputDevice(int inputDevice);
    CCoreMIDIEndpointRef getEndpointRefForOutputDevice(int outputDevice);

    CCoreMIDIEndpointRef createSource(const string& sourceName);
    CCoreMIDIEndpointRef createDestination(const string& destinationName, CCoreMIDIReadProc callback, void *userData);

    void disposeSource(CCoreMIDIEndpointRef sourceRef);
    void disposeDestination(CCoreMIDIEndpointRef destinationRef);

    CCoreMIDIPortRef MIDIInputPortCreate(const string& portName, CCoreMIDIReadProc callback, void *userData);
    void MIDIPortConnectSource(CCoreMIDIPortRef port, CCoreMIDIEndpointRef source);
    void MIDIPortDisconnectSource(CCoreMIDIPortRef port, CCoreMIDIEndpointRef source);
    void MIDIPortDispose(CCoreMIDIPortRef port);

    void sendShort(CCoreMIDIEndpointRef endpointRef, char midiStatusByte, char midiDataByte1, char midiDataByte2);

    void registerForMIDISystemStateChangeNotifications(CCoreMIDINotificationProc callback, void *userData);

private:
    void _coreMIDISystemStateChanged(const MIDINotification *message);
    bool _sourceIsVirtualSource(MIDIEndpointRef endpointRef);
    bool _destinationIsVirtualDestination(MIDIEndpointRef endpointRef);

private:
    MIDIClientRef         _myClientRef;
    MIDIPortRef           _myOutputPort;
    MIDIPortRef           _myInputPort;
    list<MIDIEndpointRef> _myMIDISources;
    list<MIDIEndpointRef> _myMIDIDestinations;
    list<MIDIPortRef> _myPorts;
    list<pair<CCoreMIDINotificationProc, void *> > _notificationCallbacks;

    friend void _cCoreMIDINotificationProc(const MIDINotification *message, void *refCon);
};

#endif // __CCoreMIDI_h__
