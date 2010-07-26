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

#include "messageMK.h"
	
	/*
			
void messagePack_mk_keydown(t_message *message,  uint8 x, uint8 y)
{
    message->data0 = (kMessageType_mk_keydown << 4);
    message->data1 = (x << 4) | y;
}

void messagePack_mk_keyup(t_message *message,  uint8 x, uint8 y)
{
    message->data0 = (kMessageType_mk_keyup << 4);
    message->data1 = (x << 4) | y;
}
*/

void messagePack_mk_led_on(t_message *message,  uint8 x, uint8 y)
{
    message->data0 = (kMessageType_mk_led_on << 4); 
    message->data1 = (x << 4) | y;
}

void messagePack_mk_led_off(t_message *message,  uint8 x, uint8 y)
{
    message->data0 = (kMessageType_mk_led_off << 4) ; 
    message->data1 = (x << 4) | y;
	
	
}

void messagePack_mk_led_row1(t_message *message, uint8 rowIndex, uint8 state)
{
    message->data0 = (kMessageType_mk_led_row1 << 4) | (rowIndex & 0xF);
    message->data1 = state;
}

void messagePack_mk_led_col1(t_message *message, uint8 columnIndex, uint8 state)
{
    message->data0 = (kMessageType_mk_led_col1 << 4) | (columnIndex & 0xF);
    message->data1 = state;
}

void messagePack_mk_led_row2(t_mk_3byte_message *message, uint8 rowIndex, uint8 state, uint8 stateB)
{
    message->data0 = (kMessageType_mk_led_row2 << 4) | (rowIndex & 0xF);
    message->data1 = state;
	message->data2 = stateB;
}

void messagePack_mk_led_col2(t_mk_3byte_message *message, uint8 columnIndex, uint8 state, uint8 stateB) //3bytes
{
    message->data0 = (kMessageType_mk_led_col2 << 4) | (columnIndex & 0xF);
    message->data1 = state;
	message->data2 = stateB;
}

void messagePack_mk_led_frame(t_mk_frame_message *message, uint8 quadrant, uint8 row0,uint8 row1,uint8 row2,uint8 row3,uint8 row4,uint8 row5,uint8 row6,uint8 row7)
{
/* 
bytes:		9
format:		iiii..qq aaaaaaaa bbbbbbbb cccccccc dddddddd eeeeeeee ffffffff gggggggg hhhhhhhh
			i (message id) = 8
			q (quadrant) = 0-3 (2 bits)
			a-h (row data 0-7, per row) = 0-255 (8 bits)
encode:		byte 0 = ((id) << 4) | q = 128 + q
			byte 1,2,3,4,5,6,7,8 = a,b,c,d,e,f,g,h
note:		quadrants are from top left to bottom right, as shown:
			0 1
			2 3
*/
 message->data0 = (kMessageType_mk_led_frame << 4) |  (quadrant & 0xF);
message->data1 = row0;
message->data2 = row1;
message->data3 = row2;
message->data4 = row3;
message->data5 = row4;
message->data6 = row5;
message->data7 = row6;
message->data8 = row7;
}

void messagePack_mk_clear(t_mk_1byte_message *message, uint8 state) //1 byte
{
    message->data0 = (kMessageType_mk_clear << 4) |  (state ? on : off);
}


void messagePack_mk_intensity(t_mk_1byte_message *message, uint8 intensity) //1 byte
{
    message->data0 = (kMessageType_mk_intensity << 4) | (intensity & 0xF); 
	
}

void messagePack_mk_mode(t_mk_1byte_message *message, uint8 mode) //1 byte
{
  message->data0 = (kMessageType_mk_mode << 4) | (mode & 0xF);
}

void messagePack_mk_grids(t_mk_1byte_message *message, uint8 numGrids)
{
	message->data0 = (kMessageType_mk_grids << 4) | (numGrids & 0x0F);
}

void messagePack_mk_auxin(t_mk_3byte_message *message, uint8 subOp, uint8 portFmask, uint8 portAmask)
{
	message->data0 = (kMessageType_mk_auxin << 4) | (subOp & 0x0F);
	message->data1 = portFmask;
	message->data2 = portAmask;
}

void messagePack_mk_auxout(t_mk_3byte_message *message, uint8 subOp, uint8 portFmask, uint8 portAmask)
{
	message->data0 = (kMessageType_mk_auxout << 4) | (subOp & 0x0F);
	message->data1 = portFmask;
	message->data2 = portAmask;
}