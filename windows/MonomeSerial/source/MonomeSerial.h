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

// this GUI file is created for Windows only by Daniel Battaglia


// MonomeSerial.h : main header file for the PROJECT_NAME application
//

#ifndef __MONOMESERIAL_H__
#define __MONOMESERIAL_H__

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "../res/resource.h"		// main symbols


// CMonomeSerialApp:
// See MonomeSerial.cpp for the implementation of this class
//

class CMonomeSerialApp : public CWinApp
{
public:
	CMonomeSerialApp();

// Overrides
	public:
	virtual BOOL InitInstance();


// Implementation

	DECLARE_MESSAGE_MAP()
};


extern CMonomeSerialApp theApp;

#endif