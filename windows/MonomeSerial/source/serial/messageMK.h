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
 
 //this file is to be used or cloned for all new types (64/128/256) 
 // as they essentially share the same serial protocol
 
#ifndef __MESSAGE_MK_H__
#define __MESSAGE_MK_H__

#include "message.h"

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum {
     kMessageType_mk_keydown = 0,
     kMessageType_mk_keyup,
	 kMessageType_mk_led_on,
	 kMessageType_mk_led_off,
	 kMessageType_mk_led_row1,
	 kMessageType_mk_led_col1,
	 kMessageType_mk_led_row2,
	 kMessageType_mk_led_col2,
	 kMessageType_mk_led_frame,
	 kMessageType_mk_clear,
	 kMessageType_mk_intensity,
	 kMessageType_mk_mode,
	 kMessageType_mk_grids,
	 kMessageType_mk_auxin,
	 kMessageType_mk_auxout, //14
	 
    kMessage_mk_NumTypes //15
} eMkMessageTypes;
	
typedef enum {
	kMessage_AuxIn_Version = 0,
	kMessage_AuxIn_Enable,
	kMessage_AuxIn_Direction,
	kMessage_AuxIn_State,
	kMessage_AuxIn_NumTypes
} eMkMessage_AuxIn_Types;

typedef enum {
	kMessage_AuxOut_Version = 0,
	kMessage_AuxOut_Analog,
	kMessage_AuxOut_Digital,
	kMessage_AuxOut_Encoder,
	kMessage_AuxOut_NumTypes
} eMkMessage_AuxOut_Types;
	
//as noted in message256.h, this doesn't seem to be used anywhere...
const int kMessageSize_mk[15] = {2,2,2,2, 2,2,3,3,
								  9,1,1,1, 1,3,3};


typedef struct {
    uint8 data0;
    uint8 data1;
    uint8 data2;
    uint8 data3;
	uint8 data4;
    uint8 data5;
	uint8 data6;
    uint8 data7;
	uint8 data8;
} t_mk_frame_message;

typedef struct{
uint8 data0;
} t_mk_1byte_message;

typedef struct{
uint8 data0;
uint8 data1;
uint8 data2;
} t_mk_3byte_message;



#define messageGet_mk_Type(message)                ((message).data0 >> 4)
#define messageGet_mk_ButtonX(message)             ((message).data1 >> 4)
#define messageGet_mk_ButtonY(message)             ((message).data1 & 0xF)
#define messageGet_mk_WhichPort(message)           ((message).data0 & 0xF)
#define messageGet_mk_AuxData(message)             ((message).data1)
	
#define messageGet_mk_AuxOutType(message)          ((message).data0 & 0xF)
#define messageGet_mk_NumGrids(message)            ((message).data0 &0x0F)
#define messageGet_mk_Version(message)             ((message).data1)
#define messageGet_mk_AnalogPort(message)          ((message).data1)
#define messageGet_mk_AnalogValue(message)         ((message).data2)
#define messageGet_mk_DigitalNumber(message)       ((message).data1)
#define messageGet_mk_DigitalState(message)        ((message).data2)
#define messageGet_mk_EncoderNumber(message)       ((message).data1)
#define messageGet_mk_EncoderChange(message)       ((message).data2)
	
void messagePack_mk_led_on(t_message *message, uint8 x, uint8 y);
void messagePack_mk_led_off(t_message *message, uint8 x, uint8 y);
void messagePack_mk_led_row1(t_message *message, uint8 rowIndex, uint8 state);
void messagePack_mk_led_col1(t_message *message, uint8 columnIndex, uint8 state);
void messagePack_mk_led_row2(t_mk_3byte_message *message, uint8 rowIndex, uint8 state, uint8 stateB); //B is (row data 8-15)
void messagePack_mk_led_col2(t_mk_3byte_message *message, uint8 columnIndex, uint8 state, uint8 stateB);//B is (col data 8-15)
void messagePack_mk_led_frame(t_mk_frame_message *message, uint8 quadrant, uint8 row0,uint8 row1,uint8 row2,uint8 row3,uint8 row4,uint8 row5,uint8 row6,uint8 row7);
void messagePack_mk_clear(t_mk_1byte_message *message, uint8 state);
void messagePack_mk_intensity(t_mk_1byte_message *message, uint8 intensity);
void messagePack_mk_mode(t_mk_1byte_message *message, uint8 mode);
void messagePack_mk_grids(t_mk_1byte_message *message, uint8 numGrids);
void messagePack_mk_auxin(t_mk_3byte_message *message, uint8 subOp, uint8 portFmask, uint8 portAmask);
void messagePack_mk_auxout(t_mk_3byte_message *message, uint8 subOp, uint8 portFmask, uint8 portAmask);
	
//void messagePack_mk_activatePort(t_mk_1byte_message *message, uint8 whichport);
//void messagePack_mk_deactivatePort(t_mk_1byte_message *message, uint8 whichport);
//void messagePack_mk_deactivatePort(t_mk_1byte_message *message, uint8 whichport); //64 actually
//void messagePack_mk_auxiliaryInput(t_message *message, uint8 whichport, uint8 data);

	
#ifdef __cplusplus
}
#endif

#endif
