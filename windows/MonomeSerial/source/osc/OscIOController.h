#ifndef __OSCIOCONTROLLER_H__
#define __OSCIOCONTROLLER_H__

#include "../stdafx.h"
#include "oscpack/OscReceivedElements.h"
#include "oscpack/OscPacketListener.h"
#include "oscpack/OscOutboundPacketStream.h"
#include "oscpack/ip/UdpSocket.h"
#include "oscpack/ip/IpEndpointName.h"

#include "OscException.h"

#include <vector>
#include <string>
#include <utility>
#include <cstring>
#include <map>

using namespace osc;
using namespace std;

#define LO_DEF_TYPE_SIZE	8
#define LO_DEF_DATA_SIZE	8
#define OUTPUT_BUFFER_SIZE  8096


// liblo defines and typedefs
namespace liblointerface {

	// handlers
	typedef int (*lo_method_handler)(const osc::ReceivedMessage& msg, void *user_data);

	// types
	typedef void *lo_server_thread;
	typedef void *lo_method;
	typedef void *lo_address;

									
	// interface
	int lo_server_thread_get_port(lo_server_thread st);

	void lo_server_thread_start(lo_server_thread st);

	void lo_server_thread_stop(lo_server_thread st);

	void lo_server_thread_free(lo_server_thread st);

	lo_server_thread lo_server_thread_new (lo_address port);

	void  lo_server_thread_add_method(lo_server_thread st, lo_method_handler h, void *user_data);

	void  lo_server_thread_del_method(lo_server_thread thread);

	int lo_send_message(lo_address targ, const char* data, int size);

	lo_address lo_address_new(const char *host, const char *port);

	void lo_address_free(lo_address t);

	//----------------

	// OscListener receives OscPackets from its parent UdpListeningReceiveSocket, and passes them to the loaded callbacks
	class OscListener : public OscPacketListener 
	{
	public:
		OscListener();
		virtual ~OscListener();
		virtual void ProcessMessage( const osc::ReceivedMessage& msg, const IpEndpointName& remoteEndpoint);
		void updateGenericHandler(lo_method_handler methodHandler, void *userData);
		void removeGenericHandler();

	private:
		lo_method_handler _handler;
		void *_userdata;
		CRITICAL_SECTION cs;

		class OscListenerLock
		{
		public:
			OscListenerLock(OscListener *listener) {
				EnterCriticalSection(_cs = &listener->cs);
			}
			~OscListenerLock() {
				LeaveCriticalSection(_cs);
			}
		private:
			CRITICAL_SECTION *_cs;
		};
		friend class OscListenerLock;
	};


	class OscSocketListener
	{
	public:
		OscSocketListener(IpEndpointName *endpoint)
		{
			listener = new OscListener();
			receiveSocket = new UdpListeningReceiveSocket(*endpoint, listener);
			startThread();
		}

		~OscSocketListener()
		{
			killThread();
			delete listener;
			delete receiveSocket;
		}

		OscListener* getOscListener(void) const
		{
			return listener;
		}
		
	protected:
		// this method will be running on a seperate thread
		virtual void threadCallback();

	private:
		void startThread();
		void killThread();
		// starts the secondary thread, which calls threadCallback and ends immediatly afterwards
		static void threadProc(void *socketListenerPtr);

	private:
		OscListener *listener;
		UdpListeningReceiveSocket *receiveSocket;
		HANDLE thread;
	};


	class OscReceive
	{
	public:
		OscReceive(int port);
		~OscReceive();

	public:
		int  getPort(void) const;
		void updateGenericHandler(lo_method_handler methodHandler, void *userData)
		{
			if (listener) {
				listener->getOscListener()->updateGenericHandler(methodHandler, userData);
			}
		}

		void removeGenericHandler()
		{
			if (listener) {
				listener->getOscListener()->removeGenericHandler();
			}
		}

		void startSocketListener()
		{
			if (!listener) {
				listener = new OscSocketListener(ipEndpoint);
			}
		}

		void stopSocketListener()
		{
			if (listener) {
				delete listener;
			}
			listener = 0;
		}

	private:
		IpEndpointName *ipEndpoint;
		OscSocketListener *listener;
	};

	// this stores the wrapper's namespaces' state
	extern map<lo_address, OscReceive*> oscReceivers;
	extern map<lo_address, UdpTransmitSocket*> oscHosts;

} //namespace liblointerface

#endif //__OSCIOCONTROLLER_H__
