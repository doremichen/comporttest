/*
 * main.cpp
 *
 *  Created on: 2013/12/20
 *      Author: AdamChen
 */
#include <conio.h>
#include <iostream>
#include <windows.h>

using namespace std;

#define BUFF_SIZE 200


HANDLE	hPort1;

HANDLE ghWriteEvent;

HANDLE  ghReadThread;
HANDLE  ghWriteThread;

DCB		PortDCB;
LPCTSTR comNum;

DWORD WINAPI ReadThreadProc(LPVOID);
DWORD WINAPI WriteThreadProc(LPVOID);

int SetCOMConfig(HANDLE Port)
{
	cout << __FUNCTION__ << endl;
	int ret = 1;   //success
	int bStatus = 0;

	// Set Port parameters.
	// Make a call to GetCommState() first in order to fill
	// the comSettings structure with all the necessary values.
	// Then change the ones you want and call SetCommState().
	GetCommState(Port, &PortDCB);
	PortDCB.BaudRate = 460800;
	PortDCB.StopBits = ONESTOPBIT;
	PortDCB.ByteSize = 8;
	PortDCB.Parity   = NOPARITY;
	PortDCB.fParity  = FALSE;
	bStatus = SetCommState(Port, &PortDCB);

	if(bStatus == 0) {
		cerr << __FUNCTION__ << " set port config fail.." << endl;
		ret = 0;
	}

	return ret;
}

int setTimeOutConfig(HANDLE Port)
{
	cout << __FUNCTION__ << endl;
	int ret = 1; //success
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout        = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier    = 10;
	timeouts.ReadTotalTimeoutConstant    = 100;
	timeouts.WriteTotalTimeoutMultiplier    = 10;
	timeouts.WriteTotalTimeoutConstant    = 100;

	if (!SetCommTimeouts(Port, &timeouts)) {
		ret = 0;
		cerr << __FUNCTION__ << " set comm time fail..." << endl;
	}
	else {
		cout << __FUNCTION__ << " set comm time success..." << endl;
	}

	return ret;
}


int CreateThread(void)
{
	cout << __FUNCTION__ << endl;
	int ret = 1; //success

	DWORD dwRThreadID;
	DWORD dwWThreadID;

	// Create a manual-reset event object. The write thread sets this
	// object to the signaled state when it finishes writing to a
	// shared buffer.
	ghWriteEvent = CreateEvent(
	        NULL,               // default security attributes
	        FALSE,               // manual-reset event
	        FALSE,              // initial state is nonsignaled
	        TEXT("WriteEvent")  // object name
	        );

	if (ghWriteEvent == NULL) {
	        cerr << __FUNCTION__ << "CreateEvent failed ( " << GetLastError() << " )" << endl;
	        ret = 0;
	        goto EXIT;
	}


	ghReadThread = CreateThread(
			NULL,
			0,
			ReadThreadProc,
			NULL,
			0,
			&dwRThreadID);

	ghWriteThread = CreateThread(
				NULL,
				0,
				WriteThreadProc,
				NULL,
				0,
				&dwWThreadID);

	if(ghReadThread == NULL ||
	   ghWriteThread ==  NULL) {
		ret = 0;

		cerr << __FUNCTION__ << " Create thread fail.." << endl;
	}
	else {
		cout << __FUNCTION__ << " Create thread success.." << endl;
	}
EXIT:
	return ret;
}

int main(int argc, const char *argv[])
{
    int index = 0;
    char Text[10] = {0};
    char prefix[] = "\\\\.\\";
    int count = 0;   //for argv string length
    int status_config = 0;
    int status_timeconf = 0;

//    pText = new char[10];

	cout << __FUNCTION__ << " welcome ..." << endl;

//	memset(pText, 0, 10);

	index++;

//	cout << "sizeof(prefix) = " << sizeof(prefix) << endl;
//	cout << "sizeof argv[index] = " << sizeof(argv[index]) << endl;
//
//	cout << "argv[index] =  " << argv[index] << endl;


    for(int i = 0; argv[index][i]!='\0'; ++i) {
//    	cout << "argv[index][" << i << "] = " << argv[index][i] << endl;
    	count++;
    }

    cout << __FUNCTION__ << " count = "<< count << endl;

    if(count < 5) {   //com number: 0~9
    	memcpy(Text, argv[index], count);
    }
    else {				//com number: 10~xx
    	memcpy(Text, prefix, sizeof(prefix));
    	memcpy(Text+4, argv[index], count);
    }


//    cout << "Text = " << Text << endl;

    comNum = (LPCTSTR)Text;
//	comNum = (LPCTSTR)argv[index];

//	cout << comNum << endl;

	hPort1 = CreateFile(
			comNum,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			0);

	if(hPort1 == INVALID_HANDLE_VALUE) {
		cerr << __FUNCTION__ << " Open Com port fail.." << endl;
		goto Exit;
	}
	else {
		cout << __FUNCTION__ << " Open Com port success: comNum[ " << argv[index] << " ]" << endl;
	}

	status_config = SetCOMConfig(hPort1);

	if(status_config == 1) {
		cout << __FUNCTION__ << " set config success.." << endl;
	}
	else {
		cerr << __FUNCTION__ << " set config fail.." << endl;
		goto Exit;
	}

	status_timeconf = setTimeOutConfig(hPort1);

	if(status_timeconf == 1) {
		cout << __FUNCTION__ << " set time config success.." << endl;
	}
	else {
		cerr << __FUNCTION__ << " set time config fail.." << endl;
		goto Exit;
	}


	//test...
	CreateThread();

//	getch();
	while(!_kbhit()) {
//		cout << __FUNCTION__ << " hit me..." << endl;
	}

Exit:

	return 0;
}


DWORD WINAPI ReadThreadProc(LPVOID)
{
	cout << __FUNCTION__ << endl;
	BOOL bErrorFlag = FALSE;

	char szBuffer[BUFF_SIZE];
	DWORD  dwBytesRead;
	DWORD dwWaitResult;

	cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " waiting for write event..." << endl;

	dwWaitResult = WaitForSingleObject(
	        ghWriteEvent, // event handle
	        INFINITE);    // indefinite wait

	if(dwWaitResult == WAIT_FAILED) {
		cerr << __FUNCTION__ << "WaitForSingleObject failed ( " << GetLastError() << " )" << endl;
		return 0;
	}


	bErrorFlag = ReadFile(
			hPort1,
			szBuffer,
			sizeof(szBuffer),
			&dwBytesRead,
			NULL);
	if(bErrorFlag == FALSE) {
		cout << __FUNCTION__ << " read com port fail.. dwBytesRead = " << dwBytesRead << endl;
	}
	else {
		cout << __FUNCTION__ << " read com port success.. dwBytesRead = " << dwBytesRead << endl;
		cout << __FUNCTION__ << " read com port success.. szBuffer = " << szBuffer << endl;
	}

	return 0;
}

DWORD WINAPI WriteThreadProc(LPVOID)
{
	cout << __FUNCTION__ << endl;

	BOOL bErrorFlag = FALSE;

	DWORD dwBytesToWrite;
	DWORD dwBytesWritten;

	wchar_t szwcsBuffer[BUFF_SIZE] = L"11 22 33 44";
	char szBuffer[BUFF_SIZE];

//	if(!GetTextResponse(_T("11 22 33 44"),
//			szwcsBuffer, BUFF_SIZE)) {
//
//	}
	//convert to ANSI character set
	dwBytesToWrite = wcstombs(szBuffer, szwcsBuffer, BUFF_SIZE);
	szBuffer[dwBytesToWrite++] = '\r';
	szBuffer[dwBytesToWrite++] = '\n';

	cout << __FUNCTION__  << " szBuffer = " << szBuffer << endl;
	cout << __FUNCTION__  << " hPort1 = " << hPort1 << endl;

//	while(true) {
//
//		cout << __FUNCTION__ << "run..." << endl;

		bErrorFlag = WriteFile(
					hPort1,
					szBuffer,
					dwBytesToWrite,
					&dwBytesWritten,
					NULL);

			if(bErrorFlag == FALSE) {
				cout << __FUNCTION__ << " write com port fail.. dwBytesWritten = " << dwBytesWritten << endl;
			}
			else {
				cout << __FUNCTION__ << " write com port success.. dwBytesWritten = " << dwBytesWritten << endl;
				if (! SetEvent(ghWriteEvent) )
				{
				    cerr << __FUNCTION__ << "SetEvent failed ( " << GetLastError() << " )" << endl;

				}
			}
//	}
	return 0;
}
