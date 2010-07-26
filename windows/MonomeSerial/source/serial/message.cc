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


#include "../stdafx.h"
#include "message.h"

//why on earth would we send poor monome a press
//void messagePackButtonPress(t_message *message, uint8 state, uint8 x, uint8 y)
//{
//    message->data0 = (kMessageTypeButtonPress << 4) | (state ? on : off);
//    message->data1 = (x << 4) | y;
//}

#ifdef __cplusplus
extern "C" {
#endif

void messagePackAdcVal(t_message *message, uint8 port, uint16 val)
{
    message->data0 = (kMessageTypeAdcVal << 4) | ((port << 2) & 0x0C) | ((val >> 8) & 0x03);
    message->data1 = (uint8)val;
}

void messagePackLedStateChange(t_message *message, uint8 state, uint8 x, uint8 y)
{
    message->data0 = (kMessageTypeLedStateChange << 4) | state; 
    message->data1 = (x << 4) | y;
}

void messagePackLedIntensity(t_message *message, uint8 intensity)
{
    message->data0 = (kMessageTypeLedIntensity << 4);
    message->data1 = intensity;
}

void messagePackLedTest(t_message *message, uint8 state)
{
    message->data0 = (kMessageTypeLedTest << 4);
    message->data1 = state ? on : off;
}

void messagePackAdcEnable(t_message *message, uint8 adc, uint8 state)
{
    message->data0 = kMessageTypeAdcEnable << 4;
    message->data1 = (adc << 4) | (state ? on : off);
}

void messagePackShutdown(t_message *message, uint8 state)
{
    message->data0 = kMessageTypeShutdown << 4;
    message->data1 = state ? on : off;
}

void messagePackLedRow(t_message *message, uint8 rowIndex, uint8 state)
{
    message->data0 = (kMessageTypeLedSetRow << 4) | (rowIndex & 0xF);
    message->data1 = state;
}

void messagePackLedColumn(t_message *message, uint8 columnIndex, uint8 state)
{
    message->data0 = (kMessageTypeLedSetColumn << 4) | (columnIndex & 0xF);
    message->data1 = state;
}

void messagePackEncEnable(t_message *message, uint8 enc, uint8 state)
{
    message->data0 = (kMessageTypeEncEnable << 4) | (enc & 0xF);
    message->data1 = state ? 1 : 0;
}

void messagePackEncVal(t_message *message, uint8 enc, uint8 val)
{
    message->data0 = (kMessageTypeEncVal << 4) | (enc & 0xF);
    message->data1 = (uint8)val;
}

}
