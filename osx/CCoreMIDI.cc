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
#include "CCoreMIDI.h"
#include <mach/clock.h>

void _cCoreMIDINotificationProc(const MIDINotification *message, void *refCon);

CCoreMIDI::CCoreMIDI(const string& clientName)
{
    OSStatus status;

    CFStringRef clientNameAsCFString = CFStringCreateWithCString(NULL, clientName.c_str(), kCFStringEncodingMacRoman);
    status = MIDIClientCreate(clientNameAsCFString,
                              _cCoreMIDINotificationProc,
                              this,
                              &_myClientRef);

    CFStringRef outputPortNameAsCFString = CFStringCreateWithCString(NULL, 
                                                                     (clientName + string(" Output Port")).c_str(), 
                                                                     kCFStringEncodingMacRoman);
    status = MIDIOutputPortCreate(_myClientRef,
                                  outputPortNameAsCFString,
                                  &_myOutputPort);

#if 0
    CFStringRef inputPortNameAsCFString = CFStringCreateWithCString(NULL, 
                                                                    (clientName + string(" Input Port")).c_str(), 
                                                                    kCFStringEncodingMacRoman);
    status = MIDIInputPortCreate(_myClientRef,
                                 inputPortNameAsCFString,
                                 &_myInputPort);
    
#endif
}

CCoreMIDI::~CCoreMIDI()
{
    list<MIDIEndpointRef>::iterator i;
    list<MIDIPortRef>::iterator j;

    for (i = _myMIDISources.begin(); i != _myMIDISources.end(); i++) {
        MIDIEndpointRef endpointRef = *i;
        MIDIEndpointDispose(endpointRef);
    }

    for (i = _myMIDIDestinations.begin(); i != _myMIDIDestinations.end(); i++) {
        MIDIEndpointRef endpointRef = *i;
        MIDIEndpointDispose(endpointRef);
    }

    for (j = _myPorts.begin(); j != _myPorts.end(); j++) 
        MIDIPortDispose(*j);

    MIDIPortDispose(_myOutputPort);
    MIDIPortDispose(_myInputPort);

    MIDIClientDispose(_myClientRef);
}

int 
CCoreMIDI::getNumberOfInputDevices(void)
{
    return MIDIGetNumberOfSources() - _myMIDISources.size() + _myMIDIDestinations.size();
}
int 
CCoreMIDI::getNumberOfOutputDevices(void)
{
    return MIDIGetNumberOfDestinations() - _myMIDIDestinations.size() + _myMIDISources.size();
}

string
CCoreMIDI::getInputDeviceName(int inputDevice)
{
    if (inputDevice < _myMIDIDestinations.size()) {
        list<MIDIEndpointRef>::iterator i;
        int count;
        for (i = _myMIDIDestinations.begin(), count = 0; i != _myMIDIDestinations.end(), count <= inputDevice; i++, count++) {
            if (count == inputDevice) {
                char buffer[255];
                CFStringRef displayName;
                MIDIEndpointRef endpointRef = *i;

                MIDIObjectGetStringProperty(endpointRef,
                                            kMIDIPropertyDisplayName,
                                            &displayName);
                CFStringGetCString(displayName, buffer, 255, kCFStringEncodingASCII);
                
                return string(buffer);
            }
        }

        return string();
    }

    inputDevice -= _myMIDIDestinations.size();

    CFStringRef clientName;
    MIDIObjectGetStringProperty(_myClientRef, kMIDIPropertyName, &clientName);

    int iSrc, counter;
    for (iSrc = counter = 0; iSrc < MIDIGetNumberOfSources(); iSrc++) {
        MIDIEndpointRef source = MIDIGetSource(iSrc);
        
        if (_sourceIsVirtualSource(source)) 
            continue;
        else {
            if (counter == inputDevice) {
                char buffer[255];
                CFStringRef displayName;

                MIDIObjectGetStringProperty(source,
                                            kMIDIPropertyDisplayName,
                                            &displayName);
                CFStringGetCString(displayName, buffer, 255, kCFStringEncodingASCII);
                
                return string(buffer);
            }

            counter++;
        }
    }

    return string();
}
                
string
CCoreMIDI::getOutputDeviceName(int outputDevice)
{
    if (outputDevice < _myMIDISources.size()) {
        list<MIDIEndpointRef>::iterator i;
        int count;
        for (i = _myMIDISources.begin(), count = 0; i != _myMIDISources.end(), count <= outputDevice; i++, count++) {
            if (count == outputDevice) {
                char buffer[255];
                CFStringRef displayName;
                MIDIEndpointRef endpointRef = *i;

                MIDIObjectGetStringProperty(endpointRef,
                                            kMIDIPropertyDisplayName,
                                            &displayName);
                CFStringGetCString(displayName, buffer, 255, kCFStringEncodingASCII);
                
                return string(buffer);
            }
        }

        return string();
    }

    outputDevice -= _myMIDISources.size();

    CFStringRef clientName;
    MIDIObjectGetStringProperty(_myClientRef, kMIDIPropertyName, &clientName);

    int iDest, counter;
    for (iDest = counter = 0; iDest < MIDIGetNumberOfDestinations(); iDest++) {
        MIDIEndpointRef destination = MIDIGetDestination(iDest);
        
        if (_destinationIsVirtualDestination(destination)) 
            continue;
        else {
            if (counter == outputDevice) {
                char buffer[255];
                CFStringRef displayName;

                MIDIObjectGetStringProperty(destination,
                                            kMIDIPropertyDisplayName,
                                            &displayName);
                CFStringGetCString(displayName, buffer, 255, kCFStringEncodingASCII);
                
                return string(buffer);
            }

            counter++;
        }
    }

    return string();
}

CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForInputDevice(int inputDevice)
{
    if (inputDevice < _myMIDIDestinations.size()) {
        list<MIDIEndpointRef>::iterator i;
        int count;
        for (i = _myMIDIDestinations.begin(), count = 0; i != _myMIDIDestinations.end(), count <= inputDevice; i++, count++) {
            if (count == inputDevice) 
                return (CCoreMIDIEndpointRef) *i;
        }

        return (CCoreMIDIEndpointRef) 0;
    }

    inputDevice -= _myMIDIDestinations.size();

    CFStringRef clientName;
    MIDIObjectGetStringProperty(_myClientRef, kMIDIPropertyName, &clientName);

    int iSrc, counter;
    for (iSrc = counter = 0; iSrc < MIDIGetNumberOfSources(); iSrc++) {
        MIDIEndpointRef source = MIDIGetSource(iSrc);
        
        if (_sourceIsVirtualSource(source)) 
            continue;
        else {
            if (counter == inputDevice) 
                return (CCoreMIDIEndpointRef) source;

            counter++;
        }
    }

    return (CCoreMIDIEndpointRef) 0;
}

CCoreMIDIEndpointRef 
CCoreMIDI::getEndpointRefForOutputDevice(int outputDevice)
{
    if (outputDevice < _myMIDISources.size()) {
        list<MIDIEndpointRef>::iterator i;
        int count;
        for (i = _myMIDISources.begin(), count = 0; i != _myMIDISources.end(), count <= outputDevice; i++, count++) {
            if (count == outputDevice) 
                return (CCoreMIDIEndpointRef) *i;
        }

        return (CCoreMIDIEndpointRef) 0;
    }

    outputDevice -= _myMIDISources.size();

    CFStringRef clientName;
    MIDIObjectGetStringProperty(_myClientRef, kMIDIPropertyName, &clientName);

    int iDest, counter;
    for (iDest = counter = 0; iDest < MIDIGetNumberOfDestinations(); iDest++) {
        MIDIEndpointRef destination = MIDIGetDestination(iDest);

        if (_destinationIsVirtualDestination(destination)) 
            continue;
        else {
            if (counter == outputDevice) 
                return (CCoreMIDIEndpointRef) destination;

            counter++;
        }
    }

    return (CCoreMIDIEndpointRef) 0;
}

CCoreMIDIEndpointRef 
CCoreMIDI::createSource(const string& sourceName)
{
    MIDIEndpointRef endpointRef;
    CFStringRef sourceNameAsCFString = CFStringCreateWithCString(NULL, sourceName.c_str(), kCFStringEncodingMacRoman);
    OSStatus status = MIDISourceCreate(_myClientRef,
                                       sourceNameAsCFString,
                                       &endpointRef);
    if (status == 0) {
        CFStringRef clientName;
        MIDIObjectGetStringProperty(_myClientRef, kMIDIPropertyName, &clientName);

        MIDIObjectSetStringProperty(endpointRef, 
                                    kMIDIPropertyDriverOwner,
                                    clientName);

        _myMIDISources.push_back(endpointRef); // here we may want to assign it the same unique id it had the last time the
    }                                           // the application started.  We may want to give this a property like, 'IsMapdSource'.
    
    return endpointRef;                         // this should be NULL if MIDISourceCreate failed.
}

CCoreMIDIEndpointRef 
CCoreMIDI::createDestination(const string& destinationName, CCoreMIDIReadProc callback, void *userData) 
{
    MIDIEndpointRef endpointRef;
    CFStringRef destinationNameAsCFString = CFStringCreateWithCString(NULL, destinationName.c_str(), kCFStringEncodingMacRoman);
    OSStatus status = MIDIDestinationCreate(_myClientRef,
                                            destinationNameAsCFString,
                                            (MIDIReadProc)callback,
                                            userData,
                                            &endpointRef);

    if (status == 0) {
        CFStringRef clientName;
        MIDIObjectGetStringProperty(_myClientRef,kMIDIPropertyName, &clientName);
        MIDIObjectSetStringProperty(endpointRef, kMIDIPropertyDriverOwner, clientName);

        _myMIDIDestinations.push_back(endpointRef);
    }

    return endpointRef;
}

void 
CCoreMIDI::disposeSource(CCoreMIDIEndpointRef sourceRef)
{
    MIDIEndpointRef endpointRef = (MIDIEndpointRef)sourceRef;

    _myMIDISources.remove(endpointRef);
    OSStatus status = MIDIEndpointDispose(endpointRef);
}

void 
CCoreMIDI::disposeDestination(CCoreMIDIEndpointRef destinationRef)
{
    MIDIEndpointRef endpointRef = (MIDIEndpointRef)destinationRef;

    _myMIDIDestinations.remove(endpointRef);
    OSStatus status = MIDIEndpointDispose(endpointRef);
}

CCoreMIDIPortRef 
CCoreMIDI::MIDIInputPortCreate(const string& portName, CCoreMIDIReadProc callback, void *userData)
{
    MIDIPortRef portRef;

    CFStringRef portNameCFString = CFStringCreateWithCString(NULL, 
                                                             portName.c_str(), 
                                                             kCFStringEncodingMacRoman);

    ::MIDIInputPortCreate(_myClientRef, portNameCFString, callback, userData, &portRef);

    _myPorts.push_back(portRef);

    return portRef;
}

void 
CCoreMIDI::MIDIPortConnectSource(CCoreMIDIPortRef port, CCoreMIDIEndpointRef source)
{
    ::MIDIPortConnectSource((MIDIPortRef)port, (MIDIEndpointRef)source, source);
}

void 
CCoreMIDI::MIDIPortDisconnectSource(CCoreMIDIPortRef port, CCoreMIDIEndpointRef source)
{
    ::MIDIPortDisconnectSource((MIDIPortRef)port, (MIDIEndpointRef)source);
}

void 
CCoreMIDI::MIDIPortDispose(CCoreMIDIPortRef port)
{
    list<MIDIPortRef>::iterator i;

    for (i = _myPorts.begin(); i != _myPorts.end(); i++) {
        if ((*i) == port) {
            _myPorts.erase(i);
            break;
        }
    }
            
    ::MIDIPortDispose((MIDIPortRef)port);
}

void 
CCoreMIDI::sendShort(CCoreMIDIEndpointRef destinationRef, char midiStatusByte, char midiDataByte1, char midiDataByte2)
{
    OSStatus status;
    Byte buffer[256];
    MIDIPacketList *packetList = (MIDIPacketList *)buffer;
    MIDIPacket *packet = MIDIPacketListInit(packetList);
    Byte midiMessage[] = { (Byte)midiStatusByte, (Byte)midiDataByte1, (Byte)midiDataByte2 };
    packet = MIDIPacketListAdd(packetList, 
                               sizeof(buffer), 
                               packet, 
                               0, // mach_absolute_time()
                               3,
                               midiMessage);

    MIDIEndpointRef endpointRef = (MIDIEndpointRef) destinationRef;

    if (_sourceIsVirtualSource(endpointRef))
        status = MIDIReceived(endpointRef, packetList);
    else 
        status = MIDISend(_myOutputPort, endpointRef, packetList);
}

void 
CCoreMIDI::registerForMIDISystemStateChangeNotifications(CCoreMIDINotificationProc callback, void *userData)
{
    if (callback != 0)
        _notificationCallbacks.push_back(pair<CCoreMIDINotificationProc, void *>(callback, userData));
}

void 
CCoreMIDI::_coreMIDISystemStateChanged(const MIDINotification *notification)
{
    list<pair<CCoreMIDINotificationProc, void *> >::iterator i;

    for (i = _notificationCallbacks.begin(); i != _notificationCallbacks.end(); i++) {
        CCoreMIDINotificationProc callback = (*i).first;
        void *userData = (*i).second;

        if (callback != 0)
            callback(notification, userData);
    }
}

bool 
CCoreMIDI::_sourceIsVirtualSource(MIDIEndpointRef endpointRef)
{
    list<MIDIEndpointRef>::iterator i;

    for (i = _myMIDISources.begin(); i != _myMIDISources.end(); i++)
        if (*i == endpointRef)
            return true;

    return false;
}

bool 
CCoreMIDI::_destinationIsVirtualDestination(MIDIEndpointRef endpointRef)
{
    list<MIDIEndpointRef>::iterator i;

    for (i = _myMIDIDestinations.begin(); i != _myMIDIDestinations.end(); i++)
        if (*i == endpointRef)
            return true;

    return false;
}    

void _cCoreMIDINotificationProc(const MIDINotification *message, void *refCon)
{
    try {
        CCoreMIDI *cCoreMIDI = reinterpret_cast<CCoreMIDI *>(refCon);
        cCoreMIDI->_coreMIDISystemStateChanged(message);
    }
    catch (...) {
        ;
    }
}
