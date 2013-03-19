
// GrStnDlg.cpp : implementation file
//

#include "stdafx.h"
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
END_MESSAGE_MAP()


// CGrStnDlg dialog




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
END_MESSAGE_MAP()


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
			if (strcmp(strPort,((CGrStnApp*)AfxGetApp())->szComPort)==0) // that is choosen port
			{
				iComN = iComLines;
			}
		}
		
		delete [] lpCC;
	}
	if (iComLines)
	{
		m_ComPort.SetCurSel(iComN);
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
	GetDlgItem(IDC_EDIT_URL)-> GetWindowText(szTemp , MAX_PATH );
	strcpy(((CGrStnApp*)AfxGetApp())->szURL,szTemp);
}


void CGrStnDlg::OnEnChangeEditUrlPort()
{

	char szTemp[MAX_PATH];
	GetDlgItem(IDC_EDIT_URL_PORT)-> GetWindowText(szTemp , MAX_PATH );
	((CGrStnApp*)AfxGetApp())->UrlPort = atoi(szTemp);

}


void CGrStnDlg::OnCbnSelchangeComboComport()
{
	char szTemp[MAX_PATH];
	int iN;
	GetDlgItem(IDC_COMBO_COMPORT)->GetWindowText( szTemp, sizeof(szTemp) );
	strcpy(((CGrStnApp*)AfxGetApp())->szComPort,szTemp);
}


void CGrStnDlg::OnEnChangeEditStation()
{
	char szTemp[MAX_PATH];
	GetDlgItem(IDC_EDIT_STATION)-> GetWindowText(szTemp , MAX_PATH );
	strcpy(((CGrStnApp*)AfxGetApp())->g_station,szTemp);
}


void CGrStnDlg::OnBnClickedButtonPingServer()
{
	// TODO: Add your control notification handler code here
	if (((CGrStnApp*)AfxGetApp())->m_MainHttpServer == NULL)
	{
		((CGrStnApp*)AfxGetApp())->m_MainInternetConnection =
				new CInternetSession("SessionToControlServer",
				12,INTERNET_OPEN_TYPE_DIRECT,
				NULL, // proxi name
				NULL, // proxi bypass
				INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_TRANSFER_BINARY);
		try
		{
			((CGrStnApp*)AfxGetApp())->m_MainHttpServer = 
						((CGrStnApp*)AfxGetApp())->m_MainInternetConnection->GetHttpConnection( 
						((CGrStnApp*)AfxGetApp())->szURL, 
						0,
						((CGrStnApp*)AfxGetApp())->UrlPort,
						NULL,
						NULL );

		}
		catch(CInternetException *e)
		{
			((CGrStnApp*)AfxGetApp())->m_MainHttpServer = NULL;
		}
	}
	if (((CGrStnApp*)AfxGetApp())->m_MainHttpServer)
	{
		((CGrStnApp*)AfxGetApp())->SessionN = 0xffffffff;
		strcpy(((CGrStnApp*)AfxGetApp())->packet_type,"4"); // test from Ground Station
		//03/19/13 08:13:09.937
		SYSTEMTIME SystemTime;
		GetSystemTime(  &SystemTime  );
		CTime rTime ( SystemTime );
		char szTemp[MAX_PATH];
		sprintf(szTemp, "%03d",SystemTime.wMilliseconds);
		CString Ct = rTime.FormatGmt("%m/%d/%y %H:%M:%S.");
		Ct = Ct + szTemp;
		strcpy(((CGrStnApp*)AfxGetApp())->gs_time, (char*)Ct.GetString());
		strcpy(((CGrStnApp*)AfxGetApp())->d_time, (char*)Ct.GetString());
		((CGrStnApp*)AfxGetApp())->packet_no = 0;
		strcpy((char*)((CGrStnApp*)AfxGetApp())->bPacket, "test from ");
		strcat((char*)((CGrStnApp*)AfxGetApp())->bPacket,((CGrStnApp*)AfxGetApp())->g_station);

		if (((CGrStnApp*)AfxGetApp())->MakeRQ())
		{
			CHttpFile* myCHttpFile = ((CGrStnApp*)AfxGetApp())->m_MainHttpServer->OpenRequest( CHttpConnection::HTTP_VERB_GET,
					((CGrStnApp*)AfxGetApp())->szWebServerRQ,
					NULL,//((CGrStnApp*)AfxGetApp())->szLoginRQ,
					NULL,//12345678,
					NULL, 
					NULL, 
					INTERNET_FLAG_EXISTING_CONNECT|
					INTERNET_FLAG_DONT_CACHE|
					INTERNET_FLAG_RELOAD );
			if (myCHttpFile !=NULL)
			{
				myCHttpFile->SendRequest();
				memset(((CGrStnApp*)AfxGetApp())->szWebServerResp, 0, sizeof(((CGrStnApp*)AfxGetApp())->szWebServerResp));
				for (int iRea = 0; iRea < 100; iRea++)
				{
					if (myCHttpFile->Read(&(((CGrStnApp*)AfxGetApp())->szWebServerResp[iRea]),1))
					{
						if (((CGrStnApp*)AfxGetApp())->szWebServerResp[iRea] == 0x0d)
							((CGrStnApp*)AfxGetApp())->szWebServerResp[iRea] = 0;
						if (((CGrStnApp*)AfxGetApp())->szWebServerResp[iRea] == 0x0a)
							((CGrStnApp*)AfxGetApp())->szWebServerResp[iRea] = 0;
					}
					else
						break;
				}
				myCHttpFile->Close();
			}
		}
	}

}


void CGrStnDlg::OnBnClickedButtonPingGrstn()
{
	// TODO: Add your control notification handler code here
}
