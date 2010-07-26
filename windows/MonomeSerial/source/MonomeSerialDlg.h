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



#ifndef __MONOMESERIALDLG_H__
#define __MONOMESERIALDLG_H__

#include <string>
#include <list>
#include "MonomeSerial.h"

using namespace std;

class ApplicationController;
class MonomeXXhDevice;

// for passing event data when devices are discovered or terminated
typedef void (*DeviceDiscoveredCallback)(const std::string&, void*);

typedef struct _deviceDiscoveredContext {
	DeviceDiscoveredCallback callback;
	void* userData;
} DeviceDiscoveredContext;


typedef void (*DeviceTerminatedCallback)(const std::string&, void*);

typedef struct _deviceTerminatedContext {
	DeviceTerminatedCallback callback;
	void* userData;
} DeviceTerminatedContext;
//-------------------------------------------------------------


// CMonomeSerialDlg dialog
class CMonomeSerialDlg : public CDialog
{
	/* ------------- generated code ------------- */
// Construction
public:
	CMonomeSerialDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MONOMESERIAL_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	virtual afx_msg void OnDevModeChange(LPTSTR lpDeviceName);
	virtual void OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg void OnClose();

	DECLARE_MESSAGE_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	// system tray methods
	BOOL TrayMessage( DWORD dwMessage);
	BOOL ShowTaskBarButton(BOOL bVisible);

	/*  ------------- my code ------------- */

public:
	void updateProtocol();
	void updateAddressPattern();
	void updateHostPort();
	void updateListenPort();
	void updateCable();
	void updateAddressPatternPrefix();
	void updateStartingColumn();
	void updateStartingRow();
	void updateAdcOffset();
	void updateEncoderOffset();
	void updateMidiInputDevice();
	void updateMidiOutputDevice();
	void updateMidiInputChannel();
	void updateMidiOutputChannel();
	void UpdateAdcStates();
	void UpdateEncStates();

	void updateDeviceList();
	void updateAdcLabels(bool useOffsets);
	void updateEncLabels(bool useOffsets);
	void UpdateGuiProtocol(unsigned int index);

	UINT getSelectedDeviceIndex(CString &device);
	std::string getSelectedSerial(void);

	void AddMidiInputDevices(const list<string>& devices);
	void AddMidiOutputDevices(const list<string>& devices);

	void AddSerialDeviceString(const CString& str);
	void AddSerialDeviceString(const std::string& str);
	void RemoveSerialDeviceString(const CString& str);
	void RemoveSerialDeviceString(const std::string& str);

public:
	// helper conversion functions
	static int GetCWndInt(const CWnd& control);
	static const CString GetCWndCString(const CWnd& control);
	static const CString IntToCString(int i);
	static const std::string IntToString(int i);
	static int StringToInt(const std::string& str);
	static int CStringToInt(const CString& str);
	static const CString StdStringToCString(const std::string& str);
	static const std::string CStringToStdString(const CString& str);
	static const CString StdWStringToCString(const std::wstring& str);
	static const std::wstring CStringToStdWString(const CString& str);
	static const std::wstring StdStringToStdWString(const std::string& str);
	static const std::string StdWStringToStdString(const std::wstring& str);

protected:
	BOOL initialized;

private:
	ApplicationController *_appController;
	void InitializeControls();
	void GetActiveDevices();

	afx_msg void OnHostAddressKillFocus();
	afx_msg void OnProtocolChanged();
	afx_msg void OnHostPortChanged();
	afx_msg void OnListenPortChanged();
	afx_msg void OnDeviceChanged();
	afx_msg void OnCableChanged();
	afx_msg void OnPrefixChanged();
	afx_msg void OnPrefixChangedKillFocus();
	afx_msg void OnStartingColumnChanged();
	afx_msg void OnStartingRowChanged();
	afx_msg void OnAdcOffsetChanged();
	afx_msg void OnEncoderOffsetChanged();
	afx_msg void OnMidiInputDeviceChanged();
	afx_msg void OnMidiInputChannelChanged();
	afx_msg void OnMidiOutputDeviceChanged();
	afx_msg void OnMidiOutputChannelChanged();
	afx_msg void OnAdc0Changed();
	afx_msg void OnAdc1Changed();
	afx_msg void OnAdc2Changed();
	afx_msg void OnAdc3Changed();
	afx_msg void OnEnc0Changed();
	afx_msg void OnEnc1Changed();


	// callbacks for device discovery/removal
	DeviceDiscoveredContext	_discoveredContext;
	DeviceTerminatedContext	_terminatedContext;

	static void monomeDeviceDiscoveredCallback(const std::string& serialNumber, void* userData);
	static void monomeDeviceTerminatedCallback(const std::string& serialNumber, void* userData);


// controls
	CStatic LblPrefix;
	CStatic LblColumn;
	CStatic LblRow;
	CStatic LblAdc;
	CStatic LblEnc;
	
	CComboBox cbProtocol;
	CEdit tbHostAddress;	
	CEdit tbHostPort;
	CEdit tbListenPort;
	CComboBox cbDevice;
	CComboBox cbInDevice;
	CComboBox cbOutDevice;
	CComboBox cbCable;
	CEdit tbPrefix;
	CEdit tbColumn;
	CEdit tbRow;
	CEdit tbAdc;
	CEdit tbEnc;	
	CEdit tbInChannel;
	CEdit tbOutChannel;
	CButton btAdc0;
	CButton btAdc1;
	CButton btAdc2;
	CButton btAdc3;
	CButton btEnc0;
	CButton btEnc1;
	
	CSpinButtonCtrl hostSpin;
	CSpinButtonCtrl listenSpin;
	CSpinButtonCtrl colSpin;
	CSpinButtonCtrl rowSpin;
	CSpinButtonCtrl adcSpin;
	CSpinButtonCtrl encSpin;
	CSpinButtonCtrl inChanSpin;
	CSpinButtonCtrl outChanSpin;
public:
	afx_msg void OnCbnDropdownCombo4();
	afx_msg void OnCbnDropdownCombo5();

private:
	FILE* console;
};

#endif