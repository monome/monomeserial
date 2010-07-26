#include "../stdafx.h"
#include "CoreMIDITypes.h"

#ifdef DEBUG_PRINT
#include <iostream>
#endif

CCoreMIDISend::CCoreMIDISend(unsigned int deviceID)
{
	outDevice = new CMIDIOutDevice(deviceID);
}

CCoreMIDISend::CCoreMIDISend(const string &deviceName)
{
	outDevice = new CMIDIOutDevice(CCoreMIDISend::getMidiOutputDeviceByName(deviceName));
}

CCoreMIDISend::~CCoreMIDISend()
{
	outDevice->Close();
	delete outDevice;
}

string 
CCoreMIDISend::getDeviceName() const
{
	return CCoreMIDISend::getOutputDeviceName(CCoreMIDISend::getDeviceID());
}

unsigned int 
CCoreMIDISend::getDeviceID() const
{
	return outDevice->GetDevID();
}

MIDIOUTCAPS 
CCoreMIDISend::getDeviceCaps() const
{
	MIDIOUTCAPS caps;
	CMIDIOutDevice::GetDevCaps(outDevice->GetDevID(), caps);
	return caps;
}

void 
CCoreMIDISend::sendMessage(char midiStatusByte, char midiDataByte1, char midiDataByte2)
{
	CShortMsg msg(midiStatusByte, midiDataByte1, midiDataByte2, 0);
	msg.SendMsg(*outDevice);
}

//---------------------------------------------

CCoreMIDIReceive::CCoreMIDIReceive(unsigned int deviceID, CCoreMIDIReceiveCallback callback, void *userData)
{
	receiver = new CMIDIReceive(callback, this, userData);
	inDevice = new CMIDIInDevice(deviceID, *receiver);
	inDevice->StartRecording();
}

CCoreMIDIReceive::CCoreMIDIReceive(const string &deviceName, CCoreMIDIReceiveCallback callback, void *userData)
{
	receiver = new CMIDIReceive(callback, this, userData);
	inDevice = new CMIDIInDevice(CCoreMIDIReceive::getMidiInputDeviceByName(deviceName), *receiver);
	inDevice->StartRecording();
}

CCoreMIDIReceive::~CCoreMIDIReceive()
{
	inDevice->Close();
	delete receiver;
	delete inDevice;
}

string 
CCoreMIDIReceive::getDeviceName() const
{
	return CCoreMIDIReceive::getInputDeviceName(this->getDeviceID());
}

unsigned int 
CCoreMIDIReceive::getDeviceID() const
{
	return inDevice->GetDevID();
}

MIDIINCAPS 
CCoreMIDIReceive::getDeviceCaps() const
{
	MIDIINCAPS caps;
	CMIDIInDevice::GetDevCaps(this->getDeviceID(), caps);
	return caps;
}