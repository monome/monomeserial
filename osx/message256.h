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
 
#ifndef __MESSAGE256_H__
#define __MESSAGE256_H__

#include "message.h"

#ifdef __cplusplus
extern "C"
{
#endif


typedef enum {
     kMessageType_256_keydown = 0,
     kMessageType_256_keyup,
	 kMessageType_256_led_on,
	 kMessageType_256_led_off,
	 kMessageType_256_led_row1,
	 kMessageType_256_led_col1,
	 kMessageType_256_led_row2,
	 kMessageType_256_led_col2,
	 kMessageType_256_led_frame,
	  kMessageType_256_clear,
	 kMessageType_256_intensity,
	 kMessageType_256_mode,
	 kMessageType_256_activatePort,
	 kMessageType_256_deactivatePort,
	 kMessageType_256_auxiliaryInput, //14
    kMessage_256_NumTypes //15
} e256MessageTypes;

//this doesn't seem to ever be used -- xkn 07.21.2010
const int kMessageSize_256[15] = {2,2,2,2, 2,2,3,3,
								  9,1,1,1, 1,1,2};


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
} t_256_frame_message;

typedef struct{
uint8 data0;
} t_256_1byte_message;

typedef struct{
uint8 data0;
uint8 data1;
uint8 data2;
} t_256_3byte_message;



#define messageGet_256_Type(message)                ((message).data0 >> 4)
#define messageGet_256_ButtonX(message)             ((message).data1 >> 4)
#define messageGet_256_ButtonY(message)             ((message).data1 & 0xF)
#define messageGet_256_WhichPort(message)             ((message).data0 & 0xF)
#define messageGet_256_AuxData(message)        ((message).data1)
#define messageGet_256_WhichPort(message)             ((message).data0 & 0xF)

void messagePack_256_led_on(t_message *message, uint8 x, uint8 y);
void messagePack_256_led_off(t_message *message, uint8 x, uint8 y);
void messagePack_256_led_row1(t_message *message, uint8 rowIndex, uint8 state);
void messagePack_256_led_col1(t_message *message, uint8 columnIndex, uint8 state);
void messagePack_256_led_row2(t_256_3byte_message *message, uint8 rowIndex, uint8 state, uint8 stateB); //B is (row data 8-15)
void messagePack_256_led_col2(t_256_3byte_message *message, uint8 columnIndex, uint8 state, uint8 stateB);//B is (col data 8-15)
void messagePack_256_led_frame(t_256_frame_message *message, uint8 quadrant, uint8 row0,uint8 row1,uint8 row2,uint8 row3,uint8 row4,uint8 row5,uint8 row6,uint8 row7);
void messagePack_256_clear(t_256_1byte_message *message, uint8 state);
void messagePack_256_intensity(t_256_1byte_message *message, uint8 intensity);
void messagePack_256_mode(t_256_1byte_message *message, uint8 mode);
void messagePack_256_activatePort(t_256_1byte_message *message, uint8 whichport);
void messagePack_256_deactivatePort(t_256_1byte_message *message, uint8 whichport);
//void messagePack_256_deactivatePort(t_256_1byte_message *message, uint8 whichport); //64 actually
//void messagePack_256_auxiliaryInput(t_message *message, uint8 whichport, uint8 data);

	
#ifdef __cplusplus
}
#endif

#endif
