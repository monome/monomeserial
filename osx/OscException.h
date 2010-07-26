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
#ifndef __OSCEXCEPTION_H__
#define __OSCEXCEPTION_H__

class OscException
{
public:
    OscException(int type, const char *message, bool fatal = false);

    int getType(void);
    const char *getMessage(void);
    bool isFatal(void);

public:
    enum {
        kOscExceptionTypeInvalidHostAddress,
        kOscExceptionTypeInvalidAddressPattern,
        kOscExceptionTypeInvalidOscAtomType,
        kOscExceptionTypeInvalidHostAddressString,
        kOscExceptionTypeInvalidHostString,
        kOscExceptionTypeInvalidPortString,
        kOscExceptionTypeInvalidAddressPatternValueString,
        kOscExceptionTypeInvalidValueString,
        kOscExceptionTypeLibloError
    } OscExceptionType;

private:
    int _type;
    char _message[255];
    bool _fatal;
};

#endif
