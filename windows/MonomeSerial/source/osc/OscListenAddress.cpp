#include "../stdafx.h"
#include "OscListenAddress.h"
#include "OscException.h"
#include <string>
#include <iostream>
using namespace std;

using namespace liblointerface;

OscListenAddress::OscListenAddress(const string& port, lo_method_handler handler, void* owner)
	:_host("127.0.0.1"), _retainCount(0)
{
    _hostString = _host + ":" + port;
    _hostAddress = lo_address_new(_host.c_str(), port.c_str());

    if (_hostAddress == 0)  
       // throw OscException(OscException::kOscExceptionTypeInvalidHostAddressString, lo_address_errstr(_hostAddress));
	   throw OscException(OscException::kOscExceptionTypeInvalidHostAddressString, 
							"Invalid call to lo_address_new: invalid host address or port value");
	
	serverThread = lo_server_thread_new(_hostAddress);
	if (serverThread != (lo_server_thread) 0) {
        lo_server_thread_add_method(serverThread,
                                    handler, 
                                    owner);
        lo_server_thread_start(serverThread);
    }
	else {
        throw OscException(OscException::kOscExceptionTypeLibloError, 0); 
	}
}    

OscListenAddress::~OscListenAddress()
{
	if (_hostAddress != 0) {
		lo_server_thread_stop(serverThread);
		lo_server_thread_del_method(serverThread);
		lo_server_thread_free(serverThread);
        lo_address_free(_hostAddress);
	}
}

void OscListenAddress::retain(void)
{
    _retainCount++;
#ifdef DEBUG_PRINT
	std::cout << "OscListenAddress::retain " << _hostString << " -> retain count: " << _retainCount << endl;
#endif
}

void OscListenAddress::release(void)
{
    _retainCount--;
#ifdef DEBUG_PRINT
	std::cout << "OscListenAddress::release -> retain count: " << _retainCount << endl;
#endif
}

int OscListenAddress::getRetainCount(void)
{
    return _retainCount;
#ifdef DEBUG_PRINT
	std::cout << "OscListenAddress::getRetainCount -> " << _hostString << " retain count: " << _retainCount << endl;
#endif
}

string OscListenAddress::getHostString(void)
{
    return _hostString;
#ifdef DEBUG_PRINT
	std::cout << "OscListenAddress::getHostString " << _hostString << " retain count: " << _retainCount << " host string: " << _hostString << endl;
#endif
}

lo_address OscListenAddress::getHostAddress(void)
{
    return _hostAddress;
#ifdef DEBUG_PRINT
	std::cout << "OscListenAddress::getHostAddress -> retain count: " << _retainCount << " host address: " << _hostAddress << endl;
#endif
}
