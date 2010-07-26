#ifndef __SERIALDEBUGGER_H__
#define __SERIALDEBUGGER_H__

#include <string>
#include <iostream>
#include "message.h"
#include "message256.h"
#include "messageMK.h"
#include "MonomeXXhDevice.h"

using namespace std;

class SerialDebugger {
public:
	SerialDebugger(void) {}
	~SerialDebugger(void) {}

	void printString(const string& string) {
		cout << string;
	}

	void printIncomingSerialMessage(MonomeXXhDevice* device, t_message* message) {
		if (device == 0) return;
		
		if (device->type() == MonomeXXhDevice::kDeviceType_40h) {
			printIncomingSerialMessage40h(device, message);
		}
		else {
			printIncomingSerialMessage256(device, message);
		}
	}

private:
	void printIncomingSerialMessage40h(MonomeXXhDevice* device, t_message* message) {
		cout << device->serialNumber() 
			<< " : ";

		switch(messageGetType(*message)) {
			case(kMessageTypeButtonPress) : 
				cout << " Press \r\n"
					<< "\t-> x = " << messageGetButtonX(*message)
					<< "\t-> y = " << messageGetButtonY(*message)
					<< "\t-> v = " << messageGetButtonState(*message);
				break;
			case(kMessageTypeAdcVal) : 
				cout << " ADC \r\n"
					<< "\t-> port = " << messageGetAdcPort(*message)
					<< "\t-> val = " << ((float)(messageGetAdcVal(*message)) / (float)0x3FF);
				break;
			case(kMessageTypeEncVal) : 
				cout << " Encoder \r\n"
					<< "\t-> port = " << messageGetEncPort(*message)
					<< "\t->  val = " << messageGetEncVal(*message);
				break;
		}
		cout << endl;
	}

	void printIncomingSerialMessage256(MonomeXXhDevice* device, t_message* message) {
		cout << device->serialNumber() 
			<< " : ";

		switch(messageGetType(*message)) {
			case(kMessageType_256_keydown) : 
				cout << " Press \r\n"
					<< "\t-> x = " << messageGetButtonX(*message)
					<< "\t-> y = " << messageGetButtonY(*message)
					<< "\t-> v = 1";
				break;
			case(kMessageType_256_keyup) : 
				cout << " Press \r\n"
					<< "\t-> x = " << messageGetButtonX(*message)
					<< "\t-> y = " << messageGetButtonY(*message)
					<< "\t-> v = 0";
				break;
			case(kMessageType_256_auxiliaryInput) : 
				cout << " Aux \r\n"
					<< "\t-> port = " << messageGetEncPort(*message)
					<< "\t-> val = " << messageGetEncVal(*message);
				break;
			case(kMessageTypeTiltEvent) : 
				cout << " Tilt \r\n"
					<< "\t-> axis = " << messageGetTiltAxis(*message)
					<< "\t->  val = " << (int)messageGetEncVal(*message);
				break;
		}
		cout << endl;
	}
};

#endif //__SERIALDEBUGGER_H__