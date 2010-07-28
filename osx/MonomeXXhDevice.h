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
#ifndef __MonomeXXhDevice_h__
#define __MonomeXXhDevice_h__

#include "SerialDevice.h"
#include "CCoreMIDI.h"
#include <pthread.h>

#define kMonomeXXhDevice_SerialNumberLength 6

//#define _DEBUG_NEW_DEVICE_ 1

static int iquadrant[16]= {
0,1,
2,3,
2,0,
3,1,
3,2,
1,0,
1,3,
0,2};


//Left = ok
//Top = should light 2, lights 1
//right = correct quadrant but wrong rotation
//bottom = should light 1, ligts 0

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
        kMessageSize_100h = 2
    } MessageSize;


public:
    MonomeXXhDevice(const string& bsdFilePath);
    ~MonomeXXhDevice();

    const char *serialNumber(void) const { return _serialNumber; }

    unsigned int columns(void) ;// { return _columns; }
    unsigned int rows(void) ;
	unsigned int DeviceOrientation(void) ;
	
    DeviceType type(void) const { return _type; }
    unsigned int messageSize(void) const;

    void setCableOrientation(CableOrientation orientation);
    CableOrientation cableOrientation(void) ;

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
	
	void setTestModeState(bool state);
	bool testModeState(void) const;
	
	void setLastOscLedMessage(const string& message);
	const string& lastOscLedMessage(void) const;

    void setMIDIInputDevice(CCoreMIDIEndpointRef midiInputDevice);
    CCoreMIDIEndpointRef MIDIInputDevice(void) const;
    void setMIDIOutputDevice(CCoreMIDIEndpointRef midiInputDevice);
    CCoreMIDIEndpointRef MIDIOutputDevice(void) const;
    void setMIDIInputChannel(unsigned char channel);
    unsigned char MIDIInputChannel(void) const;
    void setMIDIOutputChannel(unsigned char channel);
    unsigned char MIDIOutputChannel(void) const;    
    void setMIDIInputPort(CCoreMIDIPortRef midiInputPort);
    CCoreMIDIPortRef MIDIInputPort(void) const;
    
    void convertLocalCoordinatesToOscCoordinates(unsigned int &column, unsigned int &row);
    void convertOscCoordinatesToLocalCoordinates(unsigned int &column, unsigned int &row);
    unsigned char convertLocalCoordinatesToMIDINoteNumber(unsigned int &column, unsigned int &row);
    void convertMIDINoteNumberToLocalCoordinates(unsigned char MIDINoteNumber, unsigned int &column, unsigned int &row);
 void MIDILedStateChangeEvent(unsigned char MIDINoteNumber, unsigned char MIDIVelocity);
 
    void oscLedStateChangeEvent(unsigned int column, unsigned int row, bool state);
    void oscLedIntensityChangeEvent(float intensity);
    void oscLedTestStateChangeEvent(bool testState);
	void oscLed_ModeStateChangeEvent(int State); //"led_mode" - a macro of sorts, added with 256x support but supported on 40h also
    void oscLedClearEvent(bool clear);
    void oscShutdownStateChangeEvent(bool shutdownState);
    void oscLedRowStateChangeEvent(unsigned int row, unsigned int numBitMaps, unsigned int bitMaps[]);
    void oscLedColumnStateChangeEvent(unsigned int column, unsigned int numBitMaps, unsigned int bitMaps[]);
    void oscAdcEnableStateChangeEvent(unsigned int localAdcIndex, bool adcEnableState);
    void oscEncEnableStateChangeEvent(unsigned int localEncIndex, bool encEnableState);
    void oscLedFrameEvent(unsigned int column, unsigned int row, unsigned int bitMaps[8]);
	
	//mk- aux to device
	void oscAuxVersionRequestEvent(void);
	void oscAuxEnableEvent(unsigned int portF, unsigned int portA);
	void oscAuxDirectionEvent(unsigned int portF, unsigned int portA);
	void oscAuxStateEvent(unsigned int portF, unsigned int portA);
	
	void oscGridsEvent(unsigned int nGrids);
	
	
     
	 void oscTiltEnableStateChangeEvent(bool tiltEnableState);

	int LastTiltX;//added for 64
	int LastTiltY;//added for 64
	
	//additions for mk messages
	//void oscGridsChangeEvent(unsigned int numberOfGrids);
	//void oscAuxInChangeEvent(unsigned int
	
private:
    char _serialNumber[kMonomeXXhDevice_SerialNumberLength];

    DeviceType _type;
    unsigned int _columns;
    unsigned int _rows;

		bool _state256[16][16];

    CableOrientation _orientation;

    string _oscAddressPatternPrefix;
    unsigned int _oscStartColumn;
    unsigned int _oscStartRow;

    unsigned int _oscAdcOffset;
    unsigned int _oscEncOffset;

    bool _adcState[4];
    bool _encState[2];
	
	bool _testModeState;
	string _lastLedMessage;

    CCoreMIDIEndpointRef _midiInputDevice;
    CCoreMIDIEndpointRef _midiOutputDevice;
    unsigned char _midiInputChannel;
    unsigned char _midiOutputChannel;
    CCoreMIDIPortRef _midiInputPortRef;

    pthread_mutex_t _lock;

    class MonomeXXhDeviceLock {
    public:
	
        MonomeXXhDeviceLock(const MonomeXXhDevice *device) { pthread_mutex_lock(_lock = &((const_cast<MonomeXXhDevice *>(device))->_lock)); }
        ~MonomeXXhDeviceLock() { pthread_mutex_unlock(_lock); }

    private:
        pthread_mutex_t *_lock;
    };

    friend class MonomeXXhDeviceLock;
};

#endif // __MonomeXXhDevice_h__
