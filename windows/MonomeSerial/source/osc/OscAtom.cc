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


#include "stdafx.h"
#include "OscAtom.h"

OscAtom::OscAtom()
{
    _type = kOscAtomTypeNullAtom;
    _i = 0;
    _f = 0.f;
}

OscAtom::OscAtom(int value)
{
    _type = kOscAtomTypeInt;
    _i = value;
    _f = 0.f;
}

OscAtom::OscAtom(float value)
{
    _type = kOscAtomTypeFloat;
    _f = value;
    _i = 0;
}

OscAtom::OscAtom(const string &value)
{
    _type = kOscAtomTypeString;
    _s = value;
    _i = 0;
    _f = 0.f;
}

OscAtom::OscAtom(const char *value)
{
    _type = kOscAtomTypeString;
    _s = string(value);
    _i = 0;
    _f = 0.f;
}

OscAtom::~OscAtom()
{
    ;
}

void 
OscAtom::setValue(OscAtom& atom)
{
    if (atom.isNullAtom())
        setNull();
    else {
        switch (atom.type()) {
        case kOscAtomTypeInt:
            setValue(atom.valueAsInt());
            break;

        case kOscAtomTypeFloat:
            setValue(atom.valueAsFloat());
            break;

        case kOscAtomTypeString:
            setValue(atom.valueAsString());
            break;
        }
    }
}

void 
OscAtom::setValue(int value)
{
    _type = kOscAtomTypeInt;
    _i = value;
}

void 
OscAtom::setValue(float value)
{
    _type = kOscAtomTypeFloat;
    _f = value;
}

void 
OscAtom::setValue(const string &value)
{
    _type = kOscAtomTypeString;
    _s = value;
}

void 
OscAtom::setValue(const char *value)
{
    _type = kOscAtomTypeString;
    _s = string(value);
}

void 
OscAtom::setNull(void)
{
    _type = kOscAtomTypeNullAtom;
}
    
int 
OscAtom::type(void) const
{
    return _type;
}

bool 
OscAtom::isInt(void) const
{
    if (_type == kOscAtomTypeInt)
        return true;

    if (_type == kOscAtomTypeFloat && (float)((int)_f) == _f)
        return true;
		
	return false;
}

bool 
OscAtom::isFloat(void) const
{
    return _type == kOscAtomTypeFloat;
}

bool 
OscAtom::isString(void) const
{
    return _type == kOscAtomTypeString;
}

int 
OscAtom::valueAsInt(void) const
{
    if (_type == kOscAtomTypeFloat)
        return (int)_f;
    else
        return _i;
}

float 
OscAtom::valueAsFloat(void) const
{
    return _f;
}

const string& 
OscAtom::valueAsString(void) const
{
    return _s;
}

const char *
OscAtom::valueAsCString(void) const
{
    return _s.c_str();
}

bool 
OscAtom::isNullAtom(void)
{
    return _type == kOscAtomTypeNullAtom;
}

bool operator ==(const OscAtom& atom1, const OscAtom& atom2)
{
    if (atom1.type() != atom2.type())
        return false;

    switch (atom1.type()) {
    case OscAtom::kOscAtomTypeNullAtom:
        return false;

    case OscAtom::kOscAtomTypeInt:
        return atom1.valueAsInt() == atom2.valueAsInt();

    case OscAtom::kOscAtomTypeFloat:
        return atom1.valueAsFloat() == atom2.valueAsFloat();
        
    case OscAtom::kOscAtomTypeString:
        return atom1.valueAsString() == atom2.valueAsString();

    default:
        return false;
    }
}

bool 
operator !=(const OscAtom& atom1, const OscAtom& atom2)
{
    if (atom1.type() != atom2.type())
        return true;

    switch (atom1.type()) {
    case OscAtom::kOscAtomTypeNullAtom:
        return true;

    case OscAtom::kOscAtomTypeInt:
        return atom1.valueAsInt() != atom2.valueAsInt();

    case OscAtom::kOscAtomTypeFloat:
        return atom1.valueAsFloat() != atom2.valueAsFloat();
        
    case OscAtom::kOscAtomTypeString:
        return atom1.valueAsString() != atom2.valueAsString();

    default:
        return true;
    }
}
