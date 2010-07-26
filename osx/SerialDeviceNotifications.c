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

//#include "MonomeSerialBuildFlags.h"

#include "SerialDeviceNotifications.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <regex.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

#ifdef __cplusplus
extern "C"
{
#endif

void _SetUpSerialDeviceNotifications(void *context);
void _SerialDeviceDiscoveredCallback(void *ctxt, io_iterator_t serialPortIterator);
void _SerialDeviceTerminatedCallback(void *ctxt, io_iterator_t serialPortIterator);
#ifndef	_IGNORE_40h_
static regex_t _serialDeviceNotificationRegex_40h;
#endif
#ifndef	_IGNORE_256_
static regex_t _serialDeviceNotificationRegex_256;
#endif
static regex_t _serialDeviceNotificationRegex_128;
static regex_t _serialDeviceNotificationRegex_64;
static regex_t _serialDeviceNotificationRegex_mk;
static SerialDeviceNotificationContext gContext;


/***************************************************************************************************
 *
 * DESCRIPTION: Sets up a new thread that requests notifications of device discovery and unexpected 
 *              device termination from IOKit, adds notifications to CFRunLoop and runs CFRunLoop.
 *
 * ARGUMENTS:   context - SerialDeviceNotificationContext with user-defined callbacks to be called
 *                        when devices are discovered or terminated.
 *
 * RETURNS:
 *
 * NOTES:       
 *
 ****************************************************************************************************/

void RegisterForSerialDeviceNotifications(SerialDeviceNotificationContext *context) // we should return an error code
{

    if (
	#ifndef _IGNORE_40h_
		((regcomp(&_serialDeviceNotificationRegex_40h, "m40h", REG_NOSUB)) != 0) 	||  //m40h0101
	#endif
	#ifndef	_IGNORE_256_
	    ((regcomp(&_serialDeviceNotificationRegex_256, "m256", REG_NOSUB) )!= 0) 	||  
	#endif
	      ((regcomp(&_serialDeviceNotificationRegex_128, "m128", REG_NOSUB) )!= 0) 	||
	     ((regcomp(&_serialDeviceNotificationRegex_64, "m64-", REG_NOSUB) )!= 0)  ||
		//kit
		 ((regcomp(&_serialDeviceNotificationRegex_mk, "mk", REG_NOSUB) )!= 0)
		)
 {
        fprintf(stdout, "error: regular expression failed to compile.\n");
        fflush(stdout);
        return;
    }


	gContext = *context;

    _SetUpSerialDeviceNotifications(&gContext);
}


/***************************************************************************************************
 *
 * DESCRIPTION: Requests device discovery and termination notifications from IOKit, adds them to
 *              CFRunLoop, and runs CFRunLoop.
 *
 * ARGUMENTS:   ctxt - SerialDeviceNotificationContext with user-defined callbacks to be called
 *                     when devices are discovered or terminated.
 *
 * RETURNS:
 *
 * NOTES:       
 *
 ****************************************************************************************************/

void _SetUpSerialDeviceNotifications(void *ctxt)
{
    kern_return_t kernResult;
    mach_port_t masterPort;
    IONotificationPortRef notificationDiscoveredPortRef;
    IONotificationPortRef notificationTerminatedPortRef;
    CFRunLoopSourceRef notificationDiscoveredRunLoopSource;
    CFRunLoopSourceRef notificationTerminatedRunLoopSource;
    CFMutableDictionaryRef classesToMatchDiscovered;
    CFMutableDictionaryRef classesToMatchTerminated;
    io_iterator_t serialPortDiscoveredIterator;
    io_iterator_t serialPortTerminatedIterator;
    io_object_t serialService;
    char bsdPath[MAXPATHLEN];
    SerialDeviceNotificationContext *context;

    context = (SerialDeviceNotificationContext *)ctxt;
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult) {
        printf("IOMasterPort returned %d\n", kernResult);
        return;
    }

    classesToMatchDiscovered = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatchDiscovered == NULL)
        printf("IOServiceMatching returned a NULL dictionary.\n");
    else {
        CFDictionarySetValue(classesToMatchDiscovered,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDAllTypes));
    }

    classesToMatchTerminated = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatchTerminated == NULL)
        printf("IOServiceMatching returned a NULL dictionary.\n");
    else {
        CFDictionarySetValue(classesToMatchTerminated,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDAllTypes));
    }

    notificationDiscoveredPortRef = IONotificationPortCreate(masterPort);
    if (notificationDiscoveredPortRef == NULL) {
        printf("IONotificationPortCreate return a NULL IONotificationPortRef.\n");
        return;
    }

    notificationTerminatedPortRef = IONotificationPortCreate(masterPort);
    if (notificationTerminatedPortRef == NULL) {
        printf("IONotificationPortCreate return a NULL IONotificationPortRef.\n");
        return;
    }


    notificationDiscoveredRunLoopSource = IONotificationPortGetRunLoopSource(notificationDiscoveredPortRef);
    if (notificationDiscoveredRunLoopSource == NULL) {
        printf("IONotificationPortGetRunLoopSource returned NULL CFRunLoopSourceRef.");
        return;
    }

    notificationTerminatedRunLoopSource = IONotificationPortGetRunLoopSource(notificationTerminatedPortRef);
    if (notificationTerminatedRunLoopSource == NULL) {
        printf("IONotificationPortGetRunLoopSource returned NULL CFRunLoopSourceRef.");
        return;
    }
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationDiscoveredRunLoopSource, kCFRunLoopDefaultMode);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationTerminatedRunLoopSource, kCFRunLoopDefaultMode);
    

    kernResult = IOServiceAddMatchingNotification(notificationDiscoveredPortRef, 
                                                  kIOMatchedNotification,
                                                  classesToMatchDiscovered,
                                                  _SerialDeviceDiscoveredCallback,
                                                  context,
                                                  &serialPortDiscoveredIterator);
    if (kernResult != KERN_SUCCESS) {
        printf("IOServiceAddMatchingNotification return %d", kernResult);
        return;
    }

    // Before we can receive notifications of device discovery, we have to grab
    // all the matching devices plugged in before we requested the notification.
    // We have do do this for terminated devices as well.

    while ((serialService = IOIteratorNext(serialPortDiscoveredIterator))) {
        CFTypeRef deviceFilePathAsCFString;
        deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(serialService,
                                                                   CFSTR(kIODialinDeviceKey),
                                                                   kCFAllocatorDefault,
                                                                   0);
        
        if (deviceFilePathAsCFString) {
            CFStringGetCString(deviceFilePathAsCFString,
                               bsdPath,
                               MAXPATHLEN,
                               kCFStringEncodingUTF8);
            CFRelease(deviceFilePathAsCFString);

            if (
			#ifndef	_IGNORE_40h_
			(regexec(&_serialDeviceNotificationRegex_40h, bsdPath, 0, 0, 0) == 0)||
			#endif
			#ifndef	_IGNORE_256_
				(regexec(&_serialDeviceNotificationRegex_256, bsdPath, 0, 0, 0) == 0)||
			#endif
				(regexec(&_serialDeviceNotificationRegex_128, bsdPath, 0, 0, 0) == 0)||
				(regexec(&_serialDeviceNotificationRegex_64, bsdPath, 0, 0, 0) == 0)||
				(regexec(&_serialDeviceNotificationRegex_mk, bsdPath, 0, 0, 0) == 0))
                context->serialDeviceDiscoveredCallback(bsdPath, context->userData);
        }
    }

    kernResult = IOServiceAddMatchingNotification(notificationTerminatedPortRef, 
                                                  kIOTerminatedNotification,
                                                  classesToMatchTerminated,
                                                  _SerialDeviceTerminatedCallback,
                                                  context,
                                                  &serialPortTerminatedIterator);
    if (kernResult != KERN_SUCCESS) {
        printf("IOServiceAddMatchingNotification return %d.", kernResult);
        return;
    }

    while ((serialService = IOIteratorNext(serialPortTerminatedIterator))) {
        CFTypeRef deviceFilePathAsCFString;
        deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(serialService,
                                                                   CFSTR(kIODialinDeviceKey),
                                                                   kCFAllocatorDefault,
                                                                   0);
        
        if (deviceFilePathAsCFString) {
            CFStringGetCString(deviceFilePathAsCFString,
                               bsdPath,
                               MAXPATHLEN,
                               kCFStringEncodingUTF8);
            CFRelease(deviceFilePathAsCFString);

            if (
			#ifndef	_IGNORE_40h_
				(regexec(&_serialDeviceNotificationRegex_40h, bsdPath, 0, 0, 0) == 0)||
			#endif
			#ifndef	_IGNORE_256_
				(regexec(&_serialDeviceNotificationRegex_256, bsdPath, 0, 0, 0) == 0)||
			#endif
				(regexec(&_serialDeviceNotificationRegex_128, bsdPath, 0, 0, 0) == 0)||
				(regexec(&_serialDeviceNotificationRegex_64, bsdPath, 0, 0, 0) == 0))
                context->serialDeviceTerminatedCallback(bsdPath, context->userData);
        }
    }
}


/***************************************************************************************************
 *
 * DESCRIPTION: Called when matching devices are discovered by the kernel.  Checks device file path
 *              for presence of substring 'm40h' (cheap hack since we're using ftdi's drivers) and
 *              if found calls user defined callback 
 *              ((SerialDeviceNotificationContext *)ctxt)->serialDeviceDiscoveredCallback.
 *
 * ARGUMENTS:   ctxt               - SerialDeviceNotificationContext with user-defined callbacks to 
 *                                   be called when devices are discovered or terminated.
 *              serialPortIterator - io_iterator_t for discovered devices.
 *
 * RETURNS:
 *
 * NOTES:       
 *
 ****************************************************************************************************/

void _SerialDeviceDiscoveredCallback(void *ctxt, io_iterator_t serialPortIterator)
{
    SerialDeviceNotificationContext *context;
    io_object_t serialService;
    char bsdPath[MAXPATHLEN];

    context = (SerialDeviceNotificationContext *)ctxt;
   
    while ((serialService = IOIteratorNext(serialPortIterator))) {
        CFTypeRef deviceFilePathAsCFString;
        deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(serialService,
                                                                   CFSTR(kIODialinDeviceKey),
                                                                   kCFAllocatorDefault,
                                                                   0);
        CFStringGetCString(deviceFilePathAsCFString,
                           bsdPath,
                           MAXPATHLEN,
                           kCFStringEncodingUTF8);
        CFRelease(deviceFilePathAsCFString);



            if (
			#ifndef _IGNORE_40h_
				(regexec(&_serialDeviceNotificationRegex_40h, bsdPath, 0, 0, 0) == 0)||
			#endif
			#ifndef	_IGNORE_256_
				(regexec(&_serialDeviceNotificationRegex_256, bsdPath, 0, 0, 0) == 0)||
			#endif
				(regexec(&_serialDeviceNotificationRegex_128, bsdPath, 0, 0, 0) == 0)||
				(regexec(&_serialDeviceNotificationRegex_64, bsdPath, 0, 0, 0) == 0)
				
				)
            context->serialDeviceDiscoveredCallback(bsdPath, context->userData);
    }
}


/***************************************************************************************************
 *
 * DESCRIPTION: Called when matching devices are terminated as in the case of an unexpected device
 *              removal.  Checks device file path for presence of substring 'm40h' (cheap hack since 
 *              we're using ftdi's drivers) and if found calls user defined callback 
 *              ((SerialDeviceNotificationContext *)ctxt)->serialDeviceTerminatedCallback.
 *
 * ARGUMENTS:   ctxt               - SerialDeviceNotificationContext with user-defined callbacks to 
 *                                   be called when devices are discovered or terminated.
 *              serialPortIterator - io_iterator_t for terminated devices.
 *
 * RETURNS:
 *
 * NOTES:       
 *
 ****************************************************************************************************/

void _SerialDeviceTerminatedCallback(void *ctxt, io_iterator_t serialPortIterator)
{
    SerialDeviceNotificationContext *context;
    io_object_t serialService;
    char bsdPath[MAXPATHLEN];

    context = (SerialDeviceNotificationContext *)ctxt;
   
    while ((serialService = IOIteratorNext(serialPortIterator))) {
        CFTypeRef deviceFilePathAsCFString;
        deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(serialService,
                                                                   CFSTR(kIODialinDeviceKey),
                                                                   kCFAllocatorDefault,
                                                                   0);
        CFStringGetCString(deviceFilePathAsCFString,
                           bsdPath,
                           MAXPATHLEN,
                           kCFStringEncodingUTF8);
        CFRelease(deviceFilePathAsCFString);

                if (
				#ifndef	_IGNORE_40h_
					(regexec(&_serialDeviceNotificationRegex_40h, bsdPath, 0, 0, 0) == 0)||
				#endif
				#ifndef	_IGNORE_256_
					(regexec(&_serialDeviceNotificationRegex_256, bsdPath, 0, 0, 0) == 0)||
				#endif
				(regexec(&_serialDeviceNotificationRegex_128, bsdPath, 0, 0, 0) == 0)||
				(regexec(&_serialDeviceNotificationRegex_64, bsdPath, 0, 0, 0) == 0) ||
				(regexec(&_serialDeviceNotificationRegex_mk, bsdPath, 0, 0, 0) == 0))
            context->serialDeviceTerminatedCallback(bsdPath, context->userData);
    }
}

#ifdef __cplusplus
}
#endif

                   
