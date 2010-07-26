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
#ifndef __ApplicationController_h__
#define __ApplicationController_h__

#include "MonomeXXhDevice.h"
#include "AsynchronousSerialDeviceReader.h"
#include "OscController.h"
#include "MonomeSerialDefaults.h"

#include <vector>
using namespace std;

#import "AppController.h"

class ApplicationController
{
public:
    typedef enum {
        kProtocolType_MIDI,
        kProtocolType_OpenSoundControl,
        kProtocolType_NumTypes
    } ProtocolType;
    
public:
    ApplicationController(AppController *appController);
    ~ApplicationController();
	
	void registerForSerialDeviceNotifications(void);

    // Handlers for IOKit generated events:
    void handleSerialDeviceDiscoveredEvent(const char *bsdFilePath);
    void handleSerialDeviceTerminatedEvent(const char *bsdFilePath);

    // Handlers for device generated events:
    int handleSerialDeviceMessageReceivedEvent(MonomeXXhDevice *device,  char *data, size_t len);
    void handleButtonPressEvent(MonomeXXhDevice *device, unsigned int localColumn, unsigned int localRow, bool state);
    void handleAdcValueChangeEvent(MonomeXXhDevice *device, unsigned int localAdcIndex, float value);
    void handleRotaryEncoderEvent(MonomeXXhDevice *device, unsigned int localEncoderIndex, int steps);
	void handleTiltValueChangeEvent(MonomeXXhDevice *device, int WhichAxis, int value);
	//for mk
	void handleAuxVersionReportEvent(MonomeXXhDevice *device, int version);
	void handleAuxAnalogValueChangeEvent(MonomeXXhDevice *device, int whichAux, float value);
	void handleAuxDigitalValueChangeEvent(MonomeXXhDevice *device, int whichAux, int value);
	void handleAuxEncoderValueChangeEvent(MonomeXXhDevice *device, int whichAux, signed char change);
	
    // Handlers for OpenSound Control events:
    void handleOscMessage(const string& addressPattern, list <OscAtom *> *atoms);
    void handleMIDIReceivedOnVirtualDestination(const MIDIPacketList *packetList, CCoreMIDIEndpointRef source);
    void handleMIDIReceived(const MIDIPacketList *packetList, CCoreMIDIEndpointRef source);
    void handleMIDISystemStateChanged(const MIDINotification *message);

    // Handlers for UI events:
    void protocolPopUpMenuChanged(unsigned int index);
    void oscHostAddressTextFieldChanged(const string& oscHostAddressString);
    void oscHostPortTextFieldChanged(unsigned int oscHostPort);
    void oscListenPortTextFieldChanged(unsigned int oscListenPort);

    void midiInputDeviceChanged(unsigned int deviceIndex, unsigned int midiInputDeviceIndex);
    void midiInputChannelChanged(unsigned int deviceIndex, unsigned char channel);
    void midiOutputDeviceChanged(unsigned int deviceIndex, unsigned int midiOutputDeviceIndex);
    void midiOutputChannelChanged(unsigned int deviceIndex, unsigned char channel);

    void cableOrientationPopUpMenuChanged(unsigned int deviceIndex, unsigned int orientationIndex);
	void oscAddressPatternPrefixTextFieldChanged(unsigned int deviceIndex, const string& oscAddressPatternPrefix);
    void startingColumnTextFieldChanged(unsigned int deviceIndex, unsigned int startingColumn);
    void startingRowTextFieldChanged(unsigned int deviceIndex, unsigned int startingRow);

    void oscAdcOffsetTextFieldChanged(unsigned int deviceIndex, unsigned int offset);
    void oscEncOffsetTextFieldChanged(unsigned int deviceIndex, unsigned int offset);

    void adcStateButtonChanged(unsigned int deviceIndex, unsigned int localAdcIndex, bool state);
    void encStateButtonChanged(unsigned int deviceIndex, unsigned int localEncIndex, bool state);
	
	void clearLedsButtonChanged(unsigned int deviceIndex);
	void testModeButtonChanged(unsigned int deviceIndex, bool state);

	void writePreferences(void);

    ProtocolType protocol(void) const;
    const string& oscHostAddressString(void) const;
    int oscHostPort(void) const;
    int oscListenPort(void) const;
    
    MonomeXXhDevice *deviceAtIndex(unsigned int index) const;
    unsigned int numberOfDevices(void) const;

    unsigned int numberOfMIDIInputDevices(void) const;
    const string nameOfMIDIInputDeviceAtIndex(unsigned int index) const;
    unsigned int numberOfMIDIOutputDevices(void) const;
    const string nameOfMIDIOutputDeviceAtIndex(unsigned int index) const;

	unsigned int indexOfMIDIInputDeviceForMonomeXXhDeviceIndex(unsigned int deviceIndex) const;
	unsigned int indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex(unsigned int deviceIndex) const;

    CCoreMIDI *coreMIDI(void) const;

private:
    void _initCoreMIDI(void);
    void _handleOscSystemMessage(const string& addressPattern, list <OscAtom *> *atoms);
    void _initOpenSoundControl(void);
    bool _typeCheckOscAtoms(list<OscAtom *>& atoms, const char *typetags);
    bool _typeCheckRowOrColumnMessage(list<OscAtom *>& atoms);
	void _handleMIDIMessage(MonomeXXhDevice *device, unsigned char status, unsigned char data1, unsigned char data2);

private:
    ProtocolType _protocol;
    string _oscHostAddressString;
    unsigned int _oscHostPort;
    unsigned int _oscListenPort;

    CCoreMIDI *_cCoreMIDI;
    CCoreMIDIEndpointRef _fromMonomeSerial;
    CCoreMIDIEndpointRef _toMonomeSerial;

    vector<MonomeXXhDevice *> _devices;

    AsynchronousSerialDeviceReader _deviceReader;
	
    OscController _oscController;
    OscHostRef _oscHostRef;

	AppController *_appController;

    MonomeSerialDefaults *_defaults;
};

#endif // __ApplicationController_h__
