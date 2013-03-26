
// GrStnDlg.h : header file
//

#pragma once
#include <afxinet.h>
#include "clisten.h"
#include "cclient.h"


// CGrStnDlg dialog
class CGrStnDlg : public CDialogEx
{
// Construction
public:
	CGrStnDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_GRSTN_DIALOG };

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
	DECLARE_MESSAGE_MAP()
public:
//	CEdit m_Uplink;
//	CEdit m_URL;
//	CEdit m_URL;
//	CEdit m_URL_PORT;
//	CStatic m_GrStnStatus;
//	CStatic m_ServerStatus;
//	CEdit m_DownLink;
	CComboBox m_ComPort;
//	CStatic m_Session_N;
	CString m_URL;
	CString m_URL_PORT;
	CString m_Uplink;
	CString m_DownLink;
	CString m_GrStnStatus;
	CString m_ServerStatus;
	CString m_SessionN;
	afx_msg void OnEnChangeEditUrl();
	afx_msg void OnEnChangeEditUrlPort();
	afx_msg void OnCbnSelchangeComboComport();
	CString m_Station;
	afx_msg void OnEnChangeEditStation();
	afx_msg void OnBnClickedButtonPingServer();
	afx_msg void OnBnClickedButtonPingGrstn();
	CGrStnApp *ptrApp;
	void CloseAllConnectionAndDisconnect();
	BOOL CreateOnlyOneSocket(void);
	void KillSocket ( CClient* pSocket );
	void CheckIdleConnects();
	void OnAccept();


	CListen*	m_pSocket ;			// our document's one and only listening socket (service port)
	CPtrList	m_listConnects ;	// list of active connections
	BOOL FirstRun;
	afx_msg void OnBnClickedOk();
//	afx_msg void OnWmKillSocket();
//	afx_msg void OnIdno();
//	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	afx_msg LRESULT OnKillSocket(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
