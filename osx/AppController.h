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
/* AppController */

#import <Cocoa/Cocoa.h>
class ApplicationController;

@interface AppController : NSObject
{
    IBOutlet NSPopUpButton *cableOrientationPopUpButton;
    IBOutlet NSPopUpButton *devicePopUpButton;
    IBOutlet NSTabView *deviceSpecificProtocolTabView;
    IBOutlet NSPopUpButton *ioProtocolPopUpButton;

    IBOutlet NSPopUpButton *midiInputChannelPopUpButton;
    IBOutlet NSPopUpButton *midiInputDevicePopUpButton;
    IBOutlet NSPopUpButton *midiOutputChannelPopUpButton;
    IBOutlet NSPopUpButton *midiOutputDevicePopUpButton;

    IBOutlet NSTextField *oscAddressPatternPrefixTextField;
    IBOutlet NSTextField *oscHostAddressTextField;
    IBOutlet NSTextField *oscHostPortTextField;
    IBOutlet NSTextField *oscListenPortTextField;
    IBOutlet NSTextField *oscStartColumnTextField;
    IBOutlet NSTextField *oscStartRowTextField;
	IBOutlet NSTextField *oscAdcOffsetTextField;
	IBOutlet NSTextField *oscEncOffsetTextField;

	IBOutlet NSButton *adc0SwitchButton;
	IBOutlet NSButton *adc1SwitchButton;
	IBOutlet NSButton *adc2SwitchButton;
	IBOutlet NSButton *adc3SwitchButton;
	IBOutlet NSButton *enc0SwitchButton;
	IBOutlet NSButton *enc1SwitchButton;
	
	IBOutlet NSButton *testModeButton;
	IBOutlet NSButton *clearLedsButton;
	
	ApplicationController *_appController;
}
- (IBAction)cableOrientationSelected:(id)sender;
- (IBAction)deviceSelected:(id)sender;
- (IBAction)ioProtocolSelected:(id)sender;
- (IBAction)midiInputChannelSelected:(id)sender;
- (IBAction)midiInputDeviceSelected:(id)sender;
- (IBAction)midiOutputChannelSelected:(id)sender;
- (IBAction)midiOutputDeviceSelected:(id)sender;

- (IBAction)adc0SwitchButtonClicked:(id)sender;
- (IBAction)adc1SwitchButtonClicked:(id)sender;
- (IBAction)adc2SwitchButtonClicked:(id)sender;
- (IBAction)adc3SwitchButtonClicked:(id)sender;
- (IBAction)enc0SwitchButtonClicked:(id)sender;
- (IBAction)enc1SwitchButtonClicked:(id)sender;

- (IBAction)clearLedsButtonClicked:(id)sender;
- (IBAction)testModeButtonClicked:(id)sender;

- (void)controlTextDidBeginEditing:(NSNotification *)aNotification;
- (void)controlTextDidChange:(NSNotification *)aNotification;
- (void)controlTextDidEndEditing:(NSNotification *)aNotification;

- (void)updateDeviceList;
- (void)updateMIDIDevices;
- (void)updateOscAddressPatternPrefix;
- (void)updateCableOrientation;
- (void)updateOscStartRowAndColumn;
- (void)updateAdcStates;
- (void)updateAdcLabels;
- (void)updateEncStates;
- (void)updateEncLabels;
- (void)updateTestState;

- (BOOL)_convertNSString:(NSString *)aString toInt:(int *)anInt;
@end
