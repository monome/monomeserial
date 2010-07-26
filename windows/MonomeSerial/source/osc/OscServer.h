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

// this file created for Windows only, by Daniel Battaglia

/* 
 * NOTE: the purpose of this file is to abstract away all C-language calls to the OSC liblo library, handling all calls
 *  	 and forwarding them to the C++ OO OscPack classes.  The only OscPack value used outside of this file is
 *  	 IpEndpointName, in the OscHostAddress, for convenience only.  All other calls stick as close as possible to the
 *  	 original OSX/liblo implimentation.
 *
 *		 Parts of this code is taken from the liblo project.
 *  
 *  	 - daniel battaglia
*/
#include "stdafx.h"

#ifndef __OSCSERVER_H__
#define __OSCSERVER_H__

#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "ip/IpEndpointName.h"

#include "OscHostAddress.h"
#include "OscException.h"
#include "OscAtom.h"

#ifdef DEBUG_PRINT
#include <iostream>
#endif

using namespace osc;


#define LO_DEF_TYPE_SIZE 8
#define LO_DEF_DATA_SIZE 8

typedef void* lo_server_thread;
typedef void* lo_method;

typedef void (*lo_err_handler)(int num, const char *msg, const char *where);
typedef void (*lo_method_handler)(const osc::ReceivedMessage& msg, const IpEndpointName& remoteEndpoint, void* userData);


// ------ wrapper functions for linking OscListener to the Liblo interface ------

void  lo_server_thread_add_method(lo_server_thread st, const char *path,
									   const char *typespec, lo_method_handler h,
									   void *user_data);

void  lo_server_thread_del_method(lo_server_thread thread, 
								  const char *path, 
								  const char *typespec);

void  lo_server_thread_start(lo_server_thread st);

void  lo_server_thread_stop(lo_server_thread st);

int  lo_server_thread_get_port(lo_server_thread st);

void  lo_server_thread_free(lo_server_thread st);

lo_server_thread  lo_server_thread_new(const char *address, int port);


class OscSender
{
public:
	OscSender(void);
	~OscSender(void);

private:
};


// OscListener receives OscPackets from its parent UdpListeningReceiveSocket, and passes them to the loaded callbacks
class OscListener : public OscPacketListener 
{
protected:

public:
	OscListener();
	~OscListener();


	virtual void ProcessMessage( const osc::ReceivedMessage&, const IpEndpointName& );

	void AddHandler(lo_method_handler newHandler, void* oscControllerRef)
	{
#ifdef DEBUG_PRINT
		cout << "OscListener: handler added" << endl;
#endif

		EnterCriticalSection(&cs);
		handler = newHandler;
		oscController = oscControllerRef;
		LeaveCriticalSection(&cs);
	}

	void RemoveHandler()
	{
		EnterCriticalSection(&cs);
		handler = (lo_method_handler)0;
		oscController = NULL;
	}


private:
	lo_method_handler handler;
	void* oscController;
	CRITICAL_SECTION cs;
};



// encapsulates an OscPack C++ UdpListeningReceiveSocket. its public interface is accessed via the 
// liblo C function calls used by MonomeSerial
class ServerThread
{
public:
	ServerThread(string &address, int port);
	~ServerThread(void);

	void addHandler(lo_method_handler handler, void* oscController);
	void removeHandler();

public:  // inline public Properties
	UdpListeningReceiveSocket* getListenerSocket() const 
	{
		return listener;
	}

	OscListener* getOscListener() const
	{
		return oscListen;
	}

	const IpEndpointName* getEndpoint() const 
	{
		if (name == (IpEndpointName*)NULL) {
			return (IpEndpointName*)NULL;
		}

		return name;
	}

	const void* getOwner()
	{
		return owner;
	}

	HANDLE getThreadHandle()
	{
		return thread;
	}

	void setThreadHandle(HANDLE threadHandle)
	{
		thread = threadHandle;
	}

private:
	HANDLE thread;
	UdpListeningReceiveSocket* listener;
	OscListener *oscListen;
	void* owner;
	CRITICAL_SECTION cs;

	// incoming endpoint
	const IpEndpointName *name;
};

#endif  // __OSCSERVER_H__