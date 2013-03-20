/////////////////////////////////////////////////////////////////////////////
// CClient.cpp : implementation of the CClient class
//
// This is a part of the Webster HTTP Server application
// for Microsoft Systems Journal
//

#include "stdafx.h"
#include <afxinet.h>
#include <afxsock.h>
#include <Mmsystem.h>
#include "resource.h"

#include "clisten.h"
#include "cclient.h"
#include "GrStn.h"
#include "GrStnDlg.h"

extern CGrStnDlg *CurentDlgBox;
/////////////////////////////////////////////////////////////////////////////
// CClient Construction

CClient::CClient(CGrStnDlg* m_pParentDoc)
{
	m_pDoc = m_pParentDoc ;	// cache this for lots of use later
	m_bDone = FALSE ;
	m_irx = 0 ;
	m_buf = NULL ;
	m_bHTTP10 = FALSE ;		// assume HTTP 0.9
	m_bHTTP11 = FALSE ;		// assume HTTP 0.9
	m_nMethod = METHOD_UNSUPPORTED ;

	time_t tNow ;
	time ( &tNow ) ;
	CTime cNow ( tNow ) ;
	datetime	= cNow ;	// this is our birth date
	client	= "" ;
	//inetd		= "-" ;
	//username	= "-" ;
	request	= "" ;
	status	= 200 ;
	bytes		= 0 ;


	m_pHE = NULL ;
	m_AcceptTime = timeGetTime();
	memset(m_szStoredCommand,0,sizeof(m_szStoredCommand));
	memset(m_szOriginalCommand,0,sizeof(m_szOriginalCommand));
	m_WapSupportGif = FALSE;
	m_WapCam = 0;
}

CClient::~CClient()
{
	if ( m_buf )
	{
		free ( m_buf ) ;
		m_buf = NULL ;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSocket Implementation

#ifdef _DEBUG
void CClient::AssertValid() const
{
	CSocket::AssertValid();
}

void CClient::Dump(CDumpContext& dc) const
{
	CSocket::Dump(dc);
}
#endif //_DEBUG

IMPLEMENT_DYNAMIC(CClient, CSocket)

/////////////////////////////////////////////////////////////////////////////
// CClient Overridable callbacks

void CClient::OnReceive(int nErrorCode)
{
	CSocket::OnReceive(nErrorCode);

	// If the read processing was successful, then we've done our job.
	// If not, the service request has not completed yet.
	// Caution: The bDone boolean is used elsewhere,
	//				don't try to tighten up this check.
	if (nErrorCode != 0)
	{
		if (CurentDlgBox)
			CurentDlgBox->PostMessage ( WM_KILL_SOCKET,
									   (WPARAM)0,
						 			   (LPARAM)this ) ;
		return;
	}
	if ( ProcessPendingRead() )
	{
		if ( m_isocRez == SOCKET_ERROR  || m_isocRez == 0)
		{
			if (CurentDlgBox)
				CurentDlgBox->PostMessage ( WM_KILL_SOCKET,
									   (WPARAM)0,
						 			   (LPARAM)this ) ;
		}
		else
		{
			// send request off for processing
			ProcessReq() ;	// process the request
			m_bDone = TRUE ;

			if (CurentDlgBox)
				CurentDlgBox->PostMessage ( WM_KILL_SOCKET,
									   (WPARAM)0,
						 			   (LPARAM)this ) ;
		}
	}
}

#ifdef	_DEBUG
// This is called to notify a blocked sender that the socket is unblocked
// now. Not required for retail operation.
void CClient::OnSend(int nErrorCode)
{
	CSocket::OnSend(nErrorCode);
}

// This is called when the client wants to abort the data download.
// Also not required for retail operation.
void CClient::OnClose(int nErrorCode)
{
	CSocket::OnClose(nErrorCode);
}
#endif	// _DEBUG
