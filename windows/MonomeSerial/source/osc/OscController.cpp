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


#include "../stdafx.h"

#include "OscException.h"
#include "OscController.h"

extern "C" int OscControllerLoMethodHandler(const osc::ReceivedMessage &receivedMessage, void *user_data);


OscController::OscController() : _listenAddress("127.0.0.1")
{
    _hostAddresses = new hash_map<string, OscHostAddress *>;
    _listenAddresses = new hash_map<string, OscListenAddress *>;
}

OscController::~OscController()
{
	// clear hosts
    hash_map<string, OscHostAddress *>::iterator iHostAddresses;
    OscHostAddress *currentHostAddress;

    iHostAddresses = _hostAddresses->begin();
    while (iHostAddresses != _hostAddresses->end()) {
        currentHostAddress = ((std::pair<string, OscHostAddress *>)*iHostAddresses).second;
		if (currentHostAddress != 0) {
			delete currentHostAddress;
			currentHostAddress = 0;
		}
        iHostAddresses++;
    }

    _hostAddresses->clear();

	if (_hostAddresses) {
		delete _hostAddresses;
		_hostAddresses = 0;
	}

	// clear listeners
	hash_map<string, OscListenAddress *>::iterator iListenAddresses;
    OscListenAddress *currentListenAddress;

    iListenAddresses = _listenAddresses->begin();
    while (iListenAddresses != _listenAddresses->end()) {
        currentListenAddress = ((std::pair<string, OscListenAddress *>)*iListenAddresses).second;
		if (currentListenAddress != 0) {
			delete currentListenAddress;
			currentListenAddress = 0;
		}
        iListenAddresses++;
    }

    _listenAddresses->clear();

	if (_listenAddresses) {
		delete _listenAddresses;
		_listenAddresses = 0;
	}
}

OscListenRef OscController::getOscListenRef(const string& port, bool retain)
{
	const string host;
    string hostString = "127.0.0.1:";
    OscListenAddress *listenAddress;
    hash_map<string, OscListenAddress *>::iterator i;

    hostString += port;

    i = _listenAddresses->find(hostString);

    if (i != _listenAddresses->end()) {
		if (retain) {
			(*_listenAddresses)[hostString]->retain();
		}
        
        return (OscListenRef)((*_listenAddresses)[hostString]);
    }

    listenAddress = new OscListenAddress(port, OscControllerLoMethodHandler, this);
	listenAddress->retain(); // always retain on creation

    (*_listenAddresses)[hostString] = listenAddress;

    return (OscListenRef) listenAddress;
}

void OscController::releaseOscListenRef(OscListenRef listenRef)
{
    if (listenRef == 0)
        return;

    string hostString;
    OscListenAddress *listenAddress;

    listenAddress = (OscListenAddress *)listenRef;
    hostString = listenAddress->getHostString();

    listenAddress->release();

    if (listenAddress->getRetainCount() == 0) {
        hash_map<string, OscListenAddress *>::iterator i = _listenAddresses->find(hostString);

        if (i != _listenAddresses->end()) {
            hostString = ((std::pair<string, OscListenAddress *>)*i).first;
            _listenAddresses->erase(i);

			if (listenAddress != 0) {
				delete listenAddress;
				listenAddress = 0;
			}
        }
    }
}

OscHostRef OscController::getOscHostRef(const string& host, const string& port, bool retain)
{
    string hostString;
    OscHostAddress *hostAddress;
    hash_map<string, OscHostAddress *>::iterator i;

    hostString = host + ":" + port;

    i = _hostAddresses->find(hostString);

    if (i != _hostAddresses->end()) {
		if (retain) {
			(*_hostAddresses)[hostString]->retain();
		}
        
        return (OscHostRef)((*_hostAddresses)[hostString]);
    }

    hostAddress = new OscHostAddress(host, port);
	hostAddress->retain(); // always retain on creation

    (*_hostAddresses)[hostString] = hostAddress;

    return (OscHostRef) hostAddress;
}

void OscController::releaseOscHostRef(OscHostRef hostRef)
{
    if (hostRef == 0)
        return;

    string hostString;
    OscHostAddress *hostAddress;

    hostAddress = (OscHostAddress *)hostRef;
    hostString = hostAddress->getHostString();

    hostAddress->release();

    if (hostAddress->getRetainCount() == 0) {
        hash_map<string, OscHostAddress *>::iterator i = _hostAddresses->find(hostString);

        if (i != _hostAddresses->end()) {
            hostString = ((std::pair<string, OscHostAddress *>)*i).first;
            _hostAddresses->erase(i);

			if (hostAddress != 0) {
				delete hostAddress;
				hostAddress = 0;
			}
        }
    }
}

OscHostAddress *OscController::_getOscHostAddress(const string& hostString)
{
    OscHostAddress *hostAddress = (*_hostAddresses)[hostString];
    return hostAddress;
}


void OscController::send(OscHostRef hostRef, const osc::OutboundPacketStream &stream)
{
    if (hostRef == 0)
        return;

	OscHostAddress *hostAddress;
    hostAddress = static_cast<OscHostAddress*>(hostRef);

    lo_send_message(hostAddress->getHostAddress(), stream.Data(), stream.Size());
}

/*

OscListenRef OscController::startListening(const string& port)
{
	string listenString;
    OscListenAddress *listenAddress;
    hash_map<string, OscListenAddress *>::iterator i;

    listenString = _listenAddress + ":" + port;

    i = _listenAddresses->find(listenString);

    if (i != _listenAddresses->end()) {
        (*_listenAddresses)[listenString]->retain();
        
        return (OscListenRef)((*_listenAddresses)[listenString]);
    }

    listenAddress = new OscListenAddress(port, OscControllerLoMethodHandler, this);
    listenAddress->retain();

    (*_listenAddresses)[listenString] = listenAddress;

    return (OscListenRef) listenAddress;
}

void OscController::stopListening(OscListenRef oscListenRef)
{
	if (oscListenRef == 0)
        return;

	string listenString;
    OscListenAddress *listenAddress;

    listenAddress = static_cast<OscListenAddress*>(oscListenRef);
    listenString = listenAddress->getHostString();

    listenAddress->release();

    if (listenAddress->getRetainCount() == 0) {
        hash_map<string, OscListenAddress *>::iterator i = _listenAddresses->find(listenString);

        if (i != _listenAddresses->end()) {
            listenString = ((std::pair<string, OscListenAddress *>)*i).first;
            _listenAddresses->erase(i);

            if (listenAddress != 0) delete listenAddress;
        }
    }
}

void OscController::stopListening(void)
{
	hash_map<string, OscListenAddress *>::iterator i;
	for (i = _listenAddresses->begin(); i != _listenAddresses->end(); ++i) {
		if (i->second == static_cast<OscListenAddress*>(NULL)) {
			continue;
		}
		while (i->second->getRetainCount() > 1) {
			this->stopListening(i->second);
		}
	}
}
*/

void 
OscController::addGenericOscMessageHandler(OscMessageHandler handler, void *userData)
{
    if (handler == 0)
        return;

    OscMessageHandlerContext context = { handler, userData };

    _oscGenericMessageHandlers.push_back(context);
}


void OscController::loMethodHandler(const osc::ReceivedMessage &receivedMessage)
{
    hash_map<string, OscMessageHandlerContext *>::iterator iMessageHandlers;
    OscMessageHandlerContext *handlerContext;

    for (vector<OscMessageHandlerContext>::iterator j = _oscGenericMessageHandlers.begin(); j != _oscGenericMessageHandlers.end(); j++) {
        OscMessageHandlerContext& ctxt = *j;
        
		if (ctxt.handler)
            ctxt.handler(receivedMessage, ctxt.userData);
    }
}


int OscControllerLoMethodHandler(const osc::ReceivedMessage &receivedMessage, void *user_data)
{
    OscController *controller;

    controller = (OscController *)user_data;
    controller->loMethodHandler(receivedMessage);

    return 0;
}

