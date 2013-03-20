
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


// CGrStnApp construction

CGrStnApp::CGrStnApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_MainHttpServer = NULL;
	m_MainInternetConnection = NULL;
	m_wwwPort = 80;			// well-known WWW port number
	m_nMaxConnects = 32;	// max number of connections we'll allow
	m_nTimeOut = 20;		// idle-client disconnect limit
	m_nSanityTime = 60;		// watchdog timer period
}


// The one and only CGrStnApp object

CGrStnApp theApp;
HINSTANCE hMainInstance;

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


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CGrStnApp::MakeRQ(void)
{
	//sprintf(szWebServerRQ,"http://%s:%d/Post.aspx?packet_type=%s&session_no=%ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&bPacket=%s",
	//	szURL, UrlPort, packet_type,SessionN,packet_no,g_station,gs_time,d_time,bPacket);
	sprintf(szWebServerRQ,"Post.aspx?packet_type=%s&session_no=%ld&packet_no=%ld&g_station=%s&gs_time=%s&d_time=%s&package=%s",
		packet_type,SessionN,packet_no,g_station,gs_time,d_time,bPacket);
	return TRUE;
}