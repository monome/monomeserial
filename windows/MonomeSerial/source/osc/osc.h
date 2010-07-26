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


/* 
 *  Copyright (C) 2006, Joe Lake, monome.org
 * 
 *  This file is part of serialio.
 *
 *  serialio is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  serialio is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with serialio; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  $Id: osc.h,v. 1.1.1.1 2006/04/20 11:56:12
 */

#ifndef __OSC_H__
#define __OSC_H__

#include "../serial/message.h"
#include "../serial/message256.h"

#define kOscTypeTagInt                        "i"
#define kOscTypeTagFloat                      "f"
#define kOscTypeTagString                     "s"

#define kOscDefaultAddrPatternButtonPressSuffix  "/press"
#define kOscDefaultAddrPatternLedStateSuffix     "/led"
#define kOscDefaultAddrPatternLedIntensitySuffix "/intensity"
#define kOscDefaultAddrPatternLedTestSuffix      "/test"
#define kOscDefaultAddrPatternAdcEnableSuffix    "/adc_enable"
#define kOscDefaultAddrPatternShutdownSuffix     "/shutdown"
#define kOscDefaultAddrPatternLedClearSuffix     "/clear"
#define kOscDefaultAddrPatternAdcValueSuffix     "/adc"
#define kOscDefaultAddrPatternTiltValueSuffix     "/tilt"
#define kOscDefaultAddrPatternLedRowSuffix       "/led_row"
#define kOscDefaultAddrPatternLedColumnSuffix    "/led_col"
#define kOscDefaultAddrPatternEncEnableSuffix    "/enc_enable"
#define kOscDefaultAddrPatternEncValueSuffix     "/enc"
#define kOscDefaultAddrPatternLedFrameSuffix     "/frame"
#define kOscDefaultAddrPatternLed_ModeSuffix	"/led_mode"
#define kOscDefaultAddrPatternTilt_ModeSuffix	"/tiltmode"

#define kOscDefaultAddrPatternSystemPrefix       "/sys/prefix"
#define kOscDefaultAddrPatternSystemCable        "/sys/cable"
#define kOscDefaultAddrPatternSystemOffset       "/sys/offset"
#define kOscDefaultAddrPatternSystemLedIntensity "/sys/intensity"
#define kOscDefaultAddrPatternSystemLedTest      "/sys/test"
#define kOscDefaultAddrPatternSystemReport       "/sys/report"
#define kOscDefaultAddrPatternSystemNumDevices	 "/sys/devices"
#define kOscDefaultAddrPatternSystemDevType		 "/sys/type"
#define kOscDefaultAddrPatternSystemDevSerial	 "/sys/serial"

#define kOscDeviceType40h						"40h"
#define kOscDeviceType64						"64"
#define kOscDeviceType128						"128"
#define kOscDeviceType256						"256"

#define kOscDeviceOrientationUp					"up"
#define kOscDeviceOrientationRight				"right"
#define kOscDeviceOrientationBottom				"bottom"
#define kOscDeviceOrientationLeft				"left"

#define kOscDefaultTypeTagsButtonPress        kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsLedState           kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsLedIntensity       kOscTypeTagFloat
#define kOscDefaultTypeTagsLedTest            kOscTypeTagInt
#define kOscDefaultTypeTagsAdcEnable          kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsShutdown           kOscTypeTagInt
#define kOscDefaultTypeTagsLedClear           kOscTypeTagInt
#define kOscDefaultTypeTagsAdcValue           kOscTypeTagInt kOscTypeTagFloat
#define kOscDefaultTypeTagsLedRow             kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsLedColumn          kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsEncEnable          kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsEncValue           kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsLedFrame           kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsLedOffsetFrame     kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsTiltMode			  kOscTypeTagInt // added by dan

#define kOscDefaultTypeTagsSysPrefixAll          kOscTypeTagString
#define kOscDefaultTypeTagsSysPrefixSingle       kOscTypeTagInt kOscTypeTagString
#define kOscDefaultTypeTagsSysPrefixSingleSerial kOscTypeTagString kOscTypeTagString

#define kOscDefaultTypeTagsSysCableAll           kOscTypeTagString
#define kOscDefaultTypeTagsSysCableSingle        kOscTypeTagInt kOscTypeTagString
#define kOscDefaultTypeTagsSysCableSingleSerial  kOscTypeTagString kOscTypeTagString

#define kOscDefaultTypeTagsSysOffsetAll          kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsSysOffsetSingle       kOscTypeTagInt kOscTypeTagInt kOscTypeTagInt
#define kOscDefaultTypeTagsSysOffsetSingleSerial kOscTypeTagString kOscTypeTagInt kOscTypeTagInt

#define kOscDefaultTypeTagsSysLedIntensityAll			kOscDefaultTypeTagsLedIntensity
#define kOscDefaultTypeTagsSysLedIntensitySingle		kOscTypeTagInt kOscDefaultTypeTagsLedIntensity
#define kOscDefaultTypeTagsSysLedIntensitySingleSerial	kOscTypeTagString kOscDefaultTypeTagsLedIntensity

#define kOscDefaultTypeTagsSysLedTestAll          kOscDefaultTypeTagsLedTest
#define kOscDefaultTypeTagsSysLedTestSingle       kOscTypeTagInt kOscDefaultTypeTagsLedTest
#define kOscDefaultTypeTagsSysLedTestSingleSerial kOscTypeTagString kOscDefaultTypeTagsLedTest

#define kOscDefaultTypeTagsSysReportSingle       kOscTypeTagInt
#define kOscDefaultTypeTagsSysReportSingleSerial kOscTypeTagString

#endif

