#pragma once
#include <winsock2.h>
#include <windows.h>
#include "RingBuffer.h"
#include "MiddleWare.h"

#include "Constant.h"


struct st_Session
{
public:
	DWORD dwID;
	DWORD dwLastRecvTime;
	void* pClient;
	CHAR IsValid;
	SOCKET clientSock;
	RingBuffer* pRecvRB;
	RingBuffer* pSendRB;
};



struct st_DisconInfo
{
	st_Session* DisconInfoArr[MAX_SESSION];
	DWORD dwDisconNum = 0;
};

extern st_Session* g_pSessionArr[MAX_SESSION];
extern DWORD g_dwSessionNum;
extern st_DisconInfo g_DisconInfo;
extern DWORD g_dwHandleOffset;



extern st_Session* g_pSessionArr[MAX_SESSION];

__forceinline BOOL IsSessionInValid(st_Session* pSession)
{
	return (pSession->IsValid == INVALID);
}

__forceinline BOOL IsSessionFull()
{
	return (g_dwSessionNum >= MAX_SESSION);
}

__forceinline BOOL ReserveSessionDisconnected(st_Session* pSession)
{
	pSession->IsValid = INVALID;
	g_DisconInfo.DisconInfoArr[g_DisconInfo.dwDisconNum++] = pSession;
	return TRUE;
}

void RegisterSession(SOCKET sock, DWORD dwID, DWORD dwRecvTime);
void RemoveSession(st_Session* pSession);
BOOL InitSessionState();
void ClearSessionInfo();



