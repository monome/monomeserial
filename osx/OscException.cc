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
#include <stdio.h>
#include "OscException.h"

OscException::OscException(int type, const char *message, bool fatal)
{
    _type = type;
    _fatal = fatal;

    switch (_type) {
    case kOscExceptionTypeInvalidHostAddress:
        snprintf(_message, 255, "error: invalid host address %s.", message);
        break;

    case kOscExceptionTypeInvalidAddressPattern:
        snprintf(_message, 255, "error: invalid address pattern %s.", message);
        break;

    case kOscExceptionTypeInvalidOscAtomType:
        snprintf(_message, 255, "error: invalid osc atom type.");

    case kOscExceptionTypeInvalidHostAddressString:
        snprintf(_message, 255, "error: invalid osc host address and port string %s.", message);
        break;
    
    case kOscExceptionTypeInvalidHostString:
        snprintf(_message, 255, "error: invalid host %s.", message);
        break;

    case kOscExceptionTypeInvalidPortString:
        snprintf(_message, 255, "error: invalid port %s.", message);

    case kOscExceptionTypeInvalidAddressPatternValueString:
        snprintf(_message, 255, "error: invalid address pattern value string %s", message);
        break;

    case kOscExceptionTypeInvalidValueString:
        snprintf(_message, 255, "error: invalid value string %s.", message);
        break;
        
    case kOscExceptionTypeLibloError:
        snprintf(_message, 255, "liblo error raised as OscException: %s", message);
        break;

    default:
        snprintf(_message, 255, "error: unknown exception type: %s", message);
        break;
    }
}

int OscException::getType(void)
{
    return _type;
}

const char *OscException::getMessage(void)
{
    return _message;
}

bool OscException::isFatal(void)
{
    return _fatal;
}
