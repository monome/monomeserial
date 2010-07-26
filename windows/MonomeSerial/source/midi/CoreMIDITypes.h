#ifndef __CCOREMIDITYPES_H__
#define __CCOREMIDITYPES_H__

#include "MIDIInDevice.h"
#include "midi.h"
#include "MIDIOutDevice.h"
#include "ShortMsg.h"

#include <string>
#include <sstream>
#include <list>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

using namespace std;
using namespace midi;

typedef void *CCoreMIDIEndpointRef;
typedef void (*CCoreMIDIReceiveCallback)(DWORD msg, CCoreMIDIEndpointRef source, void* userData);


class CCoreMIDIEndpoint
{
public:
	CCoreMIDIEndpoint() {}
	virtual ~CCoreMIDIEndpoint() {}
	virtual string getDeviceName() const = 0;
	virtual unsigned int getDeviceID() const = 0;
};


class CMIDIReceive : public CMIDIReceiver
{
public:
	CMIDIReceive(CCoreMIDIReceiveCallback callbackMethod, CCoreMIDIEndpointRef coreMIDIReceive, void* userData)
	{
		owner = userData;
		callback = callbackMethod;
		midiReceive = coreMIDIReceive;
	}
	~CMIDIReceive(void) {}

private:
    // Receives short messages
	void ReceiveMsg(DWORD Msg, DWORD TimeStamp) {
		callback(Msg, midiReceive, owner);
	}

    // Called when an invalid short message is received
	void OnError(DWORD Msg, DWORD TimeStamp) {
		// ----
	}

	void ReceiveMsg(LPSTR Msg, DWORD bytesRecorded, DWORD TimeStamp) {}
	void OnError(LPSTR Msg, DWORD BytesRecorded, DWORD TimeStamp) {}

private:
	CCoreMIDIReceiveCallback callback;
	CCoreMIDIEndpointRef midiReceive;
	void* owner;
};


class CCoreMIDISend : public CCoreMIDIEndpoint
{
public:
	CCoreMIDISend(unsigned int deviceID);
	CCoreMIDISend(const string &deviceName);
	virtual ~CCoreMIDISend();

public:
	virtual string getDeviceName() const;
	virtual unsigned int getDeviceID() const;
	MIDIOUTCAPS getDeviceCaps() const;
	void sendMessage(char midiStatusByte, char midiDataByte1, char midiDataByte2);

#pragma region Static Output Interface
public:
	static int getNumberOfOutputDevices(void) { return CMIDIOutDevice::GetNumDevs(); }

	static string getOutputDeviceName(int outputDevice)	
	{
		try {
			if (outputDevice < CCoreMIDISend::getNumberOfOutputDevices()) {
				MIDIOUTCAPS caps;
				CMIDIOutDevice::GetDevCaps(outputDevice, caps);

				string temp;
				int i = 0;
				while (caps.szPname[i] != 0 && i < (sizeof(caps.szPname)/sizeof(WCHAR)) ) {
					temp += (char)caps.szPname[i++];
				}

				return temp;
			}
		}
		catch (CMIDIOutException &ex) {
#ifdef DEBUG_PRINT
			cout << "Error in CoreMIDITypes.h->getOutputDeviceName : CMIDIOutException = " << ex.what() << endl;
#endif
		}

		return string();
	}

	static int getMidiOutputDeviceByName(const string &device) 
	{
		int len = CCoreMIDISend::getNumberOfOutputDevices();

		for (int i = 0; i < len; i++) {
		if (CCoreMIDISend::getOutputDeviceName(i) == device)
			return i;
		}

		return -1;
	}

	static const list<string> getMidiOutputDeviceSources() 
	{
		list<string> names;
		
		int len = CCoreMIDISend::getNumberOfOutputDevices();
		for (int i = 0; i < len; i++) {
			string name = CCoreMIDISend::getOutputDeviceName(i);
			if (name != "") {
				names.push_back(name);
			}
		}
		return names;
	}
#pragma endregion

private:
	CMIDIOutDevice *outDevice;
};



class CCoreMIDIReceive : public CCoreMIDIEndpoint
{
public:
	CCoreMIDIReceive(unsigned int deviceID, CCoreMIDIReceiveCallback callback, void *userData);
	CCoreMIDIReceive(const string &deviceName, CCoreMIDIReceiveCallback callback, void *userData);
	virtual ~CCoreMIDIReceive();

public:
	virtual string getDeviceName() const;
	virtual unsigned int getDeviceID() const;
	MIDIINCAPS getDeviceCaps() const;


#pragma region Static Input Interface
public:
	static int getNumberOfInputDevices(void) { return CMIDIInDevice::GetNumDevs(); }

    static string getInputDeviceName(int inputDevice) 
	{
		try {
			if (inputDevice < CCoreMIDIReceive::getNumberOfInputDevices()) {
				MIDIINCAPS caps;
				CMIDIInDevice::GetDevCaps(inputDevice, caps);

				string temp;
				int i = 0;
				while (caps.szPname[i] != 0 && i < (sizeof(caps.szPname)/sizeof(WCHAR)) ) {
					temp += (char)caps.szPname[i++];
				}

				return temp;
			}
		}
		catch (CMIDIInException &ex) {
#ifdef DEBUG_PRINT
			cout << "Error in CoreMIDITypes.h->getInputDeviceName : CMIDIInException = " << ex.what() << endl;
#endif
		}

		return string();
	}

	static int getMidiInputDeviceByName(const string &device) 
	{
		int len = CCoreMIDIReceive::getNumberOfInputDevices();
		for (int i = 0; i < len; i++) {
			if (CCoreMIDIReceive::getInputDeviceName(i) == device) {
				return i;
			}
		}

		return -1;
	}

	static const list<string> getMidiInputDeviceSources() 
	{
		list<string> names;

		int len = CCoreMIDIReceive::getNumberOfInputDevices();
		for (int i = 0; i < len; i++) {
			string name = CCoreMIDIReceive::getInputDeviceName(i);
			if (name != "") {
				names.push_back(name);
			}
		}
		return names;
	}
#pragma endregion

private:
	CMIDIInDevice *inDevice;
	CMIDIReceive *receiver;
};


#endif