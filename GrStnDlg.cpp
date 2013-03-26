
// GrStnDlg.cpp : implementation file
//

#include "stdafx.h"
#include <afxinet.h>
#include <afxsock.h>
#include <Mmsystem.h>
#include "GrStn.h"
#include "GrStnDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnButtonPingServer();
	afx_msg void OnButtonPingGrstn();
protected:
//	afx_msg LRESULT OnKillSocket(WPARAM wParam, LPARAM lParam);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_COMMAND(IDC_BUTTON_PING_SERVER, &CAboutDlg::OnButtonPingServer)
	ON_COMMAND(IDC_BUTTON_PING_GRSTN, &CAboutDlg::OnButtonPingGrstn)
//	ON_MESSAGE(WM_KILL_SOCKET, &CAboutDlg::OnKillSocket)
END_MESSAGE_MAP()


// CGrStnDlg dialog

CGrStnDlg *CurentDlgBox;


CGrStnDlg::CGrStnDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGrStnDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_URL = _T("");
	m_URL_PORT = _T("");
	m_Uplink = _T("");
	m_DownLink = _T("");
	m_GrStnStatus = _T("");
	m_ServerStatus = _T("");
	m_SessionN = _T("");
	m_Station = _T("");
	ptrApp = ((CGrStnApp*)AfxGetApp());
	CurentDlgBox = this;
	FirstRun = FALSE;
}

void CGrStnDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, IDC_EDIT_UPLINK, m_Uplink);
	//  DDX_Control(pDX, IDC_EDIT_URL_PORT, m_URL);
	//  DDX_Control(pDX, IDC_EDIT_URL, m_URL);
	//  DDX_Control(pDX, IDC_EDIT_URL_PORT, m_URL_PORT);
	//  DDX_Control(pDX, IDC_STATIC_GRSTN_STATUS, m_GrStnStatus);
	//  DDX_Control(pDX, IDC_STATIC_SERVER_STATUS, m_ServerStatus);
	//  DDX_Control(pDX, IDC_EDIT_DOWNLINK, m_DownLink);
	DDX_Control(pDX, IDC_COMBO_COMPORT, m_ComPort);
	//  DDX_Control(pDX, IDC_STATIC_SESSION_N, m_Session_N);
	DDX_Text(pDX, IDC_EDIT_URL, m_URL);
	DDX_Text(pDX, IDC_EDIT_URL_PORT, m_URL_PORT);
	DDX_Text(pDX, IDC_EDIT_UPLINK, m_Uplink);
	DDX_Text(pDX, IDC_EDIT_DOWNLINK, m_DownLink);
	DDX_Text(pDX, IDC_STATIC_GRSTN_STATUS, m_GrStnStatus);
	DDX_Text(pDX, IDC_STATIC_SERVER_STATUS, m_ServerStatus);
	DDX_Text(pDX, IDC_STATIC_SESSION_N, m_SessionN);
	DDX_Text(pDX, IDC_EDIT_STATION, m_Station);
	DDV_MaxChars(pDX, m_Station, 1);
}

BEGIN_MESSAGE_MAP(CGrStnDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT_URL, &CGrStnDlg::OnEnChangeEditUrl)
	ON_EN_CHANGE(IDC_EDIT_URL_PORT, &CGrStnDlg::OnEnChangeEditUrlPort)
	ON_CBN_SELCHANGE(IDC_COMBO_COMPORT, &CGrStnDlg::OnCbnSelchangeComboComport)
	ON_EN_CHANGE(IDC_EDIT_STATION, &CGrStnDlg::OnEnChangeEditStation)
	ON_BN_CLICKED(IDC_BUTTON_PING_SERVER, &CGrStnDlg::OnBnClickedButtonPingServer)
	ON_BN_CLICKED(IDC_BUTTON_PING_GRSTN, &CGrStnDlg::OnBnClickedButtonPingGrstn)
	ON_BN_CLICKED(IDOK, &CGrStnDlg::OnBnClickedOk)
//	ON_COMMAND(WM_KILL_SOCKET, &CGrStnDlg::OnWmKillSocket)
//	ON_COMMAND(IDNO, &CGrStnDlg::OnIdno)
//ON_WM_KILLFOCUS()
ON_MESSAGE(WM_KILL_SOCKET, &CGrStnDlg::OnKillSocket)
ON_WM_TIMER()
END_MESSAGE_MAP()

//ON_MESSAGE(WM_KILL_SOCKET, OnKillSocket)

// CGrStnDlg message handlers

BOOL CGrStnDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	int iComN = 0;
	int iComLines = 0;
	for(int i=1; i<=8; i++)	
	{
		char strPort[MAX_PATH];
		sprintf(strPort, "COM%d",i);
	
		DWORD dwSize = 1;
		LPCOMMCONFIG lpCC = (LPCOMMCONFIG) new BYTE[1];
		BOOL ret = GetDefaultCommConfig(strPort, lpCC, &dwSize);
		delete [] lpCC;	

		lpCC = (LPCOMMCONFIG) new BYTE[dwSize];
		ret = GetDefaultCommConfig(strPort, lpCC, &dwSize);
		
		if(ret)
		{
			iComLines++;
			m_ComPort.AddString(strPort);
			if (strcmp(strPort,ptrApp->szComPort)==0) // that is choosen port
			{
				iComN = iComLines;
			}
		}
		
		delete [] lpCC;
	}
	if (iComLines)
	{
		m_ComPort.SetCurSel(iComN);
		// it is possible to open com port
		ptrApp->ItIsPOssibelToOpenComm = ptrApp->OpenCommPort(ptrApp->szComPort);
	}
	else
	{
		// com port was not choosed et
	}


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGrStnDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGrStnDlg::OnPaint()
{
	if (FirstRun == FALSE)
	{
		FirstRun = TRUE;
		
		CreateOnlyOneSocket();
		SetTimer(901,10000,NULL);
	}
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGrStnDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CAboutDlg::OnButtonPingServer()
{
	// TODO: Add your command handler code here
	
}


void CAboutDlg::OnButtonPingGrstn()
{
}


void CGrStnDlg::OnEnChangeEditUrl()
{
	char szTemp[MAX_PATH];
	memset(szTemp,0, sizeof(szTemp));
	GetDlgItem(IDC_EDIT_URL)-> GetWindowText(szTemp , MAX_PATH );
	strcpy(ptrApp->szURL,szTemp);
}


void CGrStnDlg::OnEnChangeEditUrlPort()
{

	char szTemp[MAX_PATH];
	memset(szTemp,0, sizeof(szTemp));
	GetDlgItem(IDC_EDIT_URL_PORT)-> GetWindowText(szTemp , MAX_PATH );
	ptrApp->UrlPort = atoi(szTemp);

}


void CGrStnDlg::OnCbnSelchangeComboComport()
{
	char szTemp[MAX_PATH];
	int iN;
	memset(szTemp,0, sizeof(szTemp));
	GetDlgItem(IDC_COMBO_COMPORT)->GetWindowText( szTemp, sizeof(szTemp) );
	strcpy(ptrApp->szComPort,szTemp);
	ptrApp->ItIsPOssibelToOpenComm = ptrApp->OpenCommPort(ptrApp->szComPort);
}


void CGrStnDlg::OnEnChangeEditStation()
{
	char szTemp[MAX_PATH];
	memset(szTemp,0, sizeof(szTemp));
	GetDlgItem(IDC_EDIT_STATION)-> GetWindowText(szTemp , MAX_PATH );
	strcpy(ptrApp->g_station,szTemp);
}


void CGrStnDlg::OnBnClickedButtonPingServer()
{
	// TODO: Add your control notification handler code here
	if (ptrApp->m_MainHttpServer == NULL)
	{
		ptrApp->m_MainInternetConnection =
				new CInternetSession("SessionToControlServer",
				12,INTERNET_OPEN_TYPE_DIRECT,
				NULL, // proxi name
				NULL, // proxi bypass
				INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_TRANSFER_BINARY);
		try
		{
			ptrApp->m_MainHttpServer = 
						ptrApp->m_MainInternetConnection->GetHttpConnection( 
						ptrApp->szURL, 
						0,
						ptrApp->UrlPort,
						NULL,
						NULL );

		}
		catch(CInternetException *e)
		{
			ptrApp->m_MainHttpServer = NULL;
		}
	}
	if (((CGrStnApp*)AfxGetApp())->m_MainHttpServer)
	{
		ptrApp->SessionN = 0xffffffff;
		strcpy(ptrApp->packet_type,"4"); // test from Ground Station
		//03/19/13 08:13:09.937
		SYSTEMTIME SystemTime;
		GetSystemTime(  &SystemTime  );
		CTime rTime ( SystemTime );
		char szTemp[MAX_PATH];
		sprintf(szTemp, "%03d",SystemTime.wMilliseconds);
		CString Ct = rTime.FormatGmt("%m/%d/%y %H:%M:%S.");
		Ct = Ct + szTemp;
		strcpy(ptrApp->gs_time, (char*)Ct.GetString());
		strcpy(ptrApp->d_time, (char*)Ct.GetString());
		ptrApp->packet_no = 0;
		strcpy((char*)ptrApp->bPacket, "test from ");
		strcat((char*)ptrApp->bPacket,ptrApp->g_station);

		if (ptrApp->MakeRQ())
		{
			CHttpFile* myCHttpFile = NULL;
			try
			{
				myCHttpFile = ptrApp->m_MainHttpServer->OpenRequest( CHttpConnection::HTTP_VERB_GET,
					ptrApp->szWebServerRQ,
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
					memset(ptrApp->szWebServerResp, 0, sizeof(ptrApp->szWebServerResp));
					{
						DWORD dwSize;
						CString strSize;
						myCHttpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH,strSize);
						dwSize = atoi(strSize.GetString());
						if (dwSize > (sizeof(ptrApp->szWebServerResp)-1))
						{
							for (DWORD dwread=0; dwread < dwSize; dwread+= (sizeof(ptrApp->szWebServerResp)-1))
							{
								if ((dwSize - dwread) > (sizeof(ptrApp->szWebServerResp)-1))
									myCHttpFile->Read(&ptrApp->szWebServerResp,(sizeof(ptrApp->szWebServerResp)-1));
								else
									myCHttpFile->Read(&ptrApp->szWebServerResp,(dwSize - dwread));
							}
						}
						else
							myCHttpFile->Read(&ptrApp->szWebServerResp,dwSize);
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

}


void CGrStnDlg::OnBnClickedButtonPingGrstn()
{
	// TODO: Add your control notification handler code here
}
/////////////////////////////////////////////////////////////////////////////
// Server handlers

void CGrStnDlg::OnAccept()
{
#if _MFC_VER < 0x0400
	// In order to fix the 'dead socket' race problem on Win95, we need to
	// make sure that all sockets that have been closed are indeed dead
	// before requesting a new one. This prevents reallocating a socket that
	// hasn't fully run down yet.
	// This is a feature of MFC prior to 4.0 and is no longer necessary
	// in subsequent versions.
	MSG msg ;
	while ( ::PeekMessage ( &msg, NULL,
							WM_SOCKET_NOTIFY, WM_SOCKET_DEAD,
							PM_REMOVE )	)
	{
		::DispatchMessage ( &msg ) ;
	}
#endif
	time_t tNow ;	// add time tag for MikeAh
	time( &tNow ) ;
	CTime cNow ( tNow ) ;
//	Message ( cNow.Format ( "%m/%d/%y %H:%M:%S" ) ) ;
//	Message ( " - Connection request:" ) ;

	// create a client object
	CClient *pClient = new CClient ( this ) ;

	if ( pClient == NULL )
	{
//		Message ( ">> Unable to create client socket! <<\n" ) ;
		return ;
	}
	pClient->MyLinger.l_onoff = 1; 

	pClient->MyLinger.l_linger = 5; 
	//setsockopt ( pClient->m_hSocket, SOL_SOCKET, SO_LINGER, (const char *)&(pClient->MyLinger), sizeof(pClient->MyLinger) ); 

	if ( ! m_pSocket->Accept ( *pClient ) )
	{
//		Message ( ">> Unable to accept client connecton! <<\n" ) ;
		delete pClient ;
		return ;
	}
	pClient->AsyncSelect( );
	//pClient->ResolveClientName ( ((CCgApp*)AfxGetApp())->m_bResolveClientname ) ;

	// have we hit our resource limit?
	if ( m_listConnects.GetCount() >= (int)(ptrApp->m_nMaxConnects ))
	{
		// yes, send failure msg to client
		pClient->SendCannedMsg ( 503 ) ;
		delete pClient ;
//		Message ( "  Connection rejected - MaxConnects\n" ) ;
		return ;
	}
//	Message ( "  Connection accepted!!!\n" ) ;
//	EnterCriticalSection(&m_csAddTail);

	// add this client to our list
	m_listConnects.AddTail ( pClient ) ;
//    LeaveCriticalSection(&m_csAddTail);
	// Service Agent has the 'tater now...

}	// OnAccept()

// This routine is called periodically from the MainFrame sanity timer
// handler. We're checking to see if any clients are loitering and, if so,
// clobber them.
void CGrStnDlg::CheckIdleConnects()
{
	// compute the age threshold
	time_t tNow ;
	time( &tNow ) ;
	CTime cNow ( tNow ) ;
	CTimeSpan cTimeOut ( 0, 0, 0, ptrApp->m_nTimeOut ) ;
	cNow -= cTimeOut ;	// anyone created before this time will get zapped

//	DbgMessage ( "--- Checking for idle connections ---\n" ) ;
	for ( POSITION pos = m_listConnects.GetHeadPosition() ; pos != NULL ; )
	{
		CClient* pClient = (CClient*)m_listConnects.GetNext ( pos ) ;
		// anyone lanquishing in the list?
		if ( pClient->m_bDone )
		{
			KillSocket ( pClient ) ;
		}
		// anyone timed out?
		else if ( pClient->datetime < cNow )
		{
//			char msg[80] ;
//			wsprintf ( msg, ">>> Idle timeout on client: %s\n", pClient->m_PeerIP ) ;
//			Message ( msg ) ;
			KillSocket ( pClient ) ;
		}
	}
	// flush the log file buffer, while we're at it
//	if ( ((CCgApp*)AfxGetApp())->m_bLogEnable && m_fileLog.is_open() )
//		m_fileLog.flush() ;
}	// CheckIdleConnects()

// This routine is called from the MainFrame when a client has notified the
// aforementioned that it is done. Since the document owns the client
// objects, the document is responsible for cleaning up after it.
void CGrStnDlg::KillSocket ( CClient* pSocket )
{
	BOOL bFoundIt = FALSE ;
	// remove this client from the connection list
	for ( POSITION pos = m_listConnects.GetHeadPosition() ; pos != NULL ; )
	{
		POSITION temp = pos ;
		CClient* pSock = (CClient*)m_listConnects.GetNext ( pos ) ;
		if ( pSock == pSocket )
		{
			bFoundIt = TRUE ;
			m_listConnects.RemoveAt ( temp ) ;
//(dec)...debug...
// looking for cause of accvio when client cancels transfer
// AsyncSelect causes accvio after Send has failed
			if ( pSocket->AsyncSelect(0) == 0 )
				DWORD err = GetLastError() ;
			pSocket->Close() ;	//...debug...
//(dec)...end of debug...
			delete pSocket ;	// destructor calls Close()
			pSocket = NULL ;	// make sure its no longer accessible
//			Message ( "  Connection closed.\n" ) ;
			break ;
		}
	}
	if ( ! bFoundIt )
	{
//		DbgMessage ( ">> Uh oh! - Might have a sync problem.\n" ) ;
//		DbgMessage ( ">> Couldn't find delete-pending socket in client list.\n" ) ;
	}
}	// KillSocket()

BOOL CGrStnDlg::CreateOnlyOneSocket(void)
{
		// Create our one and only listener socket.
	// OnCloseDocument() takes care of deleting m_pSocket.
	m_pSocket = new CListen ( this ) ;
	 
	if ( ! m_pSocket )
	{
		AfxMessageBox ( "Unable to allocate memory for listener socket!" ) ;
		return ( FALSE ) ;
	}

	m_pSocket->MyLinger.l_onoff = 1; 

	m_pSocket->MyLinger.l_linger = 5; 

	//setsockopt ( m_pSocket->m_hSocket, SOL_SOCKET, SO_LINGER, (const char *)&(m_pSocket->MyLinger), sizeof(m_pSocket->MyLinger) ); 

	if ( ! m_pSocket->Create ( ptrApp->m_wwwPort) )
	{
		DWORD dwErr = m_pSocket->GetLastError() ;
		switch ( dwErr )
		{
		case WSAEADDRINUSE:	// example of expected error handler
				AfxMessageBox ( "The WWW port is already in use!" ) ;
				break ;

		default:					// example of generic error handler
////				msg.Format ( "Listener socket Create failed: %s\n",
////							  theApp.MapErrMsg(dwErr) ) ;
				AfxMessageBox ( "WWW socket Create failed" ) ;
				;
		}
		return ( FALSE ) ;
	}

	m_pSocket->AsyncSelect( );
	// start listening for requests and set running state
	BOOL ret = m_pSocket->Listen() ;
	if ( ret )
	{
		ptrApp->m_State = CGrStnApp::ST_WAITING ;
//		msg.Format ( "Port: %d", theApp.m_wwwPort ) ;
//		SetTitle ( msg ) ;
	}
	else
	{
		DWORD dwErr = m_pSocket->GetLastError() ;
////		msg.Format ( "Listener socket Listen failed: %s\n",
////					 theApp.MapErrMsg(dwErr) ) ;
//		AfxMessageBox ( "Listener socket Listen failed" ) ;
	}
	return ( ret ) ;

}
void CGrStnDlg::CloseAllConnectionAndDisconnect()
{
		// clobber everyone still connected
	for ( POSITION pos = m_listConnects.GetHeadPosition() ; pos != NULL ; )
	{
		CClient* pClient = (CClient*)m_listConnects.GetNext ( pos ) ;
		KillSocket ( pClient ) ;
	}

	m_pSocket->Close();
	delete m_pSocket ;	// release the listening socket

}


void CGrStnDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CurentDlgBox = NULL;
	KillTimer(901);
	CloseAllConnectionAndDisconnect();
	CDialogEx::OnOK();
}


//void CGrStnDlg::OnWmKillSocket()
//{
//	// TODO: Add your command handler code here
//}


//void CGrStnDlg::OnIdno()
//{
//	// TODO: Add your command handler code here
//}


//void CGrStnDlg::OnKillFocus(CWnd* pNewWnd)
//{
//	CDialogEx::OnKillFocus(pNewWnd);
//
//	// TODO: Add your message handler code here
//}


//afx_msg LRESULT CAboutDlg::OnKillSocket(WPARAM wParam, LPARAM lParam)
//{
//	return 0;
//}


afx_msg LRESULT CGrStnDlg::OnKillSocket(WPARAM wParam, LPARAM lParam)
{
	KillSocket ( (CClient*)lParam ) ;
	//Invalidate();
	//UpdateWindow();
	//SetModified();
	//m_Station = "q";
	return 0;
}


void CGrStnDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CheckIdleConnects();

	CDialogEx::OnTimer(nIDEvent);
}
