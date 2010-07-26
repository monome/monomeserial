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


#ifndef __OSCCONTROLLER_H__
#define __OSCCONTROLLER_H__

#include "OscHostAddress.h"
#include "OscListenAddress.h"

#include "oscpack/OscOutboundPacketStream.h"

// console window only created if 
#ifdef DEBUG_PRINT
#include <iostream>
#endif

#include <string>
#include <list>
#include <vector>

#include <hash_map>
using namespace stdext;
using namespace std;
using namespace osc;

typedef void (*OscMessageHandler)(const osc::ReceivedMessage &receivedMessage, void *userData);
typedef struct _OscMessageHandlerContext {
    OscMessageHandler handler;
    void *userData;
} OscMessageHandlerContext;


class OscController 
{
public:
    OscController();
    ~OscController();

    OscHostRef getOscHostRef(const string& host, const string &port, bool retain);
    void releaseOscHostRef(OscHostRef hostRef);

	OscListenRef getOscListenRef(const string& port, bool retain);
	void releaseOscListenRef(OscListenRef listenRef);

    void send(OscHostRef hostRef, const osc::OutboundPacketStream &stream);
/*
    OscListenRef startListening(const string& port);
	void stopListening(OscListenRef oscListenRef);
    void stopListening(void);
*/
	// NOTE - removed per-addresspattern method handlers, they are a good idea but unused
    void addGenericOscMessageHandler(OscMessageHandler handler, void *userData);
    
	void loMethodHandler(const osc::ReceivedMessage &receivedMessage);

private:
    OscHostAddress *_getOscHostAddress(const string& hostString);

	hash_map<string, OscHostAddress *> *_hostAddresses;
    vector<OscMessageHandlerContext> _oscGenericMessageHandlers;
    hash_map<string, OscListenAddress *> *_listenAddresses;

	string _listenAddress;
};


#endif
