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
#include "ApplicationController.h"
#include <sstream>
using namespace std;

@implementation AppController

- (id)init
{
	if (self = [super init]) {
		try {
			_appController = new ApplicationController(self);
		}
		catch (const char *error) {
			NSString *aString = [NSString stringWithCString:error encoding:NSASCIIStringEncoding];
			NSRunAlertPanel(@"MonomeSerial Error", aString, @"Ok", NULL, NULL); // change this to fatal error panel or whatever.
			exit(0);
		}
		
		_appController->registerForSerialDeviceNotifications();
	}
		
	return self;
}

- (void)awakeFromNib
{
	[ioProtocolPopUpButton selectItemAtIndex:(int)_appController->protocol()];
	[self ioProtocolSelected:self];
	
	NSString *text = [NSString stringWithCString:_appController->oscHostAddressString().c_str() encoding:NSASCIIStringEncoding];
	//[NSAutoreleasePool addObject:text];
	
	[oscHostAddressTextField setStringValue:text];
	[oscHostPortTextField setIntValue:_appController->oscHostPort()];	
	[oscListenPortTextField setIntValue:_appController->oscListenPort()];

	[self updateMIDIDevices];
	[self updateDeviceList];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	_appController->writePreferences();
}

- (IBAction)cableOrientationSelected:(id)sender
{
	_appController->cableOrientationPopUpMenuChanged([devicePopUpButton indexOfSelectedItem], [cableOrientationPopUpButton indexOfSelectedItem]);
}

- (IBAction)deviceSelected:(id)sender
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);
	
	if (device == 0) {
		[cableOrientationPopUpButton selectItemAtIndex:0];
		[midiInputDevicePopUpButton selectItemAtIndex:0];
		[midiOutputDevicePopUpButton selectItemAtIndex:0];
		[oscAddressPatternPrefixTextField setStringValue:@"/40h"];
		[oscStartRowTextField setIntValue:0];
		[oscStartColumnTextField setIntValue:0];
		[oscAdcOffsetTextField setIntValue:0];
		[oscEncOffsetTextField setIntValue:0];
		[midiInputDevicePopUpButton selectItemAtIndex:0];
		[midiOutputDevicePopUpButton selectItemAtIndex:0];
		[midiInputChannelPopUpButton selectItemAtIndex:0];
		[midiOutputChannelPopUpButton selectItemAtIndex:0];
		
		[self updateTestState];
		
		[self updateAdcStates];
		[self updateAdcLabels];
		[self updateEncStates];
		[self updateEncLabels];
		
		return;
	}
		

	
	if  (device->type() ==  MonomeXXhDevice::kDeviceType_128)
	[cableOrientationPopUpButton selectItemAtIndex:(int)((device->cableOrientation()+1)%4)]; //ugly hack, this is to get TOP to default, I'm sorry its convoluted
		else [cableOrientationPopUpButton selectItemAtIndex:(int)device->cableOrientation()];
		
	NSString *prefix_text = [NSString stringWithCString:device->oscAddressPatternPrefix().c_str() encoding:NSASCIIStringEncoding];
	//[NSAutoreleasePool addObject:text];
	
	[midiInputDevicePopUpButton selectItemAtIndex:_appController->indexOfMIDIInputDeviceForMonomeXXhDeviceIndex([devicePopUpButton indexOfSelectedItem])];
	[midiOutputDevicePopUpButton selectItemAtIndex:_appController->indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex([devicePopUpButton indexOfSelectedItem])];
	[midiInputChannelPopUpButton selectItemAtIndex:device->MIDIInputChannel()];
	[midiOutputChannelPopUpButton selectItemAtIndex:device->MIDIOutputChannel()];
	
	[oscAddressPatternPrefixTextField setStringValue:prefix_text];
	[oscStartRowTextField setIntValue:device->oscStartRow()];
	[oscStartColumnTextField setIntValue:device->oscStartColumn()];
	[oscAdcOffsetTextField setIntValue:device->oscAdcOffset()];
	[oscEncOffsetTextField setIntValue:device->oscEncOffset()];
	
	[self updateTestState];
	
	[self updateAdcStates];
	[self updateAdcLabels];
	[self updateEncStates];
	[self updateEncLabels];
}

- (IBAction)ioProtocolSelected:(id)sender
{
	unsigned int index = [ioProtocolPopUpButton indexOfSelectedItem];
	
	_appController->protocolPopUpMenuChanged(index);
	[deviceSpecificProtocolTabView selectTabViewItemAtIndex:index];
	
	if (index == ApplicationController::kProtocolType_OpenSoundControl) {
		[oscHostAddressTextField setEnabled:TRUE];
		[oscHostPortTextField setEnabled:TRUE];
		[oscListenPortTextField setEnabled:TRUE];	
	}
	else {
		[oscHostAddressTextField setEnabled:FALSE];
		[oscHostPortTextField setEnabled:FALSE];
		[oscListenPortTextField setEnabled:FALSE];	
	}		
}

- (IBAction)midiInputChannelSelected:(id)sender
{
	_appController->midiInputChannelChanged([devicePopUpButton indexOfSelectedItem], [midiInputChannelPopUpButton indexOfSelectedItem]);
}

- (IBAction)midiInputDeviceSelected:(id)sender
{
	_appController->midiInputDeviceChanged([devicePopUpButton indexOfSelectedItem], [midiInputDevicePopUpButton indexOfSelectedItem]);
}

- (IBAction)midiOutputChannelSelected:(id)sender
{
	_appController->midiOutputChannelChanged([devicePopUpButton indexOfSelectedItem], [midiOutputChannelPopUpButton indexOfSelectedItem]);
}

- (IBAction)midiOutputDeviceSelected:(id)sender
{
	_appController->midiOutputDeviceChanged([devicePopUpButton indexOfSelectedItem], [midiOutputDevicePopUpButton indexOfSelectedItem]);
}

- (IBAction)adc0SwitchButtonClicked:(id)sender
{
	bool state = [adc0SwitchButton state] == NSOnState ? true : false;
	
	_appController->adcStateButtonChanged([devicePopUpButton indexOfSelectedItem], 0, state);
	
	if (state)
		[self updateEncStates];
}

- (IBAction)adc1SwitchButtonClicked:(id)sender
{
	bool state = [adc1SwitchButton state] == NSOnState ? true : false;
	
	_appController->adcStateButtonChanged([devicePopUpButton indexOfSelectedItem], 1, state);
	
	if (state)
		[self updateEncStates];
}

- (IBAction)adc2SwitchButtonClicked:(id)sender
{
	bool state = [adc2SwitchButton state] == NSOnState ? true : false;
	
	_appController->adcStateButtonChanged([devicePopUpButton indexOfSelectedItem], 2, state);
	
	if (state)
		[self updateEncStates];
}

- (IBAction)adc3SwitchButtonClicked:(id)sender
{
	bool state = [adc3SwitchButton state] == NSOnState ? true : false;
	
	_appController->adcStateButtonChanged([devicePopUpButton indexOfSelectedItem], 3, state);
	
	if (state)
		[self updateEncStates];
}

- (IBAction)enc0SwitchButtonClicked:(id)sender
{
	bool state = [enc0SwitchButton state] == NSOnState ? true : false;
	
	_appController->encStateButtonChanged([devicePopUpButton indexOfSelectedItem], 0, state);
	
	if (state)
		[self updateAdcStates];
}

- (IBAction)enc1SwitchButtonClicked:(id)sender
{	
	bool state = [enc1SwitchButton state] == NSOnState ? true : false;
	
	_appController->encStateButtonChanged([devicePopUpButton indexOfSelectedItem], 1, state);
	
	if (state)
		[self updateAdcStates];
}

- (IBAction)clearLedsButtonClicked:(id)sender
{	
	_appController->clearLedsButtonChanged([devicePopUpButton indexOfSelectedItem]);
}

- (IBAction)testModeButtonClicked:(id)sender
{	
	bool state = [testModeButton state] == NSOnState ? true : false;
	
	_appController->testModeButtonChanged([devicePopUpButton indexOfSelectedItem], state);
	
	if (state)
		[self updateAdcStates];
}

- (void)controlTextDidBeginEditing:(NSNotification *)aNotification
{
	;
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
	id anObject = [aNotification object];
	
	if (anObject == oscAddressPatternPrefixTextField) {
		;
	}
	
	else if (anObject == oscHostAddressTextField) {
		;
	}
	
    else if (anObject == oscHostPortTextField) {
		int anIntValue;
		NSString *value = [oscHostPortTextField stringValue];
		
		if (![self _convertNSString:value toInt:&anIntValue])
			[oscHostPortTextField setIntValue:anIntValue];
	}
	
	else if (anObject == oscListenPortTextField) {
		int anIntValue;
		NSString *value = [oscListenPortTextField stringValue];
		
		if (![self _convertNSString:value toInt:&anIntValue])
			[oscListenPortTextField setIntValue:anIntValue];
	}
	
    else if (anObject == oscStartColumnTextField) {
		int anIntValue;
		NSString *value = [oscStartColumnTextField stringValue];
		
		if (![self _convertNSString:value toInt:&anIntValue])
			[oscStartColumnTextField setIntValue:anIntValue];
	}
	
    else if (anObject == oscStartRowTextField) {
		int anIntValue;
		NSString *value = [oscStartRowTextField stringValue];
		
		if (![self _convertNSString:value toInt:&anIntValue])
			[oscStartRowTextField setIntValue:anIntValue];
	}

#if DEBUG_PRINT
	NSLog(@"[AppController controlTextDidChange]");
#endif
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
	id anObject = [aNotification object];
	
	if (anObject == oscAddressPatternPrefixTextField) {
		_appController->oscAddressPatternPrefixTextFieldChanged([devicePopUpButton indexOfSelectedItem], [[oscAddressPatternPrefixTextField stringValue] cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	else if (anObject == oscHostAddressTextField) {
		try {
			//string aStdString([[oscHostAddressTextField stringValue] cStringUsingEncoding:NSASCIIStringEncoding]);
			_appController->oscHostAddressTextFieldChanged([[oscHostAddressTextField stringValue] cStringUsingEncoding:NSASCIIStringEncoding]);
		}
		catch (...) {
			NSRunAlertPanel(@"MonomeSerial Warning!", 
							@"The host address you entered is not valid.  Addresses must be entered in dotted-decimal notation.  Please enter a valid IP address.",
							@"Okay", NULL, NULL); // NSRunAlertPanel goes here
		}
	}
	
    else if (anObject == oscHostPortTextField) {
		try {
			_appController->oscHostPortTextFieldChanged([oscHostPortTextField intValue]);
		}
		catch (...) {
			NSRunAlertPanel(@"MonomeSerial Warning!",
							@"The port number entered is not valid.  Please enter a valid port number.",
							@"Okay", NULL, NULL);
		}
	}
	
	else if (anObject == oscListenPortTextField) {
		try {
			_appController->oscListenPortTextFieldChanged([oscListenPortTextField intValue]);
		}
		catch (...) {
			NSRunAlertPanel(@"MonomeSerial Warning!",
							@"Cannot listen on specified port.  The port may already be in use, or you may not have permission to listen on that port.",
							@"Okay", NULL, NULL);
		}	
	}
	
    else if (anObject == oscStartColumnTextField) {
		_appController->startingColumnTextFieldChanged([devicePopUpButton indexOfSelectedItem], [oscStartColumnTextField intValue]);
	}
	
    else if (anObject == oscStartRowTextField) {
		_appController->startingRowTextFieldChanged([devicePopUpButton indexOfSelectedItem], [oscStartRowTextField intValue]);
	}
	else if (anObject == oscAdcOffsetTextField) {
		_appController->oscAdcOffsetTextFieldChanged([devicePopUpButton indexOfSelectedItem], [oscAdcOffsetTextField intValue]);	
		[self updateAdcLabels];
	}
	else if (anObject == oscEncOffsetTextField) {
		_appController->oscEncOffsetTextFieldChanged([devicePopUpButton indexOfSelectedItem], [oscEncOffsetTextField intValue]);
		[self updateEncLabels];
	}
	//else if (anObject == 
	
#if DEBUG_PRINT
	NSLog(@"[AppController controlTextDidEndEditing]");
#endif
}

- (void)updateDeviceList
{
	unsigned int index = [devicePopUpButton indexOfSelectedItem];
	
	[devicePopUpButton removeAllItems];
	
	if (_appController->numberOfDevices() == 0) {
		[devicePopUpButton addItemWithTitle:@"No Devices Available"];

		[cableOrientationPopUpButton setEnabled:false];
		[midiInputChannelPopUpButton setEnabled:false];
		[midiInputDevicePopUpButton setEnabled:false];
		[midiOutputChannelPopUpButton setEnabled:false];
		[midiOutputDevicePopUpButton setEnabled:false];
		[oscAddressPatternPrefixTextField setEnabled:false];
		[oscStartColumnTextField setEnabled:false];
		[oscStartRowTextField setEnabled:false];
		[oscAdcOffsetTextField setEnabled:false];
		[oscEncOffsetTextField setEnabled:false];
		[adc0SwitchButton setEnabled:false];
		[adc1SwitchButton setEnabled:false];
		[adc2SwitchButton setEnabled:false];
		[adc3SwitchButton setEnabled:false];
		[enc0SwitchButton setEnabled:false];
		[enc1SwitchButton setEnabled:false];
		[testModeButton setEnabled:false];
	}
	else {
		unsigned int i;
		MonomeXXhDevice *device;
		
		for (i = 0; i < _appController->numberOfDevices(); i++) {
			device = _appController->deviceAtIndex(i);
			
			if (device != 0) {
				NSString *title = [NSString stringWithCString:device->bsdFilePath().c_str() encoding:NSASCIIStringEncoding];
				//[NSAutoreleasePool addObject:title];
				
				[devicePopUpButton addItemWithTitle:[title substringFromIndex:19]];
			}
		}
	
		if (index < [devicePopUpButton numberOfItems])
			[devicePopUpButton selectItemAtIndex:index];
			
		[cableOrientationPopUpButton setEnabled:true];
		[midiInputChannelPopUpButton setEnabled:true];
		[midiInputDevicePopUpButton setEnabled:true];
		[midiOutputChannelPopUpButton setEnabled:true];
		[midiOutputDevicePopUpButton setEnabled:true];
		[oscAddressPatternPrefixTextField setEnabled:true];
		[oscStartColumnTextField setEnabled:true];
		[oscStartRowTextField setEnabled:true];
		[oscAdcOffsetTextField setEnabled:true];
		[oscEncOffsetTextField setEnabled:true];
		[adc0SwitchButton setEnabled:true];
		[adc1SwitchButton setEnabled:true];
		[adc2SwitchButton setEnabled:true];
		[adc3SwitchButton setEnabled:true];		
		[enc0SwitchButton setEnabled:true];
		[enc1SwitchButton setEnabled:true];
		[testModeButton setEnabled:true];
	}
	
	[self deviceSelected:nil];
}

- (void)updateMIDIDevices
{
	unsigned int numberOfMIDIInputDevices = _appController->numberOfMIDIInputDevices();
	unsigned int numberOfMIDIOutputDevices = _appController->numberOfMIDIOutputDevices();
	
	[midiInputDevicePopUpButton removeAllItems];
	
	if (numberOfMIDIInputDevices == 0)
		[midiInputDevicePopUpButton addItemWithTitle:@"No Devices"];
	else {
		for (unsigned int i = 0; i < numberOfMIDIInputDevices; i++) {
			const string& deviceTitleString = _appController->nameOfMIDIInputDeviceAtIndex(i);
			NSString *title = [NSString stringWithCString:deviceTitleString.c_str() encoding:NSASCIIStringEncoding];

			[midiInputDevicePopUpButton addItemWithTitle:title];
		}
	}
	
	[midiOutputDevicePopUpButton removeAllItems];
	
	if (numberOfMIDIOutputDevices == 0)
		[midiOutputDevicePopUpButton addItemWithTitle:@"No Devices"];
	else {
		for (unsigned int i = 0; i < numberOfMIDIOutputDevices; i++) {
			const string& deviceTitleString = _appController->nameOfMIDIOutputDeviceAtIndex(i);
			NSString *title = [NSString stringWithCString:deviceTitleString.c_str() encoding:NSASCIIStringEncoding];
			
			[midiOutputDevicePopUpButton addItemWithTitle:title];
		}
	}
	
	[midiInputDevicePopUpButton selectItemAtIndex:_appController->indexOfMIDIInputDeviceForMonomeXXhDeviceIndex([devicePopUpButton indexOfSelectedItem])];
	[midiOutputDevicePopUpButton selectItemAtIndex:_appController->indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex([devicePopUpButton indexOfSelectedItem])];
}

- (void)updateOscAddressPatternPrefix
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) return;
	
NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSString *aString = [NSString stringWithCString:device->oscAddressPatternPrefix().c_str() encoding:NSASCIIStringEncoding];
	[oscAddressPatternPrefixTextField setStringValue:aString];
	
[pool release];

}

- (void)updateCableOrientation
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0)
		return;

	[cableOrientationPopUpButton selectItemAtIndex:device->cableOrientation()];
}

- (void)updateOscStartRowAndColumn
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0)
		return;

	[oscStartColumnTextField setIntValue:device->oscStartColumn()];
    [oscStartRowTextField setIntValue:device->oscStartRow()];
}

- (void)updateAdcStates
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) {
		[adc0SwitchButton setState:NSOffState];
		[adc1SwitchButton setState:NSOffState];
		[adc2SwitchButton setState:NSOffState];
		[adc3SwitchButton setState:NSOffState];		
	}
	else {		
		[adc0SwitchButton setState:device->adcEnableState(0) ? NSOnState : NSOffState];
		[adc1SwitchButton setState:device->adcEnableState(1) ? NSOnState : NSOffState];
		[adc2SwitchButton setState:device->adcEnableState(2) ? NSOnState : NSOffState];
		[adc3SwitchButton setState:device->adcEnableState(3) ? NSOnState : NSOffState];		
	}
}

- (void)updateAdcLabels
{
	char stringBuf[256];
	
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) {
		[adc0SwitchButton setTitle:@"ADC 0"];
		[adc1SwitchButton setTitle:@"ADC 1"];
		[adc2SwitchButton setTitle:@"ADC 2"];
		[adc3SwitchButton setTitle:@"ADC 3"];
	}
	else {
		snprintf(stringBuf, 256, "ADC %d", device->oscAdcOffset());
		NSString *aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[adc0SwitchButton setTitle:aString];
	
		snprintf(stringBuf, 256, "ADC %d", device->oscAdcOffset() + 1);
		aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[adc1SwitchButton setTitle:aString];

		snprintf(stringBuf, 256, "ADC %d", device->oscAdcOffset() + 2);
		aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[adc2SwitchButton setTitle:aString];

		snprintf(stringBuf, 256, "ADC %d", device->oscAdcOffset() + 3);
		aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[adc3SwitchButton setTitle:aString];
	}
}

- (void)updateEncStates
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) {
		[enc0SwitchButton setState:NSOffState];
		[enc1SwitchButton setState:NSOffState];
	}
	else {
		[enc0SwitchButton setState:device->encEnableState(0) ? NSOnState : NSOffState];	
		[enc1SwitchButton setState:device->encEnableState(1) ? NSOnState : NSOffState];
	}
}

- (void)updateEncLabels
{
	char stringBuf[256];
	
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) {
		[enc0SwitchButton setTitle:@"Enc 0"];
		[enc1SwitchButton setTitle:@"Enc 1"];
	}
	else {
		snprintf(stringBuf, 256, "Enc %d", device->oscEncOffset());
		NSString *aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[enc0SwitchButton setTitle:aString];
	
		snprintf(stringBuf, 256, "Enc %d", device->oscEncOffset() + 1);
		aString = [NSString stringWithCString:stringBuf encoding:NSASCIIStringEncoding];
		[enc1SwitchButton setTitle:aString];
	}
}

- (void)updateTestState
{
	MonomeXXhDevice *device = _appController->deviceAtIndex([devicePopUpButton indexOfSelectedItem]);

	if (device == 0) {
		[testModeButton setState:NSOffState];	
	}
	else {		
		[testModeButton setState:device->testModeState() ? NSOnState : NSOffState];
	}
}



- (BOOL)_convertNSString:(NSString *)aString toInt:(int *)anInt
{
	static NSCharacterSet *characterSet = 0;
	unichar buffer[255];
	unsigned int indexFrom, indexTo;
	
	if (characterSet == 0)
		characterSet = [NSCharacterSet decimalDigitCharacterSet];
	
	for (indexFrom = 0; indexFrom < [aString length]; indexFrom++) {
		if ([aString characterAtIndex:indexFrom] != '0')
			break;
	}
	
	for (indexTo = 0; indexFrom < [aString length]; indexFrom++) {
		if ([characterSet characterIsMember:[aString characterAtIndex:indexFrom]])
			buffer[indexTo++] = [aString characterAtIndex:indexFrom];
	}
	
	if (indexTo) {
		NSString *temporaryString = [NSString stringWithCharacters:buffer length:indexTo];		
		*anInt = [temporaryString intValue];
	}		
	else 
		*anInt = 0;

	return (indexTo == indexFrom);
}

@end
