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
#ifndef __OSCHOSTADDRESS_H__
#define __OSCHOSTADDRESS_H__

#include <lo/lo.h>
#include <string>
#include <list>
using namespace std;

class OscHostAddress {
public:
    OscHostAddress(const string& host, const string& port);
    ~OscHostAddress();

    void retain(void);
    void release(void);
    int getRetainCount(void);
    const string& getHostString(void);
    lo_address getHostAddress(void);

private:
    string _hostString;
    lo_address _hostAddress;
    int _retainCount;
};

#endif
