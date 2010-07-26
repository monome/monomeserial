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

#include "stdafx.h"
#include "ApplicationController.h"

#include "serial/message.h"
#include "serial/message256.h"
#include "osc/osc.h"
#include "midi/ShortMsg.h"

#include <exception>
#include <sstream>

#ifdef DEBUG_PRINT
	#include <iostream>
	#include "serial/SerialDebugger.h"
	SerialDebugger serialDebugger;
#endif

using namespace std;
using namespace osc;


extern "C" int _ApplicationController_SerialDeviceMessageReceivedCallback(SerialDevice *device, char *data, size_t len, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    return SELF->handleSerialDeviceMessageReceivedEvent(static_cast<MonomeXXhDevice *>(device), data, len);
}

extern "C" void _ApplicationController_OSCMessageReceivedCallback(const osc::ReceivedMessage &msg, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleOscMessage(msg);
}

extern "C" void _ApplicationController_MIDIReceivedCallback(DWORD msg, CCoreMIDIEndpointRef source, void* userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleMIDIReceived(msg, source);
}


ApplicationController::ApplicationController(CMonomeSerialDlg* appController)
{
	_appController = appController;
	
    _protocol = kProtocolType_OpenSoundControl;

    _oscHostAddressString = "127.0.0.1";
    _oscHostPort = "8000";
    _oscListenPort = "8080";

    _initCoreMIDI();

	InitializeCriticalSection(&_readLock);

	_defaults = new MonomeSerialDefaults(this);

	_initOpenSoundControl();

	_deviceReader.startReading();

#ifdef DEBUG_PRINT
	cout << "successfully created the ApplicationController." << endl;
#endif
}

ApplicationController::~ApplicationController(void)
{
    while (_devices.size()) {
        MonomeXXhDevice *device = _devices.front();
		_deviceReader.removeSerialDevice(device); // this will call device->setUnexpectedDeviceRemovalFlag() and kill read thread
        _devices.erase(_devices.begin());

        if (device != 0)
		{
            delete device;
			device = 0;
		}
    }

	OscListenRef listenRef = _oscController.getOscListenRef(_oscListenPort, false);
	_oscController.releaseOscListenRef(listenRef);

	if (_defaults != 0)
	{
		delete _defaults;
		_defaults = 0;
	}
	
	if (_cCoreMIDI != 0)
	{
		delete _cCoreMIDI;
		_cCoreMIDI = 0;
	}

	DeleteCriticalSection(&_readLock);

#ifdef DEBUG_PRINT
	cout << "successfully destroyed the ApplicationController." << endl;
#endif
}

void
ApplicationController::handleSerialDeviceDiscoveredEvent(const string& serialNumber)
{
	// limit of 8 devices set in MonomeSerialDefaults, so i'll stick with it here as well
	if (_devices.size() > 8)
		return;

	MonomeXXhDevice *device = new MonomeXXhDevice(serialNumber);

    _defaults->setDeviceStateFromDefaults(device);

    _devices.push_back(device);

    _deviceReader.addSerialDevice(device, device->messageSize(), _ApplicationController_SerialDeviceMessageReceivedCallback, this);

	device->oscLedClearEvent(false);

	_appController->updateDeviceList();


#ifdef DEBUG_PRINT
	cout << "successfully added serial devices " << serialNumber << " to the ApplicationController." << endl;
#endif
}

// TODO - terminated calls made via WM_DEVICEMODECHANGE messages
void 
ApplicationController::handleSerialDeviceTerminatedEvent(const string& serialNumber)
{
    vector<MonomeXXhDevice *>::iterator i;

    for (i = _devices.begin(); i != _devices.end(); i++) {
        MonomeXXhDevice *device = *i;

		if (device != 0 && device->serialNumber() == serialNumber) {
            _deviceReader.removeSerialDevice(device);
			_devices.erase(i);
            delete device;
			device = 0;
			break;
        }
    }
	_appController->updateDeviceList();
}

int
ApplicationController::handleSerialDeviceMessageReceivedEvent(MonomeXXhDevice *device, char *data, size_t len)
{
	//ApplicationControllerLock(this);

    if (device == 0)
        return 0;

	unsigned int i = 0;

	while (i < len) {
		t_message *message = (t_message *)data;

#ifdef DEBUG_PRINT
		serialDebugger.printIncomingSerialMessage(device, message);
#endif

		if (device->type() == MonomeXXhDevice::kDeviceType_40h) {
			switch (messageGetType(*message)) {
				case kMessageTypeButtonPress:
					handleButtonPressEvent(device, 
										   messageGetButtonX(*message), 
										   messageGetButtonY(*message),
										   messageGetButtonState(*message) > 0);
					break;

				case kMessageTypeAdcVal:
					handleAdcValueChangeEvent(device, messageGetAdcPort(*message), (float)(messageGetAdcVal(*message)) / (float)0x3FF);
					break;

				case kMessageTypeEncVal:
					handleRotaryEncoderEvent(device, messageGetEncPort(*message), messageGetEncVal(*message));
					break;

				default:
					return -1;
			}
		}
		else if (device->type() <= MonomeXXhDevice::kDeviceType_64) {//always 2 bytes from 256device {
			switch (messageGetType(*message)) {
				case kMessageType_256_keydown:
					handleButtonPressEvent(device, 
						                   messageGetButtonX(*message), 
							               messageGetButtonY(*message), 1);
					break;

				case kMessageType_256_keyup:
					handleButtonPressEvent(	device, 
											messageGetButtonX(*message), 
											messageGetButtonY(*message), 0);
					break;	
									   
				case kMessageType_256_auxiliaryInput	:
					handleRotaryEncoderEvent(	device, 
												messageGetEncPort(*message), 
												messageGetEncVal(*message));		
					break;
									
				case kMessageTypeTiltEvent:
					handleTiltValueChangeEvent(	device, 	
												messageGetTiltAxis(*message),  
												(messageGetEncVal(*message)));		
					break;	
																								
            default:
                return -1;
				
            }
		}

		data += device->messageSize();
		i += device->messageSize();
	}

	return 0;
}

void 
ApplicationController::handleButtonPressEvent(MonomeXXhDevice *device, unsigned int localColumn, unsigned int localRow, bool state)
{
	if (device == 0)
        return;

    if (_protocol == kProtocolType_OpenSoundControl) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer) / sizeof(char));

        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternButtonPressSuffix;

        device->convertLocalCoordinatesToOscCoordinates(localColumn, localRow);

		stream << osc::BeginMessage( oscAddressPattern.c_str() ) 
			<< (int)localColumn << (int)localRow << (state ? 1 : 0)
			<< osc::EndMessage;

		_oscController.send(device->OscHostRef(), stream);
    }
    else if(_protocol == kProtocolType_MIDI){
        CCoreMIDIEndpointRef endpointRef;
        unsigned char MIDINoteNumber;

        if ((endpointRef = device->MIDIOutputDevice()) == 0)
            return;

		unsigned char Channelspill = device->MIDIOutputChannel();
	
			MIDINoteNumber = device->convertLocalCoordinatesToMIDINoteNumber(localColumn, localRow);
		
			//wrap > 127 notes from 256	
		if (MIDINoteNumber > 127) //wrap MIDI notes to next channel
			{
			MIDINoteNumber = (MIDINoteNumber%128); 
			Channelspill++;
			if (Channelspill > 16) Channelspill = 1;
			}
		
        _cCoreMIDI->sendShort(endpointRef, 0x90 | Channelspill, MIDINoteNumber, state ? 127 : 0);
    }

//#ifdef DEBUG_PRINT
//	device->oscLedStateChangeEvent(localColumn, localRow, state);
//#endif
}

void 
ApplicationController::handleAdcValueChangeEvent(MonomeXXhDevice *device, unsigned int localAdcIndex, float value)
{
    if (device == 0)
        return;
    
    if (_protocol == kProtocolType_OpenSoundControl) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer) / sizeof(char));

        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternAdcValueSuffix;
        
		stream << osc::BeginMessage(oscAddressPattern.c_str())
			<< (int)(device->oscAdcOffset() + localAdcIndex) << (float)value
			<< osc::EndMessage;
        
		_oscController.send(device->OscHostRef(), stream);
    }
    else {
        CCoreMIDIEndpointRef endpointRef;
        unsigned char ccValue;
        
        if ((endpointRef = device->MIDIOutputDevice()) == 0)
            return;
        
        if (value >= 1.f)
            ccValue = 127;
        else if (value <= 0.f)
            ccValue = 0;
        else
            ccValue = (unsigned char)(value * 127.f);
                
        _cCoreMIDI->sendShort(endpointRef, 0xB0 | device->MIDIOutputChannel(), localAdcIndex, ccValue);
    }
}

void 
ApplicationController::handleTiltValueChangeEvent(MonomeXXhDevice *device, int WhichAxis, float value)
{
    if (device == 0)
        return;
		
    if (WhichAxis == 0)
	device->LastTiltX = value;
	else device->LastTiltY = value;
	
    if (_protocol == kProtocolType_OpenSoundControl) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer) / sizeof(char));

        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternTiltValueSuffix;
        
		stream << osc::BeginMessage(oscAddressPattern.c_str())
			<< (float)device->LastTiltX
			<< (float)device->LastTiltY
			<< osc::EndMessage;
        
        _oscController.send(device->OscHostRef(), stream);
    }
    else {
        CCoreMIDIEndpointRef endpointRef;
        unsigned char ccValue;
        
        if ((endpointRef = device->MIDIOutputDevice()) == 0)
            return;
        
        if (value >= 1.f)
            ccValue = 127;
        else if (value <= 0.f)
            ccValue = 0;
        else
            ccValue = (unsigned char)(value * 127.f);
                
        _cCoreMIDI->sendShort(endpointRef, 0xB0 | device->MIDIOutputChannel(), 6, ccValue);
    }
}

void 
ApplicationController::handleRotaryEncoderEvent(MonomeXXhDevice *device, unsigned int localEncoderIndex, int steps)
{
    if (device == 0)
        return;

    if (_protocol == kProtocolType_OpenSoundControl) {
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer) / sizeof(char));

        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternEncValueSuffix;

		stream << osc::BeginMessage(oscAddressPattern.c_str())
			<< (int)(device->oscEncOffset() + localEncoderIndex) << steps // the oscEncOffset was oscAdcOffset for some reason
			<< osc::EndMessage;

		_oscController.send(device->OscHostRef(), stream);
    }
    else {
        CCoreMIDIEndpointRef endpointRef;
        
        if ((endpointRef = device->MIDIOutputDevice()) == 0)
            return;
        
		while (steps > 0) {
			unsigned char stepsUsed;

			if (steps > 63)
				stepsUsed = 63;
			else 
				stepsUsed = steps;
				
			_cCoreMIDI->sendShort(endpointRef, 0xB0 | device->MIDIOutputChannel(), localEncoderIndex + 4, stepsUsed + 64); // offset by the number of adcs
			
			steps -= stepsUsed;
		}
		
		while (steps < 0) {
			char stepsUsed;

			if (steps < -64)
				stepsUsed = -64;
			else 
				stepsUsed = steps;
				
			_cCoreMIDI->sendShort(endpointRef, 0xB0 | device->MIDIOutputChannel(), localEncoderIndex + 4, stepsUsed + 64); // offset by the number of adcs
			
			steps += stepsUsed;
		}
    }
}

void 
ApplicationController::handleOscMessage(const osc::ReceivedMessage &recmsg)
{
	if (_protocol != kProtocolType_OpenSoundControl) {
        return;
	}

	OscMessageStream stream(recmsg);

    static vector<MonomeXXhDevice *> matchingDevices;
	string suffix(stream.getAddressPatternSuffix());

	if (stream.getAddressPattern().substr(0, 5) == "/sys/") {
        _handleOscSystemMessage(stream);
        return;
    }

    vector<MonomeXXhDevice *>::iterator i;

    matchingDevices.clear();
    for (i = _devices.begin(); i != _devices.end(); i++) {
		if ((*i)->oscAddressPatternPrefix() == stream.getAddressPatternPrefix())
            matchingDevices.push_back(*i);
    }

    if (matchingDevices.size() == 0)
        return;


    if (suffix == kOscDefaultAddrPatternLedStateSuffix) {  /* prefix/led */

		if (!stream.typetagMatch(kOscDefaultTypeTagsLedState))
            return;

		unsigned int column = stream.getInt32();
        unsigned int row = stream.getInt32();
        bool state = stream.getInt32() ? true : false;

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedStateChangeEvent(column, row, state);
	}
    else if (suffix == kOscDefaultAddrPatternLedIntensitySuffix) { /* prefix/intensity */
		if (!stream.typetagMatch(kOscDefaultTypeTagsLedIntensity))
            return;

		float intensity = stream.getFloat();

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedIntensityChangeEvent(intensity);
    }
    else if (suffix == kOscDefaultAddrPatternLedTestSuffix) { /* prefix/test */
		if (!stream.typetagMatch(kOscDefaultTypeTagsLedTest))
            return;

		bool testState = stream.getInt32() ? true : false;

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedTestStateChangeEvent(testState);
    }
    else if (suffix == kOscDefaultAddrPatternLedClearSuffix) { /* prefix/clear */
		bool clear;
		
		if (stream.argumentCount() == 0) 
			clear = false;
		else if (!stream.typetagMatch(kOscDefaultTypeTagsLedTest))
            return;
		else 
			clear = stream.getInt32() > 0;
			
		for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedClearEvent(clear);
    }
    else if (suffix == kOscDefaultAddrPatternAdcEnableSuffix) { /* prefix/adc_enable */
		if (!stream.typetagMatch(kOscDefaultTypeTagsAdcEnable))
            return;

		int adcIndex = stream.getInt32();
		bool adcState = stream.getInt32() > 0;
        
		for (i = matchingDevices.begin(); i != matchingDevices.end(); i++) {
            (*i)->oscAdcEnableStateChangeEvent(adcIndex, adcState);
			
			// update GUI
			_appController->UpdateAdcStates();
		}
    }
    else if (suffix == kOscDefaultAddrPatternShutdownSuffix) { /* prefix/shutdown */
		if (!stream.typetagMatch(kOscDefaultTypeTagsShutdown))
            return;

		bool shutdownState = stream.getInt32() ? true : false;

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscShutdownStateChangeEvent(shutdownState);
    }
	else if (suffix == kOscDefaultAddrPatternLed_ModeSuffix) { /* prefix/led_mode */
		unsigned int stateout =5;
		if (stream.typetagMatch(kOscTypeTagString)) { 	

			string testmode(stream.getString());
			if (testmode == "off")  stateout = 0;
			else  if (testmode == "on") stateout = 1;
			else if (testmode == "normal") stateout = 2;
			else return;
		}
		else stateout = stream.getInt32();
		
        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
			(*i)->oscLed_ModeStateChangeEvent(stateout);
	}
    else if (suffix == kOscDefaultAddrPatternLedRowSuffix) { /* prefix/led_row */
        if (!_typeCheckRowOrColumnMessage(stream))
            return;

		unsigned int row = stream.getInt32();

        unsigned int index = 0;
        unsigned char bitmap[256];

		while (!stream.endOfStream()) 
			bitmap[index++] = stream.getInt32();

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedRowStateChangeEvent(row, index, bitmap);
    }
    else if (suffix == kOscDefaultAddrPatternLedColumnSuffix) { /* prefix/led_col */
        if (!_typeCheckRowOrColumnMessage(stream))
            return;

		unsigned int column = stream.getInt32();

        unsigned int index = 0;
        unsigned char bitmap[256];

		while(!stream.endOfStream()) {
			bitmap[index++] = stream.getInt32();
		}

        for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
            (*i)->oscLedColumnStateChangeEvent(column, index, bitmap);
    }
    else if (suffix == kOscDefaultAddrPatternEncEnableSuffix) { /* prefix/enc_enable */
		if (!stream.typetagMatch(kOscDefaultTypeTagsEncEnable))
            return;

		int encIndex = stream.getInt32();
		bool encState = stream.getInt32() ? true : false;
        
		for (i = matchingDevices.begin(); i != matchingDevices.end(); i++) {
            (*i)->oscEncEnableStateChangeEvent(encIndex, encState);

			// update GUI
			_appController->UpdateEncStates();
		}
    }
    else if (suffix == kOscDefaultAddrPatternLedFrameSuffix) { /* prefix/frame */
		if (stream.typetagMatch(kOscDefaultTypeTagsLedOffsetFrame)) { // 10 Ints, first 2 for offset
          unsigned char bitmap[8];

			unsigned int column = stream.getInt32();
			unsigned int row = stream.getInt32();
			unsigned int index = 0;

			while (!stream.endOfStream()) {
				bitmap[index++] = stream.getInt32();
			}

            for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
                (*i)->oscLedFrameEvent(column, row, bitmap);   
		       
        }
		else if (stream.typetagMatch(kOscDefaultTypeTagsLedFrame)) { // 8 Ints
            unsigned int index = 0;
            unsigned char bitmap[8];

			while (!stream.endOfStream()) {
				bitmap[index++] = stream.getInt32();
			}

            for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
                (*i)->oscLedFrameEvent(0, 0, bitmap);        
        }
    } //end /frame
	// Tilt - 64 only! - (added by Steve)
	else if (suffix == kOscDefaultAddrPatternTilt_ModeSuffix) { // 1 int, as bool
		if (!stream.typetagMatch(kOscDefaultTypeTagsTiltMode))
			return;

		bool tiltmode = stream.getInt32() ? true : false;
			
		for (i = matchingDevices.begin(); i != matchingDevices.end(); i++)
			(*i)->oscTiltEnableStateChangeEvent(tiltmode);  		 
	}
}

void 
ApplicationController::handleMIDIReceived(DWORD msg, CCoreMIDIEndpointRef source)
{
	if (this->protocol() != this->kProtocolType_MIDI)
		return;

    vector<MonomeXXhDevice *>::iterator i;

    for (i = _devices.begin(); i != _devices.end(); i++) {
        MonomeXXhDevice *device = *i;

        if (device->MIDIInputDevice() == source) {
			unsigned char status, data1, data2;
			CShortMsg::UnpackShortMsg(msg, status, data1, data2);
			_handleMIDIMessage(device, status, data1, data2);
		}
	}
}


void 
ApplicationController::_initCoreMIDI(void)
{
    _cCoreMIDI = new CCoreMIDI(_ApplicationController_MIDIReceivedCallback, this);

#ifdef DEBUG_PRINT
	cout << "successfully created CCoreMIDI midi controller." << endl;
#endif
}


void 
ApplicationController::_handleOscSystemMessage(OscMessageStream msg)
{
    MonomeXXhDevice *device = 0;
    vector<MonomeXXhDevice *>::iterator i;
	const string addressPattern(msg.getAddressPattern());

    unsigned int index;
    static const string systemPrefixString = kOscDefaultAddrPatternSystemPrefix;

#ifdef DEBUG_PRINT
	cout << "/sys/prefix message = " << msg.getAddressPattern();
#endif

	if (addressPattern == kOscDefaultAddrPatternSystemPrefix) {
		if (msg.typetagMatch(kOscDefaultTypeTagsSysPrefixAll)) {
			string pre = msg.getString();
			const string& newPrefix = pre[0] == '/' ? pre : ("/" + pre);

            for (index = 0; index < numberOfDevices(); index++) {
                if ((device = deviceAtIndex(index)) != 0) {
					char buffer[OUTPUT_BUFFER_SIZE];
					osc::OutboundPacketStream packet( buffer, OUTPUT_BUFFER_SIZE );

					device->setOscAddressPatternPrefix(newPrefix);

					packet << osc::BeginMessage(systemPrefixString.c_str()) << (int)index << newPrefix.c_str() << osc::EndMessage;

					_oscController.send(device->OscHostRef(), packet);
					_appController->updateAddressPatternPrefix();
				}
			}
		}
		else {
			if (msg.typetagMatch(kOscDefaultTypeTagsSysPrefixSingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysPrefixSingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (!device) {
				return;
			}


			string pre = msg.getString();
			const string& newPrefix = pre[0] == '/' ? pre : ("/" + pre);

			device->setOscAddressPatternPrefix(newPrefix);

			char buffer[OUTPUT_BUFFER_SIZE];
			osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
			p << osc::BeginMessage( systemPrefixString.c_str() ) << (int)index 
				<< device->oscAddressPatternPrefix().c_str() << osc::EndMessage;

			_oscController.send(device->OscHostRef(), p);
			if ( device->serialNumber() == _appController->getSelectedSerial() ) {
				// update GUI
				_appController->updateAddressPatternPrefix();
			}
		}
	}
	else if (addressPattern == kOscDefaultAddrPatternSystemCable) {
		if (msg.typetagMatch(kOscDefaultTypeTagsSysCableAll)) {
            MonomeXXhDevice::CableOrientation o;
			const string& orientation = msg.getString();

            if (orientation == "left") 
                o = MonomeXXhDevice::kCableOrientation_Left;
            else if (orientation == "right")
                o = MonomeXXhDevice::kCableOrientation_Right;
            else if (orientation == "up")
                o = MonomeXXhDevice::kCableOrientation_Top;
            else if (orientation == "down")
                o = MonomeXXhDevice::kCableOrientation_Bottom;
            else
                return;

            for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->setCableOrientation(o);
				
			// update GUI

			_appController->updateCable();
		}
		else {
            if (msg.typetagMatch(kOscDefaultTypeTagsSysCableSingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysCableSingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (!device) {
				return;
			}
	
			MonomeXXhDevice::CableOrientation o;
			const string& orientation = msg.getString();

            if (orientation == "left") 
                o = MonomeXXhDevice::kCableOrientation_Left;
            else if (orientation == "right")
                o = MonomeXXhDevice::kCableOrientation_Right;
            else if (orientation == "up")
                o = MonomeXXhDevice::kCableOrientation_Top;
            else if (orientation == "down")
                o = MonomeXXhDevice::kCableOrientation_Bottom;
            else
                return;

			device->setCableOrientation(o);

			if ( device->serialNumber() == _appController->getSelectedSerial() ) {
				// update GUI
				_appController->updateCable();
			}
		}
    }
    else if (addressPattern == kOscDefaultAddrPatternSystemOffset) {
		if (msg.typetagMatch(kOscDefaultTypeTagsSysOffsetAll)) {

			unsigned int x = msg.getInt32();
			unsigned int y = msg.getInt32();

            for (i = _devices.begin(); i != _devices.end(); i++) {
                device = *i;
                device->setOscStartColumn(x);
                device->setOscStartRow(y);
            }

			// update GUI
			_appController->updateStartingColumn();
			_appController->updateStartingRow();
        }
		else {
			if (msg.typetagMatch(kOscDefaultTypeTagsSysOffsetSingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysOffsetSingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (!device) {
				return;
			}

			unsigned int x = msg.getInt32();
            unsigned int y = msg.getInt32();
            
			device->setOscStartColumn(x);
            device->setOscStartRow(y);	
			// update GUI
			if ( device->serialNumber() == _appController->getSelectedSerial() ) {
				_appController->updateStartingColumn();
				_appController->updateStartingRow();
			}
        }
    }
    else if (addressPattern == kOscDefaultAddrPatternSystemLedIntensity) {
		if (msg.typetagMatch(kOscDefaultTypeTagsSysLedIntensityAll)) {
			float intensity = msg.getFloat();

            for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->oscLedIntensityChangeEvent(intensity);
        }
		else {
			if (msg.typetagMatch(kOscDefaultTypeTagsSysLedIntensitySingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysLedIntensitySingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (!device) {
				return;
			}

			float intensity = msg.getFloat();

            device->oscLedIntensityChangeEvent(intensity);
		}
    }
    else if (addressPattern == kOscDefaultAddrPatternSystemLedTest) {
		if (msg.typetagMatch(kOscDefaultTypeTagsSysLedTestAll)) {
			bool state = msg.getInt32() ? true : false;

            for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->oscLedTestStateChangeEvent(state);
        }
		else {
			if (msg.typetagMatch(kOscDefaultTypeTagsSysLedTestSingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysLedTestSingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (!device) {
				return;
			}
			
            bool state = msg.getInt32() ? true : false;

            device->oscLedTestStateChangeEvent(state);
        }
    }
    else if (addressPattern == kOscDefaultAddrPatternSystemReport) {

		if (msg.argumentCount() == 0) {
			char buffer[OUTPUT_BUFFER_SIZE];
			osc::OutboundPacketStream packet( buffer, OUTPUT_BUFFER_SIZE );
			
			// send /sys/devices message
			packet << osc::BeginMessage( kOscDefaultAddrPatternSystemNumDevices ) 
			  << (int)numberOfDevices() << osc::EndMessage;

			_oscController.send(device->OscHostRef(), packet);


			// send /sys/prefix messages
			for (index = 0; index < numberOfDevices(); index++) {
				packet.Clear();

				device = this->deviceAtIndex(index);
				if (!device) {
					continue;
				}

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemPrefix )
					<< (int)index 
					<< device->oscAddressPatternPrefix().c_str() 
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
			}

			// send /sys/type messages
			for (index = 0; index < numberOfDevices(); index++) {
				packet.Clear();

				device = this->deviceAtIndex(index);
				if (!device) {
					continue;
				}

				char* deviceType = 0;

				switch(device->type()) {
					case MonomeXXhDevice::kDeviceType_40h:
						deviceType = kOscDeviceType40h;
						break;
					case MonomeXXhDevice::kDeviceType_64:
						deviceType = kOscDeviceType64;
						break;
					case MonomeXXhDevice::kDeviceType_128:
						deviceType = kOscDeviceType128;
						break;
					case MonomeXXhDevice::kDeviceType_256:
						deviceType = kOscDeviceType256;
						break;
				}

				if (!deviceType) {
					continue;
				}

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemDevType )
					<< (int)index 
					<< deviceType
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
			}

			// send /sys/cable messages
			for (index = 0; index < numberOfDevices(); index++) {
				packet.Clear();

				device = this->deviceAtIndex(index);
				if (!device) {
					continue;
				}

				char* deviceOrientation = 0;

				switch(device->cableOrientation()) {
					case MonomeXXhDevice::kCableOrientation_Bottom:
						deviceOrientation = kOscDeviceOrientationBottom;
						break;
					case MonomeXXhDevice::kCableOrientation_Left:
						deviceOrientation = kOscDeviceOrientationLeft;
						break;
					case MonomeXXhDevice::kCableOrientation_Right:
						deviceOrientation = kOscDeviceOrientationRight;
						break;
					case MonomeXXhDevice::kCableOrientation_Top:
						deviceOrientation = kOscDeviceOrientationUp;
						break;
				}

				if (!deviceOrientation) {
					continue;
				}

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemCable )
					<< (int)index 
					<< deviceOrientation
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
			}

			// send /sys/offset messages
			for (index = 0; index < numberOfDevices(); index++) {
				packet.Clear();

				device = this->deviceAtIndex(index);
				if (!device) {
					continue;
				}

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemOffset )
					<< (int)index 
					<< (int)device->oscStartColumn() 
					<< (int)device->oscStartRow()
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
			}

			// send /sys/serial messages
			for (index = 0; index < numberOfDevices(); index++) {
				packet.Clear();

				device = this->deviceAtIndex(index);
				if (!device) {
					continue;
				}

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemDevSerial )
					<< (int)index 
					<< device->serialNumber().c_str()
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
			}
		}
		else {
			if (msg.typetagMatch(kOscDefaultTypeTagsSysReportSingle)) {
				index = msg.getInt32();
				device = deviceAtIndex(index);
			}
			else if(msg.typetagMatch(kOscDefaultTypeTagsSysReportSingleSerial)) {
				std::string serialNum = msg.getString();
				device = deviceBySerial(serialNum, index);
			}
			else {
				return;
			}

			if (device) {
				char buffer[OUTPUT_BUFFER_SIZE];
				osc::OutboundPacketStream packet( buffer, OUTPUT_BUFFER_SIZE );

				// send /sys/prefix messages
				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemPrefix )
					<< device->oscAddressPatternPrefix().c_str() 
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);


				// send /sys/type messages
				packet.Clear();

				char* deviceType = 0;

				switch(device->type()) {
					case MonomeXXhDevice::kDeviceType_40h:
						deviceType = kOscDeviceType40h;
						break;
					case MonomeXXhDevice::kDeviceType_64:
						deviceType = kOscDeviceType64;
						break;
					case MonomeXXhDevice::kDeviceType_128:
						deviceType = kOscDeviceType128;
						break;
					case MonomeXXhDevice::kDeviceType_256:
						deviceType = kOscDeviceType256;
						break;
				}

				if (deviceType) {
					packet << osc::BeginMessage( kOscDefaultAddrPatternSystemDevType )
						<< deviceType
						<< osc::EndMessage;

					_oscController.send(device->OscHostRef(), packet);
				}


				// send /sys/cable messages
				packet.Clear();

				char* deviceOrientation = 0;

				switch(device->cableOrientation()) {
					case MonomeXXhDevice::kCableOrientation_Bottom:
						deviceOrientation = kOscDeviceOrientationBottom;
						break;
					case MonomeXXhDevice::kCableOrientation_Left:
						deviceOrientation = kOscDeviceOrientationLeft;
						break;
					case MonomeXXhDevice::kCableOrientation_Right:
						deviceOrientation = kOscDeviceOrientationRight;
						break;
					case MonomeXXhDevice::kCableOrientation_Top:
						deviceOrientation = kOscDeviceOrientationUp;
						break;
				}

				if (deviceOrientation) {
					packet << osc::BeginMessage( kOscDefaultAddrPatternSystemCable )
						<< deviceOrientation
						<< osc::EndMessage;

					_oscController.send(device->OscHostRef(), packet);
				}


				// send /sys/offset messages
				packet.Clear();

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemOffset )
						<< (int)device->oscStartColumn() 
						<< (int)device->oscStartRow()
						<< osc::EndMessage;

					_oscController.send(device->OscHostRef(), packet);

				// send /sys/serial messages
				packet.Clear();

				packet << osc::BeginMessage( kOscDefaultAddrPatternSystemDevSerial )
					<< device->serialNumber().c_str()
					<< osc::EndMessage;

				_oscController.send(device->OscHostRef(), packet);
            }
        }
    }
}

void 
ApplicationController::protocolPopUpMenuChanged(unsigned int index)
{
    if (index == 1)
        _protocol = kProtocolType_OpenSoundControl;
    else
        _protocol = kProtocolType_MIDI;
}

void 
ApplicationController::oscHostAddressTextFieldChanged(unsigned int deviceIndex, const string& oscHostAddressString)
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);
	if (!device) return;

	const string& oldHostAddressStr = device->OscHostAddress();
	unsigned int oscHostPort = device->OscHostPort();
	const string oscHostPortStr = CMonomeSerialDlg::IntToString(oscHostPort);

	if (oscHostAddressString == oldHostAddressStr)
		return;

	// validate new ipaddress. GetHostByName() located in oscpack->NetworkingUtils.cpp
	unsigned long validAddress = GetHostByName(oscHostAddressString.c_str());
	if (validAddress == 0) {
		throw std::exception("Invalid OSC Host Address string");
	}

	OscHostRef newHostRef = _oscController.getOscHostRef(oscHostAddressString, oscHostPortStr, false);

    if (newHostRef == 0) {
		throw std::exception("Invalid OSC Host Port string");
	}

	device->setOscHostAddress(oscHostAddressString);

	OscHostRef oldHostRef = device->OscHostRef();
	device->setOscHostRef(newHostRef);

    _oscController.releaseOscHostRef(oldHostRef);
}

void 
ApplicationController::oscHostPortTextFieldChanged(unsigned int deviceIndex, const string& oscHostPortString)
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);
	if (!device) return;

	unsigned int oldOscHostPort = device->OscHostPort();
	unsigned int newOscHostPort = CMonomeSerialDlg::StringToInt(oscHostPortString);
	string oldOscHostPortStr = CMonomeSerialDlg::IntToString(oldOscHostPort);

	// if ports are the same, don't do anything
	if (oldOscHostPort == newOscHostPort)
		return;

	OscHostRef newHostRef = 0;
	newHostRef = _oscController.getOscHostRef(device->OscHostAddress(), oscHostPortString, false);

    if (newHostRef == 0) { // changed from throwing -1 to std::exception (why wasn't this done originally?)
		throw std::exception("Invalid OSC Host Port or Address string");
	}

	OscHostRef oldHostRef = device->OscHostRef();

	_oscController.releaseOscHostRef(oldHostRef);

	device->setOscHostPort(newOscHostPort);

	device->setOscHostRef(newHostRef);
    

#ifdef DEBUG_PRINT
    cout << "ApplicationController::oscHostPortTextFieldChanged" << endl;
#endif
}

void 
ApplicationController::oscListenPortTextFieldChanged(unsigned int deviceIndex, const string& oscListenPortString)
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);
	if (!device) return;

	unsigned int oldOscListenPort = device->OscListenPort();
	unsigned int newOscListenPort = CMonomeSerialDlg::StringToInt(oscListenPortString);
	string oldOscListenPortStr = CMonomeSerialDlg::IntToString(oldOscListenPort);

	if (oldOscListenPort == newOscListenPort)
		return;

	OscListenRef newListenRef = 0;
    newListenRef = _oscController.getOscListenRef(oscListenPortString, false);

	if (newListenRef == 0) { // changed from throwing -1 to std::exception (why wasn't this done originally?)
		throw std::exception("Invalid OSC Listen Port string");
	}

	OscListenRef oldListenRef = device->OscListenRef();
	_oscController.releaseOscListenRef(oldListenRef);

	device->setOSCListenPort(newOscListenPort);
	device->setOscListenRef(newListenRef);

#ifdef DEBUG_PRINT
	cout << "ApplicationController::oscListenPortTextFieldChanged : " << endl;
#endif
}


void 
ApplicationController::midiInputDeviceChanged(unsigned int deviceIndex, unsigned int midiInputDeviceIndex)
{
    MonomeXXhDevice *device;

    if ((device = deviceAtIndex(deviceIndex)) == 0)
        return;

    CCoreMIDIEndpointRef oldEndpoint, endpoint;

    oldEndpoint = device->MIDIInputDevice();

	try {
		if (midiInputDeviceIndex != (unsigned int)-1) { // -1 for unsigned int is max unsigned int value
			if ((endpoint = _cCoreMIDI->getEndpointRefForInputDevice(midiInputDeviceIndex)) == 0) {
				endpoint = _cCoreMIDI->createInputDevice(midiInputDeviceIndex);
			}
		}
		else {
			endpoint = 0;
		}
	}
	catch(...) {
		endpoint = 0;
		_appController->updateMidiInputDevice();
	}

	device->setMIDIInputDevice(endpoint);

	// if oldEndpoint is NULL, or endpoint == oldEndpoint (no change), we can return...
    if (oldEndpoint == 0 || oldEndpoint == endpoint)
        return;

	// ... otherwise we might have to delete the oldEndpoint if no other MonomeXXhDevices reference it
	bool destroy = true;
	for (vector<MonomeXXhDevice*>::iterator i = _devices.begin(); i != _devices.end(); i++) {
		if (*i != device && (*i)->MIDIInputDevice() == oldEndpoint) {
			destroy = false;
			break;
		}
	}
	if (destroy) {
		_cCoreMIDI->closeInputDevice(oldEndpoint);
	}
}

void 
ApplicationController::midiInputChannelChanged(unsigned int deviceIndex, unsigned char channel)
{
    MonomeXXhDevice *device;

    if ((device = deviceAtIndex(deviceIndex)) == 0)
        return;

    device->setMIDIInputChannel(channel);
}

void 
ApplicationController::midiOutputDeviceChanged(unsigned int deviceIndex, unsigned int midiOutputDeviceIndex)
{
    MonomeXXhDevice *device;

    if ((device = deviceAtIndex(deviceIndex)) == 0)
        return;

    CCoreMIDIEndpointRef endpoint, oldEndpoint;

	oldEndpoint = device->MIDIOutputDevice();

	try {
		if (midiOutputDeviceIndex != (unsigned int)-1) {
			if ((endpoint = _cCoreMIDI->getEndpointRefForOutputDevice(midiOutputDeviceIndex)) == 0) {
				endpoint = _cCoreMIDI->createOutputDevice(midiOutputDeviceIndex);
			}
		}
		else {
			endpoint = 0;
		}
	}
	catch(...) {
		endpoint = 0;
		_appController->updateMidiOutputDevice();
	}

	device->setMIDIOutputDevice(endpoint);
    
	if (oldEndpoint == 0 || endpoint == oldEndpoint)
		return;

	bool destroy = true;
	for (vector<MonomeXXhDevice*>::iterator i = _devices.begin(); i != _devices.end(); i++) {
		if ((*i)->MIDIOutputDevice() == oldEndpoint) {
			destroy = false;
			break;
		}
	}

	if (destroy) {
		_cCoreMIDI->closeOutputDevice(oldEndpoint);
	}
}

void 
ApplicationController::midiOutputChannelChanged(unsigned int deviceIndex, unsigned char channel)
{
    MonomeXXhDevice *device;

    if ((device = deviceAtIndex(deviceIndex)) == 0)
        return;

    device->setMIDIOutputChannel(channel);
}

void 
ApplicationController::cableOrientationPopUpMenuChanged(unsigned int deviceIndex, unsigned int orientationIndex)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    switch (orientationIndex) {
    case 0:
        device->setCableOrientation(MonomeXXhDevice::kCableOrientation_Left);
        break;

    case 1:
        device->setCableOrientation(MonomeXXhDevice::kCableOrientation_Top);
        break;

    case 2:
        device->setCableOrientation(MonomeXXhDevice::kCableOrientation_Right);
        break;

    case 3:
        device->setCableOrientation(MonomeXXhDevice::kCableOrientation_Bottom);
        break;

    default:
        device->setCableOrientation(MonomeXXhDevice::kCableOrientation_Left);
        break;
    }
}


void 
ApplicationController::oscAddressPatternPrefixTextFieldChanged(unsigned int deviceIndex, const string& oscAddressPatternPrefix)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    static const string systemPrefixString = kOscDefaultAddrPatternSystemPrefix;

    if (device == 0)
        return;

    device->setOscAddressPatternPrefix(oscAddressPatternPrefix);

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream stream(buffer, sizeof(buffer) / sizeof(char));

	stream << osc::BeginMessage( systemPrefixString.c_str() ) 
		<< (int)deviceIndex << device->oscAddressPatternPrefix().c_str()
		<< osc::EndMessage;

    _oscController.send(device->OscHostRef(), stream);
}

void 
ApplicationController::startingColumnTextFieldChanged(unsigned int deviceIndex, unsigned int startingColumn)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->setOscStartColumn(startingColumn);    
}

void 
ApplicationController::startingRowTextFieldChanged(unsigned int deviceIndex, unsigned int startingRow)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->setOscStartRow(startingRow);
}

void 
ApplicationController::oscAdcOffsetTextFieldChanged(unsigned int deviceIndex, unsigned int offset)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->setOscAdcOffset(offset);
}

void 
ApplicationController::oscEncOffsetTextFieldChanged(unsigned int deviceIndex, unsigned int offset)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->setOscEncOffset(offset);
}

void 
ApplicationController::adcStateButtonChanged(unsigned int deviceIndex, unsigned int localAdcIndex, bool state)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->oscAdcEnableStateChangeEvent(localAdcIndex, state);
}

void 
ApplicationController::encStateButtonChanged(unsigned int deviceIndex, unsigned int localEncIndex, bool state)
{
    MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

    if (device == 0)
        return;

    device->oscEncEnableStateChangeEvent(localEncIndex, state);
}


void 
ApplicationController::writePreferences(void)
{
	if (_defaults != 0) {
		for (unsigned int i = 0; i < _devices.size(); i++)
			_defaults->setDefaultsFromDeviceState(_devices[i]);
			
		_defaults->writePreferences();
	}
}

ApplicationController::ProtocolType 
ApplicationController::protocol(void) const
{
    return _protocol;
}

const string& 
ApplicationController::oscHostAddressString(void) const
{
    return _oscHostAddressString;
}

const string& 
ApplicationController::oscHostPort(void) const
{
    return _oscHostPort;
}

const string& 
ApplicationController::oscListenPort(void) const
{
    return _oscListenPort;
}

MonomeXXhDevice *
ApplicationController::deviceAtIndex(unsigned int index) const
{
    if (index >= _devices.size())
        return 0;

    return _devices[index];
}

MonomeXXhDevice *
ApplicationController::deviceBySerial(const std::string& serialNum, unsigned int& index)
{
	MonomeXXhDevice *device = 0;
	index = 0;

    for(unsigned int i = 0; i < _devices.size(); i++) {
		if (serialNum == _devices[i]->serialNumber()) {
			device = _devices[i];
			index = i;
			break;
		}
	}

	return device;
}

unsigned int 
ApplicationController::numberOfDevices(void) const
{
    return (unsigned int)_devices.size();
}

unsigned int 
ApplicationController::numberOfMIDIInputDevices(void) const
{
    return _cCoreMIDI->getNumberOfInputDevices();
}

const string
ApplicationController::nameOfMIDIInputDeviceAtIndex(unsigned int index) const
{
    return _cCoreMIDI->getInputDeviceName(index);
}

unsigned int 
ApplicationController::numberOfMIDIOutputDevices(void) const
{
    return _cCoreMIDI->getNumberOfOutputDevices();
}

const string 
ApplicationController::nameOfMIDIOutputDeviceAtIndex(unsigned int index) const
{
    return _cCoreMIDI->getOutputDeviceName(index);
}

unsigned int 
ApplicationController::indexOfMIDIInputDeviceForMonomeXXhDeviceIndex(unsigned int deviceIndex) const
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

	if (device == 0)
		return -1;

	CCoreMIDIEndpointRef endpoint = device->MIDIInputDevice();
	if (endpoint == 0)
		return -1;

	return _cCoreMIDI->getInputDeviceIDForEndpointRef(endpoint);
}
	
unsigned int 
ApplicationController::indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex(unsigned int deviceIndex) const
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

	if (device == 0)
		return -1;

	CCoreMIDIEndpointRef endpoint = device->MIDIOutputDevice();
	if (endpoint == 0)
		return -1;

	return _cCoreMIDI->getOutputDeviceIDForEndpointRef(endpoint);
}

CCoreMIDI *
ApplicationController::coreMIDI(void) const
{
    return _cCoreMIDI;
}

void 
ApplicationController::_initOpenSoundControl(void)
{
    //_oscHostRef = _oscController.getOscHostRef(_oscHostAddressString, _oscHostPort, true);

	_oscController.addGenericOscMessageHandler(_ApplicationController_OSCMessageReceivedCallback, this);

	/*try {
		_oscController.getOscListenRef(_oscListenPort, true);
	}
	catch (...) {
		throw "MonomeSerial failed to start because port 8080 is already in use!";
	}*/

#ifdef DEBUG_PRINT
	cout << "successfully created OscController." << endl;
#endif
}

bool 
ApplicationController::_typeCheckOscAtoms(const osc::ReceivedMessage &msg, const char *typetags)
{
    size_t ntypetags = strlen(typetags);

	if (msg.ArgumentCount() != ntypetags)
        return false;

	for (osc::ReceivedMessageArgumentIterator i = msg.ArgumentsBegin(); i != msg.ArgumentsEnd(); ++i) {

        switch (*typetags++) {
        case 'i':
			if (i->IsInt32())
                break;
            else
                return false;

        case 'f':
			if (i->IsFloat())
                break;
            else
                return false;

        case 's':
			if (i->IsString())
                break;
            else
                return false;
        }
    }

    return true;
}

bool 
ApplicationController::_typeCheckRowOrColumnMessage(OscMessageStream msg)
{
	if (msg.argumentCount() < 2)
        return false;

	return msg.typetagMatch("ii");
}

void 
ApplicationController::_handleMIDIMessage(MonomeXXhDevice *device, unsigned char status, unsigned char data1, unsigned char data2)
{
	if (device == 0)
		return;

	if ((status & 0xF0) == 0x90 && (status & 0xF) == device->MIDIInputChannel()) {
		device->MIDILedStateChangeEvent(data1, data2);
	}
	else if ((status & 0xF0) == 0x80 && (status & 0xF) == device->MIDIInputChannel()) {
		device->MIDILedStateChangeEvent(data1, 0);
	}
	else if ((status & 0xF0) == 0xB0 && (status & 0xF) == device->MIDIInputChannel()) {
		if (data1 < 4) {
			device->oscAdcEnableStateChangeEvent(data1, data2 >= 64);

			// update GUI
			_appController->UpdateAdcStates();
		}
		else if (data1 < 6) {
			device->oscEncEnableStateChangeEvent(data1 - 4, data2 >= 64);

				// update GUI
			_appController->UpdateEncStates();
		}                                    
	}

	if ((status & 0xF0) == 0x90 && (status & 0xF) == (device->MIDIInputChannel()+1)%16) //so next channel can reach the other half of the 256 LED's
		device->MIDILedStateChangeEvent(data1+128, data2);
	else if ((status & 0xF0) == 0x80 && (status & 0xF) == (device->MIDIInputChannel()+1)%16) 
		device->MIDILedStateChangeEvent(data1+128, 0);	
}

