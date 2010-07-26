#ifndef __OSCLISTENADDRESS_H__
#define __OSCLISTENADDRESS_H__

#include "OscIOController.h"
#include <string>
#include <list>
using namespace std;

typedef void* OscListenRef;

// liblo to oscpack wrapper is in this namespace...
using namespace liblointerface;

class OscListenAddress {
public:
    OscListenAddress(const string& port, lo_method_handler handler, void* owner);
    ~OscListenAddress();

    void retain(void);
    void release(void);
    int getRetainCount(void);
    string getHostString(void);
    lo_address getHostAddress(void);

private:
	string _host;
    string _hostString;
    lo_address _hostAddress;
    int _retainCount;
	lo_server_thread serverThread;
};

#endif //__OSCLISTENADDRESS_H__