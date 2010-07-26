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


#ifndef __MonomeXXhDevice_h__
#define __MonomeXXhDevice_h__

#include "SerialDevice.h"
#include "../midi/CCoreMIDI.h"

#define kMonomeXXhDevice_SerialNumberLength 8 // changed to 8, thats what i use

#define _DEBUG_NEW_DEVICE_ 1

static int iquadrant[16]= {
0,1,
2,3,
2,0,
3,1,
3,2,
1,0,
1,3,
0,2};

class MonomeXXhDevice : public SerialDevice
{
public:
    typedef enum { //do not reorder!
        kDeviceType_40h,
        kDeviceType_100h,    //unused
		kDeviceType_256, //16x16
		kDeviceType_128, //8x16 or 16x8
		kDeviceType_64,   //8x8
		kDeviceType_mk,
        kDeviceType_NumTypes
    } DeviceType;

    typedef enum {
        kCableOrientation_Left,
        kCableOrientation_Top,
        kCableOrientation_Right,
        kCableOrientation_Bottom,
        kCableOrientation_NumOrientations
    } CableOrientation;

    typedef enum {
        kMessageSize_40h = 2,
        kMessageSize_100h = 4
    } MessageSize;


public:
    MonomeXXhDevice(const string& serialNumber);
    ~MonomeXXhDevice();

    unsigned int columns(void) const;
    unsigned int rows(void) const;
	unsigned int DeviceOrientation(void) const ;

    DeviceType type(void) const { return _type; }
    unsigned int messageSize(void) const;

    void setCableOrientation(CableOrientation orientation);
    CableOrientation cableOrientation(void) const;

    void setOscAddressPatternPrefix(const string& oscAddressPatternPrefix);
    const string& oscAddressPatternPrefix(void) const;

    void setOscStartColumn(unsigned int column);
    unsigned int oscStartColumn(void) const;

    void setOscStartRow(unsigned int row);
    unsigned int oscStartRow(void) const;

    void setOscAdcOffset(unsigned int offset);
    unsigned int oscAdcOffset(void) const;

    void setOscEncOffset(unsigned int offset);
    unsigned int oscEncOffset(void) const;

    bool adcEnableState(unsigned int localIndex);
    bool encEnableState(unsigned int localIndex);
	
	bool tiltEnableState(void);

    void setMIDIInputDevice(CCoreMIDIEndpointRef midiInputDevice);
    CCoreMIDIEndpointRef MIDIInputDevice(void) const;
    void setMIDIOutputDevice(CCoreMIDIEndpointRef midiInputDevice);
    CCoreMIDIEndpointRef MIDIOutputDevice(void) const;

    void setMIDIInputChannel(unsigned char channel);
    unsigned char MIDIInputChannel(void) const;
    void setMIDIOutputChannel(unsigned char channel);
    unsigned char MIDIOutputChannel(void) const;
	
	void setOscHostPort(unsigned int port);
	unsigned int OscHostPort(void);
	void setOSCListenPort(unsigned int port);
	unsigned int OscListenPort(void);
	void setOscHostAddress(const string &hostAddress);
	const string& OscHostAddress(void);
	void* OscHostRef(void);
	void setOscHostRef(void* hostRef);
	void* OscListenRef(void);
	void setOscListenRef(void* listenRef);
    
    void convertLocalCoordinatesToOscCoordinates(unsigned int &column, unsigned int &row);
    void convertOscCoordinatesToLocalCoordinates(unsigned int &column, unsigned int &row);
    unsigned char convertLocalCoordinatesToMIDINoteNumber(unsigned int &column, unsigned int &row);
    void convertMIDINoteNumberToLocalCoordinates(unsigned char MIDINoteNumber, unsigned int &column, unsigned int &row);

    void oscLedStateChangeEvent(unsigned int column, unsigned int row, bool state);
    void oscLedIntensityChangeEvent(float intensity);
    void oscLedTestStateChangeEvent(bool testState);
	void oscLed_ModeStateChangeEvent(int State); //"led_mode" - a macro of sorts, added with 256x support but supported on 40h also
    void oscLedClearEvent(bool clear);
	void oscShutdownStateChangeEvent(bool shutdownState);
    void oscLedRowStateChangeEvent(unsigned int row, unsigned int numBitMaps, unsigned char bitMaps[]);
    void oscLedColumnStateChangeEvent(unsigned int column, unsigned int numBitMaps, unsigned char bitMaps[]);
    void oscAdcEnableStateChangeEvent(unsigned int localAdcIndex, bool adcEnableState);
    void oscEncEnableStateChangeEvent(unsigned int localEncIndex, bool encEnableState);
    void oscLedFrameEvent(unsigned int column, unsigned int row, unsigned char bitMaps[8]);

    void MIDILedStateChangeEvent(unsigned char MIDINoteNumber, unsigned char MIDIVelocity);

	void oscTiltEnableStateChangeEvent(bool tiltEnableState); // for the 64 only!

		//mk- aux to device
	void oscAuxVersionRequestEvent(void);
	void oscAuxEnableEvent(unsigned int portF, unsigned int portA);
	void oscAuxDirectionEvent(unsigned int portF, unsigned int portA);
	void oscAuxStateEvent(unsigned int portF, unsigned int portA);
	

	float LastTiltX;//added for 64
	float LastTiltY;//added for 64

private:

    DeviceType _type;
    unsigned int _columns;
    unsigned int _rows;

    CableOrientation _orientation;

    string _oscAddressPatternPrefix;
    unsigned int _oscStartColumn;
    unsigned int _oscStartRow;

    unsigned int _oscAdcOffset;
    unsigned int _oscEncOffset;

    bool _adcState[4];
    bool _encState[2];

	bool _tiltState;

    CCoreMIDIEndpointRef _midiInputDevice;
    CCoreMIDIEndpointRef _midiOutputDevice;
    unsigned char _midiInputChannel;
    unsigned char _midiOutputChannel;

	unsigned int _hostPort;
	unsigned int _listenPort;
	string _hostAddress;
	void* _oscHostRef;
	void* _oscListenRef;

	CRITICAL_SECTION _lock;

	class MonomeXXhDeviceLock {
	public:
		MonomeXXhDeviceLock(const MonomeXXhDevice *device) 
		{ 
			EnterCriticalSection( _lock = &( (const_cast<MonomeXXhDevice *>(device))->_lock ) );
		}
		~MonomeXXhDeviceLock() 
		{ 
			LeaveCriticalSection(_lock); 
		}

	private:
		CRITICAL_SECTION *_lock;
	};

	friend class MonomeXXhDeviceLock;
};

#endif // __MonomeXXhDevice_h__
