#include "../stdafx.h"
#include "OscIOController.h"

//----------------------------------------------------------------------------------------------------------

/* ------------ NOTES / TODO --------------
 *
 * attempt to make windows and osx OscController classes almost identical by wrapping oscpack calls in a liblo interface
 *    - only part im unsure of is if i should wrap the oscpack received message object to liblo type, since the liblo
 *		type is immediatly converted to MonomeSerial OscAtoms type (this may be an unnecessary performance hit)
 *
 * MUST UPDATE OSCHOSTADDRESS!!!
 *	  - now using liblo functions for getting lo_address (implimented with oscpack, easy since liblo returns void*'s)
 *
 * also, impliment osc send functions and add_xxx functions with oscpack
 *
 * With any luck, the high-level osc code in monome serial can be nearly identical, and regardless this new code
 * should be much cleaner, since im taking more time creating it.
 *
 * when done, remove current OscServer class/codefile/header
 */

namespace liblointerface {
//----------------------------------------------------------------------------------------------------------

map<lo_address, OscReceive*> oscReceivers; 
map<lo_address, UdpTransmitSocket*> oscHosts;

//----------------------------------------------------------------------------------------------------------

OscListener::OscListener() 
{
	_handler = 0;
	_userdata = 0;
	InitializeCriticalSection(&cs);
}

OscListener::~OscListener() 
{
	DeleteCriticalSection(&cs);
}

void 
OscListener::updateGenericHandler(lo_method_handler methodHandler, void *userData)
{
	OscListenerLock(this);

	_handler = methodHandler;
	_userdata = userData;
}

void 
OscListener::removeGenericHandler()
{
	OscListenerLock(this);

	_handler = 0;
	_userdata = 0;
}

void
OscListener::ProcessMessage(const osc::ReceivedMessage& msg, const IpEndpointName& remoteEndpoint)
{
	OscListenerLock(this);

	if (_handler) {
		_handler(msg, _userdata);
	}
}

//----------------------------------------------------------------------------------------------------------

void
OscSocketListener::startThread()
{
	thread = CreateThread(0, 0, (::LPTHREAD_START_ROUTINE)threadProc, this, 0, 0);
	if (thread == INVALID_HANDLE_VALUE) {
		throw OscException(OscException::kOscExceptionTypeLibloError, 
			"Failed to create a new thread for OscSocketListener object.", true);
	}
}

void
OscSocketListener::threadProc(void *socketListenerPtr)
{
	OscSocketListener *owner = static_cast<OscSocketListener*>(socketListenerPtr);
	if (owner == NULL) {
		throw OscException(OscException::kOscExceptionTypeLibloError, 
			"Invalid pointer passed to thread process in OscReceive object.", true);
	}

	owner->threadCallback();
}

void
OscSocketListener::killThread()
{
	receiveSocket->AsynchronousBreak();

	// wait 1/2 second, then kill the thread if waiting fails
	if (WaitForSingleObject(thread, 500) == WAIT_TIMEOUT) {
		assert(TerminateThread(thread, 0));
	}
}

void
OscSocketListener::threadCallback()
{
	receiveSocket->Run();
#ifdef DEBUG_PRINT
	cout << "Done with OSC Callback..." << endl;
#endif
	return;
}

//----------------------------------------------------------------------------------------------------------

OscReceive::OscReceive(int port)
{
	ipEndpoint = new IpEndpointName("127.0.0.1", port);
	listener = new OscSocketListener(ipEndpoint);
}

OscReceive::~OscReceive()
{
	if (listener) {
		delete listener;
	}

	delete ipEndpoint;
}

int 
OscReceive::getPort(void) const
{
	if (!ipEndpoint) {
		return 0;
	}

	return ipEndpoint->port;
}


//----------------------------------------------------------------------------------------------------------

// C++ implimentation of C liblo functions
int 
lo_server_thread_get_port(lo_server_thread st)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == st) {
			break;
		}
	}

	if (i == oscReceivers.end()) {
		return 0;
	}

	OscReceive* rec = static_cast<OscReceive*>(st);
	if (rec) {
		return rec->getPort();
	}
	return 0;
}

lo_server_thread 
lo_server_thread_new(lo_address address)
{
	IpEndpointName* ep = static_cast<IpEndpointName*>(address);
	if (ep == static_cast<IpEndpointName*>(NULL) || oscReceivers.find(address) != oscReceivers.end()) {
		return NULL;
	}
	int myport = ep->port;
	if (myport == 0) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "Invalid port specified");
	}

	OscReceive *receiver;
	try {
		receiver = new OscReceive(myport);
		oscReceivers[address] = receiver;
	}
	catch(...) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "Error creating new lo_server_thread");
	}

	return receiver;
}

void 
lo_server_thread_start(lo_server_thread st)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == st) {
			break;
		}
	}
	if (i == oscReceivers.end()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "lo_server_thread does not exist.");
	}

	OscReceive *oscRec = i->second;

	if (oscRec) {
		oscRec->startSocketListener();
	}
}

void 
lo_server_thread_stop(lo_server_thread st)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == st) {
			break;
		}
	}
	if (i == oscReceivers.end()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "lo_server_thread does not exist.");
	}

	OscReceive *oscRec = i->second;

	if (oscRec) {
		oscRec->stopSocketListener();
	}
}

void 
lo_server_thread_free(lo_server_thread st)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == st) {
			break;
		}
	}
	if (i == oscReceivers.end()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "lo_server_thread does not exist.");
	}

	OscReceive *oscRec = i->second;

	if (oscRec) {
		delete oscRec;
	}

	oscReceivers.erase(i);
}

void  
lo_server_thread_add_method(lo_server_thread st, lo_method_handler h, void *user_data)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == st) {
			break;
		}
	}
	if (i == oscReceivers.end()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "lo_server_thread does not exist.");
	}

	OscReceive *oscRec = i->second;

	if (oscRec) {
		oscRec->updateGenericHandler(h, user_data);
	}
}

void  
lo_server_thread_del_method(lo_server_thread thread)
{
	map<lo_address, OscReceive*>::iterator i;
	for (i = oscReceivers.begin(); i != oscReceivers.end(); ++i) {
		if (i->second == thread) {
			break;
		}
	}
	if (i == oscReceivers.end()) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "lo_server_thread does not exist.");
	}

	OscReceive *oscRec = i->second;

	if (oscRec) {
		oscRec->removeGenericHandler();
	}
}

int lo_send_message(lo_address targ, const char* data, int size)
{
	UdpTransmitSocket* socket;
	IpEndpointName* endpoint = static_cast<IpEndpointName*>(targ);
	if (targ == 0) {
		return -1;
	}

	map<lo_address, UdpTransmitSocket*>::iterator i = oscHosts.find(targ);
	if (i != oscHosts.end()) {
		socket = i->second;
	}
	else {
		socket = new UdpTransmitSocket(*endpoint);
		oscHosts[targ] = socket;
	}

	socket->Send(data, size);

	return 0;
}

lo_address lo_address_new(const char *host, const char *port)
{
	int myport = atoi(port);
	if (myport <= 0) {
		throw OscException(OscException::kOscExceptionTypeLibloError, "Invalid port specified for lo_address_new()");
	}

	IpEndpointName *endpoint = new IpEndpointName(host, myport);
	return static_cast<lo_address>(endpoint);
}

void lo_address_free(lo_address t)
{
	// !!! NOTE: removing the host socket is only safe currently if used by a single thread !!!
	if (!t) {
		return;
	}

	// only remove for send sockets, receiver cleanup is handled elsewhere
	map<lo_address, UdpTransmitSocket*>::iterator i = oscHosts.find(t);
	if (i != oscHosts.end()) {
		// host exists for this lo_address, we must delete it and remove it from the map
		oscHosts.erase(i);
	}

	delete t;
}

} //namespace liblointerface