
// GrStn.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxinet.h>
#include <afxsock.h>
#include <Mmsystem.h>
#include "GrStn.h"
#include "GrStnDlg.h"
#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CGrStnApp

BEGIN_MESSAGE_MAP(CGrStnApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


extern CGrStnDlg *CurentDlgBox;
// CGrStnApp construction
int iGMeasurements;
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
	packet_no = -1;
    SessionNOldProcessed = 0;
    WriteRequested = FALSE;
    StatusDistance = 0;
    memset(DTimeEarth, 0, sizeof(DTimeEarth));
    memset(DTimeLuna, 0, sizeof(DTimeLuna));
    memset(Distance, 0, sizeof(Distance));
    iMeasurements = 0;
    iGMeasurements = iMeasurements;
    Medium = 0.0;
}


// The one and only CGrStnApp object


CGrStnApp theApp;
HINSTANCE hMainInstance;
int Icount = 0;
UINT CallbackThread_Proc(LPVOID lParm)   
{   
	UINT uResult;
	DWORD ResWait;
	HANDLE hList[3];
	BOOL bFound;
	BOOL bRes;
	int iRea;
	long bToRead;
    DWORD Err;
	
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
                    Err = GetLastError();
                    if (Err == ERROR_IO_PENDING ) // read started == it is possible to wait
                    {
                        hList[1] = theApp.Ovlpd.hEvent;
                        break;
                    }
                    else
                    {
                        //error on read
                        PurgeComm(theApp.hComHandle, PURGE_RXCLEAR );
                        ::ResetEvent(theApp.Ovlpd.hEvent);
                        theApp.BytesDownLinkRead = 0;
                        continue;
                    }
				}
                if (theApp.BytesDownLinkRead)
                {
				    // read something in overlapped operation = need to send data to a SatCtrl
				    memset((void*)theApp.bPacket, 0, sizeof(theApp.bPacket));
				    memcpy((void*)theApp.bPacket, (void*)(theApp.bPacketDownLink), theApp.BytesDownLinkRead);
                    for (int iAn=0; iAn <theApp.BytesDownLinkRead; iAn++)
                    {
                        theApp.PrePorocess(theApp.bPacket[iAn]);
                    }
				    theApp.MakeHex();
                    int iSizeToSend = 512;
				    for (int iSend = 0; iSend< theApp.iOutptr; iSend+=iSizeToSend)
				    {
					    if ((theApp.iOutptr - iSend) <= 512)
						    iSizeToSend = (theApp.iOutptr - iSend);
                        else
                        {
                            if (theApp.tmpWebServerResp[iSend+iSizeToSend] == '%')
                                ;
                            else if (theApp.tmpWebServerResp[iSend + iSizeToSend - 2] == '%')
                                iSizeToSend -= 2;
                        }
					    theApp.packet_no++;
					    theApp.SendDownLink("2", &theApp.tmpWebServerResp[iSend], iSizeToSend);
				    }
                }
				theApp.BytesDownLinkRead = 0;
				::ResetEvent(theApp.Ovlpd.hEvent);
			}
		}
		else // just another read ??
		{
			if (GetOverlappedResult(theApp.hComHandle, &theApp.Ovlpd, &theApp.BytesDownLinkRead, FALSE)) // how many was read?
			{
                while(1) 
                {
                    Err = GetLastError();
				    if (theApp.BytesDownLinkRead != 0) // process only when it reads something
				    {
					    memset((void*)theApp.bPacket, 0, sizeof(theApp.bPacket));
					    memcpy((void*)theApp.bPacket, (void*)(theApp.bPacketDownLink), theApp.BytesDownLinkRead);
                        for (int iAn=0; iAn <theApp.BytesDownLinkRead; iAn++)
                        {
                            theApp.PrePorocess(theApp.bPacket[iAn]);
                        }
                        // scan for distance packet
#ifdef _TEST_BIN_DATA
                        theApp.BytesDownLinkRead = 128;
                        for (int iInptr = 0; iInptr< 128; iInptr++)
                        {
                            theApp.bPacket[iInptr] = Icount++;
                        }
#endif
					    theApp.MakeHex();
                        int iSizeToSend = 512;
					    for (int iSend = 0; iSend< theApp.iOutptr; iSend+=iSizeToSend)
					    {
						    
						    if ((theApp.iOutptr - iSend) <= 512)
							    iSizeToSend = (theApp.iOutptr - iSend);
                            else
                            {
                                if (theApp.tmpWebServerResp[iSend+iSizeToSend] == '%')
                                    ;
                                else if (theApp.tmpWebServerResp[iSend + iSizeToSend - 2] == '%')
                                    iSizeToSend -= 2;
                            }
						    theApp.packet_no++;
						    theApp.SendDownLink("2", &theApp.tmpWebServerResp[iSend], iSizeToSend);
					    }
                    }
                    char MyByffer[256];
                    double dSL = 299792458.0;
                    double distance = 0.0;
                    
                    double MediumCount = 1.0;
                    int i = 0;
                    if (theApp.Medium == 0.0)
                       theApp.Medium = (long int)theApp.Distance[0];
                    //else
                    //    i=-1;
                    
                    
                    //theApp.Medium = (long int)theApp.Distance[i];
                    for (int j = i; j< iGMeasurements;j++)
                    {
                        theApp.Medium /=MediumCount;
                        if (((long int)theApp.Distance[j] < (theApp.Medium*10.0)) && ((long int)theApp.Distance[j] > (-theApp.Medium*10.0)))
                        {
                            theApp.Medium *=MediumCount;
                            theApp.Medium += (long int)theApp.Distance[j];
                            MediumCount+=1.0;
                        }
                        else
                            theApp.Medium *=MediumCount;
                    }
                    theApp.Medium/=MediumCount;
                    double Disp = (long int)theApp.Distance[i] - theApp.Medium;
                    Disp *=Disp;
                    for (int j = i+1; j< iGMeasurements;j++)
                    {
                        if (((long int)theApp.Distance[j] < (theApp.Medium*10.0)) && ((long int)theApp.Distance[j] > (theApp.Medium*10.0)))
                        {
                            Disp += ((long int)theApp.Distance[j]-theApp.Medium) * ((long int)theApp.Distance[j]-theApp.Medium);
                        }
                    }
                    Disp = sqrt(Disp)/MediumCount;
                    if (theApp.Distance[0])
                    {
                        sprintf(MyByffer,"%d",(long int)theApp.Distance[i]);
                    
                        CurentDlgBox->GetDlgItem(IDC_EDITCICLES)-> SetWindowText( MyByffer );
                        //sprintf(MyByffer,"%d",(theApp.Distance[0]-210));
                        distance = (long int)theApp.Distance[0]-14;
                        distance = distance/16000000.0 * dSL;
                        distance /=2;
                        sprintf(MyByffer,"%f",distance);
                        CurentDlgBox->GetDlgItem(IDC_EDITDISTANCE_M)-> SetWindowText( MyByffer );

                        sprintf(MyByffer,"%f",theApp.Medium);
                        CurentDlgBox->GetDlgItem(IDC_EDITDISTANCE_M2)-> SetWindowText( MyByffer );

                        sprintf(MyByffer,"%f",Disp);
                        CurentDlgBox->GetDlgItem(IDC_EDITDISP)-> SetWindowText( MyByffer );
                        
                    }
					// in buffer (theApp.bPacketDownLink) already data == needs to send data to SatCtrl
					theApp.BytesDownLinkRead = 0;
					::ResetEvent(theApp.Ovlpd.hEvent);
					memset((void*)(theApp.bPacketDownLink), 0, sizeof(theApp.bPacketDownLink));
				    if (FALSE ==ReadFile(theApp.hComHandle, (LPVOID)((char*)(theApp.bPacketDownLink)+theApp.BytesDownLinkRead), sizeof(theApp.bPacketDownLink)-theApp.BytesDownLinkRead, &theApp.BytesDownLinkRead, &theApp.Ovlpd))
                    {
                        Err = GetLastError();
                        if (Err == ERROR_IO_PENDING ) // read started == it is possible to wait
                            break;
                        else // error on read needs to clear error
                        {
                            PurgeComm(theApp.hComHandle, PURGE_RXCLEAR );
                            continue;
                        }
				    }
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
		ct.ReadTotalTimeoutMultiplier = 1;
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
			m_dcb.fOutxCtsFlow    = TRUE;//FALSE;
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
BOOL CGrStnApp:: WriteFileComHex(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToWrite, LPDWORD lpNBytesWritten, LPOVERLAPPED lpOverlapped)
{
    BYTE *bByte=  &bPacketUpLinkBin[0];
    DWORD BytesToWrite = 0;
    BOOL bRet = FALSE; 
    int iStat=0;
    if (bByte)
    {
        for (int i = 0; i < nNBytesToWrite; i++)
        {
            switch(iStat)
            {
            case 0: 
                if (((BYTE *)lpBuffer)[i] == '%')
                    iStat = 1;
                else
                    bByte[BytesToWrite++] = ((BYTE *)lpBuffer)[i]; 
                break;
            case 1:
                if (((BYTE *)lpBuffer)[i] <= '9')
                    bByte[BytesToWrite] = (((BYTE *)lpBuffer)[i] - '0')<<4;
                else if (((BYTE *)lpBuffer)[i] <= 'F')
                        bByte[BytesToWrite] = (((BYTE *)lpBuffer)[i] - 'A' + 10)<<4;
                     else
                         bByte[BytesToWrite] = (((BYTE *)lpBuffer)[i] - 'a' + 10)<<4;
                iStat = 2;
                break;
            case 2:
                if (((BYTE *)lpBuffer)[i] <= '9')
                    bByte[BytesToWrite] |= (((BYTE *)lpBuffer)[i] - '0');
                else if (((BYTE *)lpBuffer)[i] <= 'F')
                        bByte[BytesToWrite] |= (((BYTE *)lpBuffer)[i] - 'A' + 10);
                     else
                         bByte[BytesToWrite] |= (((BYTE *)lpBuffer)[i] - 'a' + 10);
                BytesToWrite++;
                iStat = 0;
                break;
            }
        }
        //bRet = WriteFileCom(hCommFile, bByte, BytesToWrite, lpNBytesWritten, lpOverlapped);
        // check that prev operation was finished??
        if (WriteRequested)
        {
            GetOverlappedResult(theApp.hComHandle, lpOverlapped, &theApp.BytesUplinkWritten, TRUE);
        }
        ::ResetEvent(lpOverlapped->hEvent);
        if (WriteFile(hCommFile, bByte, BytesToWrite, lpNBytesWritten, lpOverlapped) == FALSE)  // opeartion was started 
        {
            WriteRequested = TRUE;
        }
        else // was an error
        {
            DWORD dwErr = GetLastError();
            PurgeComm( hCommFile, PURGE_TXCLEAR	| PURGE_RXCLEAR );
            WriteRequested = FALSE;
        }
    }
    //delete bByte;
    return bRet;
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
#define RESPONCE_WAS_SENT 0x40
    unsigned char ATCMD =0;
    ATCMD |= RESPONCE_WAS_SENT;
    if (!(ATCMD & RESPONCE_WAS_SENT))
       ATCMD &= (RESPONCE_WAS_SENT ^0xff);
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

	::ResetEvent(hWaitForHandleExit);
	::ResetEvent(hWaitForHandleToReadCommFile);

	memset(&theApp.OvlpdWrite, 0, sizeof(theApp.OvlpdWrite));
	theApp.OvlpdWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ;

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

BOOL CGrStnApp::PrePorocess(unsigned char bByte)
{
    switch(StatusDistance)
    {
    case 0: 
        if (bByte == '9')
            StatusDistance =1;
        break;
    case 1:
        if (bByte == '9')
            StatusDistance =2;
        else
            StatusDistance =0;
        break;
    case 2:
        if (bByte == 'D')
        {
            StatusDistance =3;
            StatusEsc = 0;
            DM2=DM1;
        }
        else
            StatusDistance =0;
        break;
    case 3: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXaTmr1 = bByte;
        StatusDistance =4;
        break;
    case 4:if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXaTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 5;
        break;
    case 5: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXaTmr1H = bByte;
        StatusDistance = 6;
        break;
    case 6: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXaTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 7;
        break;
    case 7: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXbTmr1 = bByte;
        StatusDistance = 8;
        break;
    case 8: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXbTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 9;
        break;
    case 9: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXbTmr1H = bByte;
        StatusDistance = 10;
        break;
    case 10: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.RXbTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 11;
        break;
    case 11: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXaTmr1 = bByte;
        StatusDistance = 12;
        break;
    case 12: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXaTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 13;
        break;
    case 13: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXaTmr1H = bByte;
        StatusDistance = 14;
        break;
    case 14: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXaTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 15;
        break;
    case 15: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXbTmr1 = bByte;
        StatusDistance = 16;
        break;
    case 16: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXbTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 17;
        break;
    case 17: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXbTmr1H = bByte;
        StatusDistance = 18;
        break;
    case 18: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXbTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 19;
        break;
    case 19: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXPeriod = bByte;
        StatusDistance = 20;
        break;
    case 20: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.TXPeriod |= ((WORD)bByte)<<8;
        StatusDistance = 21;
        break;

   case 21:if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXaTmr1 = bByte;
        StatusDistance = 22;
        break;
 
    case 22:if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXaTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 23;
        break;
    case 23: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXaTmr1H = bByte;
        StatusDistance = 24;
        break;
    case 24: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXaTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 25;
        break;
    case 25: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXbTmr1 = bByte;
        StatusDistance = 26;
        break;
    case 26: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXbTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 27;
        break;
    case 27: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXbTmr1H = bByte;
        StatusDistance = 28;
        break;
    case 28: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ERXbTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 29;
        break;
    case 29: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXaTmr1 = bByte;
        StatusDistance = 30;
        break;
    case 30: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXaTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 31;
        break;
    case 31: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXaTmr1H = bByte;
        StatusDistance = 32;
        break;
    case 32: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXaTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 33;
        break;
    case 33: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXbTmr1 = bByte;
        StatusDistance = 34;
        break;
    case 34: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXbTmr1 |= ((WORD)bByte)<<8;
        StatusDistance = 35;
        break;
    case 35: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXbTmr1H = bByte;
        StatusDistance = 36;
        break;
    case 36: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXbTmr1H |= ((WORD)bByte)<<8;
        StatusDistance = 37;
        break;
    case 37: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXPeriod = bByte;
        StatusDistance = 38;
        break;
    case 38: if (StatusEsc) StatusEsc=0; else if (bByte == '#') { StatusEsc = 1; break; }
        DM1.ETXPeriod |= ((WORD)bByte)<<8;
        StatusDistance = 39;
        break;
    case 39:
        if (bByte == '9')
        {
            for (int i= 399; i >0; i --)
            {
                DTimeEarth[i]=DTimeEarth[i-1];
                DTimeLuna[i]=DTimeLuna[i-1];
                Distance[i]=Distance[i-1];
            }
            iMeasurements++;
            iGMeasurements = iMeasurements;
            if (iMeasurements>(sizeof(Distance)/sizeof(DWORD)))
                iMeasurements = (sizeof(Distance)/sizeof(DWORD));

            DTimeEarth[0] = (DWORD)(DM2.ERXaTmr1H - DM2.ETXaTmr1H -1)*(0x10000 - (DWORD)DM2.ETXPeriod) + 
                (0x10000-(DWORD)DM2.ETXaTmr1) + 
                (DWORD)DM2.ERXaTmr1-(DWORD)DM2.ETXPeriod;
            DTimeLuna[0] = ((DWORD)DM1.TXaTmr1H - (DWORD)DM1.RXbTmr1H-1)*(0x10000 - (DWORD)DM2.TXPeriod) +
            (0x10000 - (DWORD)DM1.RXbTmr1) +
                ((DWORD)(DWORD)DM1.TXaTmr1 - (DWORD)DM1.TXPeriod);
            Distance[0]= DTimeEarth[0] - DTimeLuna[0];
            //39 44 
            //2F E5 CB 4F 
            //D8 E3 BF 4F <-rx 4fbf e3d8
            //E7 5B C4 4F ->tx 4fc4 5be7
            //E9 5B B8 4F 
            //45 40 
            //87 92 F9 2D <-RX 2df9 9287
            //EE 93 ED 2D 
            //C9 5B F5 2D ->TX 2df5 5bc9
            //CA 5B E9 2D 
            //27 40 
            //39 
            //
            //39 44 
            //95 E6 D7 4F 
            //2F E5 CB 4F <-rx 4fcb e52f
            //E8 5B D0 4F ->tx 4fd0 5be8
            //E7 5B C4 4F 
            //45 40 
            //21 91 05 2E <-RX 2e05 9121
            //87 92 F9 2D 
            //C9 5B 01 2E ->TX 2e01 5bc9
            //C9 5B F5 2D 
            //27 40 
            //39 

            StatusDistance =1;
        }
        else
        {

            StatusDistance =0;
        }
        break;
    }
    return TRUE;
}
BOOL CGrStnApp::MakeHex(void)
{
	int iInptr;
	memset(tmpWebServerResp, 0, sizeof(tmpWebServerResp));
    
	for (iInptr= 0,iOutptr=0; iInptr< BytesDownLinkRead;iInptr++)
	{
		if ((bPacket[iInptr] >= '0' && bPacket[iInptr] <= '9') ||
			(bPacket[iInptr] >= 'a' && bPacket[iInptr] <= 'z') ||
			(bPacket[iInptr] >= 'A' && bPacket[iInptr] <= 'Z') //||
		   )
		{
			tmpWebServerResp[iOutptr++] = bPacket[iInptr];
		}
		else
		{
			tmpWebServerResp[iOutptr++]='%';
			if (((bPacket[iInptr]>>4)&0x0f) >= 10)
				tmpWebServerResp[iOutptr++] = 'A'+((bPacket[iInptr]>>4)&0x0f)-10;
			else
				tmpWebServerResp[iOutptr++] = '0'+((bPacket[iInptr]>>4)&0x0f);
			if ((bPacket[iInptr]&0x0f) >= 10)
				tmpWebServerResp[iOutptr++] = 'A'+(bPacket[iInptr]& 0x0f)-10;
			else
				tmpWebServerResp[iOutptr++] = '0'+(bPacket[iInptr]& 0x0f);
		}
	}
	tmpWebServerResp[iOutptr++]=0;
	return TRUE;
}

BOOL CGrStnApp::MakeRQ(char *StartPtr, int iLen)
{
	char szTemp[4096*4];
	int iInptr;
	SYSTEMTIME SystemTime;
	GetSystemTime(  &SystemTime  );
	CTime rTime ( SystemTime );
	sprintf(szTemp, "%03d",SystemTime.wMilliseconds);
	//CString Ct = rTime.FormatGmt("%m/%d/%y %H:%M:%S.");
    char szFormatTime[100];
    sprintf(szFormatTime, "%02d/%02d/%02d %02d:%02d%:%02d.",SystemTime.wYear-2000,SystemTime.wMonth,SystemTime.wDay,SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond);
    CString Ct = szFormatTime;
	Ct = Ct + szTemp;
	strcpy(gs_time, (char*)Ct.GetString());
	strcpy(d_time, (char*)Ct.GetString());
	memset(szTemp, 0, sizeof(szTemp));
	memcpy(szTemp, StartPtr, iLen);

	//sprintf(szWebServerRQ,"http://%s:%d/Post.aspx?packet_type=%s&session_no=%ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&bPacket=%s",
	//	szURL, UrlPort, packet_type,SessionN,packet_no,g_station,gs_time,d_time,bPacket);
	if (memcmp(szURL,"localhost",sizeof("localhost")-1)==0)
		sprintf(szWebServerRQ,"Post.aspx?packet_type=%s&session_no=%010ld&packet_no=%05ld&g_station=%s&gs_time=%s&d_time=%s&package=%s",
		packet_type,SessionN,packet_no,g_station,gs_time,d_time,szTemp);
	else
		sprintf(szWebServerRQ,"SatCtrl/Post.aspx?packet_type=%s&session_no=%010ld&packet_no=%05ld&g_station=%s&gs_time=%s&d_time=%s&package=%s",
		packet_type,SessionN,packet_no,g_station,gs_time,d_time,szTemp);

	return TRUE;
}

BOOL CGrStnApp::OnIdle(LONG lCount)
{
	// TODO: Add your specialized code here and/or call the base class
	if (CurentDlgBox)
		CurentDlgBox->CheckIdleConnects();
	return CWinApp::OnIdle(lCount);

}

BOOL CGrStnApp::SendDownLink(char *pktType, char *StartData, int iSizeToSend)
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
		if (MakeRQ(StartData, iSizeToSend))
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
							int iNL = -1;
							int iCountNl = 0;
							CString strPart = strTemp;
							while((iNL = strPart.Find("\r\n"))>=0)
							{
								iCountNl++;
								strPart = strPart.Mid(iNL+2);
							}
							if (iCountNl >MAX_LINES)
							{
								strPart = strTemp;
								for (int iCut = 0; iCut < iCountNl -MAX_LINES; iCut++)
								{
									iNL = strPart.Find("\r\n");
									strPart = strPart.Mid(iNL+2);
								}
								strTemp = strPart;
							}
							
							strTemp += "\r\n";
							//strTemp += (char*)bPacket;
                            strTemp += MakeItReadable((char*)bPacket, BytesDownLinkRead);
							CurentDlgBox->m_DownLink.SetWindowText(strTemp);
							//CurentDlgBox->GetDlgItem(IDC_EDIT_DOWNLINK)->SetWindowTextA(strTemp);
							
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
CString CGrStnApp::MakeItReadable(char * packet, int length)
{
    CString StrOutput;
    for (int i = 0; i < length; i++)
    {
        if ((packet[i] >= ' ') && (packet[i] <= '~'))
        {
            StrOutput += packet[i];
        }
        else
        {
            StrOutput += "%";

            char Simb1 = (packet[i]>>4)&0x0f;
            char Simb2 = (packet[i]& 0x0f);
            if (Simb1>=10)
                Simb1 = 'A' + Simb1 - 10;
            else
                Simb1 = '0' + Simb1;
            if (Simb2>=10)
                Simb2 = 'A' + Simb2 - 10;
            else
                Simb2 = '0' + Simb2;
            StrOutput += Simb1;
            StrOutput += Simb2;
        }
            

    }
    return StrOutput;
}


