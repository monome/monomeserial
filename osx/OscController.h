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
#ifndef __OSCCONTROLLER_H__
#define __OSCCONTROLLER_H__

#include "OscAtom.h"
#include "OscContext.h"
#include "OscHostAddress.h"
#include <lo/lo.h>
#include <string>
#include <list>
using namespace std;
#include <hash_map.h>
using namespace __gnu_cxx;

#ifndef __GNU_CXX_HASH_STRING_FIX
#define __GNU_CXX_HASH_STRING_FIX

namespace __gnu_cxx {
    template<> struct hash<std::string>
    {
        size_t operator()(const std::string& s) const
        {
            return __stl_hash_string(s.c_str());
        }
    };
}

#endif

struct oscHostAddressEqstr { // change the name of this.
    bool operator()(const string& str1, const string& str2) const
    {
        return str1 == str2;
    }
};

typedef void (*OscMessageHandler)(const string& addressPattern, list <OscAtom *> *atoms, void *userData);
typedef struct _OscMessageHandlerContext {
    OscMessageHandler handler;
    void *userData;
} OscMessageHandlerContext;

class OscController 
{
public:
    OscController();
    ~OscController();

    OscHostRef getOscHostRef(const string& host, const string &port);
    void releaseOscHostRef(OscHostRef hostRef);

    void send(OscHostRef hostRef, const string& addressPattern, list<OscAtom *> *atoms);
    void send(OscHostRef hostRef, const string& addressPattern, list<OscAtom> *atoms);

    void startListening(const string& port);
    void stopListening(void);

    void addOscMessageHandler(const string& addressPattern, OscMessageHandler handler, void *userData);
    void addGenericOscMessageHandler(OscMessageHandler handler, void *userData);
    void removeOscMessageHandler(const string& addressPattern);
    
    void loMethodHandler(string path, const char *types, lo_arg **argv, int argc, lo_message msg);

private:
    OscHostAddress *_getOscHostAddress(const string& hostString);

    hash_map<string, OscHostAddress *, hash<string>, oscHostAddressEqstr> *_hostAddresses;
    hash_map<string, OscMessageHandlerContext *, hash<string>, oscHostAddressEqstr> *_oscMessageHandlers;
    vector<OscMessageHandlerContext> _oscGenericMessageHandlers;
    lo_server_thread _oscServerThread;
};

#endif
