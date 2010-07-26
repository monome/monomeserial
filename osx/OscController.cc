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
#include "OscController.h"
#include "OscHostAddress.h"
#include "OscException.h"

extern "C" int OscControllerLoMethodHandler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data);
extern "C" void OscControllerLoErrorHandler(int num, const char *msg, const char *where);

//static int _oscErrNo;
//static string _oscErrMsg;
//static string _oscErrWhere;

OscController::OscController()
{
    _hostAddresses = new hash_map<string, OscHostAddress *, hash<string>, oscHostAddressEqstr>;
    _oscMessageHandlers = new hash_map<string, OscMessageHandlerContext *, hash<string>, oscHostAddressEqstr>;
    _oscServerThread = (lo_server_thread) 0;
}

OscController::~OscController()
{
    hash_map<string, OscHostAddress *, hash<string>, oscHostAddressEqstr>::iterator iHostAddresses;
    hash_map<string, OscMessageHandlerContext *, hash<string>, oscHostAddressEqstr>::iterator iOscMessageHandlers;
    OscHostAddress *currentAddress;
    string currentAddressPattern;
    OscMessageHandlerContext *currentOscHandlerContext;


    iHostAddresses = _hostAddresses->begin();
    while (iHostAddresses != _hostAddresses->end()) {
        currentAddress = ((std::pair<string, OscHostAddress *>)*iHostAddresses).second;
        if (currentAddress != 0) delete currentAddress;
        iHostAddresses++;
    }

    _hostAddresses->clear();
    delete _hostAddresses;

    iOscMessageHandlers = _oscMessageHandlers->begin();
    while (iOscMessageHandlers != _oscMessageHandlers->end()) {
        currentOscHandlerContext = ((std::pair<string, OscMessageHandlerContext *>)*iOscMessageHandlers).second;

        iOscMessageHandlers++;

        if (currentOscHandlerContext != 0) delete currentOscHandlerContext;
    }

    _oscMessageHandlers->clear();
    delete _oscMessageHandlers;

    if (_oscServerThread != (lo_server_thread) 0) {
        lo_server_thread_stop(_oscServerThread);
        lo_server_thread_del_method(_oscServerThread, (const char *) 0, (const char *) 0);
        lo_server_thread_free(_oscServerThread);
    }
}

OscHostRef OscController::getOscHostRef(const string& host, const string& port)
{
    string hostString;
    OscHostAddress *hostAddress;
    hash_map<string, OscHostAddress *, hash<string>, oscHostAddressEqstr>::iterator i;

    hostString = host + ":" + port;

    i = _hostAddresses->find(hostString);

    if (i != _hostAddresses->end()) {
        (*_hostAddresses)[hostString]->retain();
        
        return (OscHostRef)((*_hostAddresses)[hostString]);
    }

    hostAddress = new OscHostAddress(host, port);
    hostAddress->retain();

    (*_hostAddresses)[hostString] = hostAddress;

    return (OscHostRef) hostAddress;
}

void OscController::releaseOscHostRef(OscHostRef hostRef)
{
    string hostString;
    OscHostAddress *hostAddress;

    if (hostRef == 0)
        return;

    hostAddress = (OscHostAddress *)hostRef;
    hostString = hostAddress->getHostString();

    hostAddress->release();

    if (hostAddress->getRetainCount() == 0) {
        hash_map<string, OscHostAddress *, hash<string>, oscHostAddressEqstr>::iterator i = _hostAddresses->find(hostString);

        if (i != _hostAddresses->end()) {
            hostString = ((std::pair<string, OscHostAddress *>)*i).first;
            _hostAddresses->erase(i);

            if (hostAddress != 0) delete hostAddress;
        }
    }
}

OscHostAddress *OscController::_getOscHostAddress(const string& hostString)
{
    OscHostAddress *hostAddress = (*_hostAddresses)[hostString];
    return hostAddress;
}

void OscController::send(OscHostRef hostRef, const string& addressPattern, list<OscAtom *> *atoms)
{
    OscHostAddress *hostAddress;
    lo_message message;
    list<OscAtom *>::iterator i;
    OscAtom *atom;
    int error;

    if (hostRef == 0 || addressPattern.length() == 0)
        return;

    hostAddress = (OscHostAddress *)hostRef;
    message = lo_message_new();

    for (i = atoms->begin(); i != atoms->end(); ++i) {
        atom = *i;

        switch (atom->type()) {
        case OscAtom::kOscAtomTypeInt:
            lo_message_add_int32(message, atom->valueAsInt());
            break;

        case OscAtom::kOscAtomTypeFloat:
            lo_message_add_float(message, atom->valueAsFloat());
            break;

        case OscAtom::kOscAtomTypeString:
            lo_message_add_string(message, atom->valueAsString().c_str());
            break;

        default:
            throw OscException(OscException::kOscExceptionTypeInvalidOscAtomType, 0);
        }
    }

    error = lo_send_message(hostAddress->getHostAddress(), addressPattern.c_str(), message);
    //cout << error << endl;
    //cout << strerror(lo_address_errno(hostAddress->getHostAddress())) << endl;
    //lo_message_pp(message);
    lo_message_free(message);
}

void OscController::send(OscHostRef hostRef, const string& addressPattern, list<OscAtom> *atoms)
{
    OscHostAddress *hostAddress;
    lo_message message;
    list<OscAtom>::iterator i;

    if (hostRef == 0 || addressPattern.length() == 0)
        return;

    hostAddress = (OscHostAddress *)hostRef;
    message = lo_message_new();

    for (i = atoms->begin(); i != atoms->end(); ++i) {
        switch ((*i).type()) {
        case OscAtom::kOscAtomTypeInt:
            lo_message_add_int32(message, (*i).valueAsInt());
            break;

        case OscAtom::kOscAtomTypeFloat:
            lo_message_add_float(message, (*i).valueAsFloat());
            break;

        case OscAtom::kOscAtomTypeString:
            lo_message_add_string(message, (*i).valueAsString().c_str());
            break;

        default:
            throw OscException(OscException::kOscExceptionTypeInvalidOscAtomType, 0);
        }
    }

    lo_send_message(hostAddress->getHostAddress(), addressPattern.c_str(), message);
    lo_message_free(message);
}


void OscController::startListening(const string& port)
{
    const char *portCStr;
    int portAsInt;
    char *endptr;

    portCStr = port.c_str();

    portAsInt = (int)strtol(portCStr, &endptr, 0);
    if (endptr != portCStr + strlen(portCStr))
        throw OscException(OscException::kOscExceptionTypeInvalidPortString, portCStr);

    if (_oscServerThread != (lo_server_thread) 0) {
        if (portAsInt == lo_server_thread_get_port(_oscServerThread))
            return;

        lo_server_thread_stop(_oscServerThread);
        lo_server_thread_del_method(_oscServerThread, (const char *) 0, (const char *) 0);
        lo_server_thread_free(_oscServerThread);
        _oscServerThread = (lo_server_thread) 0;
    }

    _oscServerThread = lo_server_thread_new(portCStr, OscControllerLoErrorHandler);

    if (_oscServerThread != (lo_server_thread) 0) {
        lo_server_thread_add_method(_oscServerThread, 
                                    (const char *) 0, 
                                    (const char *) 0, 
                                    OscControllerLoMethodHandler, 
                                    this);
        lo_server_thread_start(_oscServerThread);
    }
    else 
        throw OscException(OscException::kOscExceptionTypeLibloError, 0); 
}

void OscController::stopListening(void)
{
    if (_oscServerThread == (lo_server_thread) 0)
        return;

    lo_server_thread_stop(_oscServerThread);
}

void OscController::addOscMessageHandler(const string& addressPattern, OscMessageHandler handler, void *userData)
{
    hash_map<string, OscMessageHandlerContext *, hash<string>, oscHostAddressEqstr>::iterator i;
    OscMessageHandlerContext *handlerContext;

    i = _oscMessageHandlers->find(addressPattern);

    if (i != _oscMessageHandlers->end()) {
        handlerContext = (*_oscMessageHandlers)[addressPattern];
        handlerContext->handler = handler;
        handlerContext->userData = userData;
    }
    else {
        handlerContext = new OscMessageHandlerContext;
        handlerContext->handler = handler;
        handlerContext->userData = userData;
        
        (*_oscMessageHandlers)[addressPattern] = handlerContext; 
    }
}

void 
OscController::addGenericOscMessageHandler(OscMessageHandler handler, void *userData)
{
    if (handler == 0)
        return;

    OscMessageHandlerContext context = { handler, userData };

    _oscGenericMessageHandlers.push_back(context);
}

void OscController::removeOscMessageHandler(const string& addressPattern)
{
    hash_map<string, OscMessageHandlerContext *, hash<string>, oscHostAddressEqstr>::iterator i;
    OscMessageHandlerContext *handlerContext;

    i = _oscMessageHandlers->find(addressPattern);

    if (i != _oscMessageHandlers->end()) {
        handlerContext = ((std::pair<string, OscMessageHandlerContext *>)*i).second;

        _oscMessageHandlers->erase(i);

        if (handlerContext != 0) delete handlerContext;
    }
}

/** IMPORTANT: since this method allocates memory for the atoms that get passed the user-defined callback and
 *             deletes these atoms before returning, the user must handle the message immediately or copy the list
 *             of atoms if he wants to defer handling.  also, for string atoms, the atom's value just points into argv.
 *             no way to know what will be there after this method returns.  
 *             this should be documented somewhere other than here.
 */
void OscController::loMethodHandler(string path, const char *types, lo_arg **argv, int argc, lo_message msg)
{
    hash_map<string, OscMessageHandlerContext *,hash<string>, oscHostAddressEqstr>::iterator iMessageHandlers;
    OscMessageHandlerContext *handlerContext;
    list <OscAtom *> atoms;
    int i;
    OscAtom *currentAtom;

    (void) msg;
    
    do {
        for (i = 0; i < argc; i++) {
            switch (types[i]) {
            case 'i':
                currentAtom = new OscAtom(argv[i]->i);
                atoms.push_back(currentAtom);
                break;
                
            case 'f':
                currentAtom = new OscAtom(argv[i]->f);
                atoms.push_back(currentAtom);
                break;
                
            case 's':
                currentAtom = new OscAtom(&(argv[i]->s));
                atoms.push_back(currentAtom);
                break;
            }
        }
        
        for (vector<OscMessageHandlerContext>::iterator j = _oscGenericMessageHandlers.begin(); j != _oscGenericMessageHandlers.end(); j++) {
            OscMessageHandlerContext& ctxt = *j;
            
            if (ctxt.handler)
                ctxt.handler(path, &atoms, ctxt.userData);
        }
        
        iMessageHandlers = _oscMessageHandlers->find(path);
        if (iMessageHandlers == _oscMessageHandlers->end())
            break;
        
        handlerContext = (*_oscMessageHandlers)[path];
        if (handlerContext->handler == 0)
            break;
        
        
        handlerContext->handler(path, &atoms, handlerContext->userData);
    } while (0);

    while (atoms.size()) {
        currentAtom = atoms.front();
        atoms.pop_front();

        if (currentAtom != 0)
            delete currentAtom;
    }
}

extern "C" int OscControllerLoMethodHandler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data)
{
    OscController *controller;

    controller = (OscController *) user_data;
    controller->loMethodHandler(path, types, argv, argc, msg);

    return 0;
}

extern "C" void OscControllerLoErrorHandler(int num, const char *msg, const char *where)
{
//    _oscErrNo = num;
//   _oscErrMsg = msg;
//    _oscErrWhere = where;
}
