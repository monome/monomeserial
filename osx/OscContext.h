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
#ifndef __OSCCONTEXT_H__
#define __OSCCONTEXT_H__


#include <regex.h>
#include <string>
#include <list>
using namespace std;

typedef void * OscHostRef;

#include "OscAtom.h"

class OscContext
{
public:
    OscContext();
    OscContext(const string& host_address_string, const string& address_pattern_value_string);
    ~OscContext();

    void setHostAddressString(const string& host_address_string);
    void setAddressPatternValueString(const string& address_pattern_value_string);

    void setOscHostRef(OscHostRef oscHostRef);
    OscHostRef oscHostRef(void) const;

    const string& hostAddressString(void) const;
    const string& addressPatternValueString(void) const;
    const string& host(void) const;
    const string& port(void) const;
    const string& addressPattern(void) const;
    const string& valueString(void) const;
    const string& hostAddressAddressPatternHashKey(void) const;

    list<OscAtom *> *atoms(void) const;

    void setValueFromAtomicValueString(unsigned int index, const string& atomic_value_string);
    void setValue(unsigned int index, OscAtom& value);
    void setValue(unsigned int index, int value);
    void setValue(unsigned int index, float value);
    void setValue(unsigned int index, const string& value);

    static void convertValueStringToAtom(OscAtom *atom, const string& atomic_value_string);
    static void convertAtomToValueString(string& valueString, OscAtom *atom);
    static void convertAtomToValueString(char *valueString, size_t len, OscAtom *atom);

    bool isValidHostAddressString(void);
    bool isValidAddressPatternValueString(void);

private:
    void _createSubString(string& dest, const string& src, int start, int end);
    void _atomize(const string& atomic_value_string);

private:
    OscHostRef _oscHostRef;         // normally I don't like to leave this kind of data public, but we'll do because 
    list <OscAtom *> *_atoms;

    string _host_address_string;
    string _address_pattern_value_string;

    string _host;
    string _port;
    string _address_pattern;
    string _value_string;

    string _host_address_address_pattern_hash_key;

    bool _valid_host_address_string;
    bool _valid_address_pattern_value_string;

    regex_t _hostAddressStringRegex;
    regex_t _addressPatternValueStringRegex;
    regex_t _valueStringRegex;

    list <std::pair<int, OscAtom *> > _atomic_placeholders;
};

#endif
