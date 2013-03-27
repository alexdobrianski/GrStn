
// GrStn.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxinet.h>
#include <afxsock.h>
#include <Mmsystem.h>
#include "GrStn.h"
#include "GrStnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGrStnApp

BEGIN_MESSAGE_MAP(CGrStnApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


extern CGrStnDlg *CurentDlgBox;
// CGrStnApp construction

CGrStnApp::CGrStnApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_MainHttpServer = NULL;
	m_MainInternetConnection = NULL;
	m_wwwPort = 6921;			// well-known WWW port number
	m_nMaxConnects = 32;	// max number of connections we'll allow
	m_nTimeOut = 20;		// idle-client disconnect limit
	m_nSanityTime = 60;		// watchdog timer period
	CurentDlgBox = NULL;
	ItIsPOssibelToOpenComm = FALSE;
	hComHandle = INVALID_HANDLE_VALUE;
	ServerOnline = FALSE;
}


// The one and only CGrStnApp object

CGrStnApp theApp;
HINSTANCE hMainInstance;

UINT CallbackThread_Proc(LPVOID lParm)   
{   
	UINT uResult;
	DWORD ResWait;
	HANDLE hList[3];
	BOOL bFound;
	BOOL bRes;
	int iRea;
	long bToRead;
	
	hList[0] = theApp.hWaitForHandleExit;
	hList[1] = theApp.hWaitForHandleToReadCommFile;
	theApp.BytesDownLinkRead = 0;
	while(ResWait = ::WaitForMultipleObjects(2,hList,FALSE,INFINITE) != WAIT_OBJECT_0)
	{
		if (hList[1] == theApp.hWaitForHandleToReadCommFile) // ?? needs to start read oparation
		{
			memset(&theApp.Ovlpd, 0, sizeof(theApp.Ovlpd));
			theApp.Ovlpd.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ;
			while(1)
			{
				if (FALSE == ReadFile(theApp.hComHandle, (LPVOID)((char*)(theApp.bPacketDownLink)+theApp.BytesDownLinkRead), sizeof(theApp.bPacketDownLink)-theApp.BytesDownLinkRead, &theApp.BytesDownLinkRead, &theApp.Ovlpd))
				{
					hList[1] = theApp.Ovlpd.hEvent;
					break;
				}
				// read something in overlapped operation = need to send data to a SatCtrl
			}
		}
		else // just another read ??
		{
			if (GetOverlappedResult(theApp.hComHandle, &theApp.Ovlpd, &theApp.BytesDownLinkRead, FALSE)) // how many was read?
			{
				while(1) 
				{
					memset((void*)theApp.bPacket, 0, sizeof(theApp.bPacket));
					memcpy((void*)theApp.bPacket, (void*)(theApp.bPacketDownLink), theApp.BytesDownLinkRead);
					theApp.packet_no++;
					theApp.SendDownLink("2");
					// in buffer (theApp.bPacketDownLink) already data == needs to send data to SatCtrl
					theApp.BytesDownLinkRead = 0;
					::ResetEvent(theApp.Ovlpd.hEvent);
					memset((void*)(theApp.bPacketDownLink), 0, sizeof(theApp.bPacketDownLink));
				    if (FALSE ==ReadFile(theApp.hComHandle, (LPVOID)((char*)(theApp.bPacketDownLink)+theApp.BytesDownLinkRead), sizeof(theApp.bPacketDownLink)-theApp.BytesDownLinkRead, &theApp.BytesDownLinkRead, &theApp.Ovlpd))
						break;
				}
			}
		}
	}
	::SetEvent(theApp.hWaitForHandleDoneExit);
	
	uResult = 0;   
	return(uResult);   
}

BOOL CGrStnApp::OpenCommPort(char *szCom)
{
	COMMTIMEOUTS ct;
	DCB m_dcb;
	DWORD EvtMask = EV_RXCHAR | EV_ERR; 
	if (INVALID_HANDLE_VALUE != hComHandle)
	{
		CloseHandle(hComHandle);
		hComHandle = INVALID_HANDLE_VALUE;
	}
	hComHandle = CreateFile(szCom,
							GENERIC_READ | GENERIC_WRITE,
                            0, 
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED,//NULL,
                            NULL);
	if (INVALID_HANDLE_VALUE != hComHandle)
    {
		GetCommTimeouts( hComHandle, &ct );
		ct.ReadIntervalTimeout = 100;
		ct.ReadTotalTimeoutMultiplier = MAXDWORD;
		ct.ReadTotalTimeoutConstant = 100000;
		ct.WriteTotalTimeoutMultiplier = 5;
		ct.WriteTotalTimeoutConstant = 1000;
		SetCommTimeouts( hComHandle, &ct );

		m_dcb.DCBlength = sizeof( m_dcb );
		if ( GetCommState( hComHandle, &m_dcb ) )
		{
			m_dcb.BaudRate = CBR_57600;
			m_dcb.ByteSize = 8;
			m_dcb.Parity   = NOPARITY;
			m_dcb.StopBits = ONESTOPBIT;
			m_dcb.fBinary  = TRUE;
			m_dcb.fParity  = FALSE;
			m_dcb.fOutxCtsFlow    = FALSE;
			m_dcb.fOutxDsrFlow    = FALSE;
			m_dcb.fRtsControl     = RTS_CONTROL_DISABLE;
			m_dcb.fDtrControl     = DTR_CONTROL_DISABLE;
			m_dcb.fDsrSensitivity = FALSE;
			m_dcb.fOutX           = FALSE;
			m_dcb.fInX            = FALSE;
			m_dcb.fTXContinueOnXoff = FALSE;
			m_dcb.fErrorChar        = FALSE;
			m_dcb.fNull             = FALSE;
			m_dcb.fAbortOnError     = FALSE;//TRUE; // handle errors
			m_dcb.ErrorChar         = 0xff; // error replacement character 
			SetCommState( hComHandle, &m_dcb );
			SetCommMask(  hComHandle, EvtMask);
            EscapeCommFunction( hComHandle,CLRDTR);
			EscapeCommFunction( hComHandle,SETRTS);
			// now needs to inform the thread to run read and wait for IO finished.
			::SetEvent(theApp.hWaitForHandleToReadCommFile);

			return TRUE;
		}
		CloseHandle(hComHandle);
		hComHandle = INVALID_HANDLE_VALUE;
	}
	return FALSE;
}
BOOL CGrStnApp:: WriteFileCom(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToWrite, LPDWORD lpNBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (WriteFile(hCommFile, lpBuffer, nNBytesToWrite, lpNBytesWritten, lpOverlapped))
	{
		FlushFileBuffers(hCommFile );
 
		return TRUE;
	}
	else
	{
		//for (int iatt=0; iatt<5; iatt++)
		{
			DWORD dwErr = GetLastError();
			if (PurgeComm( hCommFile, PURGE_TXCLEAR	| PURGE_RXCLEAR ))
			{
				if (WriteFile(hCommFile, lpBuffer, nNBytesToWrite, lpNBytesWritten, lpOverlapped))
				{
					FlushFileBuffers(hCommFile );
					return TRUE;
				}
			}
		}

	}
	return FALSE;
}
BOOL CGrStnApp::ReadFileCom(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToRead, LPDWORD lpNBytesRead, LPOVERLAPPED lpOverlapped)
{
	if (ReadFile(hCommFile, (LPVOID)lpBuffer, nNBytesToRead, lpNBytesRead, lpOverlapped))
	{
		//FlushFileBuffers(hFile );
 
		return TRUE;
	}
	else
	{
		//for (int iatt=0; iatt<5; iatt++)
		{
			DWORD dwErr = GetLastError();
			if (PurgeComm( hCommFile, PURGE_TXCLEAR	| PURGE_RXCLEAR ))
			{
				if (ReadFile(hCommFile, (LPVOID)lpBuffer, nNBytesToRead, lpNBytesRead, lpOverlapped))
				{
					//FlushFileBuffers(hFile );
					return TRUE;
				}
			}
		}

	}
	return FALSE;
}


// CGrStnApp initialization

BOOL CGrStnApp::InitInstance()
{
	char szMutexName[_MAX_PATH];
	hMainInstance = m_hInstance ;
	GetModuleFileName( m_hInstance, szFileName, sizeof(szFileName));
	strcpy(szMutexName,szFileName);
	for(int imtxn = 0; imtxn < strlen(szMutexName); imtxn++)
	{
		switch(szMutexName[imtxn])
		{
		case '/':
		case '\\':
		case ' ':
		case ':':
		case '.':
			szMutexName[imtxn] = '_';

		default:
			break;
		}
	}

	MyMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, szMutexName); 
	if (MyMutex == NULL)
	{
		MyMutex = CreateMutex( NULL, TRUE, szMutexName); 
	}
	else
		return FALSE;
	AfxSocketInit();
	GetModuleFileName( m_hInstance, szIniFileName, sizeof(szIniFileName));
	if (strrchr(szIniFileName, '.'))
	{
		*strrchr(szIniFileName, '.') = 0;
		strcat(szIniFileName, ".ini");
	}

	char szTemp[MAX_PATH];

	GetModuleFileName( m_hInstance, szTemp, sizeof(szTemp));
	if (strrchr(szTemp,'\\'))
	{
		* (strrchr(szTemp,'\\')) = 0;
	}
	GetPrivateProfileString( "Server", "URL", "192.168.0.102", szTemp, sizeof(szTemp), szIniFileName);
	strcpy(szURL,szTemp);

	GetPrivateProfileString( "Station", "Station", "V", szTemp, sizeof(szTemp), szIniFileName);
	strcpy(g_station,szTemp);

	GetPrivateProfileString( "Server", "URLPORT", "80", szTemp, sizeof(szTemp), szIniFileName);
	UrlPort = atoi(szTemp);

	GetPrivateProfileString( "Station", "PORT", "COM1", szTemp, sizeof(szTemp), szIniFileName);
	strcpy(szComPort, szTemp);


	CWinApp::InitInstance();

	hWaitForHandleExit = ::CreateEvent(NULL, TRUE, TRUE, NULL);
	hWaitForHandleToReadCommFile = ::CreateEvent(NULL, TRUE, TRUE, NULL);
	hWaitForHandleDoneExit = ::CreateEvent(NULL, TRUE, TRUE, NULL);

	hWaitForHandleToReadCommFile = ::CreateEvent(NULL, TRUE, TRUE, NULL);
	::ResetEvent(hWaitForHandleExit);
	::ResetEvent(hWaitForHandleToReadCommFile);

	Callback_ReadThread = ::CreateThread( NULL,0x1000, (LPTHREAD_START_ROUTINE)CallbackThread_Proc, NULL, 0L, &dwServiceStateThreadID ) ;




	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CGrStnDlg dlg;
	m_pMainWnd = &dlg;
	dlg.m_URL = szURL;
	sprintf(szTemp, "%d",UrlPort);
	dlg.m_URL_PORT = szTemp;
	
	dlg.m_Station = g_station;

	INT_PTR nResponse = dlg.DoModal();
	::SetEvent(hWaitForHandleExit);
	
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	WritePrivateProfileString("Server", "URL", szURL,szIniFileName);
	sprintf(szTemp, "%d",UrlPort);
	WritePrivateProfileString("Server", "URLPORT", szTemp,szIniFileName);
	WritePrivateProfileString("Station", "Station", g_station,szIniFileName);
	WritePrivateProfileString("Station", "PORT", szComPort,szIniFileName);
	
	if (m_MainHttpServer)
	{
		m_MainHttpServer->Close();
		delete m_MainHttpServer;
	}

	if (m_MainInternetConnection)
	{
		m_MainInternetConnection->Close();
		delete m_MainInternetConnection;
	}
	::WaitForSingleObject(hWaitForHandleDoneExit,5000);
	::CloseHandle(hWaitForHandleDoneExit);
	::CloseHandle(hWaitForHandleExit);
	::CloseHandle(hWaitForHandleToReadCommFile);

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CGrStnApp::MakeRQ(void)
{
	char tmpWebServerResp[4096*3];
	int iOutptr, iInptr;
	SYSTEMTIME SystemTime;
	GetSystemTime(  &SystemTime  );
	CTime rTime ( SystemTime );
	char szTemp[MAX_PATH];
	sprintf(szTemp, "%03d",SystemTime.wMilliseconds);
	CString Ct = rTime.FormatGmt("%m/%d/%y %H:%M:%S.");
	Ct = Ct + szTemp;
	strcpy(gs_time, (char*)Ct.GetString());
	strcpy(d_time, (char*)Ct.GetString());
	memset(tmpWebServerResp, 0, sizeof(tmpWebServerResp));
	for (iInptr= 0,iOutptr=0; iInptr< BytesDownLinkRead;iInptr++)
	{
		if ((bPacket[iInptr] >= '0' && bPacket[iInptr] <= '9') ||
			(bPacket[iInptr] >= 'a' && bPacket[iInptr] <= 'z') ||
			(bPacket[iInptr] >= 'A' && bPacket[iInptr] <= 'Z') ||
			(bPacket[iInptr] == '/') ||
			(bPacket[iInptr] == ';') ||
			(bPacket[iInptr] == '=') ||
			(bPacket[iInptr] == '?') ||
			(bPacket[iInptr] == '@') ||
			(bPacket[iInptr] == ':') ||
			(bPacket[iInptr] == '!') ||
            (bPacket[iInptr] == '#') ||
			(bPacket[iInptr] == '*') ||
			(bPacket[iInptr] == '_') ||
			(bPacket[iInptr] == '\'')
			)
		{
			tmpWebServerResp[iOutptr++] = bPacket[iInptr];
		}
		else
		{
			tmpWebServerResp[iOutptr++]='%';
			if ((bPacket[iInptr]>>4) >= 10)
				tmpWebServerResp[iOutptr++] = 'A'+(bPacket[iInptr]>>4)-10;
			else
				tmpWebServerResp[iOutptr++] = '0'+(bPacket[iInptr]>>4);
			if ((bPacket[iInptr]&0x0f) >= 10)
				tmpWebServerResp[iOutptr++] = 'A'+(bPacket[iInptr]& 0x0f)-10;
			else
				tmpWebServerResp[iOutptr++] = '0'+(bPacket[iInptr]& 0x0f);
		}
	}
	tmpWebServerResp[iOutptr++]=0;

	//sprintf(szWebServerRQ,"http://%s:%d/Post.aspx?packet_type=%s&session_no=%ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&bPacket=%s",
	//	szURL, UrlPort, packet_type,SessionN,packet_no,g_station,gs_time,d_time,bPacket);
	if (memcmp(szURL,"localhost",sizeof("localhost")-1)==0)
		sprintf(szWebServerRQ,"Post.aspx?packet_type=%s&session_no=%010ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&package=%s",
		packet_type,SessionN,packet_no,g_station,gs_time,d_time,tmpWebServerResp);
	else
		sprintf(szWebServerRQ,"SatCtrl/Post.aspx?packet_type=%s&session_no=%010ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&package=%s",
		packet_type,SessionN,packet_no,g_station,gs_time,d_time,tmpWebServerResp);

	return TRUE;
}

BOOL CGrStnApp::OnIdle(LONG lCount)
{
	// TODO: Add your specialized code here and/or call the base class
	if (CurentDlgBox)
		CurentDlgBox->CheckIdleConnects();
	return CWinApp::OnIdle(lCount);

}

BOOL CGrStnApp::SendDownLink(char *pktType)
{
	char szTemp[_MAX_PATH];
	ServerOnline = FALSE;
	if (m_MainHttpServer == NULL)
	{
		m_MainInternetConnection =
				new CInternetSession("SessionToControlServer",
				12,INTERNET_OPEN_TYPE_DIRECT,
				NULL, // proxi name
				NULL, // proxi bypass
				INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_TRANSFER_BINARY);
		try
		{
			m_MainHttpServer = 
						m_MainInternetConnection->GetHttpConnection( 
						szURL, 
						0,
						UrlPort,
						NULL,
						NULL );

		}
		catch(CInternetException *e)
		{
			m_MainHttpServer = NULL;
		}
	}
	if (m_MainHttpServer)
	{
		if (SessionN == 0)
			SessionN = 0xffffffff;
		strcpy(packet_type,pktType); // test from Ground Station
		//03/19/13 08:13:09.937
		if (MakeRQ())
		{
			CHttpFile* myCHttpFile = NULL;
			try
			{
				myCHttpFile = m_MainHttpServer->OpenRequest( CHttpConnection::HTTP_VERB_GET,
					szWebServerRQ,
					NULL,//((CGrStnApp*)AfxGetApp())->szLoginRQ,
					NULL,//12345678,
					NULL, 
					NULL, 
					INTERNET_FLAG_EXISTING_CONNECT|
					INTERNET_FLAG_DONT_CACHE|
					INTERNET_FLAG_RELOAD );
			}
			catch(CInternetException *e)
			{
				myCHttpFile = NULL;
			}

			if (myCHttpFile !=NULL)
			{
				try
				{
					myCHttpFile->SendRequest();
					memset(szWebServerResp, 0, sizeof(szWebServerResp));
					{
						DWORD dwSize;
						CString strSize;
						myCHttpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH,strSize);
						dwSize = atoi(strSize.GetString());
						ServerOnline = TRUE;
						if (dwSize > (sizeof(szWebServerResp)-1))
						{
							for (DWORD dwread=0; dwread < dwSize; dwread+= (sizeof(szWebServerResp)-1))
							{
								if ((dwSize - dwread) > (sizeof(szWebServerResp)-1))
									myCHttpFile->Read(&szWebServerResp,(sizeof(szWebServerResp)-1));
								else
									myCHttpFile->Read(&szWebServerResp,(dwSize - dwread));
							}
						}
						else
							myCHttpFile->Read(&szWebServerResp,dwSize);
						if (strstr((char*)szWebServerResp,"MAX_session_no="))
						{
							char *szNum = strstr((char*)szWebServerResp,"MAX_session_no=");
							SessionN = atol(szNum+sizeof("MAX_session_no=")-1);
							SessionNSet = TRUE;
							sprintf(szTemp,"%ld", SessionN);
							CurentDlgBox->GetDlgItem(IDC_STATIC_SESSION_N)->SetWindowTextA(szTemp);
							CString strTemp;
							CurentDlgBox->GetDlgItem(IDC_EDIT_DOWNLINK)->GetWindowTextA(strTemp);
							strTemp += "\r\n\0";
							strTemp += (char*)bPacket;
							CurentDlgBox->GetDlgItem(IDC_EDIT_DOWNLINK)->SetWindowTextA(strTemp);
							
							CurentDlgBox->GetDlgItem(IDC_STATIC_SESSION_N)->SetWindowTextA(szTemp);
						}
					}
					
				}
				catch(CInternetException *e)
				{
					//ptrApp->m_MainHttpServer = NULL;
				}
				myCHttpFile->Close();
				delete myCHttpFile;
			}
		}
	}
	if (ServerOnline)
		CurentDlgBox->GetDlgItem(IDC_STATIC_SERVER_STATUS)->SetWindowTextA("OnLine");
	else
		CurentDlgBox->GetDlgItem(IDC_STATIC_SERVER_STATUS)->SetWindowTextA("OffLine");
	return TRUE;
}


