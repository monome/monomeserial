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
#include "../stdafx.h"
#include "MonomeXXhDevice.h"
#include "message.h"
#include "message256.h"
#include <cstdlib>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

// TODO : add per-device port support in state-saving ...(cont)
//		  perhaps change device saving code to use std::fstream interface (in case i do OSX code too)
//		  update OscHostAddress, OscController and ApplicationController to deal with per-device OSC Ports
//		  update GUI to handle per-device ports
// FINISHED : MonomeXXhDevice per-device osc

static const unsigned char myswap[] = 
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

/*
m40h0200 - 40h serial number 200
m256-004 - 256 serial number 4
m128-020 - 128 serial number 20
m64-0084 - 64 serial number 84
*/

MonomeXXhDevice::MonomeXXhDevice(const string& serialNumber)
    : SerialDevice(serialNumber)
{
//global inits, same for all devices

	_oscStartColumn = 0;
    _oscStartRow = 0;
    _oscAdcOffset = 0;
    _oscEncOffset = 0;
    _midiInputDevice = 0;
    _midiOutputDevice = 0;
    _midiInputChannel = 0; 
    _midiOutputChannel = 0; 
    _adcState[0] = false;
    _adcState[1] = false;
    _adcState[2] = false;
    _adcState[3] = false;
    _encState[0] = false;
    _encState[1] = false;
	_tiltState = false;
	_orientation = kCableOrientation_Left;
	_hostPort = 8000;
	_listenPort = 8080;
	_hostAddress = "127.0.0.1";
	_oscHostRef = 0;
	_oscListenRef = 0;
	
// device-specific stuff
// first determines device type- simple checks, would need improving in theory
// if different models have same char 1

#ifdef DEBUG_PRINT
	fprintf(stderr, "Monome New Device, 4-chars read from serial DeviceName is: [%-4.4s]\n", serialNumber.c_str());
#endif		  
			  	  
		
	if (serialNumber[1] == '4') //40h
	  _type = kDeviceType_40h;//kDeviceType_256 kDeviceType_40h;  //change temp to work on 256
		else if(serialNumber[1] == '2') //256
		_type = kDeviceType_256;
			else if(serialNumber[1] == '1') //128
				_type = kDeviceType_128;
					else if(serialNumber[1] == '6') //64
						_type = kDeviceType_64;
	#ifdef DEBUG_PRINT
					else fprintf(stderr, "***WARNING- NEW MONOME DEVICE IS OF UNKNOWN TYPE \n", serialNumber.c_str());
	#endif	

switch (_type)
	{ 
	 case kDeviceType_40h:
	 _columns = _rows = 8; 
	 _oscAddressPatternPrefix = "/40h"; // this should be some #defined value
		#ifdef DEBUG_PRINT
			fprintf(stderr, "** 40h has been detected! \n", serialNumber.c_str());
		#endif		

	 break;
	 
	case kDeviceType_256:
	 _columns = _rows = 16; 
	 _oscAddressPatternPrefix = "/256"; // this should be some #defined value
	 	#ifdef DEBUG_PRINT
			fprintf(stderr, "** 256 has been detected! \n", serialNumber.c_str());
		#endif		

	//begin (m256-006) stolen serial check
	if ((serialNumber[5] == '0') && (serialNumber[6] == '0') && (serialNumber[7]=='6'))
	{
		#ifdef WIN32
			std::wstring urlString = L"http://www.xferrecords.com/duda/stolen_monome/";
			ShellExecute(NULL, L"open", urlString.c_str(), NULL, NULL, SW_SHOWNORMAL);
		#else //macOS
			std::string urlString = "open http://www.xferrecords.com/duda/stolen_monome/";
			system( urlString );
		#endif
	}
	//end stolen serial check 
		
	 break;
	 
      case kDeviceType_128: // _columns and _rows for _cable == left or right only, always use columns() and rows() to check values
	 _columns = 16;
	 _rows = 8; 
	 _oscAddressPatternPrefix = "/128"; // this should be some #defined value
	  	#ifdef DEBUG_PRINT
			fprintf(stderr, "** 128 has been detected! \n", serialNumber.c_str());
		#endif		
	 break;
	 
	case kDeviceType_64:
	 _columns = _rows = 8;
	 _oscAddressPatternPrefix = "/m64"; // this should be some #defined value
	 	#ifdef DEBUG_PRINT
			fprintf(stderr, "** m64 has been detected! \n", serialNumber.c_str());
		#endif		
	 break;
	}


	InitializeCriticalSection(&_lock);
}

MonomeXXhDevice::~MonomeXXhDevice()
{
	DeleteCriticalSection(&_lock);
}

unsigned int 
MonomeXXhDevice::columns(void) const
{
	if (_type == kDeviceType_128) {
		if (DeviceOrientation() == kCableOrientation_Left || DeviceOrientation() == kCableOrientation_Right) {
			return _columns;
		}
		else {
			return _rows;
		}
	}

	return _columns;
}

unsigned int 
MonomeXXhDevice::rows(void) const
{
	if (_type == kDeviceType_128) {
		if (DeviceOrientation() == kCableOrientation_Left || DeviceOrientation() == kCableOrientation_Right) {
			return _rows;
		}
		else {
			return _columns;
		}
	}

	return _rows;
}

// message size of input (button/adc/enc) packets, in bytes
unsigned int 
MonomeXXhDevice::messageSize(void) const
{
    return kMessageSize_40h; // this method is called for incoming data only, which is always a bytesize of 2
}

unsigned int
MonomeXXhDevice::DeviceOrientation(void) const
{

	if (_type == kDeviceType_128) return (_orientation+3)%4;
	return _orientation;

}

void 
MonomeXXhDevice::setCableOrientation(CableOrientation orientation)
{
    MonomeXXhDeviceLock(this);

    if (orientation >= kCableOrientation_NumOrientations)
        _orientation = kCableOrientation_Left;
    else
        _orientation = orientation;
}

MonomeXXhDevice::CableOrientation 
MonomeXXhDevice::cableOrientation(void) const 
{ 
    return (MonomeXXhDevice::CableOrientation)(DeviceOrientation()); 
} 

void 
MonomeXXhDevice::setOscAddressPatternPrefix(const string& oscAddressPatternPrefix)
{
    MonomeXXhDeviceLock(this);

	if (oscAddressPatternPrefix.size() == 0)
		_oscAddressPatternPrefix = "/box"; // added by dan, so that sending an empty prefix doesn't break anything
    else if (oscAddressPatternPrefix[oscAddressPatternPrefix.size() - 1] == '/')
        _oscAddressPatternPrefix = oscAddressPatternPrefix.substr(0, oscAddressPatternPrefix.size() - 1);
    else
        _oscAddressPatternPrefix = oscAddressPatternPrefix;
}

const string& 
MonomeXXhDevice::oscAddressPatternPrefix(void) const 
{ 
    return _oscAddressPatternPrefix; 
}

void 
MonomeXXhDevice::setOscStartColumn(unsigned int column) 
{ 
    MonomeXXhDeviceLock(this);

    _oscStartColumn = column; 
}

unsigned int 
MonomeXXhDevice::oscStartColumn(void) const 
{
    return _oscStartColumn; 
}

void 
MonomeXXhDevice::setOscStartRow(unsigned int row) 
{ 
    MonomeXXhDeviceLock(this);

    _oscStartRow = row; 
}

unsigned int 
MonomeXXhDevice::oscStartRow(void) const 
{ 
    return _oscStartRow; 
}

void 
MonomeXXhDevice::setOscAdcOffset(unsigned int offset)
{
    MonomeXXhDeviceLock(this);

    _oscAdcOffset = offset;
}

unsigned int 
MonomeXXhDevice::oscAdcOffset(void) const
{
    return _oscAdcOffset;
}

void 
MonomeXXhDevice::setOscEncOffset(unsigned int offset)
{
    MonomeXXhDeviceLock(this);

    _oscEncOffset = offset;
}

unsigned int 
MonomeXXhDevice::oscEncOffset(void) const
{
    return _oscEncOffset;
}

bool 
MonomeXXhDevice::adcEnableState(unsigned int localIndex)
{
    if (localIndex >= 4)
        return false;

    return _adcState[localIndex];
}

bool 
MonomeXXhDevice::encEnableState(unsigned int localIndex)
{
    if (localIndex >= 2)
        return false;

    return _encState[localIndex];
}

bool 
MonomeXXhDevice::tiltEnableState(void)
{
	return _tiltState;
}

void 
MonomeXXhDevice::setMIDIInputDevice(CCoreMIDIEndpointRef midiInputDevice)
{
	MonomeXXhDeviceLock(this);

    _midiInputDevice = midiInputDevice;
}

CCoreMIDIEndpointRef 
MonomeXXhDevice::MIDIInputDevice(void) const
{
    return _midiInputDevice;
}

void 
MonomeXXhDevice::setMIDIOutputDevice(CCoreMIDIEndpointRef midiOutputDevice)
{
	MonomeXXhDeviceLock(this);

    _midiOutputDevice = midiOutputDevice;
}

CCoreMIDIEndpointRef 
MonomeXXhDevice::MIDIOutputDevice(void) const
{
	return _midiOutputDevice;
}

void 
MonomeXXhDevice::setMIDIInputChannel(unsigned char channel)
{
	MonomeXXhDeviceLock(this);

    _midiInputChannel = (channel & 0xF);
}

unsigned char 
MonomeXXhDevice::MIDIInputChannel(void) const
{
	return _midiInputChannel;
}

void 
MonomeXXhDevice::setMIDIOutputChannel(unsigned char channel)
{
	MonomeXXhDeviceLock(this);

    _midiOutputChannel = (channel & 0xF);
}

unsigned char 
MonomeXXhDevice::MIDIOutputChannel(void) const
{
	return _midiOutputChannel;
}

void 
MonomeXXhDevice::setOscHostPort(unsigned int port)
{
	MonomeXXhDeviceLock(this);

	_hostPort = port;
}

unsigned int 
MonomeXXhDevice::OscHostPort(void)
{
	return _hostPort;
}

void 
MonomeXXhDevice::setOSCListenPort(unsigned int port)
{
	MonomeXXhDeviceLock(this);

	_listenPort = port;
}

unsigned int 
MonomeXXhDevice::OscListenPort(void)
{
	return _listenPort;
}

void 
MonomeXXhDevice::setOscHostAddress(const string &hostAddress)
{
	MonomeXXhDeviceLock(this);

	_hostAddress = hostAddress;
}

const string& 
MonomeXXhDevice::OscHostAddress(void)
{
	return _hostAddress;
}

void*  
MonomeXXhDevice::OscHostRef(void) 
{
	return _oscHostRef;
}

void 
MonomeXXhDevice::setOscHostRef(void* hostRef)
{
	_oscHostRef = hostRef;
}

void*  
MonomeXXhDevice::OscListenRef(void) 
{
	return _oscListenRef;
}

void 
MonomeXXhDevice::setOscListenRef(void* listenRef)
{
	_oscListenRef = listenRef;
}
void 
MonomeXXhDevice::convertLocalCoordinatesToOscCoordinates(unsigned int &column, unsigned int &row)
{
    unsigned int c, r;
//fprintf(stderr, "\n local to osc: %i, %i", column, row);

    switch (DeviceOrientation()) {
		case kCableOrientation_Left:
			c = column;
			r = row;
			break;

		case kCableOrientation_Top:
			c = columns() - row - 1;
			r = column;        
			break;

		case kCableOrientation_Right:
			c = columns() - column - 1;
			r = rows() - row - 1;
			break;

		case kCableOrientation_Bottom:
			c = row;
			r = rows() - column - 1;
			break;
    }

    column = c + _oscStartColumn;
    row = r + _oscStartRow;
		//fprintf(stderr, ".....to osc: %i, %i", column, row);

}

void 
MonomeXXhDevice::convertOscCoordinatesToLocalCoordinates(unsigned int &column, unsigned int &row)
{
    unsigned int c, r;

    c = column - _oscStartColumn;
    r = row - _oscStartRow;

    switch (DeviceOrientation()) {
		case kCableOrientation_Left:
			column = c;
			row = r;
			break;

		case kCableOrientation_Top:
			column = r;
			row = columns() - c - 1;
			break;

		case kCableOrientation_Right:
			column = columns() - c - 1;
			row = rows() - r - 1;
			break;

		case kCableOrientation_Bottom:
			column = rows() - r - 1;
			row = c;
			break;
    }

}

unsigned char 
MonomeXXhDevice::convertLocalCoordinatesToMIDINoteNumber(unsigned int &column, unsigned int &row)
{
    unsigned int c, r;
	//fprintf(stderr, "LOCALtoMIDI:  c %i and r %i", c, r);
	
    switch (DeviceOrientation()) {
		case kCableOrientation_Left:
			c = column;
			r = row;
			break;

		case kCableOrientation_Top:
			c = (columns()-1) - row;
			r = column;        
			break;

		case kCableOrientation_Right:
			c = (columns() - 1) - column;
			r = (rows()     -1) - row;
			break;

		case kCableOrientation_Bottom:
			c = row;
			r = (rows() - 1) - column;
			break;
    }

    if (_type >= kDeviceType_40h) 
	{
	//	fprintf(stderr, "\n LOCALtoMIDI returns:  %i ",r * columns() + c);
        return r * columns() + c;
		
		}
    else	
        return 0;
}

void 
MonomeXXhDevice::convertMIDINoteNumberToLocalCoordinates(unsigned char MIDINoteNumber, unsigned int &column, unsigned int &row)
{
    unsigned int c, r;
//fprintf (stderr, "\n incoming convert %i", MIDINoteNumber);
//fprintf (stderr, "\n cols %i, rows %i       ", columns(), rows());   	 
	   if (_type >= kDeviceType_40h) 
		{
			c = MIDINoteNumber % columns();
			r = MIDINoteNumber / columns();
		}
    
    else {
        c = r = 0;
    }
	//fprintf(stderr, " .....  c=%i, r=%i", c, r);
    switch (DeviceOrientation()) 
	{
		case kCableOrientation_Left:
				column = c;
				row = r;
			break;
		case kCableOrientation_Top:
				column = r;
				row = columns() - c - 1;
			break;
		case kCableOrientation_Right:
				row = rows() - r - 1; 
				column = columns() - c - 1;
			break;
		case kCableOrientation_Bottom:
				column = rows() - r - 1;
				row = c;
			break;
    }


}

void 
MonomeXXhDevice::oscLedStateChangeEvent(unsigned int column, unsigned int row, bool state) //m256
{
	convertOscCoordinatesToLocalCoordinates(column, row);
	
	if (column >= _columns || row >= _rows)
		return;

	if (_type == kDeviceType_40h) {
		t_message message;
		messagePackLedStateChange(&message, state ? 1 : 0, column, row);	
		write((char *)&message, sizeof(t_message));
		}
	else  if (_type <= kDeviceType_64) {//256 128 64
		t_message message;
		if (state == 1) messagePack_256_led_on(&message, column, row);
		else messagePack_256_led_off(&message, column, row);
		write((char *)&message, sizeof(t_message));
	}
}

void 
MonomeXXhDevice::oscLedIntensityChangeEvent(float intensity) //m256
{
    uint8 i;
    
    if (intensity > 1.f)
        intensity = 1.f;
    else if (intensity < 0.f)
        intensity = 0.f;

    i = ((unsigned char)(intensity * (float)0xF)) & 0xF;
	
	if (_type == kDeviceType_40h) { 
		t_message message;
		messagePackLedIntensity(&message, i);
		write((char *)&message, sizeof(t_message));
	}

	else if (_type <= kDeviceType_64) {//256 128 64  
		t_256_1byte_message message;
		messagePack_256_intensity(&message, i);
		write((char *)&message, sizeof(t_256_1byte_message));
	 }
}

void 
MonomeXXhDevice::oscLedTestStateChangeEvent(bool testState)  //m256 ?
{
	if (_type == kDeviceType_40h) {
		t_message message;
		messagePackLedTest(&message, testState ? 1 : 0);
	    write((char *)&message, sizeof(t_message));
	}
	else {
		t_256_1byte_message message;
		messagePack_256_mode(&message, testState ? 1 : 0);
		write((char *)&message, sizeof(t_256_1byte_message));
	}
	
	// hmm, need mode 0 for m256?
	//		mode = 0 : normal
	//		mode = 1 : test (all leds on)
	//		mode = 2 : shutdown (all leds off)
	
	/*
	we should add a new OSC message, /led_mode perhaps, and "mask" the old messages:

/shutdown 1 = /led_mode off
/test 1 = /led_mode on
/shutdown 0 = /test 0 = /led_mode normal
*/
}
	
void MonomeXXhDevice::oscLed_ModeStateChangeEvent(int State) //added for m256, combines shutdown 1 (0), Test 1 (1), and shutdown0/test0 (2) 
{

	if (State == 0) {
		oscShutdownStateChangeEvent(1);
	}
	else if (State == 1) {
		oscLedTestStateChangeEvent(1);
	}
	else if (State == 2) {
		oscShutdownStateChangeEvent(0);
	}
}

void 
MonomeXXhDevice::oscLedClearEvent(bool clear)  //m256
{
	if (_type == kDeviceType_40h) {
		t_message message;
		for (unsigned int i = 0; i < _rows; i++) {
			messagePackLedRow(&message, i, clear ? 0xFF : 0x00);
			write((char *)&message, sizeof(t_message));
		}
	}
	else {  //m256 (and new devices?) has clear message   
		t_256_1byte_message message;
		messagePack_256_clear(&message, clear ? 1 : 0);
		write((char *)&message, sizeof(t_256_1byte_message));
	}
}

void 
MonomeXXhDevice::oscShutdownStateChangeEvent(bool shutdownState) //m256
{
   
	if (_type == kDeviceType_40h){ 
		t_message message;
		messagePackShutdown(&message, shutdownState ? 0 : 1);
	    write((char *)&message, sizeof(t_message));
	}
	else { 
		t_256_1byte_message message;
		messagePack_256_mode(&message, shutdownState ? 2 : 0);
		write((char *)&message, sizeof(t_256_1byte_message));
	}
}

void 
MonomeXXhDevice::oscLedRowStateChangeEvent(unsigned int row, unsigned int numBitMaps, unsigned char bitMaps[])
{
	// added by dan - check that column offset is not greater than the highest value in the bitmap
    if (row < _oscStartRow || row >= _oscStartRow + rows() || _oscStartColumn >= numBitMaps * 8)
        return;

	// use columns() and rows() instead of _columns and _rows to get correct values in case _type == kDeviceType_128
	unsigned int r = row - _oscStartRow; 
	unsigned int index = _oscStartColumn / 8; 
	unsigned int shift = _oscStartColumn % 8; 
	unsigned char bitMap = bitMaps[index] >> shift; 

	if (index + 1 < numBitMaps)
		bitMap |= bitMaps[index + 1] << (8 - shift);

	if (_type == kDeviceType_40h) {  // maybe include 64 here too, and select the appropriate messagePackLedRow macro?
		t_message message;

		switch (DeviceOrientation()) 
		{
			case kCableOrientation_Left:
				messagePackLedRow(&message, r, bitMap);
				break;
		
			case kCableOrientation_Top:
				messagePackLedColumn(&message, r, myswap[bitMap]);
				break;

			case kCableOrientation_Right:
				messagePackLedRow(&message, rows() - r - 1, myswap[bitMap]);
				break;

			case kCableOrientation_Bottom:
				messagePackLedColumn(&message, columns() - r - 1, bitMap);
				break;
		}    

		write((char *)&message, sizeof(t_message));    
	}//40h
	else if (_type <= kDeviceType_64) // 256, 128 and 64
	{
		// 1-byte row command:
		//	- 64 -> always use this, since there is only 1 byte-row
		//  - 256 -> if there is only 1 bitMap, or offsets allow use of last bitMap only
		//  - 126 -> cable is up or down, or if left or right then offsets allow use of last bitMap only
		if (columns() <= 8 || index + 1 == numBitMaps)
		{
			t_message message;

			switch (DeviceOrientation()) 
			{
				case kCableOrientation_Left:		
					messagePack_256_led_row1(&message, r, bitMap);
					break;

				case kCableOrientation_Top:
					messagePack_256_led_col1(&message, r, myswap[bitMap]);
					break;

				case kCableOrientation_Right:
					messagePack_256_led_row1(&message, rows() - r - 1, myswap[bitMap]);
					break;

				case kCableOrientation_Bottom:
					messagePack_256_led_col1(&message, columns() - r - 1, bitMap);
					break;
			}
			write((char *)&message, sizeof(t_message));  
		}
		// 2-byte row command: numBitMaps > 1
		//  - 256 -> column offset not in last bitMap
		//  - 128 -> cable is left or right, AND column offset not in last bitMap
		else {
			t_256_3byte_message message3;
			unsigned char bitMap2 = bitMaps[index + 1] >> shift; 

			if (index + 2 < numBitMaps)
				bitMap2 |= bitMaps[index + 2] << (8 - shift);

			switch (DeviceOrientation())
			{
				case kCableOrientation_Left:	
					messagePack_256_led_row2(&message3, r, bitMap, bitMap2);								  
					break;
				case kCableOrientation_Top:		
					messagePack_256_led_col2(&message3, r, myswap[bitMap2], myswap[bitMap]);				  
					break;
				case kCableOrientation_Right:	
					messagePack_256_led_row2(&message3, rows() - r - 1, myswap[bitMap2], myswap[bitMap]); 
					break;
				case kCableOrientation_Bottom:	
					messagePack_256_led_col2(&message3, columns() - r - 1, bitMap, bitMap2);
					break;
			}
			write((char *)&message3, sizeof(t_256_3byte_message));  
		}
	} // 256/128/64
}

void 
MonomeXXhDevice::oscLedColumnStateChangeEvent(unsigned int column, unsigned int numBitMaps, unsigned char bitMaps[])
{
	// added by dan - check that column offset is not greater than the highest value in the bitmap
    if (column < _oscStartColumn || column >= _oscStartColumn + columns() || _oscStartRow >= numBitMaps * 8)
        return;

	// use columns() and rows() instead of _columns and _rows to get correct values in case _type == kDeviceType_128
	unsigned int c = column - _oscStartColumn; 
	unsigned int index = _oscStartRow / 8; 
	unsigned int shift = _oscStartRow % 8; 
	unsigned char bitMap = bitMaps[index] >> shift; 

	if (index + 1 < numBitMaps)
		bitMap |= bitMaps[index + 1] << (8 - shift);

	if (_type == kDeviceType_40h) {  // maybe include 64 here too, and select the appropriate messagePackLedRow macro?
		t_message message;

		switch (DeviceOrientation()) 
		{
			case kCableOrientation_Left:
				messagePackLedColumn(&message, c, bitMap);
				break;
		
			case kCableOrientation_Top:
				messagePackLedRow(&message, _rows - c - 1, bitMap);
				break;

			case kCableOrientation_Right:
				messagePackLedColumn(&message, _columns - c - 1, myswap[bitMap]);
				break;

			case kCableOrientation_Bottom:
				messagePackLedRow(&message, c, myswap[bitMap]);
				break;
		}    

		write((char *)&message, sizeof(t_message));    
	}//40h
	else if (_type <= kDeviceType_64) // 256, 128 and 64
	{
		// 1-byte col command:
		//	- 64 -> always use this, since there is only 1 byte-col
		//  - 256 -> if there is only 1 bitMap, or offsets allow use of last bitMap only
		//  - 126 -> cable is up or down, or if left or right then offsets allow use of last bitMap only
		if (rows() <= 8 || index + 1 == numBitMaps)
		{
			t_message message;

			switch (DeviceOrientation()) 
			{
				case kCableOrientation_Left:	
					messagePack_256_led_col1(&message, c, bitMap);		
					break;
				case kCableOrientation_Top:		
					messagePack_256_led_row1(&message, _rows - c - 1, bitMap);	
					break;
				case kCableOrientation_Right:   
					messagePack_256_led_col1(&message, _columns - c - 1, myswap[bitMap]);
					break;
				case kCableOrientation_Bottom:  
					messagePack_256_led_row1(&message, c, myswap[bitMap]);
					break;
			}
			write((char *)&message, sizeof(t_message));  

		}
		// 2-byte col command: numBitMaps > 1
		//  - 256 -> row offset not in last bitMap
		//  - 128 -> cable is left or right, AND row offset not in last bitMap
		else {
			t_256_3byte_message message3;
			unsigned char bitMap2 = bitMaps[index + 1] >> shift; 

			if (index + 2 < numBitMaps)
				bitMap2 |= bitMaps[index + 2] << (8 - shift);

			switch (DeviceOrientation())
			{
				case kCableOrientation_Left:    
					messagePack_256_led_col2(&message3, c, bitMap, bitMap2);									
					break;
				case kCableOrientation_Top:     
					messagePack_256_led_row2(&message3, _rows - c - 1, bitMap, bitMap2);						
					break;
				case kCableOrientation_Right:   
					messagePack_256_led_col2(&message3, _columns - c - 1,  myswap[bitMap2], myswap[bitMap]);	
					break;
				case kCableOrientation_Bottom:  
					messagePack_256_led_row2(&message3, c, myswap[bitMap2],  myswap[bitMap]);					
					break;
			}
			write((char *)&message3, sizeof(t_256_3byte_message));  
		}
	} // 256/128/64
}

void
MonomeXXhDevice::oscLedFrameEvent(unsigned int column, unsigned int row, unsigned char bitMaps[8])
{
	unsigned char map[8], rmap[8];
	unsigned int i, shift;
	unsigned int r, c;

	if (column >= _oscStartColumn + columns() || row >= _oscStartRow + _rows ||
		        column + 7 < _oscStartColumn || row + 7 < _oscStartRow)
        return;

	if (_type == kDeviceType_40h)
	{
		t_message message;

		if (column >= _oscStartColumn) {
			shift = column - _oscStartColumn;

			for (i = 0; i < 8; i++) {
				if (_oscStartRow + i < row)
					map[i] = 0;
				else if (_oscStartRow + i >= row + 8)
					map[i] = 0;
				else
					map[i] = bitMaps[_oscStartRow + i - row] << shift;
			}
		}
		else {
			shift = _oscStartColumn - column;

			for (i = 0; i < 8; i++) {
				if (_oscStartRow + i < row)
					map[i] = 0;
				else if (_oscStartRow + i >= row + 8)
					map[i] = 0;
				else
					map[i] = bitMaps[_oscStartRow + i - row] >> shift;
			}
		}

		switch (DeviceOrientation()) 
		{
			case kCableOrientation_Left:
				for (i = 0; i < 8; i++) {
					messagePackLedRow(&message, i, map[i]);
					write((char *)&message, sizeof(t_message));
				}
				break;

			case kCableOrientation_Top:
				memset(rmap, 0, 8);

				for (r = 0; r < 8; r++) {
					for (c = 0; c < 8; c++) {
						if (map[r] & (0x80 >> c))
							rmap[c] |= 0x80 >> (7 - r);
					}
				}

				for (i = 0; i < 8; i++) {
					messagePackLedRow(&message, i, rmap[i]);
					write((char *)&message, sizeof(t_message));
				}
				break;

			case kCableOrientation_Right:
				for (i = 0; i < 8; i++) {
					messagePackLedRow(&message, 7 - i, myswap[map[i]]);
					write((char *)&message, sizeof(t_message));
				}

				break;

			case kCableOrientation_Bottom:
				memset(rmap, 0, 8);

				for (r = 0; r < 8; r++) {
					for (c = 0; c < 8; c++) {
						if (map[r] & (0x80 >> c))
							rmap[7 - c] |= 0x80 >> r;
					}
				}

				for (i = 0; i < 8; i++) {
					messagePackLedRow(&message, i, rmap[i]);
					write((char *)&message, sizeof(t_message));
				}
				break;
		}
	}
	else if (_type <= kDeviceType_64)  //<- will this work for other devices than 256?
	{
		int quadrant = ((column - _oscStartColumn) / 8) + (((row - _oscStartRow) / 8) * 2);
		if (quadrant > 0) {
			if (_type == kDeviceType_64) {
				return;
			}
			else if (quadrant > 1 && _type == kDeviceType_128) {
				return;
			}
		}

		t_256_frame_message message;

		if ((row == _oscStartRow || row == _oscStartRow + 8) && (column == _oscStartColumn || column == _oscStartColumn + 8))
		{
			memcpy(map, bitMaps, 8);

			switch (_orientation) 
			{
				case kCableOrientation_Left:
					for (r = 0; r < 8; r++) {
						rmap[r] = map[r];
					}
					break;

				case kCableOrientation_Top:
					memset(rmap, 0, 8);
					for (r = 0; r < 8; r++) {
						for (c = 0; c < 8; c++) {
							if (map[r] & (0x80 >> c)) {
								rmap[c] |= 0x80 >> (7 - r);
								quadrant = iquadrant[quadrant+4];
							}
						}
					}
					break;

				case kCableOrientation_Right:
					for (r = 0; r < 8; r++) {
						rmap[r] = map[7-r];
						quadrant = iquadrant[quadrant+8];
					}
					break;

				case kCableOrientation_Bottom:
					memset(rmap, 0, 8);
					for (r = 0; r < 8; r++) {
						for (c = 0; c < 8; c++) {
							if (map[r] & (0x80 >> c)) {
								rmap[7 - c] |= 0x80 >> r;
								quadrant = iquadrant[quadrant+12];
							}
						}
					}
					break;
			} //endswitch

			messagePack_256_led_frame(&message, quadrant, rmap[0], rmap[1], rmap[2], 
									  rmap[3], rmap[4], rmap[5], rmap[6], rmap[7]);

			write((char *)&message, sizeof(t_256_frame_message));
		}
		else
		{
			for (int i = 0; i < 8; i++) {
				unsigned int localRow = row + i;
				if (localRow >= _oscStartRow && localRow < _oscStartRow + 16) { // don't call if not in this row
					unsigned int size = (column / 8) + 1;
					unsigned char* rowBitMap;

					if (column < _oscStartColumn) {
						rowBitMap = new unsigned char[size];
						memset(rowBitMap, 0, size);

						shift = (_oscStartColumn - column) % 8;
						rowBitMap[size - 1] = bitMaps[i] >> shift;
						oscLedRowStateChangeEvent(localRow, size, rowBitMap);
						delete[] rowBitMap;
					}
					else {
						size++;
						rowBitMap = new unsigned char[size];
						memset(rowBitMap, 0, size);

						shift = (column - _oscStartColumn) % 8;

						if (column < _oscStartColumn + 8) {
							rowBitMap[size - 2] = bitMaps[i] << shift;
							rowBitMap[size - 1] = bitMaps[i] >> (8 - shift);
						}
						else {
							rowBitMap[size - 2] = bitMaps[i] << shift;
						}
						
						oscLedRowStateChangeEvent(localRow, size, rowBitMap);
						delete[] rowBitMap;
					}

				}
			}
		}
	}
}

void 
MonomeXXhDevice::oscAdcEnableStateChangeEvent(unsigned int adcIndex, bool adcEnableState)
{

		if (adcIndex >= 4)
			return;
		_adcState[adcIndex] = adcEnableState;
			
		if (adcEnableState) {
						for (unsigned int i = 0; i < 2; i++)
								{
								_encState[i] = false;
								oscEncEnableStateChangeEvent(i, false);
								}
							}
		
	if (_type == kDeviceType_40h)
		{
		 t_message message;
		messagePackAdcEnable(&message, adcIndex, adcEnableState ? 1 : 0);
		write((char *)&message, sizeof(t_message));
		}   
	else if (_type <= kDeviceType_64) //256 128 64
		{
		t_256_1byte_message  message;
		if (adcEnableState) messagePack_256_activatePort(&message, adcIndex);
		else				messagePack_256_deactivatePort(&message, adcIndex);
		
		write((char *)&message, sizeof(t_256_1byte_message));
		}
		
}   

void 
MonomeXXhDevice::oscTiltEnableStateChangeEvent(bool tiltEnableState)
{


	if (_type == kDeviceType_40h) return;

	else if (_type <= kDeviceType_64) {//256 128 64
		t_256_1byte_message  message;
		if (tiltEnableState) messagePack_256_activatePort(&message, 1);
		else				messagePack_256_activatePort(&message, 0); //was *de*activate, but its msg 12 either way
		
		write((char *)&message, sizeof(t_256_1byte_message));
		//fprintf(stderr, "sent message to enable tiltmode %i", int(tiltEnableState)); //bobo
	}
		
}    

void 
MonomeXXhDevice::oscEncEnableStateChangeEvent(unsigned int encIndex, bool encEnableState)
{
    t_message message;
if (_type != kDeviceType_40h) return;
    if (encIndex >= 2)
        return;

    _encState[encIndex] = encEnableState;
	
	if (encEnableState) {
		for (unsigned int i = 0; i < 4; i++) {
			_adcState[i] = false;
			oscAdcEnableStateChangeEvent(i, false);
		}
	}

    messagePackEncEnable(&message, encIndex, encEnableState ? 1 : 0);
    write((char *)&message, sizeof(t_message));
}    


void 
MonomeXXhDevice::MIDILedStateChangeEvent(unsigned char MIDINoteNumber, unsigned char MIDIVelocity)
{
    unsigned int column, row;
    t_message message;
 
    column = row = 0;
 
    convertMIDINoteNumberToLocalCoordinates(MIDINoteNumber, column, row);
 
	if (_type == kDeviceType_40h) {
		messagePackLedStateChange(&message, MIDIVelocity ? 1 : 0, column, row);
	}
	// fixed by steve -> new devices have a seperate led on and off serial command, 40h has 1 command with on/off as 
	else  if (_type <= kDeviceType_64) { //256 128 64
		if (MIDIVelocity > 0) messagePack_256_led_on(&message, column, row);
		else messagePack_256_led_off(&message, column, row);
	}
 
    write((char *)&message, sizeof(t_message));
}
