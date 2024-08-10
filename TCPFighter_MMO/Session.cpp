#include "Session.h"
#include "MiddleWare.h"

#include "MemoryPool.h"
#include "Constant.h"

#include <stdio.h>

MEMORYPOOL g_SessionMemoryPool;
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
	pSession->recvRB.iInPos_ = pSession->recvRB.iOutPos_ = 0;
	pSession->sendRB.iInPos_ = pSession->sendRB.iOutPos_ = 0;
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
	handle = HackHandleFromClient(pSession->pClient);
	RemoveClient(pSession->pClient);
	g_pSessionArr[(DWORD)handle] = g_pSessionArr[g_dwSessionNum - 1];
	closesocket(pSession->clientSock);
	RetMemoryToPool(g_SessionMemoryPool, pSession);
	--g_dwSessionNum;
	AlertClientOfHandleChange(g_pSessionArr[handle], handle);
}

void RemoveSession(NETWORK_HANDLE handle)
{
	closesocket(g_pSessionArr[(DWORD)handle]->clientSock);
	RetMemoryToPool(g_SessionMemoryPool, g_pSessionArr[(DWORD)handle]);
	g_pSessionArr[(DWORD)handle] = g_pSessionArr[g_dwSessionNum - 1];
	--g_dwSessionNum;
	*((NETWORK_HANDLE*)g_pSessionArr[(DWORD)handle] + g_dwHandleOffset) = handle;
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
	for (DWORD i = 0; i < g_dwSessionNum; ++i)
	{
		RemoveSession(g_pSessionArr[i]);
	}

	Leak = ReportLeak(g_pSessionArr);
	if (Leak != 0)
	{
		__debugbreak();
		printf("Session Struct Memory Leak %d Num Occured!\n",Leak);
	}
}
