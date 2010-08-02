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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of5
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


// MonomeSerialDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MonomeSerial.h"
#include "MonomeSerialDlg.h"
//#include "MonomeRegistry.h"
#include "serial/FTD2xx.h"

#include "ApplicationController.h"

#include <memory>
#include <list>
#include <sstream>

using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ------ FTD2XX.h calling functions for enumerating devices ------
int 
GetNumberOfSerialDevices(void)
{
	FT_STATUS ftStatus;
	int numDevs;
	
	ftStatus = FT_ListDevices(&numDevs,NULL,FT_LIST_NUMBER_ONLY);
	if (ftStatus != FT_OK) {
		throw runtime_error("Error in FTDI API : FT_ListDevices");
	}

	return numDevs;
}

auto_ptr< list<string> > GetSerialDeviceNames(void)
{
	FT_STATUS ftStatus;

	auto_ptr< list<string> > devices(new list<string>());
	int numdevs = GetNumberOfSerialDevices();

	for (int i = 0; i < numdevs; i++) {
		char buffer[64];
		memset(buffer, 0, 64);
		void* param1 = reinterpret_cast<void*>(i);

		ftStatus = FT_ListDevices(param1, buffer, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
		if (ftStatus != FT_OK) {
			throw runtime_error("Error in FTDI API : FT_ListDevices");
		}
		devices->push_back(string(buffer));
	}

	return devices;
}
//-----------------------

// CMonomeSerialDlg dialog

CMonomeSerialDlg::CMonomeSerialDlg(CWnd* pParent /*=NULL*/) 
 : CDialog(CMonomeSerialDlg::IDD, pParent)
{
	initialized = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CMonomeSerialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonomeSerialDlg)

	DDX_Control(pDX, IDC_COMBO2, cbDevice);
	DDX_Control(pDX, IDC_LABEL, LblPrefix);
	DDX_Control(pDX, IDC_LABEL2, LblColumn);
	DDX_Control(pDX, IDC_LABEL3, LblRow);
	DDX_Control(pDX, IDC_LABEL4, LblAdc);
	DDX_Control(pDX, IDC_LABEL5, LblEnc);
	DDX_Control(pDX, IDC_COMBO1, cbProtocol);
	DDX_Control(pDX, IDC_EDIT11, tbHostAddress);
	DDX_Control(pDX, IDC_EDIT1, tbHostPort);
	DDX_Control(pDX, IDC_EDIT2, tbListenPort);	
	DDX_Control(pDX, IDC_COMBO3, cbCable);
	DDX_Control(pDX, IDC_EDIT4, tbPrefix);
	DDX_Control(pDX, IDC_EDIT15, tbColumn);
	DDX_Control(pDX, IDC_EDIT12, tbRow);
	DDX_Control(pDX, IDC_EDIT16, tbAdc);
	DDX_Control(pDX, IDC_EDIT14, tbEnc);
	DDX_Control(pDX, IDC_COMBO4, cbInDevice);
	DDX_Control(pDX, IDC_COMBO5, cbOutDevice);
	DDX_Control(pDX, IDC_EDIT9, tbInChannel);
	DDX_Control(pDX, IDC_EDIT13, tbOutChannel);
	DDX_Control(pDX, IDC_CHECK1, btAdc0);
	DDX_Control(pDX, IDC_CHECK2, btAdc1);
	DDX_Control(pDX, IDC_CHECK3, btAdc2);
	DDX_Control(pDX, IDC_CHECK4, btAdc3);
	DDX_Control(pDX, IDC_CHECK5, btEnc0);
	DDX_Control(pDX, IDC_CHECK6, btEnc1);
	DDX_Control(pDX, IDC_SPIN1, hostSpin);
	DDX_Control(pDX, IDC_SPIN2, listenSpin);
	DDX_Control(pDX, IDC_SPIN7, colSpin);
	DDX_Control(pDX, IDC_SPIN4, rowSpin);
	DDX_Control(pDX, IDC_SPIN8, adcSpin);
	DDX_Control(pDX, IDC_SPIN6, encSpin);
	DDX_Control(pDX, IDC_SPIN3, inChanSpin);
	DDX_Control(pDX, IDC_SPIN5, outChanSpin);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMonomeSerialDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_EN_KILLFOCUS(IDC_EDIT11, &CMonomeSerialDlg::OnHostAddressKillFocus)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CMonomeSerialDlg::OnProtocolChanged)
	ON_EN_CHANGE(IDC_EDIT2, &CMonomeSerialDlg::OnListenPortChanged)
	ON_EN_CHANGE(IDC_EDIT1, &CMonomeSerialDlg::OnHostPortChanged)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CMonomeSerialDlg::OnDeviceChanged)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CMonomeSerialDlg::OnCableChanged)
	ON_EN_KILLFOCUS(IDC_EDIT4, &CMonomeSerialDlg::OnPrefixChangedKillFocus)
	ON_EN_CHANGE(IDC_EDIT15, &CMonomeSerialDlg::OnStartingColumnChanged)
	ON_EN_CHANGE(IDC_EDIT12, &CMonomeSerialDlg::OnStartingRowChanged)
	ON_EN_CHANGE(IDC_EDIT16, &CMonomeSerialDlg::OnAdcOffsetChanged)
	ON_EN_CHANGE(IDC_EDIT14, &CMonomeSerialDlg::OnEncoderOffsetChanged)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CMonomeSerialDlg::OnMidiInputDeviceChanged)
	ON_EN_CHANGE(IDC_EDIT9, &CMonomeSerialDlg::OnMidiInputChannelChanged)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CMonomeSerialDlg::OnMidiOutputDeviceChanged)
	ON_EN_CHANGE(IDC_EDIT13, &CMonomeSerialDlg::OnMidiOutputChannelChanged)
	ON_BN_CLICKED(IDC_CHECK1, &CMonomeSerialDlg::OnAdc0Changed)
	ON_BN_CLICKED(IDC_CHECK2, &CMonomeSerialDlg::OnAdc1Changed)
	ON_BN_CLICKED(IDC_CHECK3, &CMonomeSerialDlg::OnAdc2Changed)
	ON_BN_CLICKED(IDC_CHECK4, &CMonomeSerialDlg::OnAdc3Changed)
	ON_BN_CLICKED(IDC_CHECK5, &CMonomeSerialDlg::OnEnc0Changed)
	ON_BN_CLICKED(IDC_CHECK6, &CMonomeSerialDlg::OnEnc1Changed)

	ON_WM_CLOSE()
	ON_CBN_DROPDOWN(IDC_COMBO4, &CMonomeSerialDlg::OnCbnDropdownCombo4)
	ON_CBN_DROPDOWN(IDC_COMBO5, &CMonomeSerialDlg::OnCbnDropdownCombo5)
END_MESSAGE_MAP()


// system tray code
BOOL CMonomeSerialDlg::TrayMessage( DWORD dwMessage)
{
	CString sTip(_T("MonomeSerial"));
	NOTIFYICONDATA tnd;
	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = m_hWnd;
	tnd.uID = IDI_ICON1;
	tnd.uFlags = NIF_MESSAGE|NIF_ICON;
	tnd.uCallbackMessage = MYWM_NOTIFYICON;
	tnd.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	VERIFY( tnd.hIcon = LoadIcon(AfxGetInstanceHandle(),
							   MAKEINTRESOURCE (IDI_ICON1)) );
	//IDR_TRAYICON
	lstrcpyn(tnd.szTip, (LPCTSTR)sTip, sizeof(tnd.szTip));

	return Shell_NotifyIcon(dwMessage, &tnd);

}

LRESULT CMonomeSerialDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// Open window when the user double-clicks the Systray Icon
	if(message == MYWM_NOTIFYICON){

		switch (lParam){
			case WM_LBUTTONDOWN:
				switch (wParam) {
					case IDI_ICON1:

					  ShowWindow(SW_NORMAL);
					  SetForegroundWindow();
					  SetFocus();
					  return TRUE;

					break;
				}
				break;
		}
	}

	return CDialog::WindowProc(message, wParam, lParam);
}

BOOL CMonomeSerialDlg::ShowTaskBarButton(BOOL bVisible)
{
    ShowWindow(SW_HIDE);

    if (bVisible)
        ModifyStyleEx(0, WS_EX_APPWINDOW);
    else
        ModifyStyleEx(WS_EX_APPWINDOW, 0);

    ShowWindow(SW_SHOW);

    return TRUE;
}


void CMonomeSerialDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	//if (nID == SC_MINIMIZE) {
	//	ShowTaskBarButton(FALSE);
	//}
	//else {
	//	ShowTaskBarButton(TRUE);
	//}

	CDialog::OnSysCommand(nID, lParam);
}


// CMonomeSerialDlg message handlers

BOOL CMonomeSerialDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	TrayMessage(NIM_ADD);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

//#undef DEBUG_PRINT
#if defined DEBUG_PRINT
	AllocConsole();
	console = freopen ("CONOUT$", "w", stdout ); 
#endif

	hostSpin.SetRange32(0, 9999);
	listenSpin.SetRange32(0, 9999);
	colSpin.SetRange32(-32768, 32767);
	rowSpin.SetRange32(-32768, 32767);
	adcSpin.SetRange32(0, 32767);
	encSpin.SetRange32(0, 32767);
	inChanSpin.SetRange32(1, 16);
	outChanSpin.SetRange32(1, 16);

	cbProtocol.SetCurSel(1);
	tbHostAddress.SetWindowText(L"127.0.0.1");
	tbHostPort.SetWindowTextW(L"8000");
	tbListenPort.SetWindowTextW(L"8080");
	tbInChannel.SetWindowTextW(L"1");
	tbOutChannel.SetWindowTextW(L"1");

	initialized = TRUE;

	_appController = new ApplicationController(this);

	const list<string> inDevices = _appController->coreMIDI()->getMidiInputDeviceSources();
	cbInDevice.AddString(L"No Device Selected");
	for (list<string>::const_iterator i = inDevices.begin(); i != inDevices.end(); i++) {
		cbInDevice.AddString(StdStringToCString(*i));
	}

	const list<string> outDevices = _appController->coreMIDI()->getMidiOutputDeviceSources();
	cbOutDevice.AddString(L"No Device Selected");
	for (list<string>::const_iterator i = outDevices.begin(); i != outDevices.end(); i++) {
		cbOutDevice.AddString(StdStringToCString(*i));
	}

	// setup callbacks from Dialog class to ApplicationController.
	_discoveredContext.callback = (DeviceDiscoveredCallback)CMonomeSerialDlg::monomeDeviceDiscoveredCallback;
	_discoveredContext.userData = _appController;

	_terminatedContext.callback = (DeviceTerminatedCallback)CMonomeSerialDlg::monomeDeviceTerminatedCallback;
	_terminatedContext.userData = _appController;

	GetActiveDevices();

	updateProtocol();
	updateAddressPattern();
	updateHostPort();
	updateListenPort();

	return TRUE;  // return TRUE unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMonomeSerialDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMonomeSerialDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMonomeSerialDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) 
		{
			case VK_ESCAPE:
				return TRUE;
			case VK_RETURN:
				if (this->GetFocus() == &tbHostAddress) {
					if (initialized)
						tbHostPort.SetFocus();
				}
				else if (this->GetFocus() == &tbPrefix) {
					if (initialized)
						tbColumn.SetFocus();
				}
				return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CMonomeSerialDlg::OnClose()
{
	TrayMessage( NIM_DELETE );

	_appController->writePreferences();
	delete _appController;
	_appController = 0;
	
#ifdef DEBUG_PRINT
	fclose(console);
	::FreeConsole();
#endif

	CDialog::OnClose();
}

void CMonomeSerialDlg::OnDevModeChange(LPTSTR lpDeviceName)
{
	AfxMessageBox(L"An external device has been added or removed.");
}

void CMonomeSerialDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	//AfxMessageBox(L"FOO");
}

// ------------------------------------------------------------------------------------

void CMonomeSerialDlg::OnProtocolChanged()
{
	if (!initialized)
		return;

	unsigned int index = cbProtocol.GetCurSel();
	
	_appController->protocolPopUpMenuChanged(index);
	
	UpdateGuiProtocol(index);
}

void CMonomeSerialDlg::OnHostAddressKillFocus()
{
	if (!initialized)
		return;

	try {
		_appController->oscHostAddressTextFieldChanged(cbDevice.GetCurSel(), CStringToStdString(GetCWndCString(tbHostAddress)));
	}
	catch (std::exception &ex) {
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnHostAddressKillFocus : std::exception = " << ex.what() << endl;
#endif
		try {
			_appController->oscHostAddressTextFieldChanged(cbDevice.GetCurSel(), _appController->oscHostAddressString());
			tbHostAddress.SetWindowTextW(StdStringToCString(_appController->oscHostAddressString()));
		}
		catch (std::exception &ex_inner) {
			_appController->oscHostAddressTextFieldChanged(cbDevice.GetCurSel(), "127.0.0.1");
			tbHostAddress.SetWindowTextW(L"127.0.0.1");
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnHostAddressKillFocus inner exception : std::exception = " << ex_inner.what() << endl;
#endif
		}
	}
}

void CMonomeSerialDlg::OnHostPortChanged()
{
	if (!initialized)
		return;

	try {
		const CString hostPortCStr = GetCWndCString(tbHostPort);
		const string hostPortStr = CStringToStdString(hostPortCStr);

		_appController->oscHostPortTextFieldChanged(cbDevice.GetCurSel(), hostPortStr);
	}
	catch (std::exception &ex) {
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnHostPortChanged : std::exception = " << ex.what() << endl;
#endif
		try {
			_appController->oscHostPortTextFieldChanged(cbDevice.GetCurSel(), _appController->oscHostPort());
			tbHostPort.SetWindowTextW(StdStringToCString(_appController->oscHostPort()));
		}
		catch (std::exception &ex_inner) {
			_appController->oscHostPortTextFieldChanged(cbDevice.GetCurSel(), "8000");
			tbHostPort.SetWindowTextW(L"8000");
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnHostPortChanged : std::exception = " << ex_inner.what() << endl;
#endif
		}
	}
}

void CMonomeSerialDlg::OnListenPortChanged()
{
	if (!initialized)
		return;

	try {
		const CString listenPortCStr = GetCWndCString(tbListenPort);
		const string listenPortStr = CStringToStdString(listenPortCStr);

		_appController->oscListenPortTextFieldChanged(cbDevice.GetCurSel(), listenPortStr);
	}
	catch (std::exception &ex) {
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnListenPortChanged : std::exception = " << ex.what() << endl;
#endif
		try {
			_appController->oscListenPortTextFieldChanged(cbDevice.GetCurSel(), _appController->oscListenPort());
			tbListenPort.SetWindowTextW(StdStringToCString(_appController->oscListenPort()));
		}
		catch (std::exception &ex_inner) {
			_appController->oscListenPortTextFieldChanged(cbDevice.GetCurSel(), "8080");
			tbListenPort.SetWindowTextW(L"8080");
#ifdef DEBUG_PRINT
		cout << "CMonomeSerialDlg::OnListenPortChanged : std::exception = " << ex_inner.what() << endl;
#endif
		}
	}
}

void CMonomeSerialDlg::OnDeviceChanged()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
		
	if (device == 0) {
		cbCable.SetCurSel(0);
		tbPrefix.SetWindowTextW(L"/40h");
		
		cbInDevice.SetCurSel(0);
		cbOutDevice.SetCurSel(0);

		tbRow.SetWindowTextW(L"0");
		tbColumn.SetWindowTextW(L"0");
		tbEnc.SetWindowTextW(L"0");
		tbAdc.SetWindowTextW(L"0");
		tbInChannel.SetWindowTextW(L"1");
		tbOutChannel.SetWindowTextW(L"1");
		btAdc0.SetCheck(0);
		btAdc1.SetCheck(0);
		btAdc2.SetCheck(0);
		btAdc3.SetCheck(0);
		btEnc0.SetCheck(0);
		btEnc1.SetCheck(0);
		
		return;
	}
		
	// get values from device
	cbCable.SetCurSel((int)device->cableOrientation());
	tbPrefix.SetWindowTextW(StdStringToCString(device->oscAddressPatternPrefix()));

	unsigned int  indev = _appController->indexOfMIDIInputDeviceForMonomeXXhDeviceIndex(cbDevice.GetCurSel());
	unsigned int outdev = _appController->indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex(cbDevice.GetCurSel());
	if (indev >= 0) {
		cbInDevice.SelectString(-1, StdStringToCString(_appController->nameOfMIDIInputDeviceAtIndex(indev)));
	}
	else {
		cbInDevice.SetCurSel(0);
	}

	if (outdev >= 0) {
		cbOutDevice.SelectString(-1, StdStringToCString(_appController->nameOfMIDIOutputDeviceAtIndex(outdev)));
	}
	else {
		cbOutDevice.SetCurSel(0);
	}

	tbInChannel.SetWindowTextW(IntToCString(device->MIDIInputChannel() + 1));
	tbOutChannel.SetWindowTextW(IntToCString(device->MIDIOutputChannel() + 1));	
	tbRow.SetWindowTextW(IntToCString(device->oscStartRow()));
	tbColumn.SetWindowTextW(IntToCString(device->oscStartColumn()));
	tbAdc.SetWindowTextW(IntToCString(device->oscAdcOffset()));
	tbEnc.SetWindowTextW(IntToCString(device->oscEncOffset()));

	tbHostAddress.SetWindowTextW(StdStringToCString(device->OscHostAddress()));
	tbHostPort.SetWindowTextW(IntToCString(device->OscHostPort()));
	tbListenPort.SetWindowTextW(IntToCString(device->OscListenPort()));

	UpdateAdcStates();
	UpdateEncStates();

	this->Invalidate();
}

void CMonomeSerialDlg::OnCableChanged()
{
	if (!initialized)
		return;

	_appController->cableOrientationPopUpMenuChanged(cbDevice.GetCurSel(), cbCable.GetCurSel());
}


void CMonomeSerialDlg::OnPrefixChangedKillFocus()
{
	if (!initialized)
		return;

	CString prefix;
	tbPrefix.GetWindowTextW(prefix);
	
	if (prefix.GetLength() <= 1) {
		
		MonomeXXhDevice* device = _appController->deviceAtIndex(cbDevice.GetCurSel());
		
		if (device == 0) 
			return;

		MonomeXXhDevice::DeviceType type = device->type();

		switch (type) 
		{
			case MonomeXXhDevice::kDeviceType_40h:
				prefix = L"/40h";
				break;
			case MonomeXXhDevice::kDeviceType_256:
				prefix = L"/256";
				break;
			case MonomeXXhDevice::kDeviceType_128:
				prefix = L"/128";
				break;
			case MonomeXXhDevice::kDeviceType_64:
				prefix = L"/64";
				break;
			default:  // this shouldn't ever occurr
				prefix = L"/box";
				break;
		}
	}
	else if (prefix[0] != '/') {
		prefix = L"/" + prefix;
	}

	tbPrefix.SetWindowTextW(prefix);

	_appController->oscAddressPatternPrefixTextFieldChanged(cbDevice.GetCurSel(), 
															CStringToStdString(GetCWndCString(tbPrefix))
															);
}


void CMonomeSerialDlg::OnStartingColumnChanged()
{
	if (!initialized)
		return;

	_appController->startingColumnTextFieldChanged(cbDevice.GetCurSel(), GetCWndInt(tbColumn));
}

void CMonomeSerialDlg::OnStartingRowChanged()
{
	if (!initialized)
		return;

	_appController->startingRowTextFieldChanged(cbDevice.GetCurSel(), GetCWndInt(tbRow));
}

void CMonomeSerialDlg::OnAdcOffsetChanged()
{
	if (!initialized)
		return;

	_appController->oscAdcOffsetTextFieldChanged(cbDevice.GetCurSel(), GetCWndInt(tbAdc));
}

void CMonomeSerialDlg::OnEncoderOffsetChanged()
{
	if (!initialized)
		return;

	_appController->oscEncOffsetTextFieldChanged(cbDevice.GetCurSel(), GetCWndInt(tbEnc));
}

void CMonomeSerialDlg::OnMidiInputDeviceChanged()
{
	if (!initialized)
		return;

	_appController->midiInputDeviceChanged(cbDevice.GetCurSel(), 
										   CCoreMIDI::getMidiInputDeviceByName(CStringToStdString(GetCWndCString(cbInDevice)))
										   );
}

void CMonomeSerialDlg::OnCbnDropdownCombo4()
{
	cbInDevice.ResetContent();
	const list<string> inDevices = _appController->coreMIDI()->getMidiInputDeviceSources();
	cbInDevice.AddString(L"No Device Selected");
	for (list<string>::const_iterator i = inDevices.begin(); i != inDevices.end(); i++) {
		cbInDevice.AddString(StdStringToCString(*i));
	}

}

void CMonomeSerialDlg::OnMidiInputChannelChanged()
{
	if (!initialized)
		return;

	unsigned char t = static_cast<unsigned char>(GetCWndInt(tbInChannel));
	
	if (t < 1) {
		t = 1;
		tbInChannel.SetWindowTextW(IntToCString(t));
	}
	else if (t > 16) {
		t = 16;
		tbInChannel.SetWindowTextW(IntToCString(t));
	}

	_appController->midiInputChannelChanged(cbDevice.GetCurSel(), t - 1);
}

void CMonomeSerialDlg::OnMidiOutputDeviceChanged()
{
	if (!initialized)
		return;

	_appController->midiOutputDeviceChanged(cbDevice.GetCurSel(), 
		CCoreMIDI::getMidiOutputDeviceByName(CStringToStdString(GetCWndCString(cbOutDevice))));
}

void CMonomeSerialDlg::OnCbnDropdownCombo5()
{
	cbOutDevice.ResetContent();
	const list<string> outDevices = _appController->coreMIDI()->getMidiOutputDeviceSources();
	cbOutDevice.AddString(L"No Device Selected");
	for (list<string>::const_iterator i = outDevices.begin(); i != outDevices.end(); i++) {
		cbOutDevice.AddString(StdStringToCString(*i));
	}
}

void CMonomeSerialDlg::OnMidiOutputChannelChanged()
{
	if (!initialized)
		return;

	unsigned char t = static_cast<unsigned char>(GetCWndInt(tbOutChannel));

	if (t < 1) {
		t = 1;
		tbOutChannel.SetWindowTextW(IntToCString(t));
	}
	else if (t > 16) {
		t = 16;
		tbOutChannel.SetWindowTextW(IntToCString(t));
	}

	_appController->midiOutputChannelChanged(cbDevice.GetCurSel(), t - 1);
}

void CMonomeSerialDlg::OnAdc0Changed()
{
	if (!initialized)
		return;

	bool state = btAdc0.GetCheck() != 0;
	_appController->adcStateButtonChanged(cbDevice.GetCurSel(), 0, state);

	if (state) {
		UpdateAdcStates();
	}
}

void CMonomeSerialDlg::OnAdc1Changed()
{
	if (!initialized)
		return;

	bool state = btAdc1.GetCheck() != 0;
	_appController->adcStateButtonChanged(cbDevice.GetCurSel(), 1, state);

	if (state) {
		UpdateAdcStates();
	}
}

void CMonomeSerialDlg::OnAdc2Changed()
{
	if (!initialized)
		return;

	bool state = btAdc2.GetCheck() != 0;
	_appController->adcStateButtonChanged(cbDevice.GetCurSel(), 2, state);

	if (state) {
		UpdateAdcStates();
	}
}

void CMonomeSerialDlg::OnAdc3Changed()
{
	if (!initialized)
		return;

	bool state = btAdc3.GetCheck() != 0;
	_appController->adcStateButtonChanged(cbDevice.GetCurSel(), 3, state);

	if (state) {
		UpdateAdcStates();
	}
}

void CMonomeSerialDlg::OnEnc0Changed()
{
	if (!initialized)
		return;

	bool state = btEnc0.GetCheck() != 0;
	_appController->encStateButtonChanged(cbDevice.GetCurSel(), 0, state);

	if (state) {
		UpdateEncStates();
	}
}

void CMonomeSerialDlg::OnEnc1Changed()
{
	if (!initialized)
		return;

	bool state = btEnc1.GetCheck() != 0;
	_appController->encStateButtonChanged(cbDevice.GetCurSel(), 1, state);

	if (state) {
		UpdateEncStates();
	}
}

void
CMonomeSerialDlg::updateProtocol()
{
	if (!initialized)
		return;

	cbProtocol.SetCurSel(_appController->protocol());
	UpdateGuiProtocol(_appController->protocol());
}

void
CMonomeSerialDlg::updateAddressPattern()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	const string& oscAddressPattern = device->OscHostAddress();
	const CString oscAddressPatternCStr = StdStringToCString(oscAddressPattern);

	tbHostAddress.SetWindowTextW(oscAddressPatternCStr);
}

void
CMonomeSerialDlg::updateAddressPatternPrefix()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbPrefix.SetWindowTextW(StdStringToCString(device->oscAddressPatternPrefix()));
}

void
CMonomeSerialDlg::updateCable()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	cbCable.SetCurSel((int)device->cableOrientation());
}

void
CMonomeSerialDlg::updateStartingColumn()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbColumn.SetWindowTextW(IntToCString(device->oscStartColumn()));
}

void
CMonomeSerialDlg::updateStartingRow()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbRow.SetWindowTextW(IntToCString(device->oscStartRow()));
}

void
CMonomeSerialDlg::updateAdcOffset()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbAdc.SetWindowTextW(IntToCString(device->oscAdcOffset()));
	updateAdcLabels(cbProtocol.GetCurSel() == 1);
}

void
CMonomeSerialDlg::updateEncoderOffset()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbEnc.SetWindowTextW(IntToCString(device->oscEncOffset()));
	updateEncLabels(cbProtocol.GetCurSel() == 1);
}

void
CMonomeSerialDlg::updateMidiInputChannel()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbInChannel.SetWindowTextW(IntToCString(device->MIDIInputChannel() + 1));
}

void
CMonomeSerialDlg::updateMidiOutputChannel()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	tbOutChannel.SetWindowTextW(IntToCString(device->MIDIOutputChannel() + 1));
}

void
CMonomeSerialDlg::updateMidiInputDevice()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	unsigned int index = _appController->indexOfMIDIInputDeviceForMonomeXXhDeviceIndex(cbDevice.GetCurSel()); 
	if (index < (unsigned int)cbInDevice.GetCount()) {
		cbInDevice.SetCurSel(index);
	}
	else {
		cbInDevice.SetCurSel(0);
	}
}

void
CMonomeSerialDlg::updateMidiOutputDevice()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	unsigned int index = _appController->indexOfMIDIOutputDeviceForMonomeXXhDeviceIndex(cbDevice.GetCurSel()); 
	if (index < (unsigned int)cbOutDevice.GetCount()) {
		cbOutDevice.SetCurSel(index);
	}
	else {
		cbOutDevice.SetCurSel(0);
	}
}

void
CMonomeSerialDlg::updateHostPort()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	const CString oscHostPortCStr = IntToCString(device->OscHostPort());

	tbHostPort.SetWindowTextW(oscHostPortCStr);
}

void
CMonomeSerialDlg::updateListenPort()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());
	if (!device) return;

	const CString oscListenPortCStr = IntToCString(device->OscListenPort());

	tbListenPort.SetWindowTextW(oscListenPortCStr);
}

void
CMonomeSerialDlg::AddSerialDeviceString(const CString& string)
{
	if (!initialized)
		return;

	this->cbDevice.AddString(string);
}

void 
CMonomeSerialDlg::AddSerialDeviceString(const std::string& device)
{
	if (!initialized)
		return;

	this->AddSerialDeviceString(StdStringToCString(device));
}

void
CMonomeSerialDlg::RemoveSerialDeviceString(const CString& string)
{
	if (!initialized)
		return;

	int result = cbDevice.FindString(-1, string);
	if (result != CB_ERR) {
		this->cbDevice.DeleteString(result);
	}
}

void 
CMonomeSerialDlg::RemoveSerialDeviceString(const std::string& device)
{
	if (!initialized)
		return;

	this->RemoveSerialDeviceString(StdStringToCString(device));
}

std::string 
CMonomeSerialDlg::getSelectedSerial()
{
	if (!initialized)
		return string();

	CString device;
	cbDevice.GetLBText(cbDevice.GetCurSel(), device);

	return CStringToStdString(device);
}

UINT 
CMonomeSerialDlg::getSelectedDeviceIndex(CString &device)
{
	if (!initialized)
		return 0;

	return cbDevice.FindString(-1, device);
}

void 
CMonomeSerialDlg::AddMidiInputDevices(const list<string>& devices)
{
	if (!initialized)
		return;

	cbInDevice.ResetContent();
	list<string>::const_iterator i;
	for (i = devices.begin(); i != devices.end(); ++i) {
		cbInDevice.AddString(StdStringToCString(*i));
	}
}

void 
CMonomeSerialDlg::AddMidiOutputDevices(const list<string>& devices)
{
	if (!initialized)
		return;

	cbOutDevice.ResetContent();
	list<string>::const_iterator i;
	for (i = devices.begin(); i != devices.end(); ++i) {
		cbOutDevice.AddString(StdStringToCString(*i));
	}
}

void 
CMonomeSerialDlg::updateAdcLabels(bool useOffsets)
{
	if (!initialized)
		return;

	if (useOffsets) {
		CString format;

		format.Format(L"Adc %d", GetCWndInt(tbAdc));
		btAdc0.SetWindowText(format);

		format.Format(L"Adc %d", GetCWndInt(tbAdc) + 1);
		btAdc1.SetWindowText(format);

		format.Format(L"Adc %d", GetCWndInt(tbAdc) + 2);
		btAdc2.SetWindowText(format);

		format.Format(L"Adc %d", GetCWndInt(tbAdc) + 3);
		btAdc3.SetWindowText(format);
		return;
	}

	btAdc0.SetWindowText(L"Adc 0");
	btAdc1.SetWindowText(L"Adc 1");
	btAdc2.SetWindowText(L"Adc 2");
	btAdc3.SetWindowText(L"Adc 3");
}
	
void 
CMonomeSerialDlg::updateEncLabels(bool useOffsets)
{
	if (!initialized)
		return;

	if (useOffsets) {
		CString format;

		format.Format(L"Enc %d", GetCWndInt(tbEnc));
		btEnc0.SetWindowText(format);

		format.Format(L"Enc %d", GetCWndInt(tbEnc) + 1);
		btEnc1.SetWindowText(format);
		return;
	}

	btEnc0.SetWindowText(L"Enc 0");
	btEnc1.SetWindowText(L"Enc 1");
}

void CMonomeSerialDlg::updateDeviceList()
{
	if (!initialized)
		return;

	cbDevice.ResetContent();
	
	if (_appController->numberOfDevices() == 0) {
		AfxMessageBox(L"No Devices Available");

		cbCable.EnableWindow(FALSE);
		tbInChannel.EnableWindow(FALSE);
		cbInDevice.EnableWindow(FALSE);
		tbOutChannel.EnableWindow(FALSE);
		cbOutDevice.EnableWindow(FALSE);
		tbPrefix.EnableWindow(FALSE);
		tbColumn.EnableWindow(FALSE);
		tbRow.EnableWindow(FALSE);
		tbAdc.EnableWindow(FALSE);
		tbEnc.EnableWindow(FALSE);
		btAdc0.EnableWindow(FALSE);
		btAdc1.EnableWindow(FALSE);
		btAdc2.EnableWindow(FALSE);
		btAdc3.EnableWindow(FALSE);
		btEnc0.EnableWindow(FALSE);
		btEnc1.EnableWindow(FALSE);
	}
	else {
		unsigned int i;
		MonomeXXhDevice *device;
		
		for (i = 0; i < _appController->numberOfDevices(); i++) {
			device = _appController->deviceAtIndex(i);
			
			if (device != 0) {
				AddSerialDeviceString(StdStringToCString(device->serialNumber()));
			}
		}
			
		cbCable.EnableWindow(TRUE);
		tbInChannel.EnableWindow(TRUE);
		cbInDevice.EnableWindow(TRUE);
		tbOutChannel.EnableWindow(TRUE);
		cbOutDevice.EnableWindow(TRUE);
		tbPrefix.EnableWindow(TRUE);
		tbColumn.EnableWindow(TRUE);
		tbRow.EnableWindow(TRUE);
		tbAdc.EnableWindow(TRUE);
		tbEnc.EnableWindow(TRUE);
		btAdc0.EnableWindow(TRUE);
		btAdc1.EnableWindow(TRUE);
		btAdc2.EnableWindow(TRUE);
		btAdc3.EnableWindow(TRUE);
		btEnc0.EnableWindow(TRUE);
		btEnc1.EnableWindow(TRUE);

		cbDevice.SetCurSel(cbDevice.GetCount() - 1);
		this->OnDeviceChanged();
	}
}


void CMonomeSerialDlg::UpdateAdcStates()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());

	if (device == 0) {
		btAdc0.SetCheck(0);
		btAdc1.SetCheck(0);
		btAdc2.SetCheck(0);
		btAdc3.SetCheck(0);
	}
	else {	
		btAdc0.SetCheck(device->adcEnableState(0));
		btAdc1.SetCheck(device->adcEnableState(1));
		btAdc2.SetCheck(device->adcEnableState(2));
		btAdc3.SetCheck(device->adcEnableState(3));
	}
}

void CMonomeSerialDlg::UpdateEncStates()
{
	if (!initialized)
		return;

	MonomeXXhDevice *device = _appController->deviceAtIndex(cbDevice.GetCurSel());

	if (device == 0) {
		btEnc0.SetCheck(0);
		btEnc1.SetCheck(0);
	}
	else {	
		btEnc0.SetCheck(device->encEnableState(0));
		btEnc1.SetCheck(device->encEnableState(1));
	}
}

void CMonomeSerialDlg::UpdateGuiProtocol(unsigned int index)
{
	if (index == ApplicationController::kProtocolType_OpenSoundControl) {
		tbHostAddress.EnableWindow(TRUE);
		tbHostPort.EnableWindow(TRUE);
		tbListenPort.EnableWindow(TRUE);

		tbPrefix.ShowWindow(SW_SHOW);
		tbColumn.ShowWindow(SW_SHOW);
		tbRow.ShowWindow(SW_SHOW);
		tbAdc.ShowWindow(SW_SHOW);
		tbEnc.ShowWindow(SW_SHOW);
		cbInDevice.ShowWindow(SW_HIDE);
		tbInChannel.ShowWindow(SW_HIDE);
		cbOutDevice.ShowWindow(SW_HIDE);
		tbOutChannel.ShowWindow(SW_HIDE);

		colSpin.ShowWindow(SW_SHOW);
		rowSpin.ShowWindow(SW_SHOW);
		adcSpin.ShowWindow(SW_SHOW);
		encSpin.ShowWindow(SW_SHOW);
		inChanSpin.ShowWindow(SW_HIDE);
		outChanSpin.ShowWindow(SW_HIDE);

		LblPrefix.SetWindowTextW(L"Address Pattern Prefix :");
		LblColumn.SetWindowTextW(L"Starting Column :");
		LblRow.SetWindowTextW(L"Starting Row :");
		LblAdc.SetWindowTextW(L"ADC Offset :");
		LblEnc.ShowWindow(SW_SHOW);
	}
	else {  // MIDI
		tbHostAddress.EnableWindow(FALSE);
		tbHostPort.EnableWindow(FALSE);
		tbListenPort.EnableWindow(FALSE);	

		tbPrefix.ShowWindow(SW_HIDE);
		tbColumn.ShowWindow(SW_HIDE);
		tbRow.ShowWindow(SW_HIDE);
		tbAdc.ShowWindow(SW_HIDE);
		tbEnc.ShowWindow(SW_HIDE);
		cbInDevice.ShowWindow(SW_SHOW);
		tbInChannel.ShowWindow(SW_SHOW);
		cbOutDevice.ShowWindow(SW_SHOW);
		tbOutChannel.ShowWindow(SW_SHOW);

		colSpin.ShowWindow(SW_HIDE);
		rowSpin.ShowWindow(SW_HIDE);
		adcSpin.ShowWindow(SW_HIDE);
		encSpin.ShowWindow(SW_HIDE);
		inChanSpin.ShowWindow(SW_SHOW);
		outChanSpin.ShowWindow(SW_SHOW);

		LblPrefix.SetWindowTextW(L"Input Device :");
		LblColumn.SetWindowTextW(L"Input Channel :");
		LblRow.SetWindowTextW(L"Output Device :");
		LblAdc.SetWindowTextW(L"Output Channel :");
		LblEnc.ShowWindow(SW_HIDE);
	}		

	this->Invalidate();
}

void CMonomeSerialDlg::GetActiveDevices()
{
	auto_ptr< list<string> > devices = GetSerialDeviceNames();
	list<string>::iterator it = devices->begin();

	for (;it != devices->end(); ++it) {
		_discoveredContext.callback(
			*it, 
			_discoveredContext.userData
		);
	}
}

//void CMonomeSerialDlg::GetActiveDevices()
//{
//	map<wstring, wstring> devices;
//	::GetActiveDevices(&devices);
//
//	if (devices.size() > 0) {
//
//		map<wstring, wstring>::iterator i;
//		for (i = devices.begin(); i != devices.end(); i++) {
//			_discoveredContext.callback(StdWStringToStdString(i->second), StdWStringToStdString(i->first), _discoveredContext.userData);
//		}
//	}
//}

const std::wstring
CMonomeSerialDlg::CStringToStdWString(const CString& str)
{
	return wstring(str.GetString());
}

const CString
CMonomeSerialDlg::StdWStringToCString(const std::wstring& str)
{
	return CString(str.c_str());
}

const CString 
CMonomeSerialDlg::StdStringToCString(const std::string& str)
{
	return CString(CA2W(str.c_str()));
}

const string 
CMonomeSerialDlg::CStringToStdString(const CString& str)
{
	return string(CW2A(str));
}

const wstring 
CMonomeSerialDlg::StdStringToStdWString(const string& str)
{
	return wstring(CA2W(str.c_str()));
}

const string 
CMonomeSerialDlg::StdWStringToStdString(const wstring& str)
{
	return string(CW2A(str.c_str()));
}

//returns the text from GetWindowText as an integer, or returns 0 if not possible.
int 
CMonomeSerialDlg::GetCWndInt(const CWnd& control)
{
	return CStringToInt(GetCWndCString(control));
}

//returns a CString by calling GetWindowText
const CString 
CMonomeSerialDlg::GetCWndCString(const CWnd& control)
{
	CString temp;
	control.GetWindowTextW(temp);
	return temp;
}

const CString 
CMonomeSerialDlg::IntToCString(int i)
{
	CString temp;
	temp.Format(L"%d", i);
	return temp;
}

const std::string 
CMonomeSerialDlg::IntToString(int i)
{
	std::ostringstream oss;
	oss << i;

	return oss.str();
}

int
CMonomeSerialDlg::StringToInt(const std::string& str)
{
	try {
		return atoi(str.c_str());
	}
	catch (...) {
		return 0;
	}
}

int 
CMonomeSerialDlg::CStringToInt(const CString& str)
{
	try {
		return _ttoi(str);
	}
	catch (...) {
		return 0;
	}
}

void CMonomeSerialDlg::monomeDeviceDiscoveredCallback(const std::string& serialNumber, void* userData)
{
	ApplicationController* app = (ApplicationController*)userData;
	app->handleSerialDeviceDiscoveredEvent(serialNumber);
}

void CMonomeSerialDlg::monomeDeviceTerminatedCallback(const std::string& serialNumber, void* userData)
{
	ApplicationController* app = (ApplicationController*)userData;
	app->handleSerialDeviceTerminatedEvent(serialNumber);
}


