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
//-------------------------------------------------
/*
 * Current code by Daniel Battaglia and Steve Duda.
 * Released under the original GPL
 */
//-------------------------------------------------

#ifndef __ApplicationController_h__
#define __ApplicationController_h__

#include "MonomeSerialDlg.h"
#include "serial/MonomeXXhDevice.h"
#include "serial/AsynchronousSerialDeviceReader.h"
#include "osc/OscController.h"
#include "osc/OscMessageStream.h"
#include "MonomeSerialDefaults.h"

#include <vector>
using namespace std;
using namespace osc;


class ApplicationController
{
public:

typedef enum {
    kProtocolType_MIDI,
    kProtocolType_OpenSoundControl,
    kProtocolType_NumTypes
} ProtocolType;

public:
	ApplicationController(CMonomeSerialDlg* appController);
	~ApplicationController(void);

	void registerForSerialDeviceNotifications(void);

    // Handlers for IOKit generated events:
    void handleSerialDeviceDiscoveredEvent(const string& serialNumber);
    void handleSerialDeviceTerminatedEvent(const string& serialNumber);

	// Handlers for device generated events:
    int  handleSerialDeviceMessageReceivedEvent(MonomeXXhDevice *device,  char *data, size_t len);
    void handleButtonPressEvent(MonomeXXhDevice *device, unsigned int localColumn, unsigned int localRow, bool state);
    void handleAdcValueChangeEvent(MonomeXXhDevice *device, unsigned int localAdcIndex, float value);
    void handleRotaryEncoderEvent(MonomeXXhDevice *device, unsigned int localEncoderIndex, int steps);

	// Handler for 64 aux message, which is Tilt - thanks steve!
	void handleTiltValueChangeEvent(MonomeXXhDevice *device, int WhichAxis, float value);

    // Handlers for OpenSoundControl/MIDI events:
    void handleOscMessage(const osc::ReceivedMessage &msg);
    void handleMIDIReceived(DWORD msg, CCoreMIDIEndpointRef source);

    // Handlers for UI events:
    void protocolPopUpMenuChanged(unsigned int index);
    void oscHostAddressTextFieldChanged(unsigned int deviceIndex, const string& oscHostAddressString);
    void oscHostPortTextFieldChanged(unsigned int deviceIndex, const string& oscHostPortString);
    void oscListenPortTextFieldChanged(unsigned int deviceIndex, const string& oscListenPortString);

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

	void writePreferences(void);

    ProtocolType protocol(void) const;
    const string& oscHostAddressString(void) const;
    const string&  oscHostPort(void) const;
    const string&  oscListenPort(void) const;
    
    MonomeXXhDevice *deviceAtIndex(unsigned int index) const;
	MonomeXXhDevice *deviceBySerial(const std::string& serialNum, unsigned int& index);
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
    void _handleOscSystemMessage(OscMessageStream msg);
    void _initOpenSoundControl(void);
    bool _typeCheckOscAtoms(const osc::ReceivedMessage &msg, const char *typetags);
    bool _typeCheckRowOrColumnMessage(OscMessageStream msg);
	void _handleMIDIMessage(MonomeXXhDevice *device, unsigned char status, unsigned char data1, unsigned char data2);


private:
    ProtocolType _protocol;
    string _oscHostAddressString;
    string _oscHostPort;
    string _oscListenPort;

    CCoreMIDI *_cCoreMIDI;

    vector<MonomeXXhDevice *> _devices;

    AsynchronousSerialDeviceReader _deviceReader;
	
    OscController _oscController;

    MonomeSerialDefaults *_defaults;

	CMonomeSerialDlg* _appController;

	// lock needed for Windows version because each device reads on a differant thread.
	CRITICAL_SECTION _readLock;

	struct {char x; char y;} _64TiltValues;

	class ApplicationControllerLock
	{
	public:
		ApplicationControllerLock(ApplicationController* controller)
		{
			EnterCriticalSection(_lock = &controller->_readLock);
		}
		~ApplicationControllerLock()
		{
			LeaveCriticalSection(_lock);
		}
	private:
		CRITICAL_SECTION *_lock;
	};

	 friend class ApplicationControllerLock;

	 // added by Daniel Battaglia for testing only: does nothing in Release build since it is inlined
	 void _printOscMessage(const osc::ReceivedMessage &msg)
	 {
#ifdef DEBUG_PRINT
		cout << msg.AddressPattern();
		for (osc::ReceivedMessageArgumentIterator q = msg.ArgumentsBegin(); q != msg.ArgumentsEnd(); ++q) {
			cout << " - ";

			if (q->IsInt32()) {
				cout << "Int32:" << q->AsInt32();
			}
			else if (q->IsFloat()) {
				cout << "Float:" << q->AsFloat();
			}
			else if(q->IsString()) {
				cout << "String:" << q->AsString();
			}
			else {
				cout << "Undefined";
			}
		}
		cout << endl;
#endif
	 }
};


#endif