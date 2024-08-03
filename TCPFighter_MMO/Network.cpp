#include <WS2spi.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include "Constant.h"
#include "Network.h"
#include "SerializeBuffer.h"
#include "MemoryPool.h"
#include "LinkedList.h"
#include "HashTable.h"
#include "RingBuffer.h"
#include "Session.h"
#include "SessionManager.h"
#include "DisconnectManager.h"
#include "Sector.h"
#include "Client.h"
#include "ClientManager.h"
#include "CStub.h"
#include "Logger.h"
#include "Constant.h"
#include "SCContents.h"


#pragma comment(lib,"ws2_32.lib")
SOCKET g_listenSock;

SHORT g_dwID = 0;
SHORT g_dwUserNum = 0;
SerializeBuffer g_sb;
SessionManager* g_pSessionManager;
DisconnectManager* g_pDisconnectManager;
ClientManager* g_pClientManager;

void SendProc(st_Session* pSession);
BOOL RecvProc(st_Session* pSession);

BOOL NetworkInitAndListen()
{
	int bindRet;
	int listenRet;
	int ioctlRet;
	int ssoRet;
	BOOL bRet;
	LINGER linger;
	u_long iMode;
	WSADATA wsa;
	SOCKADDR_IN serverAddr;
	bRet = FALSE;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf(L"WSAStartup() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}
	_LOG(dwLog_LEVEL_DEBUG, L"WSAStartup()");
	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}

	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(g_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		wprintf(L"bind() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}
	_LOG(dwLog_LEVEL_DEBUG, L"Bind OK # Port : %d", SERVERPORT);

	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wprintf(L"listen() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}
	_LOG(dwLog_LEVEL_DEBUG, L"Listen OK #");
	linger.l_onoff = 1;
	linger.l_linger = 0;
	ssoRet = setsockopt(g_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (ssoRet != 0)
	{
		wprintf(L"setsockopt() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}

	_LOG(dwLog_LEVEL_DEBUG, L"Linger OK #");

	iMode = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &iMode);
	if (ioctlRet != 0)
	{
		wprintf(L"ioctlsocket() func error code : %d\n", WSAGetLastError());
		goto lb_return;
	}

	g_pSessionManager = new SessionManager;
	g_pSessionManager->Initialize();

	g_pDisconnectManager = new DisconnectManager;
	g_pDisconnectManager->Initialize();

	g_pClientManager = new ClientManager;
	g_pClientManager->Initialize();

	g_sb.AllocBuffer();

	g_pCSProc = new CSProc;

	bRet = TRUE;
	lb_return:
	return bRet;
}

BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize)
{
	st_Session* pSession;
	RingBuffer* pSendRB;
	BOOL bRet;
	int iEnqRet;

	bRet = FALSE;
	if (g_pDisconnectManager->IsDeleted(dwID))
	{
		goto lb_return;
	}

	pSession = g_pSessionManager->Find(dwID);
	if (!pSession)
		__debugbreak();

	pSendRB = &(pSession->sendBuffer);

	// 임시

	//if (packetSize == 7 && (BYTE)pPacket[2] == dfPACKET_SC_ECHO)
	//{
		//printf("ECHO SEND TO CLI %u : ",dwID);
		//for (int i = 0; i < packetSize; ++i)
		//	printf("%x ", pPacket[i] & 0xFF);
		//printf("\n");
	//}
		//__debugbreak();

	iEnqRet = pSendRB->Enqueue(pPacket, packetSize);
	if (iEnqRet == 0)
	{
		_LOG(dwLog_LEVEL_DEBUG, L"Session ID : %u\tsendRingBuffer Full\t", dwID);
		g_pDisconnectManager->RegisterId(dwID);
		goto lb_return;
	}

	//_LOG(dwLog_LEVEL_DEBUG, L"Session ID : %u, sendRB Enqueue Size : %d\tsendRB FreeSize : %d", dwID, iEnqRet, pSendRB->GetFreeSize());
	bRet = TRUE;
lb_return:
	return bRet;
}


void CreatePlayer(st_Client* pNewClient)
{
	DWORD dwPacketSize;
	st_SECTOR_AROUND sectorAround;
	st_Client* pClient;
	LINKED_NODE* pCurLink;
	DWORD dwNumOfClient;

	// 생성된 클라이언트에게 자기 자신이 생성되엇음을 알리기
	_LOG(dwLog_LEVEL_DEBUG, L"Notify New Characters of his Location");
	dwPacketSize = MAKE_SC_CREATE_MY_CHARACTER(pNewClient->dwID, pNewClient->byViewDir, pNewClient->shX, pNewClient->shY, pNewClient->chHp);
	EnqPacketUnicast(pNewClient->dwID, g_sb.GetBufferPtr(), dwPacketSize);
	g_sb.Clear();

	// 주위 9섹터의 클라이언트들에게 자기 자신이 생성되엇음을 알리기
	dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(pNewClient->dwID, pNewClient->byViewDir, pNewClient->shX, pNewClient->shY, pNewClient->chHp);
	_LOG(dwLog_LEVEL_DEBUG, L"Notify clients in the surrounding 9 sectors of the creation of a new client");
	GetSectorAround(pNewClient->shY, pNewClient->shX, &sectorAround);
	SendPacket_Around(pNewClient, &sectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	g_sb.Clear();


	// 주위 9섹터의 클라이언트들이 생성된 클라이언트에게 자신의 존재를 통지하기
	_LOG(dwLog_LEVEL_DEBUG, L"Around Client -> New Client Notify Their Position");
	for (BYTE i = 0; i < sectorAround.byCount; ++i)
	{
		pCurLink = g_Sector[sectorAround.Around[i].shY][sectorAround.Around[i].shX].pClientLinkHead;
		dwNumOfClient = g_Sector[sectorAround.Around[i].shY][sectorAround.Around[i].shX].dwNumOfClient;
		for (DWORD i = 0; i < dwNumOfClient; ++i)
		{
			pClient = LinkToClient(pCurLink);
			// 자기자신에게는 보내지 않음.
			if (pClient != pNewClient)
			{
				dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(pClient->dwID, pClient->byViewDir, pClient->shX, pClient->shY, pClient->chHp);
				EnqPacketUnicast(pNewClient->dwID, g_sb.GetBufferPtr(), dwPacketSize);
				g_sb.Clear();
				_LOG(dwLog_LEVEL_DEBUG, L"Notify Client : %d Pos -> New Client : %d", pClient->dwID, pNewClient->dwID);
			}
			pCurLink = pCurLink->pNext;
		}
	}

	pNewClient->CurSector.shY = pNewClient->shY / df_SECTOR_HEIGHT;
	pNewClient->CurSector.shX = pNewClient->shX / df_SECTOR_WIDTH;
	AddClientAtSector(pNewClient, pNewClient->CurSector.shY, pNewClient->CurSector.shX);
}

BOOL AcceptProc()
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	BOOL bRet;
	int addrlen;
	SHORT dwNewID;
	WCHAR szIpAddr[MAX_PATH];
	st_Session* pNewSession;
	st_Client* pNewClient;

	bRet = FALSE;
	addrlen = sizeof(clientAddr);
	clientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		wprintf(L"accept func error code : %d\n", WSAGetLastError());
		__debugbreak();
		goto lb_return;
	}

	if (g_pSessionManager->IsFull())
	{
		closesocket(clientSock);
		goto lb_return;
	}

	dwNewID = g_dwID++;
	InetNtop(AF_INET, &clientAddr.sin_addr, szIpAddr, _countof(szIpAddr));
	_LOG(dwLog_LEVEL_DEBUG, L"Connect # ip : %s / SessionId : %u", szIpAddr, dwNewID);

	g_pSessionManager->RegisterSession(dwNewID, clientSock,&pNewSession);
	g_pClientManager->RegisterClient(dwNewID, pNewSession, &pNewClient);
	CreatePlayer(pNewClient);

	bRet = TRUE;
lb_return:
	return bRet;
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
	else if (iSelectRet == SOCKET_ERROR)
	{
		_LOG(dwLog_LEVEL_DEBUG, L"Select Func Error Code : %d", WSAGetLastError());
		__debugbreak();
		return;
	}

	if (FD_ISSET(g_listenSock, pReadSet))
	{
		AcceptProc();
		--iSelectRet;
	}

	for (SHORT i = 0; i < FD_SETSIZE - 1 && iSelectRet > 0; ++i)
	{
		if (FD_ISSET(ppSessionArr[i]->clientSock, pReadSet))
		{
			RecvProc(ppSessionArr[i]);
			--iSelectRet;
		}

		if (FD_ISSET(ppSessionArr[i]->clientSock, pWriteSet))
		{
			SendProc(ppSessionArr[i]);
			--iSelectRet;
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
	SHORT dwSockCount;
	dwSockCount = 0;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_SET(g_listenSock, &readSet);
	++dwSockCount;

	pSession = g_pSessionManager->GetFirst();
	while (pSession)
	{
		FD_SET(pSession->clientSock, &readSet);
		if (pSession->sendBuffer.GetUseSize() > 0)
		{
			FD_SET(pSession->clientSock, &writeSet);
		}

		if (dwSockCount - 1 == 63)
			__debugbreak();

		pSessionArrForSelect[dwSockCount-1] = pSession;
		++dwSockCount;
		pSession = g_pSessionManager->GetNext(pSession);

		if (dwSockCount >= FD_SETSIZE)
		{
			SelectProc(pSessionArrForSelect, &readSet, &writeSet);//, dwSockCount);
			dwSockCount = 0;
			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);
			FD_SET(g_listenSock, &readSet);
			++dwSockCount;
		}
	}

	if (dwSockCount > 0)
	{
		SelectProc(pSessionArrForSelect, &readSet, &writeSet);//, dwSockCount);
	}
	return TRUE;
}


int g_before;
int g_after;
BOOL IsCatch = FALSE;

void SendProc(st_Session* pSession)
{
	RingBuffer* pSendRB;
	int iUseSize;
	int iDirectDeqSize;
	int iSendSize;
	int iSendRet;
	int iErrCode;


	// 삭제해야하는 id목록에 존재한다면 그냥 넘긴다
	if (g_pDisconnectManager->IsDeleted(pSession->id))
		return;

	pSendRB = &(pSession->sendBuffer);
	//if (IsCatch)
	//	__debugbreak();
	iUseSize = pSendRB->GetUseSize();
	iDirectDeqSize = pSendRB->DirectDequeueSize();
	//printf("First Calc iUseSize : %d, iDirectDeqSize : %d\n", iUseSize, iDirectDeqSize);
	while (iUseSize > 0)
	{
		// directDeqSize가 0이거나 useSize < directDeqSize라면 sendSize == useSize이어야 하기 때문이다.
		if (iUseSize < iDirectDeqSize || iDirectDeqSize == 0)
		{
			iSendSize = iUseSize;
		}
		else
		{
			iSendSize = iDirectDeqSize;
		}

		//char* temp = pSendRB->GetReadStartPtr();
		//printf("Print Send Binary ID %u : ",pSession->id);
		//for (int i = 0; i < iSendSize; ++i)
		//{
		//	printf("%x ", temp[i] & 0xFF);
		//	if (i != iSendSize - 1 && temp[i + 1] == 0x89)
		//		printf("\n");
		//}
		printf("\n");
		iSendRet = send(pSession->clientSock, pSendRB->GetReadStartPtr(), iSendSize, 0);
		if (iSendRet == SOCKET_ERROR)
		{
			g_pDisconnectManager->RegisterId(pSession->id);
			iErrCode = WSAGetLastError();
			if (iErrCode != WSAEWOULDBLOCK)
			{
				_LOG(dwLog_LEVEL_SYSTEM, L"session ID : %d, send() func error code : %d #", pSession->id, iErrCode);
				g_pDisconnectManager->RegisterId(pSession->id);
			}
			else
			{
				_LOG(dwLog_LEVEL_SYSTEM, L"session ID : %d send WSAEWOULDBLOCK #", pSession->id);
			}
			return;
		}
		//_LOG(dwLog_LEVEL_DEBUG, L"session ID : %d\tsend size : %d\tsendRB DirectDequeueSize : %d#", pSession->id, iSendSize, pSendRB->DirectDequeueSize());
		//int tempFront = pSendRB->front_;
		//int tempSendSizeBefore = iSendSize
		//printf("센드링버퍼 센드 끝나서 프론트 옮길게\n");
		pSendRB->MoveFront(iSendSize);
		//int tempSendSizeAfter = iSendSize;
		//if (pSendRB->front_ == 1000)
		//	__debugbreak();
		if (pSendRB->front_ == 993)
		{
			IsCatch = TRUE;
		}
		//printf("iSendSize ID %u : %d front : %d -> %d, DirectDequeueSize : %d -> %d \n", pSession->id, iSendSize, tempFront, pSendRB->front_, iDirectDeqSize, pSendRB->DirectDequeueSize());
		//printf("Before iUseSize : %d, iDirectDeqSize : %d\n", iUseSize, iDirectDeqSize);
		iUseSize = pSendRB->GetUseSize();
		iDirectDeqSize = pSendRB->DirectDequeueSize();
		//printf("After iUseSize : %d, iDirectDeqSize : %d\n", iUseSize, iDirectDeqSize);
	}
}

BOOL RecvProc(st_Session* pSession)
{
	int iRecvRet;
	int iPeekRet;
	int iDeqRet;
	int iRecvSize;
	Header header;
	BOOL bRet;
	RingBuffer* pRecvRB;
	int iTempRear;
	int iTempFront;

	bRet = FALSE;
	if (g_pDisconnectManager->IsDeleted(pSession->id))
		goto lb_return;

	pRecvRB = &(pSession->recvBuffer);
	iRecvSize = pRecvRB->DirectEnqueueSize();
	if (iRecvSize == 0)
		iRecvSize = pRecvRB->GetFreeSize();

	iRecvRet = recv(pSession->clientSock, pRecvRB->GetWriteStartPtr(), iRecvSize, 0);
	if (iRecvRet == 0)
	{
		//_LOG(dwLog_LEVEL_DEBUG, L"Session ID : %d, TCP CONNECTION END", pSession->id);
		g_pDisconnectManager->RegisterId(pSession->id);
		goto lb_return;
	}
	else if (iRecvRet == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSAEWOULDBLOCK)
		{
			_LOG(dwLog_LEVEL_SYSTEM, L"Session ID : %d, recv() error code : %d\n", pSession->id, errCode);
			g_pDisconnectManager->RegisterId(pSession->id);
		}
		goto lb_return;
	}
	//_LOG(dwLog_LEVEL_DEBUG, L"Session ID : %d, recv() -> recvRB size : %d", pSession->id, iRecvRet);
	printf("리시브 끝나서 리시브 링버퍼 리어 옮길게\n");
	iTempRear = pRecvRB->rear_;
	pRecvRB->MoveRear(iRecvRet);
	printf("rear %d -> %d\n", iTempRear, pRecvRB->rear_);

	while (true)
	{
		printf("리시브 링버퍼 헤더에서 디큐할게\n");
		iTempFront = pRecvRB->front_;
		iDeqRet = pRecvRB->Dequeue((char*)&header, sizeof(header));
		printf("front %d -> %d\n", iTempFront, pRecvRB->front_);
		if (iDeqRet == 0)
			goto lb_return;
		char* temp = (char*)&header;
		printf("Print Recv Header Binary ID %u : ", pSession->id);
		for (int i = 0; i < iDeqRet; ++i)
		{
			printf("%x ", temp[i] & 0xFF);
		}
		printf("\n");

		if (header.byCode != 0x89)
		{
			__debugbreak();
			_LOG(dwLog_LEVEL_SYSTEM, L"Session ID : %u\tDisconnected by INVALID HeaderCode Packet Received", pSession->id);
			g_pDisconnectManager->RegisterId(pSession->id);
			goto lb_return;
		}
		// 직렬화버퍼이므로 매번 메시지를 처리하기 직전에는 rear_ == front_ == 0이라는것이 전제
		if (g_sb.bufferSize_ < header.bySize)
		{
			__debugbreak();
			g_sb.Resize();
		}

		printf("리시브 링버퍼 메시지 직렬화 버퍼로 디큐할게\n");
		iTempFront = pRecvRB->front_;
		iDeqRet = pRecvRB->Dequeue(g_sb.GetBufferPtr(), header.bySize);
		printf("front %d -> %d\n", iTempFront, pRecvRB->front_);
		if (iDeqRet == 0)
			goto lb_return;

		temp = g_sb.GetBufferPtr();
		printf("Print Recv Packet Binary ID %u : \n", pSession->id);
		for (int i = 0; i < iDeqRet; ++i)
		{
			printf("%x ", temp[i] & 0xFF);
		}
		printf("\n");

		g_sb.MoveWritePos(iDeqRet);
		g_pCSProc->PacketProc(pSession->id, header.byType);
	}

	bRet = TRUE;
lb_return:
	return bRet;
}

