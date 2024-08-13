#define SYNC
#include <WS2spi.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include "Constant.h"
#include "Network.h"
#include "SerializeBuffer.h"
#include "RingBuffer.h"
#include "Session.h"
#include "Logger.h"

#ifdef SYNC
#include "CSCContents.h"
#include "Client.h"
#endif





#pragma comment(lib,"ws2_32.lib")
SOCKET g_listenSock;

DWORD g_dwID = 0;
DWORD g_dwUserNum = 0;

SerializeBuffer g_sb1;
SerializeBuffer g_sb2;

extern int g_iDisconCount;
extern int g_iDisConByRBFool;

void SendProc(st_Session* pSession);
BOOL RecvProc(st_Session* pSession);

BOOL NetworkInitAndListen()
{
	int bindRet;
	int listenRet;
	int ioctlRet;
	int ssoRet;
	LINGER linger;
	u_long iMode;
	WSADATA wsa;
	SOCKADDR_IN serverAddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"WSAStartUp() Err Code %u", WSAGetLastError());
		return FALSE;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"WSAStartUp()");
	g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket() func error code : %d\n", WSAGetLastError());
		return FALSE;
	}

	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(g_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"bind() Err Code %u", WSAGetLastError());
		return FALSE;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"bind OK # Port : %d", SERVERPORT);

	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"listen() Err Code %u", WSAGetLastError());
		return FALSE;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"listen() OK");
	linger.l_onoff = 1;
	linger.l_linger = 0;
	ssoRet = setsockopt(g_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (ssoRet != 0)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"setsockopt() Err Code %u", WSAGetLastError());
		return FALSE;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"Linger OK #");

	iMode = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &iMode);
	if (ioctlRet != 0)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"ioctlsocket() func err Code %u", WSAGetLastError());
		return FALSE;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"ioctlsocket() OK", WSAGetLastError());

	InitSessionState();
	g_sb1.AllocBuffer(10000);
	g_sb2.AllocBuffer(10000);
	return TRUE;
}

BOOL EnqPacketRB(const void* pClient, char* pPacket, const DWORD packetSize)
{
	st_Session* pSession;
	int iEnqRet;

	pSession = g_pSessionArr[*(NETWORK_HANDLE*)((char*)pClient + g_dwHandleOffset)];
	if (pSession->IsValid == INVALID)
		return FALSE;


	iEnqRet = pSession->pSendRB->Enqueue(pPacket, packetSize);
	if (iEnqRet == 0)
	{
		++g_iDisConByRBFool;
		ReserveSessionDisconnected(pSession);
		return FALSE;
	}

	return TRUE;
}

BOOL AcceptProc()
{
	int addrlen;
	SOCKADDR_IN clientAddr;
	SOCKET clientSock;
	// 이거 SHORT로 만들어서 터짐
	DWORD dwNewID;
	DWORD dwRecvTime;

	addrlen = sizeof(clientAddr);
	clientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		wprintf(L"accept func error code : %d\n", WSAGetLastError());
		__debugbreak();
		return FALSE;
	}

	if(g_dwSessionNum >= MAX_SESSION)
	{
		closesocket(clientSock);
		return FALSE;
	}

	dwNewID = g_dwID++;
#ifdef _DEBUG
	WCHAR szIpAddr[MAX_PATH];
	InetNtop(AF_INET, &clientAddr.sin_addr, szIpAddr, _countof(szIpAddr));
	_LOG(dwLog_LEVEL_DEBUG, L"Connect # ip : %s / SessionId : %u", szIpAddr, dwNewID);
#endif
	dwRecvTime = timeGetTime();
	RegisterSession(clientSock, dwNewID, dwRecvTime);
	return TRUE;
}

__forceinline void SelectProc(st_Session** ppSessionArr, fd_set* pReadSet, fd_set* pWriteSet)
{
	int iSelectRet;
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	iSelectRet = select(0, pReadSet, pWriteSet, nullptr, &tv);
	if (iSelectRet == 0)
	{
		return;
	}

	if (iSelectRet == SOCKET_ERROR)
	{
		__debugbreak();
		return;
	}

	if (FD_ISSET(g_listenSock, pReadSet))
	{
		--iSelectRet;
		AcceptProc();
	}

	for (SHORT i = 0; i < FD_SETSIZE - 1 && iSelectRet > 0; ++i)
	{
		if (FD_ISSET(ppSessionArr[i]->clientSock, pReadSet))
		{
			--iSelectRet;
			RecvProc(ppSessionArr[i]);
		}

		if (FD_ISSET(ppSessionArr[i]->clientSock, pWriteSet))
		{
			--iSelectRet;
			SendProc(ppSessionArr[i]);
		}
	}
	return;
}

BOOL NetworkProc()
{
	fd_set readSet;
	fd_set writeSet;
	st_Session* pSession;
	st_Session* pSessionArrForSelect[FD_SETSIZE - 1];
	DWORD dwSockCount;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(g_listenSock, &readSet);
	dwSockCount = 1;

	for (DWORD i = 0; i < g_dwSessionNum; ++i)
	{
		pSession = g_pSessionArr[i];
		FD_SET(pSession->clientSock, &readSet);
		if (pSession->pSendRB->GetUseSize() > 0)
		{
			FD_SET(pSession->clientSock, &writeSet);
		}

		if (dwSockCount - 1 == 63)
			__debugbreak();

		pSessionArrForSelect[dwSockCount - 1] = pSession;
		++dwSockCount;

		if (dwSockCount >= FD_SETSIZE)
		{
			dwSockCount = 1;
			SelectProc(pSessionArrForSelect, &readSet, &writeSet);
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			FD_SET(g_listenSock, &readSet);
		}
	}

	if (dwSockCount > 0)
	{
		SelectProc(pSessionArrForSelect, &readSet, &writeSet);
	}

	for (DWORD i = 0; i < g_DisconInfo.dwDisconNum; ++i)
	{
		RemoveSession(g_DisconInfo.DisconInfoArr[i]);
		++g_iDisconCount;
	}
	g_DisconInfo.dwDisconNum = 0;

	return TRUE;
}

void SendProc(st_Session* pSession)
{
	RingBuffer* pSendRB;
	int iUseSize;
	int iDirectDeqSize;
	int iSendRet;
	int iErrCode;
	int iBufLen;
	DWORD dwNOBS;
	WSABUF wsa[2];

	// 삭제해야하는 id목록에 존재한다면 그냥 넘긴다
	if(pSession->IsValid ==INVALID)
		return;

	pSendRB = pSession->pSendRB;

	iUseSize = pSendRB->GetUseSize();
	iDirectDeqSize = pSendRB->DirectDequeueSize();
	if (!iUseSize)
		return;

	if (iUseSize <= iDirectDeqSize)
	{
		wsa[0].buf = pSendRB->GetReadStartPtr();
		wsa[0].len = iUseSize;
		iBufLen = 1;
	}
	else
	{
		wsa[0].buf = pSendRB->GetReadStartPtr();
		wsa[0].len = iDirectDeqSize;
		wsa[1].buf = pSendRB->Buffer_;
		wsa[1].len = iUseSize - iDirectDeqSize;
		iBufLen = 2;
	}

	iSendRet = WSASend(pSession->clientSock, wsa, iBufLen, &dwNOBS, 0, NULL, NULL);

	// WOULDBLOCK은 그냥 리턴(당연히 보내지 않앗으니 반영도못한다)
	// 10054 WSAECONNRESET은 정상종료이므로 로그 남기지않고 disconnect한다
	if (iSendRet == SOCKET_ERROR)
	{
		iErrCode = WSAGetLastError();
		if (iErrCode == WSAEWOULDBLOCK)
			goto lb_return;

		if (iErrCode == WSAECONNRESET)
			goto lb_disconnect;


		LOG(L"DISCON", ERR, TEXTFILE, L"Session ID : %u WSASend() err Code : %u", pSession->dwID, iErrCode);
	lb_disconnect:
		ReserveSessionDisconnected(pSession);
	lb_return:
		return;
	}
	
	pSendRB->MoveOutPos(dwNOBS);
}


BOOL RecvProc(st_Session* pSession)
{
	RingBuffer* pRecvRB;
	int iDirEnqSize;
	int iFreeSize;
	int iBufLen;
	DWORD dwNOBR;
	WSABUF wsa[2];
	int iErrCode;

	if(pSession->IsValid == INVALID)
		return FALSE;

	pSession->dwLastRecvTime = timeGetTime();
	pRecvRB = pSession->pRecvRB;
	
	iDirEnqSize = pRecvRB->DirectEnqueueSize();
	iFreeSize = pRecvRB->GetFreeSize();
	if (iFreeSize <= iDirEnqSize)
	{
		wsa[0].buf = pRecvRB->GetWriteStartPtr();
		wsa[0].len = iFreeSize;
		iBufLen = 1;
	}
	else
	{
		wsa[0].buf = pRecvRB->GetWriteStartPtr();
		wsa[0].len = iDirEnqSize;
		wsa[1].buf = pRecvRB->Buffer_;
		wsa[1].len = iFreeSize - iDirEnqSize;
		iBufLen = 2;
	}

	DWORD dwFlags = 0;
	int iRecvRet = WSARecv(pSession->clientSock, wsa, iBufLen, &dwNOBR, &dwFlags, NULL, NULL);
	if (iRecvRet == 0 && dwNOBR == 0)
	{
		ReserveSessionDisconnected(pSession);
		return FALSE;
	}
	else if (iRecvRet == SOCKET_ERROR)
	{
		iErrCode = WSAGetLastError();
		if (iErrCode == WSAEWOULDBLOCK)
			goto lb_return;

		if (iErrCode == WSAECONNRESET)
			goto lb_disconnect;

		LOG(L"DISCON", ERR, TEXTFILE, L"Session ID : %u WSARecv() err Code : %u", pSession->dwID, iErrCode);
	lb_disconnect:
		ReserveSessionDisconnected(pSession);
	lb_return:
		return FALSE;
	}
	pRecvRB->MoveInPos(dwNOBR);

	int iPeekRet;
	int iDeqRet;
	Header header;
	while (true)
	{
		iPeekRet = pRecvRB->Peek((char*)&header, sizeof(header));
		if (iPeekRet == 0)
			return FALSE;

		if (header.byCode != 0x89)
		{
			__debugbreak();
			LOG(L"DISCON", ERR, TEXTFILE, L"Session ID : %u\tDisconnected by INVALID HeaderCode Packet Received", pSession->dwID);
			ReserveSessionDisconnected(pSession);
			return FALSE;
		}

#ifdef SYNC
		st_Client* pClient = (st_Client*)pSession->pClient;
		if (header.byType == dfPACKET_CS_MOVE_START)
		{
			if (pClient->IsAlreadyStart)
			{
				pClient->Start2ArrivedTime = timeGetTime();
				pClient->Start2ArrivedFPS = g_fpsCheck;
			}
			else
			{
				pClient->Start1ArrivedTime = timeGetTime();
				pClient->Start1ArrivedFPS = g_fpsCheck;
			}
		}
		else if (pClient->IsAlreadyStart && header.byType == dfPACKET_CS_MOVE_STOP)
		{
			pClient->StopArrivedTime = timeGetTime();
			pClient->StopArrivedFPS = g_fpsCheck;
		}
#endif
		// 직렬화버퍼이므로 매번 메시지를 처리하기 직전에는 rear_ == front_ == 0이라는것이 전제
		if (g_sb1.bufferSize_ < header.bySize)
		{
			__debugbreak();
			g_sb1.Resize();
		}

		iDeqRet = pRecvRB->Dequeue(g_sb1.GetBufferPtr(), sizeof(header) + header.bySize);
		if (iDeqRet == 0)
			return FALSE;

#ifdef SYNC
		{
			st_Client* pClient = (st_Client*)pSession->pClient;
			if (header.byType == dfPACKET_CS_MOVE_START)
			{
				if (pClient->IsAlreadyStart)
				{
					pClient->Start2MashalingTime = timeGetTime();
					pClient->Start2MashalingFPS = g_fpsCheck;
				}
				else
				{
					pClient->Start1MashalingTime = timeGetTime();
					pClient->Start1MashalingFPS = g_fpsCheck;
					pClient->IsAlreadyStart = TRUE;
				}
			}
			else if (header.byType == dfPACKET_CS_MOVE_STOP)
			{
				pClient->StopMashallingTime = timeGetTime();
				pClient->StopMashalingFPS = g_fpsCheck;
				pClient->IsAlreadyStart = FALSE;
			}
		}
#endif

		g_sb1.MoveWritePos(iDeqRet);
		g_sb1.MoveReadPos(sizeof(header));
		packetProc(pSession->pClient, header.byType);
	}
	return TRUE;
}
