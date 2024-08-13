#include "Session.h"
#include "MiddleWare.h"

#include "MemoryPool.h"
#include "Constant.h"

#include <stdio.h>

#include "Logger.h"

MEMORYPOOL g_SessionMemoryPool;
MEMORYPOOL g_RBMemoryPool;
st_Session* g_pSessionArr[MAX_SESSION];
st_DisconInfo g_DisconInfo;

const void* pReserved = (void*)g_pSessionArr;
void* pReservedForDiscon1 = (void*)(&g_DisconInfo.dwDisconNum);
const void* pReservedForDiscon2 = (void*)g_DisconInfo.DisconInfoArr;


DWORD g_dwSessionNum;
DWORD g_dwHandleOffset;
constexpr DWORD dwReservedForState = offsetof(st_Session, IsValid);
constexpr DWORD dwReservedForLastRecvTime = offsetof(st_Session, dwLastRecvTime);

REGISTER_CLIENT RegisterClient;
REMOVE_CLIENT  RemoveClient;
PACKET_PROC packetProc;

void RegisterSession(SOCKET sock, DWORD dwID, DWORD dwRecvTime)
{
	st_Session* pSession;
	void* pClient;

	pSession = (st_Session*)AllocMemoryFromPool(g_SessionMemoryPool);
	pSession->dwID = dwID;
	pSession->clientSock = sock;
	pSession->dwLastRecvTime = dwRecvTime;
	pSession->IsValid = VALID;
	pSession->pRecvRB = (RingBuffer*)AllocMemoryFromPool(g_RBMemoryPool);
	pSession->pRecvRB->iInPos_ = pSession->pRecvRB->iOutPos_ = 0;
	pSession->pSendRB = (RingBuffer*)AllocMemoryFromPool(g_RBMemoryPool);
	pSession->pSendRB->iInPos_ = pSession->pSendRB->iOutPos_ = 0;
	g_pSessionArr[g_dwSessionNum] = pSession;

	// 클라이언트가 제공한 콜백함수 호출, 아마도 CreatePlayer가 호출될것이다.
	RegisterClient(&pClient, g_dwSessionNum, dwID);
	pSession->pClient = pClient;
	++g_dwSessionNum;
}

__forceinline NETWORK_HANDLE HackHandleFromClient(void* pClient)
{
	return (*(NETWORK_HANDLE*)((char*)pClient + g_dwHandleOffset));
}

__forceinline void AlertClientOfHandleChange(st_Session* pSession, NETWORK_HANDLE handle)
{
	*(NETWORK_HANDLE*)((char*)(pSession->pClient) + g_dwHandleOffset) = handle;
}

void RemoveSession(st_Session* pSession)
{
	NETWORK_HANDLE handle;
	st_Session* pMoveSession;

	handle = HackHandleFromClient(pSession->pClient);
	RemoveClient(pSession->pClient);

	// 끝부분은 세션을 옮겨주는 작업 필요없이 g_dwSessionNum만 감소시키면 됨
	if (g_dwSessionNum - 1 != handle)
	{
		pMoveSession = g_pSessionArr[g_dwSessionNum - 1];
		g_pSessionArr[(DWORD)handle] = pMoveSession;
		AlertClientOfHandleChange(pMoveSession, handle);
	}
	closesocket(pSession->clientSock);
	RetMemoryToPool(g_RBMemoryPool, pSession->pSendRB);
	RetMemoryToPool(g_RBMemoryPool, pSession->pRecvRB);
	RetMemoryToPool(g_SessionMemoryPool, pSession);
	--g_dwSessionNum;
}

void InitializeMiddleWare(REGISTER_CLIENT RegisterPlayer, REMOVE_CLIENT RemovePlayer, PACKET_PROC packetProcedure, DWORD HandleOffsetOwnByClient)
{
	RegisterClient = RegisterPlayer;
	RemoveClient = RemovePlayer;
	packetProc = packetProcedure;
	g_dwHandleOffset = HandleOffsetOwnByClient;
}

BOOL InitSessionState()
{
	g_SessionMemoryPool = CreateMemoryPool(sizeof(st_Session), MAX_SESSION);
	g_RBMemoryPool = CreateMemoryPool(sizeof(RingBuffer), MAX_SESSION * 2);
	return TRUE;
}

BOOL GetAllValidClient(DWORD* pOutUserNum, void** ppOutClientArr)
{
	DWORD dwClientNum = 0;
	for (DWORD i = 0; i < g_dwSessionNum; ++i)
	{
		if (g_pSessionArr[i]->IsValid == INVALID)
			continue;

		ppOutClientArr[dwClientNum] = g_pSessionArr[i]->pClient;
		++dwClientNum;
	}

	*pOutUserNum = dwClientNum;
	return TRUE;
}

void ClearSessionInfo()
{
	int Leak;

	LOG(L"TERMINATE", ERR, TEXTFILE, L"TERMINATE Process Start!!\nClear SessionInfo Current Session Num : %u, g_DisconInfoSessionNum : %u", g_dwSessionNum, g_DisconInfo.dwDisconNum);
	for (DWORD i = 0; i < g_dwSessionNum; ++i)
	{
		ReserveSessionDisconnected(g_pSessionArr[i]);
	}

	LOG(L"TERMINATE", ERR, TEXTFILE, L"Remove %u Number of Session", g_DisconInfo.dwDisconNum);
	for (DWORD i = 0; i < g_DisconInfo.dwDisconNum; ++i)
	{
		RemoveSession(g_DisconInfo.DisconInfoArr[i]);
	}

	Leak = ReportLeak(g_SessionMemoryPool);
	if (Leak != 0)
	{
		LOG(L"TERMINATE", ERR, TEXTFILE, L"Session Struct Memory Leak % d Num occured!", Leak);
	}

	Leak = ReportLeak(g_RBMemoryPool);
	if (Leak != 0)
	{
		LOG(L"TERMINATE", ERR, TEXTFILE, L"RingBuffer Struct Memory Leak %d Num Occured!", Leak);
	}
}
