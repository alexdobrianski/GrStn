/////////////////////////////////////////////////////////////////////////////
// Service.cpp : implementation of the CClient operations
//
// This is a part of the Webster HTTP Server application
// for Microsoft Systems Journal
//

#include "stdafx.h"
#include <stdio.h>
#include <afxinet.h>
#include <afxsock.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <string.h> 
#include "GrStn.h"
#include "GrStnDlg.h"
#include "clisten.h"
#include "cclient.h"

extern BOOL FlagWWWJpeg;
extern BOOL FlagWritingOutputJpegFile;

#define maxAllMessages 24
extern int nAllMessages ;
extern int nCharPos;
extern char AllMessages[maxAllMessages][64];
extern int nMaxLineSize;
extern BOOL FlagAllMessagesBusy;
BOOL TestAndSetFlagAllMessages(void);
//extern CProp *MyProp;

//extern DWORD StartTime;
//extern __int64 StartTime64;


CTime TimeStartSend;
CTime TimeEndSend;


// This routine collects the request from the client
BOOL CClient::ProcessPendingRead()
#if 1
{
// Not all clients can send a block at a time (e.g., telnet). What we're
// doing here is constructing a buffer until we get either a cr/lf pair, or
// until dwBytes to read is zero.
//
// Note that since we've been called from the OnReceive notification
// handler, then if dwBytes is zero, it must mean the client has closed the
// connection.

    int ContentLength = 0;
    int indxStartContent = 0;

    DWORD	dwBytes ;

	IOCtl ( FIONREAD, &dwBytes ) ;		// anything to get?
	if ( dwBytes == 0 )
		return ( TRUE ) ;	// we must be done!

	// allocate, or extend, our buffer - leave room for '\0' at end
	if ( m_irx == 0 )	// first time thru - malloc
		m_buf = (char *)malloc ( dwBytes+1 ) ;
	else				// otherwise - realloc
		m_buf = (char *)realloc ( m_buf, m_irx+dwBytes+1 ) ;
	// (so, like _when_ is C++ gonna support resizeable memory?)



	// get this chunk from the client
	if ( Receive ( &m_buf[m_irx], dwBytes, 0 ) == SOCKET_ERROR )
	{
		int err = GetLastError() ;
		if ( err == WSAECONNRESET )	// remote has terminated (gracefully)
		{
//			m_pADROSrv->Message ( "WSAECONNERESET\n" ) ;
			return ( TRUE ) ;	// must be done!
		}
	}
	m_irx += dwBytes ;	// this much was added to our accumulating buffer

	// This is our socket equivalent of gets()
	// If we return FALSE, it means we need more input
	if ( m_irx < 4 )	 // enough to parse?
		return ( FALSE ) ;
#define CONTENT_LENGTH "\r\nContent-Length:"
    if (memicmp(&m_buf[0], "GET ", 4) == 0)
    {
        if ( memcmp ( &m_buf[m_irx-4], "\r\n\r\n", 4 ) == 0 )	// end of line?
        {
        }
        else
            return ( FALSE ) ;
    }
    if (memicmp(&m_buf[0], "POST ", 5) == 0)
    {
        if (ContentLength == 0)
        {
            int iS;
            // search for "\r\nContent-Length:"
            for (iS = 0; iS < m_irx - strlen(CONTENT_LENGTH); iS++)
            {
                if (memicmp(&m_buf[iS], CONTENT_LENGTH,strlen(CONTENT_LENGTH)) == 0)
                {
                    int iSs;
                    // if found get length of data
                    for (iSs = iS + strlen(CONTENT_LENGTH) + 1; iSs < m_irx; iSs ++)
                    {
                        if (m_buf[iSs] != ' ')
                            ContentLength = atoi(&m_buf[iSs]);
                        break;

                    }
                    break;
                
                }
            }
            if (ContentLength)
            {
                int iSl;
                for (iSl = 0; iSl <= m_irx - 4; iSl++)
                {
                    if (memcmp(&m_buf[iSl],"\r\n\r\n", 4) == 0)
                    {
                        indxStartContent = m_irx - iSl - 4;
                        break;
                    }
                }
            }
        }
        if ((ContentLength != 0) && (ContentLength <= indxStartContent) )
        {
            // this is OK
        }
        else
        {
            // next portion to read
    		return ( FALSE ) ;
        }
    }
	// split this request up into the list of lines
	m_buf[m_irx] = '\0' ;	// Make this is an SZ string for parsing.
	char *pBOL = m_buf ;
    char *pBolLast = pBOL; 
	for ( char *pEOL = strpbrk ( pBOL, "\r" ) ;
			pEOL ;
			pEOL = strpbrk ( pBOL, "\r" ) )
	{
		*pEOL = '\0' ;	// make this chunk an SZ string
		m_cList.AddTail ( CString(pBOL,strlen(pBOL)) ) ; // add to list
		if (memcmp(pBOL,"Accept:", sizeof("Accept:")-1)==0)
		{
			if (strstr(pBOL, "image/vnd.wap.wbmp"))
				m_WapSupportGif = FALSE;
			if (strstr(pBOL, "image/gif"))
				m_WapSupportGif = TRUE;

		}
        *pEOL = '\r' ;  // restore -> may be we will need this buffer again   
		*pEOL++ ;		// skip '\0'
		*pEOL++ ;		// skip '\n'
		pBOL = pEOL ;	// point to next text
        pBolLast = pBOL;
	}
    
    // last string in post may be without \r\n
    if ((ContentLength != 0) && (ContentLength <= indxStartContent) )
    {
        // this is OK POST request with last string not terminated 0xd 0xa
        if (strlen(pBolLast))
            m_cList.AddTail ( CString(pBolLast,strlen(pBolLast)) ) ; // add to list
    }
	

	// are we in HTTP 1.0 mode yet?
	if ( (! m_bHTTP10)  && (! m_bHTTP11))
	{
		if ( m_cList.GetHead().Find ( "HTTP/1.0" ) != -1 )
			m_bHTTP10 = TRUE ;	// we are now...
		else
		{
			if ( m_cList.GetHead().Find ( "HTTP/1.1" ) != -1 )
				m_bHTTP11 = TRUE ;	// we are now...
			else
            {
                // something wrong better to exit
                m_irx = 0 ;		// reset for next chunk from client
                free ( m_buf ) ;
                m_buf = NULL ;
				return TRUE;	// we must be done
            }
		}
	}

	// We are definitely in HTTP 1.0 mode now, so look for the terminating
	// empty line. Since we've already stripped off the cr/lf, the length
	// will be zero.
    CString cReq = m_cList.GetTail() ;
    POSITION pos = m_cList.GetTailPosition();
    if (cReq.GetLength() == 0)
    {
        m_irx = 0 ;		// reset buffer - we done
        free ( m_buf ) ;
        m_buf = NULL ;
        return TRUE;
    }

    m_cList.GetPrev(pos);
    if (pos != NULL )
    {
        if (m_cList.GetAt(pos).GetLength() == 0)
        {
            m_irx = 0 ;		// reset buffer - we done
            free ( m_buf ) ;
            m_buf = NULL ;
            return TRUE;
        }
    }
    // clean list; left buffer open
    m_cList.RemoveAll();
    return FALSE;
//	return ( m_cList.GetTail().GetLength() == 0 ) ;
}	// ProcessPendingRead()

#else
{
// Not all clients can send a block at a time (e.g., telnet). What we're
// doing here is constructing a buffer until we get either a cr/lf pair, or
// until dwBytes to read is zero.
//
// Note that since we've been called from the OnReceive notification
// handler, then if dwBytes is zero, it must mean the client has closed the
// connection.

	DWORD	dwBytes ;

	IOCtl ( FIONREAD, &dwBytes ) ;		// anything to get?
	if ( dwBytes == 0 )
	{
		return ( TRUE ) ;	// we must be done!
	}

	// allocate, or extend, our buffer - leave room for '\0' at end
	if ( m_irx == 0 )	// first time thru - malloc
	{
		m_buf = (char *)malloc ( dwBytes+1 ) ;
	}
	else				// otherwise - realloc
		m_buf = (char *)realloc ( m_buf, m_irx+dwBytes+1 ) ;
	// (so, like _when_ is C++ gonna support resizeable memory?)
	if (m_buf == NULL)
	{
		m_isocRez = 0;
		m_irx = 0 ;		// reset for next chunk from client

		return ( TRUE ) ;	// must be done!
	}

	memset((void*)&m_buf[m_irx], 0, dwBytes + 1);
	// get this chunk from the client
	m_isocRez = Receive ( &m_buf[m_irx], dwBytes, 0 );
	if ( m_isocRez == SOCKET_ERROR  || m_isocRez == 0)
	{
//		int err = GetLastError() ;
//		if ( err == WSAECONNRESET )	// remote has terminated (gracefully)
//		{
////			m_pDoc->Message ( "WSAECONNERESET\n" ) ;
//			return ( TRUE ) ;	// must be done!
//		}
		m_irx = 0 ;		// reset for next chunk from client
		free ( m_buf ) ;
		m_buf = NULL ;
		return ( TRUE ) ;	// must be done!
	}
	m_irx += dwBytes ;	// this much was added to our accumulating buffer

	m_buf[m_irx] = '\0' ;	// Make this is an SZ string for parsing.

	// This is our socket equivalent of gets()
	// If we return FALSE, it means we need more input
	if ( m_irx < 2 )	 // enough to parse?
		return ( FALSE ) ;
	if ( memcmp ( &m_buf[m_irx-2], "\r\n", 2 ) != 0 )	// end of line?
		return ( FALSE ) ;

	// split this request up into the list of lines
	m_buf[m_irx] = '\0' ;	// Make this is an SZ string for parsing.
	char *pBOL = m_buf ;
	for ( char *pEOL = strpbrk ( pBOL, "\r" ) ;
			pEOL ;
			pEOL = strpbrk ( pBOL, "\r" ) )
	{
		*pEOL = '\0' ;	// make this chunk an SZ string
		m_cList.AddTail ( CString(pBOL,strlen(pBOL)) ) ; // add to list
		if (memicmp(pBOL,"Host:", 5) == 0)
		{
			//m_PeerIP = CString(pBOL+5,strlen(pBOL)-5);
			GetPeerName ( m_PeerIP, m_Port );
		}
		*pEOL++ ;		// skip '\0'
		*pEOL++ ;		// skip '\n'
		pBOL = pEOL ;	// point to next text
	}
	m_irx = 0 ;		// reset for next chunk from client
	free ( m_buf ) ;	
	m_buf = NULL ;

	// are we in HTTP 1.0 mode yet?
	if ( (! m_bHTTP10)  && (! m_bHTTP11))
	{
		if ( m_cList.GetHead().Find ( "HTTP/1.0" ) != -1 )
			m_bHTTP10 = TRUE ;	// we are now...
		else
		{
			if ( m_cList.GetHead().Find ( "HTTP/1.1" ) != -1 )
				m_bHTTP11 = TRUE ;	// we are now...
			else
				return ( TRUE ) ;	// we must be done
		}
	}

	// We are definitely in HTTP 1.0 mode now, so look for the terminating
	// empty line. Since we've already stripped off the cr/lf, the length
	// will be zero.
	return ( m_cList.GetTail().GetLength() == 0 ) ;
}	// ProcessPendingRead()
#endif
// This is a lookup table for translating the parsed method text string
// into a predefined token value. This token value will be used later
// to dispatch the request to an appropriate handler.
static struct CClient::_tagMethodTable MethodTable[] = {
	{ CClient::METHOD_GET   , "GET"    },
	{ CClient::METHOD_POST  , "POST"   },
	{ CClient::METHOD_HEAD  , "HEAD"   },
	{ CClient::METHOD_PUT   , "PUT"    },
	{ CClient::METHOD_DELETE, "DELETE" },
	{ CClient::METHOD_LINK  , "LINK"   },
	{ CClient::METHOD_UNLINK, "UNLINK" },
} ;
static const int MethodTableLen = sizeof(MethodTable)/
								  sizeof(struct CClient::_tagMethodTable) ;

void CClient::ParseReq()
{
	int i;
	CStringList cList ;	// list of parsed command tokens


	// save the request line for our log record
	if ( m_cList.IsEmpty() )	// always check IsEmpty() first
	{
//		m_pDoc->DbgVMessage ( "Command list is empty!\nSending 400 error\n" ) ;
		SendCannedMsg ( 400 ) ;
		return ;
	}
	CString cReq = m_cList.GetHead() ;
	request = cReq ;

	/////////////////////////////////////////////////////////////////////////////
	// parse the primary, and most important, request line

	// parse the request line into a list of tokens
	LPSTR tempmsg = new char[cReq.GetLength()+1] ;	// allow for EOS
	if (tempmsg == NULL)
	{
		SendCannedMsg ( 400 ) ;
		return ;
	}

	memset(tempmsg,0,cReq.GetLength()+1);
	strcpy ( tempmsg, cReq ) ;
	char *pBOL = tempmsg ;
	for ( char *pEOL = strpbrk ( pBOL, " " ) ;
		  pEOL ;
		  pEOL = strpbrk ( pBOL, " " ) )
	{
		*pEOL = '\0' ;
		CString tempToken ( pBOL, strlen(pBOL) ) ;
		*pEOL++ ;	// skip '\0'
		pBOL = pEOL ;
		cList.AddTail ( tempToken ) ;
	}
	// save whatever's left as the last token
	CString tempToken ( pBOL, strlen(pBOL) ) ;
	cList.AddTail ( tempToken ) ;
	delete tempmsg ;

	POSITION pos = cList.GetHeadPosition() ;	// prepare to scan the request

	// 1) parse the method
	if ( pos == NULL )
	{
//		m_pDoc->DbgVMessage ( "Null request method\nSending 400 error\n" ) ;
		SendCannedMsg ( 400 ) ;
		return ;		
	}
	m_cURI = cList.GetNext ( pos ) ;	// pointing to METHOD now
	for ( int i = 0 ; i < MethodTableLen ; i++ )
	{
		if ( m_cURI.CompareNoCase ( MethodTable[i].key ) == 0 )
		{
			m_nMethod = MethodTable[i].id ;
			break ;
		}
	}

	// 2) parse the URI
	if ( pos == NULL )
	{
//		m_pDoc->DbgVMessage ( "Null request URI\nSending 400 error\n" ) ;
		SendCannedMsg ( 400 ) ;
		return ;		
	}
	m_cURI = cList.GetNext ( pos ) ;	// pointing to ENTITY now
//	m_pDoc->VMessage ( "   Data request: %s\n", m_cURI ) ;

	// replace UNIX '/' with MS '\'
	for ( i = 0 ; i < m_cURI.GetLength() ; i++ )
	{
		if ( m_cURI[i] == '/' )
			m_cURI.SetAt ( i, '\\' ) ;
	}

	// add base path
	//if ( m_cURI[0] != '\\' )
	//	m_cLocalFNA = ((CCgApp*)AfxGetApp())->m_HTMLPath + CString("\\") + m_cURI ;
	//else
	//	m_cLocalFNA = ((CCgApp*)AfxGetApp())->m_HTMLPath + m_cURI ;

	// This is a real ugly little hack for MikeAh to use forms/GET
	// I just snarf the rest of the command line from the query
	// separator on...
	if ( ( i = m_cLocalFNA.Find ( '?' ) ) != -1 )
	{
	}


	// 3) parse the rest of the request lines
	if ( (! m_bHTTP10)  && (! m_bHTTP11))
		return ;	// if HTTP 0.9, we're done!

	// parse the client's capabilities here...
//	if ( ((CCgApp*)AfxGetApp())->m_bDebugOutput )
//	{
//		POSITION pos = m_cList.GetHeadPosition() ;
//		for ( int i = 0 ; i < m_cList.GetCount() ; i++ )
//		{
//			// For now, we'll just have a boo at them. Being such a simple
//			// server, let's not get too concerned with details.
////			m_pDoc->DbgVMessage ( "   %d>%s\n", i+1, m_cList.GetNext ( pos ) ) ;
//		}
//	}
}	// ParseReq()


/////////////////////////////////////////////////////////////////////////////
// dispatch service handler

void CClient::ProcessReq()
{
	// parse the request

	ParseReq() ;

	// can only handle GETs for now
	if ( m_nMethod != METHOD_GET )
	{
//		m_pDoc->VMessage ( "   Unknown method requested: %s\n", m_cURI ) ;
		SendCannedMsg ( 405, m_cURI ) ;
		return ;
	}

	// try to determine the send method based on the file type
	char *ext = strrchr ( (char*)m_cLocalFNA.GetString(), '.' ) ;
	if (  m_cURI == "\\" )		// blind request?
	{
//		m_pDoc->DbgVMessage ( "   Blind request\n" ) ;
		if ( ! SendFile ( m_cLocalFNA, m_cURI ) )
			return ;	// send failure!
	}
	else if ( ext )				// has an extension?
	{
		*ext++ ;	// point to start of file extension
//		m_pDoc->DbgVMessage ( "   File extension: <%s>\n", ext ) ;
		if ( ! SendFile ( m_cLocalFNA, m_cURI ) )
			return ;	// send failure!
	}
	else	// must be a CGI script specification
	{
//		m_pDoc->DbgVMessage ( "   CGI request: %s %s\n", m_cLocalFNA, m_cURI ) ;
		if ( ! SendCGI ( m_cLocalFNA, m_cURI ) )
		{
			// ...insert CGI-implementation dependant actions here...
			return ;
		}
	}
	SendTag() ;	// Done!!!
}	// ProcessReq()


/////////////////////////////////////////////////////////////////////////////
// CClient Service Response Operations

// this is the built-in list of MIME types we automatically recognize
static struct _tagMIME_Table
{
	char	*ext ;
	char	*type ;
} MIME_Table[] = {
	{ ".gif" , "image/gif"  },
	{ ".jpg" , "image/jpeg"  },
	{ ".jpeg" ,"image/jpeg"  },
	{ ".bmp" , "image/bmp"  },
	{ ".htm" , "text/html"  },
	{ ".html", "text/html"  },
	{ ".wav",  "audio/wav"  },
	{ ".aif",  "audio/aiff"  },
	{ ".avi",  "video/avi"  },
	{ ".mpg",  "video/mpeg"  },
	{ ".mpeg", "video/mpeg"  },
	{ ".wml" , "text/vnd.wap.wml" },
	{ ".wmlc" ,"text/vnd.wap.wmlc" },
	{ ".wmls" ,"text/vnd.wap.wmlscript" },
	{ ".wmlsc","application/vnd.wap.wmlscriptc" },
	{ ".wbmp" ,"image/vnd.wap.wbmp" },
	{ ".jad" ,"text/vnd.sun.j2me.app-descriptor" },
	{ ".jar" ,"application/java-archive" },
	{ ".txt" , "text/plain" }
} ;
static const int MIME_len = sizeof(MIME_Table)
									/ sizeof(struct _tagMIME_Table) ;

char bufm0[10000];
int sizem0 = 0;

BOOL CClient::SendReplyHeaderm0 ( char * filename, DWORD filele )
{
	if ( (! m_bHTTP10) &&  (! m_bHTTP11))	// if HTTP 0.9, response header not used
		return ( TRUE ) ;

	// Response header components:
	// 1 - HTTP 1.0 response header
	// 2 - Server time
	// 3 - Server version
	// 4 - MIME version
	// 5 - MIME type
	// 6 - Last-modified time
	// 7 - Content length
	// 8 - HTTP 1.0 End-of-header

	CString	tmp ;
	// 1

	if ( ! SendData ( "HTTP/1.0 200 OK\r\n" ) )
		return ( FALSE ) ;	// skate from here...
	// 2
//	CTime rTime = CTime::GetCurrentTime() ;
	SYSTEMTIME SystemTime;
	GetSystemTime(  &SystemTime  );
	CTime rTime ( SystemTime, -1 );

	tmp.Format ( "Date: %s\r\n",
				rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%Z") ) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...
	// 3
	if ( ! SendData ( "Server: JetStream/1.0\r\n" ))
		return ( FALSE ) ;	// skate from here...
	// 4
	if ( ! SendData ( "MIME-version: 1.0\r\n" ) )
		return ( FALSE ) ;	// skate from here...
	// 5
	char ext[5] ;
	_splitpath ( filename, NULL, NULL, NULL, ext ) ;	
	tmp = ext ;
	for ( int i = 0 ; i < MIME_len ; i++ )
	{
		if ( tmp.CompareNoCase ( MIME_Table[i].ext ) == 0 )
		{
			if ( ! SendData ( "Content-type: " ) )
				return ( FALSE ) ;	// skate from here...
			if ( ! SendData ( MIME_Table[i].type ) )
				return ( FALSE ) ;	// skate from here...
			if ( ! SendData ( "\r\n" ) )
				return ( FALSE ) ;	// skate from here...
			break ;
		}
	}
	// 6
	tmp.Format ( "Last-modified: %s\r\n",
					rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%Z") ) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...

	CTimeSpan ts( 0, 0, 30, 0 );
	rTime = rTime + ts;
	tmp.Format ( "Expires: %s\r\n",
					rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%Z") ) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...

	// 7
	tmp.Format ( "Content-length: %d\r\n", filele ) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...
	// 8
	if ( ! SendData ( "\r\n" ) )
		return ( FALSE ) ;	// skate from here...

	// end-of-header
	return ( TRUE ) ;
}	// SendReplyHeader()


BOOL CClient::SendReplyHeader ( CFile& cFile )
{
	if ( (! m_bHTTP10) && (! m_bHTTP11) )	// if HTTP 0.9, response header not used
		return ( TRUE ) ;

	// Response header components:
	// 1 - HTTP 1.0 response header
	// 2 - Server time
	// 3 - Server version
	// 4 - MIME version
	// 5 - MIME type
	// 6 - Last-modified time
	// 7 - Content length
	// 8 - HTTP 1.0 End-of-header

	CString	tmp ;
	// 1
	if ( ! SendData ( "HTTP/1.0 200 OK\r\n" ) )
		return ( FALSE ) ;	// skate from here...
	// 2
//	CTime rTime = CTime::GetCurrentTime() ;

	SYSTEMTIME SystemTime;
	GetSystemTime(  &SystemTime  );
	//GetLocalTime(  &SystemTime  );
	CTime rTime ( SystemTime, -1 );

	tmp.Format ( "Date: %s\r\n",
				rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%Z") ) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...
	// 3
	if ( ! SendData ( "Server: JetStream/1.0\r\n" ) )
		return ( FALSE ) ;	// skate from here...
	// 4
	if ( ! SendData ( "MIME-version: 1.0\r\n" ) )
		return ( FALSE ) ;	// skate from here...
	// 5
	char ext[_MAX_EXT +1] ;
	memset(ext,0,sizeof(ext));

	_splitpath ( cFile.GetFileName(), NULL, NULL, NULL, ext ) ;

	tmp = ext ;
	for ( int i = 0 ; i < MIME_len ; i++ )
	{
		if ( tmp.CompareNoCase ( MIME_Table[i].ext ) == 0 )
		{
			if ( ! SendData ( "Content-type: " ) )
				return ( FALSE ) ;	// skate from here...
			if ( ! SendData ( MIME_Table[i].type ) )
				return ( FALSE ) ;	// skate from here...
			if ( ! SendData ( "\r\n" ) )
				return ( FALSE ) ;	// skate from here...
			break ;
		}
	}

	{
		if (strstr(cFile.GetFileName(),".HTM") || strstr(cFile.GetFileName(),".CLASS"))
		{
			// 6
			tmp.Format ( "Last-modified: %s\r\n",
					rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%z") ) ;
			if ( ! SendData ( tmp ) )
				return ( FALSE ) ;	// skate from here...

			CTimeSpan ts( 0, 0, 1, 0 );
			rTime = rTime + ts;
			tmp.Format ( "Expires: %s\r\n",
					rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%z") ) ;
			//SendData ( tmp ) ;
			if ( ! SendData ( "Expires: 0\r\n" ) )
				return ( FALSE ) ;	// skate from here...
			if ( ! SendData ( "Pragma: no-cache\r\n" ) )
				return ( FALSE ) ;	// skate from here...

		}
		else
		{
			// 6
			CFileStatus rStatus ;
			if ( cFile.GetStatus ( rStatus ) )
			{
				tmp.Format ( "Last-modified: %s\r\n",
					//rTime.FormatGmt
					rStatus.m_mtime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%z") ) ;
					//rStatus.m_mtime.FormatGmt("%a, %d %b %Y %H:%M:%S %Z") ) ;
				if ( ! SendData ( tmp ) )
					return ( FALSE ) ;	// skate from here...
			}
			CTimeSpan ts( 0, 8, 0, 0 );
			rTime = rTime + ts;
			tmp.Format ( "Expires: %s\r\n",
					rTime.FormatGmt("%a, %d %b %Y %H:%M:%S GMT"));//%z") ) ;
			if ( ! SendData ( tmp ) )
				return ( FALSE ) ;	// skate from here...
			//SendData ( "Expires: 0\r\n" ) ;
			//SendData ( "Pragma: no-cache\r\n" ) ;
		}

	}
	
	// 7
	tmp.Format ( "Content-length: %d\r\n", cFile.GetLength()) ;
	if ( ! SendData ( tmp ) )
		return ( FALSE ) ;	// skate from here...
	
	// 8
	if ( ! SendData ( "\r\n" ) )
		return ( FALSE ) ;	// skate from here...	
	
	// end-of-header
	return ( TRUE ) ;
}	// SendReplyHeader()

void CClient::SendTag()
{
	// send our personalized message
	CString tagmsg ;
	BOOL ret = TRUE ;
	char szTemp[MAX_PATH];
	char szTempo[MAX_PATH];

	switch ( ((CGrStnApp*)AfxGetApp())->m_nTagType )
	{
	case CGrStnApp::TAG_AUTO:
//			tagmsg.LoadString ( IDS_TAGSTRING ) ;
//			strcpy(szTemp,tagmsg);
//			sprintf(szTempo, szTemp,
//				CgVersion/ 100, (CgVersion  - (CgVersion/100)*100) );
//
//			ret = SendData ( szTempo ) ;
		break ;
	case CGrStnApp::TAG_FILE:

			//tagmsg = ((CGrStnApp*)AfxGetApp())->m_HTMLPath + CString("\\") + ((CGrStnApp*)AfxGetApp())->m_HTMLTag ;
			//ret = SendFile ( tagmsg, ((CGrStnApp*)AfxGetApp())->m_HTMLTag, TRUE ) ;
			break ;
	case CGrStnApp::TAG_NONE:
		break ;
	}
}	// SendTag()

/////////////////////////////////////////////////////////////////////////////
// URI file handler

void SendFileDone()
{
}

BOOL CClient::SendFile ( CString& m_cLocalFNA, CString& BaseFNA,
								 BOOL bTagMsg )
{
	CFile cFile ;
	BOOL FoundIt ;
	char szTemp[MAX_PATH];
	char * szWWWROOT;
	BOOL GifFileToBeSent = FALSE;

	memset(szTemp, 0, sizeof(szTemp));
	strncpy(szTemp, m_cLocalFNA,sizeof(szTemp) - 1);
	strupr(szTemp);
	m_fDoAudioLastVideo = TRUE;
	m_TimeBroken = FALSE;



	//if this is not a jpg, html, htm, class,wav, aiv,aif,gif then nothing to do
	if ((strstr(szTemp,".JPG") != NULL ) ||
		(strstr(szTemp,".HTML") != NULL ) ||
		(strstr(szTemp,".HTM") != NULL ) ||
		(strstr(szTemp,".CLASS") != NULL ) ||
		(strstr(szTemp,".WAV") != NULL ) ||
		(strstr(szTemp,".AVI") != NULL ) ||
		(strstr(szTemp,".AIF") != NULL ) ||
		(strstr(szTemp,".AUO") != NULL ) ||
		(strstr(szTemp,".GIF") != NULL ) ||
		(strstr(szTemp,".BMP") != NULL ) ||
		(strstr(szTemp,".WML") != NULL ) ||
		(strstr(szTemp,".WMLC") != NULL ) ||
		(strstr(szTemp,".WMLS") != NULL ) ||
		(strstr(szTemp,".WMLSC") != NULL ) ||
		(strstr(szTemp,".WBMP") != NULL ) //||
		//m_cLocalFNA == ((CGrStnApp*)AfxGetApp())->m_HTMLPath + CString("\\")
	   )
	   ;
	else
	{
		SendFileDone();
		SendCannedMsg ( 404, BaseFNA ) ;
		return ( TRUE ) ;;

	}
	//if (strstr(szTemp,".AUO") != NULL)

	CTime myTime = CTime::GetCurrentTime();
	int LastFile;
	int FoundUser;
	int CurrentUser = 0;
	BOOL ThisIsANewUser = FALSE;



DO_SEND_FILE:
	CTime NowTime = CTime::GetCurrentTime();
	CTimeSpan timeSpan = ::CTimeSpan( 0, 0, 3, 0 );
	//((CGrStnApp*)AfxGetApp())->WebSendOneFileOldTime = NowTime + timeSpan;
	TimeStartSend = NowTime;
	BOOL ret = TRUE ;

	if (GifFileToBeSent)
	{
		unlink(szTemp);
    }
	SendFileDone();
	return ( ret ) ;
}	// SendFile()

/////////////////////////////////////////////////////////////////////////////
// CGI handler

BOOL CClient::SendCGI ( CString& m_cLocalFNA, CString& BaseFNA )
{
	// This is a hook for future development. Just send 404 error for now.
	SendCannedMsg ( 404, BaseFNA ) ;
	return ( TRUE ) ;
}

/////////////////////////////////////////////////////////////////////////////
// Data transmission operations

// this is for sending our own info and messages to the client
BOOL CClient::SendRawData ( LPVOID lpMessage, int count )
{
	// Use a CSocketFile for pumping the message out
	BOOL ret;
	try
	{
		if (Send ( lpMessage, count, 0 ) == SOCKET_ERROR )
		{
			ret = FALSE;

		}
		else
			ret = TRUE;
	}
	catch(...)
	{

		ret = FALSE;
	}
	return ret;
}

// this is for sending a CString message to the client
BOOL CClient::SendData ( CString& cMessage )
{
	return ( SendData ( (LPCTSTR)cMessage ) ) ;
}

// this is for sending a LPSTR message to the client
BOOL CClient::SendData ( LPCSTR lpszMessage )
{
//	m_pDoc->DbgVMessage ( ">>>Sending client message: %s\n", lpszMessage ) ;

	return ( SendRawData ( (LPVOID)lpszMessage, strlen(lpszMessage) ) ) ;
}

// this is for sending file data to the client
#define	BUF_SIZE	32000 // 4096	// same as default for CSocket as CArchive
BOOL CClient::FilePreBufm0 ( char *filename )
{
	if (sizem0 == 0)
	{
		int iFi = _open(filename,O_BINARY|O_RDONLY);
		if (iFi > 0)
		{
			sizem0 = _read (iFi,bufm0, sizeof(bufm0) );
			_close(iFi);
		}
	}
	return(TRUE);

}
BOOL CClient::SendDatam0 ( char *filename )
{

	
	// Use a CSocketFile for pumping the data out
	CSocketFile *sockFile = NULL;
	BOOL ret = TRUE;
	try
	{
		sockFile = new CSocketFile( this, FALSE ) ;

		if (sockFile)
		{
			sockFile->Write ( (LPVOID)bufm0, sizem0 ) ;
			bytes += sizem0 ;
			sockFile->Flush() ;
		}
		ret = TRUE;

	}
	catch(CFileException *e)
	{
//		delete e;
		if (sockFile)
		{
			delete sockFile;
			sockFile = NULL;
		}
		ret = FALSE;
		e->Delete();

	}
	if (sockFile)
		delete sockFile;

	return ( ret ) ;
	
}

BOOL CClient::SendData ( CFile& cFile )
{
	char	buf[BUF_SIZE] ;
	int		nBytes ;
	BOOL	ret = TRUE;

	// Use a CSocketFile for pumping the data out
	CSocketFile *sockFile = NULL;
	try
	{
		sockFile = new CSocketFile( this, FALSE ) ;

		if (sockFile)
		{

			while ( ( nBytes = cFile.Read ( buf, BUF_SIZE ) ) > 0 )
			{
				if ((strstr(cFile.GetFileName(),".HTM") != NULL) ||
					strstr(cFile.GetFileName(),".WML"))
				{
					char szTemTS[_MAX_PATH];
					char szTemWTS[_MAX_PATH];

					char szTemTS2[_MAX_PATH];
					__int64 iTimeNTS;

					char szTemATS[_MAX_PATH];
					char szTemWATS[_MAX_PATH];

					char szTemATS2[_MAX_PATH];
					__int64 iTimeNATS;

					char szHeght[_MAX_PATH];
					char szWidth[_MAX_PATH];
					char szNCamera[_MAX_PATH];

					char *ptStamp = NULL;
					

					sprintf(szTemTS,"%020I64d",iTimeNTS);
					//sprintf(szTemTS2,"%020I64d",iTimeNTS-StartTime64);

					sprintf(szTemATS,"%020I64d",iTimeNATS);
					//sprintf(szTemATS2,"%020I64d",iTimeNATS-StartTime64);

					sprintf(szTemWTS,"%c%020I64d",m_WapCam+'0',iTimeNTS);
					sprintf(szTemWATS,"%c%020I64d",m_WapCam+'0', iTimeNATS);

					//                        <!--###AutoVar=T000>
					//                        #####TIME_STAMP#####
					if (ptStamp = strstr(buf,"<!--###AutoVar=T000>"))
						memcpy(ptStamp,szTemTS,20);

					

#ifdef _PRINT_DEBUG
					FILE *FStory = fopen("FStory","a");
					if (FStory)
					{
						fprintf(FStory, "\niTimeNTS=%s",szTemTS);
						//fprintf(FStory, "\niTimeNTS2=%s",szTemTS2);
						fprintf(FStory, "\niTimeNATS=%s",szTemATS);
						//fprintf(FStory, "\niTimeNATS2=%s",szTemATS2);

						fclose(FStory);
					}
#endif

				}
				sockFile->Write ( (LPVOID)buf, nBytes ) ;
				bytes += nBytes ;
			}
			cFile.Close() ;
			sockFile->Flush() ;
			sockFile->Close();
			ret = TRUE;
		}
	

	}
	catch(CFileException *e)
	{
		cFile.Close() ;
		ret = FALSE;
		e->Delete();

	}

	if (sockFile)
	{
		try
		{
			delete sockFile;
		}
		catch(CFileException *e)
		{
			ret = FALSE;
			e->Delete();
		}

		sockFile = NULL;
	}

	return ( ret ) ;
	
}

/////////////////////////////////////////////////////////////////////////////
// CClient Utility Operations

BOOL CClient::ResolveClientName ( BOOL bUseDNS )
{
	if ( ! GetPeerName ( m_PeerIP, m_Port ) )
	{
//		m_pDoc->Message ( " Can't get client name\n" ) ;
		return ( FALSE ) ;
	}
	//m_LogRec.client = m_PeerIP ;

	if ( bUseDNS )
	{
		if ( m_PeerIP == "127.0.0.1" )	// hey, it's me!!!
		{
			client = "Local loopback" ;
//			m_pDoc->VMessage ( " Local loopback (%s)\n", m_PeerIP ) ;
		}
		else
		{
			if ( m_pHE = GetHostByAddr ( (LPCSTR)m_PeerIP ) )
			{
				client = m_pHE->h_name ;
//				m_pDoc->VMessage ( " %s (%s)\n", m_pHE->h_name, m_PeerIP ) ;
			}
			else
			{
				int err = WSAGetLastError() ;
//				m_pDoc->VMessage ( " Unable to get host name: %s. Err: %d\n",
//											m_PeerIP, err ) ;
				return ( FALSE ) ;
			}
		}
	}
	else
	{
//		m_pDoc->VMessage ( " %s\n", m_PeerIP ) ;
	}
	return ( TRUE ) ;
}

//
//	The following code is used for sending predefined messages.
//

// table of canned messages that we can handle
static struct _tagMsgTable
{
	int	id ;
	int	idStr ;
} MsgTable[] =
{
	{ 400, IDS_400_MESSAGE },
	{ 404, IDS_404_MESSAGE },
	{ 405, IDS_405_MESSAGE },
	{ 503, IDS_503_MESSAGE }
} ;
static const int MsgTableSize = sizeof(MsgTable)
										  / sizeof(struct _tagMsgTable) ;

void CClient::SendCannedMsg ( int idErr, ... )
{
	int i;
	BOOL bGotIt = FALSE ;
	BOOL ret = TRUE;
	for ( i = 0 ; i < MsgTableSize ; i++ )
	{
		if ( MsgTable[i].id == idErr )
		{
			bGotIt = TRUE ;
			break ;
		}
	}

	CString fmt ;
	char	buf[500] ;
	char    szTempoB[_MAX_PATH];
	if ( ! bGotIt )	// idErr is a bogus code!
	{
		fmt.LoadString ( IDS_500_MESSAGE ) ;
		sprintf(buf, fmt, idErr ) ; 
		//wsprintf ( buf, fmt, idErr ) ;
	}
	else
	{
		fmt.LoadString ( MsgTable[i].idStr ) ;
		va_list	ptr ;
		va_start ( ptr, idErr ) ;
		if (ptr)
		{
			memset(szTempoB,0,sizeof(szTempoB));
			char *Tem = ptr;
			strncpy(szTempoB,va_arg(ptr,char *),50);
			//wvsprintf ( buf, fmt, szTempoB ) ; 
			sprintf(buf, fmt, szTempoB ) ; 
		}
		else
			sprintf(buf, fmt, "unknown" ) ; 
			//wvsprintf ( buf, fmt, "unknown" ) ; 
		
	}

	try
	{
		if (Send ( buf, strlen(buf), 0 ) == SOCKET_ERROR )
		{
			ret = FALSE;
		}
		else
			ret = TRUE;
	}
	catch(...)
	{
		ret = FALSE;
	}

	// write log record
 	status = idErr ;
	// write status message
//	m_pDoc->DbgVMessage ( "   Sent %03d status message to client\n", idErr ) ;
}
