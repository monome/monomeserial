#ifndef MIDI_OUT_DEVICE_H
#define MIDI_OUT_DEVICE_H


/*******************************************************************************
 * MIDIOutDevice.h - Interface for CMIDIOutDevice and related classes.
 *
 * Copyright (C) 2002 Leslie Sanford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: jabberdabber@hotmail.com
 *
 * Last modified: 08/19/2003
 *
 * Note: You must link to the winmm.lib to use these classes.
 ******************************************************************************/


#pragma warning(disable:4786) // Disable annoying template warnings


//--------------------------------------------------------------------
// Dependencies
//--------------------------------------------------------------------


// Necessary for Windows data types
#include "../stdafx.h"
#include <mmsystem.h>

// Necessary for exception classes derived from std::exception
#include <exception> 

// Necessary for header queue used by CHeaderQueue
#include <queue>


namespace midi
{
    //----------------------------------------------------------------
    // Class declarations
    //----------------------------------------------------------------


    //----------------------------------------------------------------
    // CMIDIOutDevice exception classes
    //----------------------------------------------------------------


    // Encapsulates the midiOutGetErrorText messages
    class CMIDIOutException : public std::exception
    {
    public:
        CMIDIOutException(MMRESULT ErrCode) throw()
        { 
			LPWSTR temp = new wchar_t[128];
			mbstowcs(temp, m_ErrMsg, 128);
			::midiOutGetErrorText(ErrCode, temp, sizeof m_ErrMsg); 
		}
		
        const char *what() const throw() { return m_ErrMsg; }

    private:
        char m_ErrMsg[128];
    };


    // Thrown when memory allocation fails within a CMIDIOutDevice 
    // object
    class CMIDIOutMemFailure : public std::bad_alloc
    {
    public:
        const char *what() const throw()
        { return "Memory allocation within a CMIDIOutDevice object "
                 "failed."; }
    };


    // Thrown when a CMIDIOutDevice is unable to create a signalling 
    // event
    class CMIDIOutEventFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "Unable to create a signalling event for "
                 "CMIDIOutDevice object."; }
    };


    // Thrown when a CMIDIOutDevice is unable to create a worker 
    // thread
    class CMIDIOutThreadFailure : public std::exception
    {
    public:
        const char *what() const throw()
        { return "Unable to create worker thread for CMIDIOutDevice "
                 "object."; }
    };


    //----------------------------------------------------------------
    // CMIDIOutDevice
    //
    // This class represents MIDI output devices.
    //----------------------------------------------------------------


    class CMIDIOutDevice
    {
    public:
        // For constructing a CMIDIOutDevice in an closed state
        CMIDIOutDevice();

        // For constructing a CMIDIOutDevice in an opened state
        CMIDIOutDevice(UINT DeviceId);

        // Destruction
        ~CMIDIOutDevice();

        // Opens the MIDI output device
        void Open(UINT DeviceId);

        // Closes the MIDI output device
        void Close();

        // Sends short message
        void SendMsg(DWORD Msg);

        // Sends long message
        void SendMsg(LPSTR Msg, DWORD MsgLength);

        // Returns true if the device is open
        bool IsOpen() const;

        // Gets the Id for this device
        UINT GetDevID() const;

        // Gets the number of MIDI output devices on this system
        static UINT GetNumDevs() { return midiOutGetNumDevs(); }

        // Gets the capabilities of a particular MIDI output device
        // The results are stored in the MIDIOUTCAPS parameter.
        static void GetDevCaps(UINT DeviceId, MIDIOUTCAPS &Caps);


    // Private methods
    private:
        // Copying and assignment not allowed
        CMIDIOutDevice(const CMIDIOutDevice &);
        CMIDIOutDevice &operator = (const CMIDIOutDevice &);

        // Creates an event for signalling the header thread
        bool CreateEvent();

        // Called by Windows when a MIDI output event occurs
        static void CALLBACK MidiOutProc(HMIDIOUT MidiOut, UINT Msg,
                                         DWORD Instance, DWORD Param1, 
                                         DWORD Param2);

        // Thread function for managing headers
        static DWORD WINAPI HeaderProc(LPVOID Parameter);

    // Private class declarations
    private:
        // Encapsulates the MIDIHDR structure for MIDI output
        class CMIDIOutHeader
        {
        public:
            CMIDIOutHeader(HMIDIOUT DevHandle, LPSTR Msg, 
                           DWORD MsgLength);
            ~CMIDIOutHeader();

            void SendMsg();

        private:
            HMIDIOUT m_DevHandle;
            MIDIHDR  m_MIDIHdr;
        };


        // Thread safe queue for storing CMIDIOutHeader objects
        class CHeaderQueue
        {
        public:
            CHeaderQueue();
            ~CHeaderQueue();

            void AddHeader(CMIDIOutHeader *Header);
            void RemoveHeader();
            void RemoveAll();
            bool IsEmpty();

        private:
            std::queue<CMIDIOutHeader *> m_HdrQueue;
            CRITICAL_SECTION m_CriticalSection;
        };

    // Private attributes and constants
    private:
        HMIDIOUT       m_DevHandle;
        HANDLE         m_Event;
        CWinThread    *m_WorkerThread;
        CHeaderQueue   m_HdrQueue;
        enum State { CLOSED, OPENED } m_State;
    };
}


#endif
