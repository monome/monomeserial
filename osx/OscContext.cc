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
#include <string>
#include <iostream>
#include <sstream>
#include <list>
using namespace std;

#include "OscContext.h"
#include "OscException.h"

#define kOscContextHostAddressStringRegex            "^([^:]+):([0-9]+)$"
#define kOscContextAddressPatternValueStringRegex    "^(/[^[:space:]]*) (.*)$"
#define kOscContextAtomicValueStringRegex            "(\"[^\"]+\"|'[^']+'|[^[:space:]]+)"

OscContext::OscContext()
{
    _oscHostRef                         = 0;
    _valid_host_address_string          = false;
    _valid_address_pattern_value_string = false;

    if ((regcomp(&_hostAddressStringRegex, kOscContextHostAddressStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextHostAddressStringRegex << endl;
        return;
    }

    if ((regcomp(&_addressPatternValueStringRegex, kOscContextAddressPatternValueStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextAddressPatternValueStringRegex << endl;
        return;
    }

    if ((regcomp(&_valueStringRegex, kOscContextAtomicValueStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextAtomicValueStringRegex << endl;
        return;
    }

    _atoms = new list<OscAtom *>;
}

OscContext::OscContext(const string& host_address_string, const string& address_pattern_value_string)
{
    _oscHostRef                         = 0;
    _valid_host_address_string          = true;
    _valid_address_pattern_value_string = true;

    if ((regcomp(&_hostAddressStringRegex, kOscContextHostAddressStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextHostAddressStringRegex << endl;
        return;
    }

    if ((regcomp(&_addressPatternValueStringRegex, kOscContextAddressPatternValueStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextAddressPatternValueStringRegex << endl;
        return;
    }

    if ((regcomp(&_valueStringRegex, kOscContextAtomicValueStringRegex, REG_EXTENDED)) != 0) {
        cout << "error: invalid regular expression " << kOscContextAtomicValueStringRegex << endl;
        return;
    }

    _atoms = new list<OscAtom *>;

    setHostAddressString(host_address_string);
    setAddressPatternValueString(address_pattern_value_string);
}

OscContext::~OscContext()
{
    OscAtom *atom;

    while (_atoms->size()) {
        atom = _atoms->front();
        if (atom != 0) delete atom;
        _atoms->pop_front();
    }

    delete _atoms;
}

void 
OscContext::setHostAddressString(const string& host_address_string)
{
    regmatch_t match[3];

    _valid_host_address_string = true;

    _host_address_string = host_address_string;
    _host = "";
    _port = "";

    if ((regexec(&_hostAddressStringRegex, _host_address_string.c_str(), 3, match, 0)) != 0) {
        _valid_host_address_string = false;
        _host_address_address_pattern_hash_key = "";
        throw OscException(OscException::kOscExceptionTypeInvalidHostAddressString, _host_address_string.c_str());
    }

    _createSubString(_host, _host_address_string, (int)match[1].rm_so, (int)match[1].rm_eo);
    _createSubString(_port, _host_address_string, (int)match[2].rm_so, (int)match[2].rm_eo);

    _host_address_address_pattern_hash_key = _host_address_string + _address_pattern_value_string;
}

void 
OscContext::setAddressPatternValueString(const string& address_pattern_value_string)
{
    regmatch_t match[3];
    OscAtom *atom;
    string current_value, current_tokenized_value;
    list<string> tokenized_value_strings;
    list<string>::iterator i;

    _valid_address_pattern_value_string = true;

    _address_pattern = "";
    _value_string = "";

    while (_atoms->size()) {
        atom = _atoms->front();

        if (atom != 0) {
            delete atom;
        }

        _atoms->pop_front();
    }

    _atomic_placeholders.clear();

    _address_pattern_value_string = address_pattern_value_string;

    if ((regexec(&_addressPatternValueStringRegex, _address_pattern_value_string.c_str(), 3, match, 0)) != 0) {
        _valid_address_pattern_value_string = false;
        _host_address_address_pattern_hash_key = "";
        throw OscException(OscException::kOscExceptionTypeInvalidAddressPatternValueString, _address_pattern_value_string.c_str());
    }

    _createSubString(_address_pattern, _address_pattern_value_string, (int)match[1].rm_so, (int)match[1].rm_eo);
    _createSubString(_value_string, _address_pattern_value_string, (int)match[2].rm_so, (int)match[2].rm_eo);

    if (_value_string.length()) {

        current_value = _value_string;

        while (current_value.length()) {
            if ((regexec(&_valueStringRegex, current_value.c_str(), 3, match, 0)) != 0) {
                _valid_address_pattern_value_string = false;
                _host_address_address_pattern_hash_key = "";
                throw OscException(OscException::kOscExceptionTypeInvalidValueString, current_value.c_str());
            }

            _createSubString(current_tokenized_value, current_value, (int)match[1].rm_so, (int)match[1].rm_eo);
            tokenized_value_strings.push_back(current_tokenized_value);

            current_value = current_value.substr((int)match[1].rm_eo, current_value.length() - (int)match[1].rm_eo);
        }

        while (tokenized_value_strings.size()) {
            current_value = tokenized_value_strings.front();
            tokenized_value_strings.pop_front();

            _atomize(current_value);
        }
    }
    
    _host_address_address_pattern_hash_key = _host_address_string + _address_pattern_value_string;
}            

void 
OscContext::setOscHostRef(OscHostRef oscHostRef)
{
    _oscHostRef = oscHostRef;
}

OscHostRef 
OscContext::oscHostRef(void) const
{
    return _oscHostRef;
}

const string& 
OscContext::hostAddressString(void) const
{
    return _host_address_string;
}

const string& 
OscContext::addressPatternValueString(void) const
{
    return _address_pattern_value_string;
}

const string& 
OscContext::host(void) const
{
    return _host;
}

const string& 
OscContext::port(void) const
{
    return _port;
}

const string& 
OscContext::addressPattern(void) const
{
    return _address_pattern;
}

const string& 
OscContext::valueString(void) const
{
    return _value_string;
}

const string& 
OscContext::hostAddressAddressPatternHashKey(void) const
{
    return _host_address_address_pattern_hash_key;
}

list<OscAtom *> *
OscContext::atoms(void) const
{
    return _atoms;
}

void 
OscContext::setValueFromAtomicValueString(unsigned int index, const string& atomic_value_string) 
{
    unsigned int p;
    OscAtom *atom;
    list<std::pair<int, OscAtom *> >::iterator i;

    for (i = _atomic_placeholders.begin(); i != _atomic_placeholders.end(); ++i) {
        p = (*i).first;
        atom = (*i).second;

        if (p == index)
            convertValueStringToAtom(atom, atomic_value_string);
    }       
}

void
OscContext::setValue(unsigned int index, OscAtom& value)
{
    unsigned int p;
    OscAtom *atom;
    list<std::pair<int, OscAtom *> >::iterator i;

    for (i = _atomic_placeholders.begin(); i != _atomic_placeholders.end(); ++i) {
        p = (*i).first;
        atom = (*i).second;

        if (p == index)
            *atom = value;
    }       
}

void 
OscContext::setValue(unsigned int index, int value)
{
    unsigned int p;
    OscAtom *atom;
    list<std::pair<int, OscAtom *> >::iterator i;

    for (i = _atomic_placeholders.begin(); i != _atomic_placeholders.end(); ++i) {
        p = (*i).first;
        atom = (*i).second;

        if (p == index)
            atom->setValue(value);
    }       
}

void 
OscContext::setValue(unsigned int index, float value)
{
    unsigned int p;
    OscAtom *atom;
    list<std::pair<int, OscAtom *> >::iterator i;

    for (i = _atomic_placeholders.begin(); i != _atomic_placeholders.end(); ++i) {
        p = (*i).first;
        atom = (*i).second;

        if (p == index)
            atom->setValue(value);
    }       
}

void 
OscContext::setValue(unsigned int index, const string& value)
{
    unsigned int p;
    OscAtom *atom;
    list<std::pair<int, OscAtom *> >::iterator i;

    for (i = _atomic_placeholders.begin(); i != _atomic_placeholders.end(); ++i) {
        p = (*i).first;
        atom = (*i).second;

        if (p == index) 
            atom->setValue(value);
    }       
}


void 
OscContext::convertValueStringToAtom(OscAtom *atom, const string& atomic_value_string)
{
    char *endptr;

    if (atomic_value_string.length() == 0) {
        atom->setNull();
        return;
    }

    endptr = 0;
    atom->setValue((int)strtol(atomic_value_string.c_str(), &endptr, 0));

    if (endptr == atomic_value_string.c_str() + atomic_value_string.length())
        return;
   

    endptr = 0;
    atom->setValue((float)strtof(atomic_value_string.c_str(), &endptr));
        
    if (endptr == atomic_value_string.c_str() + atomic_value_string.length())
        return;

    atom->setValue(atomic_value_string);
}

void 
OscContext::convertAtomToValueString(string& valueString, OscAtom *atom)
{
    ostringstream valueStream;

    switch (atom->type()) {
    case OscAtom::kOscAtomTypeInt:
        valueStream << atom->valueAsInt() << endl;;
        break;

    case OscAtom::kOscAtomTypeFloat:
        valueStream << atom->valueAsFloat() << endl;
        break;

    case OscAtom::kOscAtomTypeString:
        valueStream << atom->valueAsString() << endl;
        break;

    default:
        valueStream << "";
    }

    valueString = valueStream.str();
}

void
OscContext::convertAtomToValueString(char *valueString, size_t len, OscAtom *atom)
{
    switch (atom->type()) {
    case OscAtom::kOscAtomTypeInt:
        snprintf(valueString, len, "%d", atom->valueAsInt());
        break;

    case OscAtom::kOscAtomTypeFloat:
        snprintf(valueString, len, "%f", atom->valueAsFloat());
        break;

    case OscAtom::kOscAtomTypeString:
        snprintf(valueString, len, "%s", atom->valueAsCString());
        break;

    default:
        *valueString = 0;
    }
}

bool 
OscContext::isValidHostAddressString(void)
{
    return _valid_host_address_string;
}

bool 
OscContext::isValidAddressPatternValueString(void)
{
    return _valid_address_pattern_value_string;
}

void 
OscContext::_createSubString(string& dest, const string& src, int start, int end)
{
    dest = src.substr(start, end - start);
}

void 
OscContext::_atomize(const string& atomic_value_string)
{
    OscAtom *atom;
    char *endptr;

    atom = new OscAtom();

    do {
        endptr = 0;
        atom->setValue((int)strtol(atomic_value_string.c_str(), &endptr, 0));

        if (endptr == atomic_value_string.c_str() + atomic_value_string.length())
            break;

        endptr = 0;
        atom->setValue((float)strtof(atomic_value_string.c_str(), &endptr));
        
        if (endptr == atomic_value_string.c_str() + atomic_value_string.length())
            break;

        if (atomic_value_string[0] == '$') {
            endptr = 0;
            atom->setValue((int)strtol(atomic_value_string.c_str() + 1, &endptr, 0));
            
            if (endptr == atomic_value_string.c_str() + atomic_value_string.length()) {
                _atomic_placeholders.push_back(std::pair<int, OscAtom *>::pair(atom->valueAsInt(), atom));
                break;
            }
        }
        
        atom->setValue(atomic_value_string);
    } while (0);

    _atoms->push_back(atom);
}

