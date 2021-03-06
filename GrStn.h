
// GrStn.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif
#include <afxinet.h>

#include "resource.h"		// main symbols

#define MAX_LINES 10
// CGrStnApp:
// See GrStn.cpp for the implementation of this class
//
#pragma pack(push)
#pragma pack(1)
typedef struct
{
WORD RXaTmr1;
WORD RXaTmr1H;
WORD RXbTmr1;
WORD RXbTmr1H;
WORD TXaTmr1;
WORD TXaTmr1H;
WORD TXbTmr1;
WORD TXbTmr1H;
WORD TXPeriod;

WORD ERXaTmr1;
WORD ERXaTmr1H;
WORD ERXbTmr1;
WORD ERXbTmr1H;
WORD ETXaTmr1;
WORD ETXaTmr1H;
WORD ETXbTmr1;
WORD ETXbTmr1H;
WORD ETXPeriod;

} DISTMEASURE;
#pragma pack(pop)
class CGrStnApp : public CWinApp
{
public:
	CGrStnApp();

// Overrides
public:
	virtual BOOL InitInstance();
	char szFileName[MAX_PATH];
	HANDLE      MyMutex;
	char szIniFileName[MAX_PATH];
	char szURL[MAX_PATH];
	int UrlPort;
	char szComPort[MAX_PATH];
	BOOL ItIsPOssibelToOpenComm;
	HANDLE hComHandle;
	OVERLAPPED Ovlpd;
    OVERLAPPED OvlpdWrite;
    BOOL WriteRequested;
	BOOL OpenCommPort(char *szCom);
	BOOL WriteFileCom(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToWrite, LPDWORD lpNBytesWritten, LPOVERLAPPED lpOverlapped);
    BOOL WriteFileComHex(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToWrite, LPDWORD lpNBytesWritten, LPOVERLAPPED lpOverlapped);
	BOOL ReadFileCom(HANDLE hCommFile, LPCVOID lpBuffer, DWORD nNBytesToRead, LPDWORD lpNBytesRead, LPOVERLAPPED lpOverlapped);

	HANDLE		hWaitForHandleExit;
	HANDLE		hWaitForHandleToReadCommFile;
	HANDLE		Callback_ReadThread;
	DWORD		dwServiceStateThreadID;
	HANDLE		hWaitForHandleDoneExit;

	CHttpConnection* m_MainHttpServer;
	CInternetSession  *m_MainInternetConnection;
	char szWebServerRQ[4098];
	char session_no[32];
	DWORD SessionN;
    DWORD SessionNOldProcessed;
	// '1' - upload '2' - download '3' - error '4'- test Ground Station 
    char packet_type[10];

    int packet_no;
    char d_time[32];
    char g_station[5];
    char gs_time[32];
	unsigned char bPacket[4096];
	int iOutptr;
	char tmpWebServerResp[4096*4];
	BOOL MakeRQ(char *StartPtr, int iLen);
	char szWebServerResp[4096*5];
	UINT		m_wwwPort ;			// well-known WWW port number
	UINT		m_nMaxConnects ;	// max number of connections we'll allow
	UINT		m_nTimeOut ;		// idle-client disconnect limit
	UINT		m_nSanityTime ;		// watchdog timer period
	enum SERVER_STATE
	{
		ST_NULL,		// not initialized yet
		ST_WAITING,		// waiting for request
		ST_SENDING		// send in progress
	} m_State ;
	enum TAG_TYPE
	{
		TAG_NONE = 0,
		TAG_AUTO,
		TAG_FILE
	} m_nTagType ;

	BOOL SessionNSet;
	BOOL PacketUpLinkSet;
	BOOL PacketPing;
	unsigned char bPacketUpLink[200000];
	unsigned char bPacketUpLinkBin[200000];
    BOOL UpLinkDone;
	unsigned char bPacketDownLink[4048];
	DWORD BytesDownLinkRead;
    DWORD BytesUplinkWritten;
	BOOL ServerOnline;
	BOOL SendDownLink(char *pktType, char *StartData, int iSizeToSend);
	BOOL MakeHex(void);
    BOOL PrePorocess(unsigned char bByte);
    int StatusDistance;
    int StatusEsc;
    DISTMEASURE DM1;
    DISTMEASURE DM2;
    DWORD DTimeEarth[400];
    DWORD DTimeLuna[400];
    DWORD Distance[400];
    double Medium;
    int iMeasurements;
    CString MakeItReadable(char * packet, int length);
// Implementation

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);
	
};

extern CGrStnApp theApp;