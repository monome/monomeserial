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


#ifndef __OSCATOM_H__
#define __OSCATOM_H__

#include <string>
using namespace std;

class OscAtomIncompatibleTypeException
{
public:
    OscAtomIncompatibleTypeException(string message) : _message(message) {}
    string message() { return _message; }

private:
    string _message;
};

class OscAtom {
public:
    OscAtom();
    OscAtom(int value);
    OscAtom(float value);
    OscAtom(const string &value);
    OscAtom(const char *value);
    ~OscAtom();

    void setValue(OscAtom& atom);
    void setValue(int value);
    void setValue(float value);
    void setValue(const string &value);
    void setValue(const char *value);
    void setNull(void);
    
    int type(void) const;
    bool isInt(void) const;
    bool isFloat(void) const;
    bool isString(void) const;

    int valueAsInt(void) const;
    float valueAsFloat(void) const;
    const string& valueAsString(void) const;
    const char *valueAsCString(void) const;

    bool isNullAtom(void);

    friend bool operator ==(const OscAtom& atom1, const OscAtom& atom2);
    friend bool operator !=(const OscAtom& atom1, const OscAtom& atom2);

public:
    enum {
        kOscAtomTypeNullAtom,
        kOscAtomTypeInt,
        kOscAtomTypeFloat,
        kOscAtomTypeString
    };

private:
    int _type;
    int _i;
    float _f;
    string _s;
};

#endif
