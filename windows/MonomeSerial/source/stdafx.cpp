// stdafx.cpp : source file that includes just the standard includes
// MonomeSerial.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <sstream>

/* helper functions for displaying useful messageboxes */

// Error codes: http://msdn2.microsoft.com/en-us/library/ms681381.aspx
void ShowLastError(LPCWSTR message)
{
	std::wostringstream erss;
	erss << GetLastError() << L"\r\n \r\n" << message;
	MessageBox(0, erss.str().c_str(), L"GetLastError() ErrorCode", 0);
}

int Alert(LPCTSTR windowText)
{
	return AfxMessageBox(windowText);
}

int Alert(LPCTSTR windowText, LPCTSTR title)
{
	return MessageBox(NULL, windowText, title, NULL);
}

int Alert(LPCTSTR windowText, LPCTSTR title, int type)
{
	return MessageBox(NULL, windowText, title, type);
}