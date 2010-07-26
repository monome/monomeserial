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

#include "stdafx.h"

#include "OscServer.h"
#include "osc/OscReceivedElements.h"
#include "osc/OscPacketListener.h"
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "ip/IpEndpointName.h"
#include "osc/OscException.h"

#include "OscController.h"
#include "OscAtom.h"

using namespace osc;


void WINAPI OscReceiveThread(void *userData)
{
	try {
		ServerThread* listener = (ServerThread*)userData;
		listener->getListenerSocket()->Run();
	}
	catch (osc::Exception &ex) {
#ifdef DEBUG_PRINT
		cout << "An osc::Exception occurred.  osc::Exception ex.what() -> " << ex.what() << endl;
#endif
	}

#ifdef DEBUG_PRINT
	cout << "*******************************************************************\n"
		<< "An Osc Listener device has exited it's read thread... \n" 
		<< "*******************************************************************\n"
		<< endl;
#endif
}


lo_server_thread  lo_server_thread_new(const char *address, int port)
{
	ServerThread* myThread = new ServerThread(string(address), port);
	return (lo_server_thread)myThread;
}

void lo_server_thread_start(lo_server_thread st)
{
	ServerThread* myThread = (ServerThread*)st;
	HANDLE thread = myThread->getThreadHandle();

	myThread->setThreadHandle( CreateThread(0, NULL, (LPTHREAD_START_ROUTINE)OscReceiveThread, 
		(void*)myThread, 0, NULL) );
}

void lo_server_thread_stop(lo_server_thread st)
{
	ServerThread* myThread = (ServerThread*)st;
	myThread->getListenerSocket()->AsynchronousBreak();
}

int lo_server_thread_get_port(lo_server_thread st)
{
	ServerThread* myThread = (ServerThread*)st;
	return myThread->getEndpoint()->port;
}

void lo_server_thread_free(lo_server_thread st)
{
	ServerThread* myThread = (ServerThread*)st;
	delete myThread;
}

// changed return from void* to void.  the return value is thrown away in the OscController,
void lo_server_thread_add_method(lo_server_thread st, const char *path, const char *typespec, 
									   lo_method_handler h, void *user_data)
{
	ServerThread* myThread = (ServerThread*)st;
	myThread->addHandler(h, user_data);
}


void lo_server_thread_del_method(lo_server_thread thread, const char *path, const char *typespec)
{
	ServerThread* myThread = (ServerThread*)thread;
	myThread->removeHandler();
}


// --------------------------------------------------------------------------------------------------------

OscListener::OscListener()
{
#ifdef DEBUG_PRINT
		cout << "OscListener: Constructor Called" << endl;
#endif
	InitializeCriticalSection(&cs);
}

OscListener::~OscListener()
{
#ifdef DEBUG_PRINT
		cout << "OscListener: Destructor Called" << endl;
#endif
	DeleteCriticalSection(&cs);
}

void
OscListener::ProcessMessage( const osc::ReceivedMessage& msg, const IpEndpointName& remoteEndpoint )
{
	this->handler(msg, remoteEndpoint, oscController);
}

//--------------------------------------------------------------------

ServerThread::ServerThread(string &address, int port) 
{
#ifdef DEBUG_PRINT
	cout << "Server Thread: Constructor Called" << endl;
#endif
	thread = INVALID_HANDLE_VALUE;
	owner = (void*)NULL;
	name = new const IpEndpointName(address.c_str(), port);;
	oscListen = new OscListener();
	listener = new UdpListeningReceiveSocket(*name, oscListen);

	InitializeCriticalSection(&cs);
}

ServerThread::~ServerThread(void) 
{
#ifdef DEBUG_PRINT
	cout << "Server Thread: Destructor Called" << endl;
#endif

	EnterCriticalSection(&cs);

	if (getThreadHandle() != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(getThreadHandle(), INFINITE);
		CloseHandle(getThreadHandle());
	}

	LeaveCriticalSection(&cs);

	delete oscListen;
	delete listener;
	delete name;
	DeleteCriticalSection(&cs);
}

void 
ServerThread::addHandler(lo_method_handler handler, void* oscControllerRef) 
{
	this->getOscListener()->AddHandler(handler, oscControllerRef);
}

void 
ServerThread::removeHandler()
{
	this->getOscListener()->RemoveHandler();
}

static void receiveCallback(void* data)
{
}

// -------------------------------------------------------------------
