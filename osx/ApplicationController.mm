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
#include "SerialDeviceNotifications.h"
#include "ApplicationController.h"
#include "message.h"
#include "message256.h"
#include "messageMK.h"
#include "osc.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
	 
#include <iostream>
#include <sstream>
using namespace std;

static void _ApplicationController_SerialDeviceDiscoveredCallback(const char *bsdFilePath, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleSerialDeviceDiscoveredEvent(bsdFilePath);
}

static void _ApplicationController_SerialDeviceTerminatedCallback(const char *bsdFilePath, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleSerialDeviceTerminatedEvent(bsdFilePath);
}

static int _ApplicationController_SerialDeviceMessageReceivedCallback(SerialDevice *device, char *data, size_t len, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    return SELF->handleSerialDeviceMessageReceivedEvent(static_cast<MonomeXXhDevice *>(device), data, len);
}

static void _ApplicationController_OSCMessageReceivedCallback(const string& addressPattern, list <OscAtom *> *atoms, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleOscMessage(addressPattern, atoms);
}

static void _ApplicationController_MIDIReceivedOnVirtualDestinationCallback(const MIDIPacketList *packetList, void *userData, CCoreMIDIEndpointRef source)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleMIDIReceivedOnVirtualDestination(packetList, source);
}

static void _ApplicationController_MIDIReceivedCallback(const MIDIPacketList *packetList, void *userData, CCoreMIDIEndpointRef source)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleMIDIReceived(packetList, source);
}


static void _ApplicationController_MIDISystemStateChangedCallback(const MIDINotification *message, void *userData)
{
    ApplicationController *SELF = (ApplicationController *)userData;
    SELF->handleMIDISystemStateChanged(message);
}

ApplicationController::ApplicationController(AppController *appController)
{
	_appController = appController;
	
    _protocol = kProtocolType_MIDI;

    _oscHostAddressString = "127.0.0.1";
    _oscHostPort = 8000;
    _oscListenPort = 8080;
	
	_initOpenSoundControl();
    _initCoreMIDI();

    _defaults = new MonomeSerialDefaults(this);
}

ApplicationController::~ApplicationController()
{
    _deviceReader.stopReading();

	if (_defaults != 0)
		delete _defaults;

    while (_devices.size()) {
        MonomeXXhDevice *device = _devices.front();
        _devices.erase(_devices.begin());

        if (device != 0)
            delete device;
    }
	
	if (_cCoreMIDI != 0)
		delete _cCoreMIDI;
}

void 
ApplicationController::registerForSerialDeviceNotifications(void)
{
    SerialDeviceNotificationContext notificationContext = { _ApplicationController_SerialDeviceDiscoveredCallback,
                                                            _ApplicationController_SerialDeviceTerminatedCallback,
															this };
    RegisterForSerialDeviceNotifications(&notificationContext);

    _deviceReader.startReading();
}

void 
ApplicationController::handleSerialDeviceDiscoveredEvent(const char *bsdFilePath)
{
    MonomeXXhDevice *device = new MonomeXXhDevice(bsdFilePath);

    device->setMIDIInputDevice(_toMonomeSerial);
    device->setMIDIOutputDevice(_fromMonomeSerial);

    _defaults->setDeviceStateFromDefaults(device);

    string portName = bsdFilePath;
    CCoreMIDIPortRef port = _cCoreMIDI->MIDIInputPortCreate(portName, _ApplicationController_MIDIReceivedCallback, this);

    device->setMIDIInputPort(port);

    _devices.push_back(device);

    _deviceReader.addSerialDevice(device, device->messageSize(), _ApplicationController_SerialDeviceMessageReceivedCallback, this);
	
	[_appController updateDeviceList];
}

void 
ApplicationController::handleSerialDeviceTerminatedEvent(const char *bsdFilePath)
{
    vector<MonomeXXhDevice *>::iterator i;

    for (i = _devices.begin(); i != _devices.end(); i++) {
        MonomeXXhDevice *device = *i;

        if (device->bsdFilePath() == bsdFilePath) {
            _devices.erase(i);

            if (device != 0) {
                device->setUnexpectedDeviceRemovalFlag(true);
                _deviceReader.removeSerialDevice(device);

                _defaults->setDefaultsFromDeviceState(device);

                delete device;
            }

            break;
        }
    }

	[_appController updateDeviceList];
}

int
ApplicationController::handleSerialDeviceMessageReceivedEvent(MonomeXXhDevice *device, char *data, size_t len)
{
    if (device == 0)
        return 0;

    if (device->type() == MonomeXXhDevice::kDeviceType_40h) {
        unsigned int i = 0;

        while (i < len) {
            t_message *message = (t_message *)data;
            switch (messageGetType(*message)) {
            case kMessageTypeButtonPress:
                handleButtonPressEvent(device, 
                                       messageGetButtonX(*message), 
                                       messageGetButtonY(*message),
                                       messageGetButtonState(*message));
									   
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

            data += device->messageSize();
            i += device->messageSize();
        }
    }
	else  if (device->type() <= MonomeXXhDevice::kDeviceType_64) //always 2 bytes from 256device
	 {
	 
	      unsigned int i = 0;
		
		 
        while (i < len) {
            t_message *message = (t_message *)data;
			
            switch (messageGetType(*message)) {
          
		  
		  case kMessageType_256_keydown:
									handleButtonPressEvent(device, 
                                       messageGetButtonX(*message), 
                                       messageGetButtonY(*message), 1);
									   break;
			
		   case kMessageType_256_keyup:
									handleButtonPressEvent(device, 
                                       messageGetButtonX(*message), 
                                       messageGetButtonY(*message), 0);
									   break;	
									   
		case kMessageType_256_auxiliaryInput	:
									handleRotaryEncoderEvent(device, messageGetEncPort(*message), messageGetEncVal(*message));		
									break;
									
			case kMessageTypeTiltEvent:
			    handleTiltValueChangeEvent(device, 	messageGetTiltAxis(*message),  (messageGetEncVal(*message)));		
				break;	
																								
		  /*
			 case kMessageTypeButtonPress:
                handleButtonPressEvent(device, 
                                       messageGetButtonX(*message), 
                                       messageGetButtonY(*message),
                                       messageGetButtonState(*message));
                break;

            case kMessageTypeAdcVal:
                handleAdcValueChangeEvent(device, messageGetAdcPort(*message), (float)(messageGetAdcVal(*message)) / (float)0x3FF);
                break;

            case kMessageTypeEncVal:
                handleRotaryEncoderEvent(device, messageGetEncPort(*message), messageGetEncVal(*message));
                break;
*/
            default:
                return -1;
				
            }

            data += device->messageSize();
            i += device->messageSize();
        }
    
	 } //end 256
	else  if (device->type() == MonomeXXhDevice::kDeviceType_mk)
	{
		
		unsigned int i = 0;
		
		
        while (i < len) {
            t_message *message = (t_message *)data;
			//t_mk_1byte_message *message_1b = (t_mk_1byte_message *)data;
			t_mk_3byte_message *message_3b = (t_mk_3byte_message *)data;
			
            switch (messageGetType(*message)) {
					
					
				case kMessageType_mk_keydown:
					handleButtonPressEvent(device, 
										   messageGetButtonX(*message), 
										   messageGetButtonY(*message), 1);
					break;
					
				case kMessageType_mk_keyup:
					handleButtonPressEvent(device, 
										   messageGetButtonX(*message), 
										   messageGetButtonY(*message), 0);
					break;	

					
				case kMessageTypeTiltEvent:
					handleTiltValueChangeEvent(device, 	messageGetTiltAxis(*message),  (messageGetEncVal(*message)));		
					break;	
					
				case kMessageType_mk_auxout:
				{
					int auxOutType = messageGet_mk_AuxOutType(*message);
					switch (auxOutType) {
						case 0:
							//version
							handleAuxVersionReportEvent(device, messageGet_mk_Version(*message_3b));
							break;
						case 1:
							//analog
							handleAuxAnalogValueChangeEvent(device,
															messageGet_mk_AnalogPort(*message_3b),
															(float)(messageGet_mk_AnalogValue(*message_3b)) / (float)0x3FF);
							break;
						case 2:
							//digital
							handleAuxDigitalValueChangeEvent(device,
															 messageGet_mk_DigitalNumber(*message_3b),
															 messageGet_mk_DigitalState(*message_3b));
							break;
						case 3:
							//encoder
							handleAuxEncoderValueChangeEvent(device,
															 messageGet_mk_EncoderNumber(*message_3b),
															 messageGet_mk_EncoderChange(*message_3b));
							break;
						default:
							break;
					}
				}

				default:
					return -1;
					
            }
			
            data += device->messageSize();
            i += device->messageSize();
        }
		
	} //end 256
	
			 
	 

#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleSerialDeviceMessageReceivedEvent" << endl;
#endif

	return 0;
}

void 
ApplicationController::handleButtonPressEvent(MonomeXXhDevice *device, unsigned int localColumn, unsigned int localRow, bool state)
{
    static list<OscAtom *> oscAtomList;
    static OscAtom atoms[3];

    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
        oscAtomList.push_back(&(atoms[2]));
    }

    if (device == 0)
        return;

    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternButtonPressSuffix;

        device->convertLocalCoordinatesToOscCoordinates(localColumn, localRow);

        atoms[0].setValue((int)localColumn);
        atoms[1].setValue((int)localRow);
        atoms[2].setValue(state ? 1 : 0);

        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
    else {
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

#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleButtonPressEvent" << endl;
#endif
}

void 
ApplicationController::handleAdcValueChangeEvent(MonomeXXhDevice *device, unsigned int localAdcIndex, float value)
{
    static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];
    
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }
    
    if (device == 0)
        return;
    
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternAdcValueSuffix;
        
        atoms[0].setValue((int)(device->oscAdcOffset() + localAdcIndex));
        atoms[1].setValue((float)value);
        
        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
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

#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAdcValueChangeEvent" << endl;
#endif
}

void 
ApplicationController::handleTiltValueChangeEvent(MonomeXXhDevice *device, int WhichAxis, int value)
{
    static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];
    
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }
    
    if (device == 0)
        return;
		
    if (WhichAxis == 0)
	device->LastTiltX = value;
	else device->LastTiltY = value;
	
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternTiltValueSuffix;
        
        atoms[0].setValue((int)device->LastTiltX);
		atoms[1].setValue((int)device->LastTiltY);
		
		
          _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
   /* Tilt MIDI, just a copy from ADC, needs tweaking if you want it to work 
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
*/
#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAdcValueChangeEvent" << endl;
#endif
}


void 
ApplicationController::handleRotaryEncoderEvent(MonomeXXhDevice *device, unsigned int localEncoderIndex, int steps)
{
    static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];

    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }

    if (device == 0)
        return;

    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternEncValueSuffix;

        atoms[0].setValue((int)(device->oscAdcOffset() + localEncoderIndex));
        atoms[1].setValue(steps);

        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
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

#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleRotaryEncoderEvent" << endl;
#endif
}

void 
ApplicationController::handleOscMessage(const string& addressPattern, list <OscAtom *> *atoms)
{
    static vector<MonomeXXhDevice *> matchingDevices;

    if (_protocol != kProtocolType_OpenSoundControl || atoms == 0)
        return;

    if (addressPattern.substr(0, 5) == "/sys/") {
        _handleOscSystemMessage(addressPattern, atoms);
        return;
    }

    unsigned int indexOfSuffix;

    if ((indexOfSuffix = addressPattern.rfind('/')) >= addressPattern.size())
        return;

    string prefix = addressPattern.substr(0, indexOfSuffix);
    string suffix = addressPattern.substr(indexOfSuffix);

    vector<MonomeXXhDevice *>::iterator deviceIter;

    matchingDevices.clear();
    for (deviceIter = _devices.begin(); deviceIter != _devices.end(); deviceIter++) {
        if ((*deviceIter)->oscAddressPatternPrefix() == prefix)
            matchingDevices.push_back(*deviceIter);
    }

    if (matchingDevices.size() == 0)
        return;

    list<OscAtom *>::iterator atomIter;

	
    if (suffix == kOscDefaultAddrPatternLedStateSuffix) {

        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedState))
            return;

        unsigned int column = (*(atomIter = atoms->begin())++)->valueAsInt();
        unsigned int row = (*atomIter++)->valueAsInt();
        bool state = (*atomIter++)->valueAsInt() ? true : false;

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedStateChangeEvent(column, row, state);
	}

    else if (suffix == kOscDefaultAddrPatternLedIntensitySuffix) {
        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedIntensity))
            return;

        float intensity = (*(atoms->begin()))->valueAsFloat();

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedIntensityChangeEvent(intensity);
    }

    else if (suffix == kOscDefaultAddrPatternLedTestSuffix) {
        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedTest))
            return;

        bool testState = (*(atoms->begin()))->valueAsInt() ? true : false;

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedTestStateChangeEvent(testState);
    }

    else if (suffix == kOscDefaultAddrPatternLedClearSuffix) {
		bool clear;
		
		if (atoms->size() == 0) 
			clear = false;
        else if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedTest))
            return;
		else 
			clear = (*(atoms->begin()))->valueAsInt() ? true : false;
			
		for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedClearEvent(clear);
    }
	else if (suffix == 	kOscDefaultAddrPatternTiltModeSuffix) { // 1 int, as bool
				 bool tiltmode = (*(atoms->begin()))->valueAsInt() ? true : false;
			      
			//fprintf(stderr, "enabling tiltmode %i", tiltmode); //bobo
            for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
                (*deviceIter)->oscTiltEnableStateChangeEvent(tiltmode);  
				 
				 }
    else if (suffix == kOscDefaultAddrPatternAdcEnableSuffix) {
        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsAdcEnable))
            return;

        int adcIndex = (*(atomIter = atoms->begin())++)->valueAsInt();
        bool adcState = (*atomIter++)->valueAsInt() ? true : false;
        
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscAdcEnableStateChangeEvent(adcIndex, adcState);        

        [_appController updateAdcStates];
        [_appController updateEncStates];
    }

    else if (suffix == kOscDefaultAddrPatternShutdownSuffix) {
        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsShutdown))
            return;

        bool shutdownState = (*(atoms->begin()))->valueAsInt() ? true : false;

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscShutdownStateChangeEvent(shutdownState);
    }
	
else if (suffix == kOscDefaultAddrPatternLed_ModeSuffix) {
unsigned int stateout =5;
        if (!_typeCheckOscAtoms(*atoms, kOscTypeTagString))
			{ 	

	const string& testmode = (*(atoms->begin()))->valueAsString();
	if (testmode == "off")  stateout = 0;
	else  if (testmode == "on") stateout = 1;
	else if (testmode == "normal") stateout = 2;
	else return;
	}
	else stateout = (*(atomIter = atoms->begin())++)->valueAsInt();
		
               for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
                 (*deviceIter)->oscLed_ModeStateChangeEvent(stateout);
				

}
    else if (suffix == kOscDefaultAddrPatternLedRowSuffix) {
        if (!_typeCheckRowOrColumnMessage(*atoms))
            return;

        unsigned int row = (*(atomIter= atoms->begin())++)->valueAsInt();

        unsigned int index;
        unsigned int bitmap[256];

        for (index = 0; atomIter != atoms->end() && index < 256; atomIter++, index++) 
            bitmap[index] = (*atomIter)->valueAsInt();

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedRowStateChangeEvent(row, index, bitmap);
    }


    else if (suffix == kOscDefaultAddrPatternLedColumnSuffix) {
        if (!_typeCheckRowOrColumnMessage(*atoms))
            return;

        if (!_typeCheckRowOrColumnMessage(*atoms))
            return;

        unsigned int column = (*(atomIter= atoms->begin())++)->valueAsInt();

        unsigned int index;
        unsigned int bitmap[256];

        for (index = 0; atomIter != atoms->end() && index < 256; atomIter++, index++) 
            bitmap[index] = (*atomIter)->valueAsInt();

        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscLedColumnStateChangeEvent(column, index, bitmap);
    }

	
    else if (suffix == kOscDefaultAddrPatternEncEnableSuffix) {
        if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsEncEnable))
            return;

        int encIndex = (*(atomIter = atoms->begin())++)->valueAsInt();
        bool encState = (*atomIter++)->valueAsInt() ? true : false;
        
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscEncEnableStateChangeEvent(encIndex, encState);        

        [_appController updateAdcStates];
        [_appController updateEncStates];
    }

    else if (suffix == kOscDefaultAddrPatternLedFrameSuffix) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedFrame)) { // 8 Ints
            unsigned int index;
            unsigned int bitmap[8];

            for (index = 0, atomIter = atoms->begin(); atomIter != atoms->end(); atomIter++, index++)
                bitmap[index] = (*atomIter)->valueAsInt();

            for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
                (*deviceIter)->oscLedFrameEvent(0, 0, bitmap);        
        }
		
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedOffsetFrame)) { // 10 Ints, first 2 for offset
            unsigned int column, row, index;
            unsigned int bitmap[8];

            column = (*(atomIter = atoms->begin())++)->valueAsInt();
            row = (*atomIter++)->valueAsInt();

            for (index = 0; atomIter != atoms->end(); atomIter++, index++)
                bitmap[index] = (*atomIter)->valueAsInt();

            for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
                (*deviceIter)->oscLedFrameEvent(column, row, bitmap);        
        }

		        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsLedOffsetFrame)) { // 2 ints, must assume its 
            unsigned int column, row, index;
            unsigned int bitmap[8];

            column = (*(atomIter = atoms->begin())++)->valueAsInt();
            row = (*atomIter++)->valueAsInt();

            for (index = 0; atomIter != atoms->end(); atomIter++, index++)
                bitmap[index] = (*atomIter)->valueAsInt();

            for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
                (*deviceIter)->oscLedFrameEvent(column, row, bitmap);        
        }
	
    } //end /frame
	
	//mk sys messages, might need to be moved up to handleOsc if they turn out to not be system messages
	
	else if (suffix == kOscDefaultAddrPatternAuxVersionSuffix)
	{
		//send version request
		//if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysAuxVersionReq))
        //    return;
		
		if (atoms->size() > 0)
		{
			//probably an error, but forgivable
#ifdef DEBUG_PRINT
			cout << "ApplicationController::handleOscMessage was passed a non-zero atom list with /prefix/aux/version" << endl;
#endif
		}
			
		
        //bool testState = (*(atoms->begin()))->valueAsInt() ? true : false;
		
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscAuxVersionRequestEvent();
		
	}
	
	else if (suffix == kOscDefaultAddrPatternAuxEnableSuffix)
	{
		//send enable message
		
		if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysAuxEnable))
            return;
		
        unsigned int portF = (*(atomIter = atoms->begin())++)->valueAsInt();
        unsigned int portA = (*atomIter++)->valueAsInt();
		
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscAuxEnableEvent(portF, portA);
			
	}
	else if (suffix == kOscDefaultAddrPatternAuxDirectionSuffix)
	{
		//send direction message
		
		if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysAuxDirection))
            return;
		
        unsigned int portF = (*(atomIter = atoms->begin())++)->valueAsInt();
        unsigned int portA = (*atomIter++)->valueAsInt();
		
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscAuxDirectionEvent(portF, portA);
	}
	
	else if (suffix == kOscDefaultAddrPatternAuxStateSuffix)
	{
		//state message
		if (!_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysAuxState))
            return;
		
        unsigned int portF = (*(atomIter = atoms->begin())++)->valueAsInt();
        unsigned int portA = (*atomIter++)->valueAsInt();
		
        for (deviceIter = matchingDevices.begin(); deviceIter != matchingDevices.end(); deviceIter++)
            (*deviceIter)->oscAuxStateEvent(portF, portA);
	}

}

void 
ApplicationController::handleMIDIReceivedOnVirtualDestination(const MIDIPacketList *packetList, CCoreMIDIEndpointRef source)
{
    handleMIDIReceived(packetList, _toMonomeSerial);

#if DEBUG_PRINT
    cout << "ApplicationController::handleMIDIReceivedOnVirtualDestination()" << endl;
#endif
}

void 
ApplicationController::handleMIDIReceived(const MIDIPacketList *packetList, CCoreMIDIEndpointRef source)
{
    vector<MonomeXXhDevice *>::iterator i;

    for (i = _devices.begin(); i != _devices.end(); i++) {
        MonomeXXhDevice *device = *i;

        if (device->MIDIInputDevice() == source) {
            const MIDIPacket *packet = &packetList->packet[0];

            for (unsigned int i = 0; i < packetList->numPackets; i++) {
                unsigned char status, data1, data2, index;

                index = 0;

                for (unsigned int j = 0; j < packet->length; j++) {
                    unsigned char midiByte = packet->data[j];

                    switch (index) {
                    case 0:
                        if (midiByte & 0x80) {
                            status = midiByte;
                            index = 1;
                        }
                           
                        break;

                    case 1:
                        if (midiByte & 0x80) {
                            status = midiByte;
                            index = 1;
                        }
                        else {
                            data1 = midiByte;
                            index = 2;
                        }

                        break;

                    case 2:
                        if (midiByte & 0x80) {
							_handleMIDIMessage(device, status, data1, data2);

                            status = midiByte;
                            index = 1;
                        }
                        else {
                            data2 = midiByte;
							_handleMIDIMessage(device, status, data1, data2);
								
                            index = 0;
                        }

                        break;
                    }
                }
				
				if (index != 0)
					_handleMIDIMessage(device, status, data1, data2);
            }
        }
    }
}

void 
ApplicationController::handleMIDISystemStateChanged(const MIDINotification *)
{
    vector<MonomeXXhDevice *>::iterator i;

    for (i = _devices.begin(); i != _devices.end(); i++) {
        MonomeXXhDevice *device = *i;
        CCoreMIDIEndpointRef endpoint = device->MIDIInputDevice();
        unsigned int j;
        bool match = false;

        for (j = 0; j < _cCoreMIDI->getNumberOfInputDevices(); j++) {
            if (endpoint == _cCoreMIDI->getEndpointRefForInputDevice(j)) {
                match = true;
                break;
            }
        }

        if (!match) {
            device->setMIDIInputDevice(_toMonomeSerial);
            device->setMIDIInputChannel(0);
        }

        match = false;
        endpoint = device->MIDIOutputDevice();

        for (j = 0; j < _cCoreMIDI->getNumberOfOutputDevices(); j++) {
            if (endpoint == _cCoreMIDI->getEndpointRefForOutputDevice(j)) {
                match = true;
                break;
            }
        }

        if (!match) {
            device->setMIDIOutputDevice(_fromMonomeSerial);
            device->setMIDIOutputChannel(0);
        }
    }

    [_appController updateMIDIDevices];
}

void 
ApplicationController::_initCoreMIDI(void)
{
    string name = "MonomeSerial";
    string fromMonomeSerialString = "from MonomeSerial 1";
    string toMonomeSerialString = "to MonomeSerial 1";

    _cCoreMIDI = new CCoreMIDI(name);
    _fromMonomeSerial = _cCoreMIDI->createSource(fromMonomeSerialString);
    _toMonomeSerial = _cCoreMIDI->createDestination(toMonomeSerialString, _ApplicationController_MIDIReceivedOnVirtualDestinationCallback, this);
	
	_cCoreMIDI->registerForMIDISystemStateChangeNotifications(_ApplicationController_MIDISystemStateChangedCallback, this);
}

void 
ApplicationController::_handleOscSystemMessage(const string& addressPattern, list <OscAtom *> *atoms)
{
    MonomeXXhDevice *device;
    vector<MonomeXXhDevice *>::iterator i;
    list<OscAtom *>::iterator j;
    unsigned int index;
    static list<OscAtom> twoAtoms(2);
    static const string systemPrefixString = kOscDefaultAddrPatternSystemPrefix;
    list<OscAtom>::iterator k;

    if (addressPattern == kOscDefaultAddrPatternSystemPrefix) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysPrefixAll)) {
            const string& newPrefix = (*(atoms->begin()))->valueAsString();
           
            for (index = 0; index < numberOfDevices(); index++) {
                if ((device = deviceAtIndex(index)) != 0) {
                    device->setOscAddressPatternPrefix(newPrefix);

                    (*(k = twoAtoms.begin())++).setValue((int)index);
                    (*k).setValue(device->oscAddressPatternPrefix());

                    _oscController.send(_oscHostRef, systemPrefixString, &twoAtoms);
                 }	
            }

			[_appController updateOscAddressPatternPrefix];
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysPrefixSingle)) {
            index = (*(j = atoms->begin())++)->valueAsInt();
            const string& newPrefix = (*j++)->valueAsString();

            if ((device = deviceAtIndex(index)) != 0) {
                device->setOscAddressPatternPrefix(newPrefix);
                
                (*(k = twoAtoms.begin())++).setValue((int)index);
                (*k).setValue(device->oscAddressPatternPrefix());

                _oscController.send(_oscHostRef, systemPrefixString, &twoAtoms);
            }	
				
			[_appController updateOscAddressPatternPrefix];
        }
    }

    else if (addressPattern == kOscDefaultAddrPatternSystemCable) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysCableAll)) {
            MonomeXXhDevice::CableOrientation o;
            const string& orientation = (*(atoms->begin()))->valueAsString();

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
				
			[_appController updateCableOrientation];
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysCableSingle)) {
            MonomeXXhDevice::CableOrientation o;
            index = (*(j = atoms->begin())++)->valueAsInt();
            const string& orientation = (*j++)->valueAsString();

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

            if ((device = deviceAtIndex(index)) != 0)
                device->setCableOrientation(o);

			[_appController updateCableOrientation];
        }
    }

    else if (addressPattern == kOscDefaultAddrPatternSystemOffset) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysOffsetAll)) {
            unsigned int x = (*(j = atoms->begin())++)->valueAsInt();
            unsigned int y = (*j++)->valueAsInt();

            for (i = _devices.begin(); i != _devices.end(); i++) {
                device = *i;
                device->setOscStartColumn(x);
                device->setOscStartRow(y);
            }

			[_appController updateOscStartRowAndColumn];
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysOffsetSingle)) {
            index = (*(j = atoms->begin())++)->valueAsInt();
            unsigned int x = (*j++)->valueAsInt();
            unsigned int y = (*j++)->valueAsInt();
            
            if ((device = deviceAtIndex(index)) != 0) {
                device->setOscStartColumn(x);
                device->setOscStartRow(y);
            }
			
			[_appController updateOscStartRowAndColumn];
        }
    }

    else if (addressPattern == kOscDefaultAddrPatternSystemLedIntensity) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysLedIntensityAll)) {
            float intensity = (*(atoms->begin())++)->valueAsFloat();

            for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->oscLedIntensityChangeEvent(intensity);
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysLedIntensitySingle)) {
            index = (*(j = atoms->begin())++)->valueAsInt();
            float intensity = (*j++)->valueAsFloat();

            if ((device = deviceAtIndex(index)) != 0)
                device->oscLedIntensityChangeEvent(intensity);
        }
    }

    else if (addressPattern == kOscDefaultAddrPatternSystemLedTest) {
        if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysLedTestAll)) {
            bool state = (*(atoms->begin())++)->valueAsInt() ? true : false;

            for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->oscLedTestStateChangeEvent(state);
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysLedTestSingle)) {
            index = (*(j = atoms->begin())++)->valueAsInt();
            bool state = (*j++)->valueAsInt() ? true : false;

            if ((device = deviceAtIndex(index)) != 0)
                device->oscLedTestStateChangeEvent(state);
        }
    }

    else if (addressPattern == kOscDefaultAddrPatternSystemReport) {
        static list<OscAtom> oneAtom(1);
        static list<OscAtom> twoAtoms(2);
        static list<OscAtom> threeAtoms(3);
		static const string systemPrefixString = kOscDefaultAddrPatternSystemPrefix;
		static const string cableOrientationString = kOscDefaultAddrPatternSystemCable;
		static const string offsetString = kOscDefaultAddrPatternSystemOffset;
		static const string boxTypeString = kOscDefaultAddrPatternSystemType;
        list<OscAtom>::iterator k;

        if (atoms->size() == 0) {
            static const string numDevicesPattern = kOscDefaultAddrPatternSystemNumDevices;
            (*(oneAtom.begin())).setValue((int)numberOfDevices());

            _oscController.send(_oscHostRef, numDevicesPattern, &oneAtom);

            for (index = 0; index < numberOfDevices(); index++) {
                device = deviceAtIndex(index);

                if (device == 0)
                    continue;

                (*(k = twoAtoms.begin())++).setValue((int)index);
                (*k).setValue(device->oscAddressPatternPrefix());

                _oscController.send(_oscHostRef, systemPrefixString, &twoAtoms);
				
				switch (device->type()) {
					case MonomeXXhDevice::kDeviceType_40h:
						(*k).setValue("40h");
						break;
					case MonomeXXhDevice::kDeviceType_256:
						(*k).setValue("256");
						break;
					case MonomeXXhDevice::kDeviceType_128:
						(*k).setValue("128");
						break;
					case MonomeXXhDevice::kDeviceType_64:
						(*k).setValue("64");
						break;
					case MonomeXXhDevice::kDeviceType_mk:
						(*k).setValue("mk");
						break;
					default:
						(*k).setValue("unknown device");
						break;
				}
				
                _oscController.send(_oscHostRef, boxTypeString, &twoAtoms);

                switch (device->cableOrientation()) {
                case MonomeXXhDevice::kCableOrientation_Top:
                    (*k).setValue("up");
                    break;

                case MonomeXXhDevice::kCableOrientation_Right:
                    (*k).setValue("right");
                    break;

                case MonomeXXhDevice::kCableOrientation_Bottom:
                    (*k).setValue("bottom");
                    break;

                default:
                    (*k).setValue("left");
                    break;
                }

                _oscController.send(_oscHostRef, cableOrientationString, &twoAtoms);

                (*(k = threeAtoms.begin())++).setValue((int)index);
                (*k++).setValue((int)device->oscStartColumn());
                (*k++).setValue((int)device->oscStartRow());

                _oscController.send(_oscHostRef, offsetString, &threeAtoms);
            }
        }
        else if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysReportSingle)) {
            index = (*(atoms->begin())++)->valueAsInt();

            if ((device = deviceAtIndex(index)) != 0) {
                (*(k = twoAtoms.begin())++).setValue((int)index);
                (*k).setValue(device->oscAddressPatternPrefix());

                _oscController.send(_oscHostRef, systemPrefixString, &twoAtoms);

                switch (device->cableOrientation()) {
                case MonomeXXhDevice::kCableOrientation_Top:
                    (*k).setValue("up");
                    break;

                case MonomeXXhDevice::kCableOrientation_Right:
                    (*k).setValue("right");
                    break;

                case MonomeXXhDevice::kCableOrientation_Bottom:
                    (*k).setValue("bottom");
                    break;

                default:
                    (*k).setValue("left");
                    break;
                }

                _oscController.send(_oscHostRef, cableOrientationString, &twoAtoms);

                (*(k = threeAtoms.begin())++).setValue((int)index);
                (*k++).setValue((int)device->oscStartColumn());
                (*k++).setValue((int)device->oscStartRow());

                _oscController.send(_oscHostRef, offsetString, &threeAtoms);
            }
        }

    } else if (addressPattern == kOscDefaultAddrPatternSystemGrids)
	{
		if (_typeCheckOscAtoms(*atoms, kOscDefaultTypeTagsSysGrids))
		{
			
			unsigned int numGrids = (*atoms->begin())->valueAsInt();
			
			for (i = _devices.begin(); i != _devices.end(); i++) 
                (*i)->oscGridsEvent(numGrids);
						
		}
	}
}


//for mk
void 
ApplicationController::handleAuxVersionReportEvent(MonomeXXhDevice *device, int version)
{
	static list<OscAtom *> oscAtomList;
    static OscAtom atom;
    
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atom));
    }
    
    if (device == 0)
        return;
	
	
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = /* device->oscAddressPatternPrefix() + */ kOscDefaultAddrPatternSystemAuxVersion;
        
        atom.setValue(version);		
		
		_oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
	
#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAuxVersionReportEvent" << endl;
#endif
	
}

void 
ApplicationController::handleAuxAnalogValueChangeEvent(MonomeXXhDevice *device, int whichAux, float value)
{
	static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];
    
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }
    
    if (device == 0)
        return;
    
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = /*device->oscAddressPatternPrefix() + */ kOscDefaultAddrPatternSystemAuxAnalog;
        
        atoms[0].setValue(whichAux);
        atoms[1].setValue((float)value);
        
        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
	/* else {
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
	} */
	
#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAuxAnalogValueChangeEvent" << endl;
#endif
	
}

void ApplicationController::handleAuxDigitalValueChangeEvent(MonomeXXhDevice *device, int whichAux, int value)
{
	//so, what about debouncing... ? done in firmware?
	static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];
	
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }
	
    if (device == 0)
        return;
	
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = /*device->oscAddressPatternPrefix() + */kOscDefaultAddrPatternSystemAuxDigital;
		
        atoms[0].setValue(whichAux);
        atoms[1].setValue(value);
		
        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
	
#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAuxDigitalValueChangeEvent" << endl;
#endif
	
}

void 
ApplicationController::handleAuxEncoderValueChangeEvent(MonomeXXhDevice *device, int whichAux, signed char change)
{
	
    static list<OscAtom *> oscAtomList;
    static OscAtom atoms[2];
	
    if (oscAtomList.size() == 0) {
        oscAtomList.push_back(&(atoms[0]));
        oscAtomList.push_back(&(atoms[1]));
    }
	
    if (device == 0)
        return;
	
    if (_protocol == kProtocolType_OpenSoundControl) {
        string oscAddressPattern = device->oscAddressPatternPrefix() + kOscDefaultAddrPatternSystemAuxEncoder;
		
        atoms[0].setValue(whichAux);
        atoms[1].setValue((signed int)change);
		
        _oscController.send(_oscHostRef, oscAddressPattern, &oscAtomList);
    }
	
#ifdef DEBUG_PRINT
    cout << "ApplicationController::handleAuxEncoderValueChangeEvent" << endl;
#endif
	
	
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
ApplicationController::oscHostAddressTextFieldChanged(const string& oscHostAddressString)
{
    ostringstream ostrstrm;
    ostrstrm << _oscHostPort;
	struct in_addr dummyAddr;

	int validAddress = inet_pton(AF_INET, oscHostAddressString.c_str(), &dummyAddr);
	if (validAddress == 0 || validAddress == -1)
		throw -1;

    OscHostRef newHostRef = _oscController.getOscHostRef(oscHostAddressString, ostrstrm.str());

	if (newHostRef == 0)
		throw -1;

    _oscHostAddressString = oscHostAddressString;

    OscHostRef oldHostRef = _oscHostRef;
    _oscHostRef = newHostRef;

    _oscController.releaseOscHostRef(oldHostRef);

#ifdef DEBUG_PRINT
    cout << "ApplicationController::oscHostAddressTextFieldChanged" << endl;
#endif
}

void 
ApplicationController::oscHostPortTextFieldChanged(unsigned int oscHostPort)
{
    ostringstream ostrstrm;
    ostrstrm << oscHostPort;
	
	// we need to check for a valid port number.

    OscHostRef newHostRef = _oscController.getOscHostRef(_oscHostAddressString.c_str(), ostrstrm.str().c_str());

    if (newHostRef == 0)
        throw -1;

    _oscHostPort = oscHostPort;

    OscHostRef oldHostRef = _oscHostRef;
    _oscHostRef = newHostRef;

    _oscController.releaseOscHostRef(oldHostRef);
    

#ifdef DEBUG_PRINT
    cout << "ApplicationController::oscHostAddressTextFieldChanged" << endl;
#endif
}

void 
ApplicationController::oscListenPortTextFieldChanged(unsigned int oscListenPort)
{
    ostringstream ostrstrm;
    ostrstrm << oscListenPort;

    try {
        _oscController.startListening(ostrstrm.str());
    }
    catch (...) {
        ostringstream ostrstrm;
        ostrstrm << _oscListenPort;

        try {
            _oscController.startListening(ostrstrm.str());
        }
        catch (...) {
            throw -1;
        }
		
		throw -1;
    }

    _oscListenPort = oscListenPort;

#ifdef DEBUG_PRINT
    cout << "ApplicationController::oscHostAddressTextFieldChanged" << endl;
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

    if ((endpoint = _cCoreMIDI->getEndpointRefForInputDevice(midiInputDeviceIndex)) == 0) 
        return;

    if (oldEndpoint == endpoint)
        return;

    if (oldEndpoint != _toMonomeSerial) 
        _cCoreMIDI->MIDIPortDisconnectSource(device->MIDIInputPort(), oldEndpoint);

    if (endpoint != _toMonomeSerial)
        _cCoreMIDI->MIDIPortConnectSource(device->MIDIInputPort(), endpoint);

    device->setMIDIInputDevice(endpoint);
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

    CCoreMIDIEndpointRef endpoint;

    if ((endpoint = _cCoreMIDI->getEndpointRefForOutputDevice(midiOutputDeviceIndex)) == 0) {
        //[_appContoller updateMIDIDeviceList];
        return;
    }

    device->setMIDIOutputDevice(endpoint);
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
	list<OscAtom>::iterator k;
    static list<OscAtom> twoAtoms(2);
    static const string systemPrefixString = kOscDefaultAddrPatternSystemPrefix;

    if (device == 0)
        return;

    device->setOscAddressPatternPrefix(oscAddressPatternPrefix);

    (*(k = twoAtoms.begin())++).setValue((int)deviceIndex);
    (*k).setValue(device->oscAddressPatternPrefix());

    _oscController.send(_oscHostRef, systemPrefixString, &twoAtoms);    
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
ApplicationController::clearLedsButtonChanged(unsigned int deviceIndex)
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);
	
	if (device == 0)
		return;
		
	device->oscLedClearEvent(0);
}

void 
ApplicationController::testModeButtonChanged(unsigned int deviceIndex, bool state)
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);
	
	if (device == 0)
		return;
		
	device->setTestModeState(state);
	device->oscLedTestStateChangeEvent(state);
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

int 
ApplicationController::oscHostPort(void) const
{
    return _oscHostPort;
}

int 
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

unsigned int 
ApplicationController::numberOfDevices(void) const
{
    return _devices.size();
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
		return 0;

	CCoreMIDIEndpointRef endpoint = device->MIDIInputDevice();
	
	for (unsigned int i = 0; i < _cCoreMIDI->getNumberOfInputDevices(); i++) {
		if (endpoint == _cCoreMIDI->getEndpointRefForInputDevice(i))
			return i;
	}
	
	return 0;
}
	
unsigned int 
ApplicationController::indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex(unsigned int deviceIndex) const
{
	MonomeXXhDevice *device = deviceAtIndex(deviceIndex);

	if (device == 0)
		return 0;

	CCoreMIDIEndpointRef endpoint = device->MIDIOutputDevice();
	
	for (unsigned int i = 0; i < _cCoreMIDI->getNumberOfOutputDevices(); i++) {
		if (endpoint == _cCoreMIDI->getEndpointRefForOutputDevice(i))
			return i;
	}
	
	return 0;
}

CCoreMIDI *
ApplicationController::coreMIDI(void) const
{
    return _cCoreMIDI;
}

void 
ApplicationController::_initOpenSoundControl(void)
{
    ostringstream ostrstrm, listen_ostrstrm;
    ostrstrm << _oscHostPort;
	
    // WARN!!!
    _oscHostRef = _oscController.getOscHostRef(_oscHostAddressString.c_str(), ostrstrm.str().c_str());

	_oscController.addGenericOscMessageHandler(_ApplicationController_OSCMessageReceivedCallback, this);

	listen_ostrstrm << _oscListenPort;

	try {
		_oscController.startListening(listen_ostrstrm.str().c_str());
	}
	catch (...) {
		throw "MonomeSerial failed to start because port 8080 is already in use!";
	}
}

bool 
ApplicationController::_typeCheckOscAtoms(list<OscAtom *>& atoms, const char *typetags)
{
    list<OscAtom *>::iterator i;
    unsigned int ntypetags;
    const char * typetag_ptr;

    ntypetags = strlen(typetags);

    if (atoms.size() != ntypetags)
        return false;

    for (i = atoms.begin(), typetag_ptr = typetags; i != atoms.end(); i++, typetag_ptr++) {
        OscAtom *atom = *i;

        switch (*typetag_ptr) {
        case 'i':
            if (atom->isInt())
                break;
            else
                return false;

        case 'f':
            if (atom->isFloat())
                break;
            else
                return false;

        case 's':
            if (atom->isString())
                break;
            else
                return false;
        }
    }

    return true;
}

bool 
ApplicationController::_typeCheckRowOrColumnMessage(list<OscAtom *>& atoms)
{
    list<OscAtom *>::iterator i;

    if (atoms.size() < 2)
        return false;

    for (i = atoms.begin(); i != atoms.end(); i++) {
        if (!(*i)->isInt())
            return false;
    }

    return true;
}

void 
ApplicationController::_handleMIDIMessage(MonomeXXhDevice *device, unsigned char status, unsigned char data1, unsigned char data2)
{
	if (device == 0)
		return;

	if ((status & 0xF0) == 0x90 && (status & 0xF) == device->MIDIInputChannel())
		device->MIDILedStateChangeEvent(data1, data2);
	else if ((status & 0xF0) == 0x80 && (status & 0xF) == device->MIDIInputChannel()) 
		device->MIDILedStateChangeEvent(data1, 0);
	else if ((status & 0xF0) == 0xB0 && (status & 0xF) == device->MIDIInputChannel()) {
		if (data1 < 4) {
				device->oscAdcEnableStateChangeEvent(data1, data2 >= 64);
				[_appController updateAdcStates];
				[_appController updateEncStates];
		}
		else if (data1 < 6) {
			device->oscEncEnableStateChangeEvent(data1 - 4, data2 >= 64);
			[_appController updateAdcStates];
			[_appController updateEncStates];
		}                                    
	}
		if ((status & 0xF0) == 0x90 && (status & 0xF) == (device->MIDIInputChannel()+1)%16) //so next channel can reach the other half of the 256 LED's
		device->MIDILedStateChangeEvent(data1+128, data2);
	else if ((status & 0xF0) == 0x80 && (status & 0xF) == (device->MIDIInputChannel()+1)%16) 
		device->MIDILedStateChangeEvent(data1+128, 0);
	
	
	
}

