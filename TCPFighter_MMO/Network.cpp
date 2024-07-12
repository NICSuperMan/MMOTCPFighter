#include <WS2spi.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
//#include <windows.h>
#include <stdio.h>
#include "Network.h"
#include "SerializeBuffer.h"
#include "MemoryPool.h"
#include "disPool.h"
#include "SessionPool.h"

#define LOG
#pragma comment(lib,"ws2_32.lib")
SOCKET g_listenSock;

constexpr int SERVERPORT = 5000;
constexpr int MAX_USER_NUM = 60;
SerializeBuffer g_sb;
unsigned g_id = 0;
unsigned g_userNum = 0;
SessionPool g_SessionPool;
DisIdPool g_DisIdPool;

bool NetworkInitAndListen()
{
	int bindRet;
	int listenRet;
	int ioctlRet;
	int ssoRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		wprintf(L"WSAStartup() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"WSAStartup #\n");
#endif
	g_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_listenSock == INVALID_SOCKET)
	{
		wprintf(L"socket() func error code : %d\n", WSAGetLastError());
		return false;
	}

	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(g_listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		wprintf(L"bind() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Bind OK # Port : %d\n", SERVERPORT);
#endif

	listenRet = listen(g_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wprintf(L"listen() func error code : %d\n", WSAGetLastError());
		return false;
	}
#ifdef LOG
	wprintf(L"Listen OK #\n");
#endif
	LINGER linger{ 1,0 };
	ssoRet = setsockopt(g_listenSock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
	if (ssoRet != 0)
	{
		wprintf(L"setsockopt() func error code : %d\n", WSAGetLastError());
		return false;
	}

#ifdef LOG
	wprintf(L"Linger OK #\n");
#endif

	u_long iMode = 1;
	ioctlRet = ioctlsocket(g_listenSock, FIONBIO, &iMode);
	if (ioctlRet != 0)
	{
		wprintf(L"ioctlsocket() func error code : %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool AcceptProc()
{
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	int addrlen = sizeof(clientAddr);
	clientSock = accept(g_listenSock, (SOCKADDR*)&clientAddr, &addrlen);
	if (clientSock == INVALID_SOCKET)
	{
		wprintf(L"accept func error code : %d\n", WSAGetLastError());
		__debugbreak();
		return false;
	}

	if (g_SessionPool.GetSize() >= SessionPool::MAX_SESSION)
	{
		closesocket(clientSock);
		return false;
	}

	int newId = g_id++;
#ifdef LOG
	WCHAR szIpAddr[MAX_PATH];
	InetNtop(AF_INET, &clientAddr.sin_addr, szIpAddr, _countof(szIpAddr));
	wprintf(L"Connect # ip : %s / SessionId:%d\n", szIpAddr, newId);
#endif 
	g_SessionPool.RegisterSession(newId, clientSock);
	return true;
}

bool NetworkProc()
{
	static bool isFirst = true;
	int selectRet;
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(g_listenSock, &readSet);
	timeval tv{ 0,0 };
	selectRet = select(0, &readSet, nullptr, nullptr, &tv);

	if (selectRet > 0)
	{
		//
	}
	else if (selectRet == SOCKET_ERROR)
	{
		//
	}

	fd_set writeSet;
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	Session* pSession = g_SessionPool.GetFirst();

	while (pSession)
	{
		Session* pTemp = pSession;
		for (int i = 0; i < 64 && pSession != nullptr; ++i)
		{
			FD_SET(pSession->clientSock, &readSet);
			if (pSession->sendBuffer.GetUseSize() > 0)
			{
				FD_SET(pSession->clientSock, &writeSet);
			}
			pSession = g_SessionPool.GetNext(pSession);
		}

		selectRet = select(0, &readSet, &writeSet, nullptr, &tv);
		while (selectRet > 0)
		{
			if (FD_ISSET(pTemp->clientSock, &readSet))
			{
				// RECVPROC

				--selectRet;
			}

			if (FD_ISSET(pTemp->clientSock, &writeSet))
			{
				SendProc(pTemp);
				--selectRet;
			}

			pTemp = g_SessionPool.GetNext(pTemp);
		}
	}
	return true;
}


// 프레임의 끝부분에서 모든 클라이언트에게 링버퍼 send
void SendProc(Session* pSession)
{
	// 삭제해야하는 id목록에 존재한다면 그냥 넘긴다
	if (g_DisIdPool.isDeletedSession(pSession->id))
	{
		return;
	}
	RingBuffer& sendRB = pSession->sendBuffer;
	int useSize = sendRB.GetUseSize();
	int directDeqSize = sendRB.DirectDequeueSize();
	while (useSize > 0)
	{
		// directDeqSize가 0이거나 useSize < directDeqSize라면 sendSize == useSize이어야 하기 때문이다.
		int sendSize;
		if (useSize < directDeqSize || directDeqSize == 0)
		{
			sendSize = useSize;
		}
		else
		{
			sendSize = directDeqSize;
		}
		int sendRet = send(pSession->clientSock, sendRB.GetReadStartPtr(), sendSize, 0);
		if (sendRet == SOCKET_ERROR)
		{
			int errCode = WSAGetLastError();
			if (errCode != WSAEWOULDBLOCK)
			{
				wprintf(L"session ID : %d, send() func error code : %d #\n", pSession->id, errCode);
				g_DisIdPool.disconnect(pSession->id);
			}
#ifdef LOG
			else
			{
				wprintf(L"session ID : %d send WSAEWOULDBLOCK #\n", pSession->id);
			}
#endif
			return;
		}
#ifdef LOG
		wprintf(L"session ID : %d\tsend size : %d\tsendRB DirectDequeueSize : %d#\n", pSession->id, sendSize, sendRB.DirectDequeueSize());
#endif
		sendRB.MoveFront(sendSize);
		useSize = sendRB.GetUseSize();
		directDeqSize = sendRB.DirectDequeueSize();
	}
}

bool EnqPacketUnicast( const int id, char* pPacket, const size_t packetSize)
{
	if (g_DisIdPool.isDeletedSession(id))
		return false;

	Session* pSession = g_SessionPool.Find(id);
	if (!pSession)
	{
		__debugbreak();
	}
	RingBuffer& sendRB = pSession->sendBuffer;
	int EnqRet = sendRB.Enqueue(pPacket, packetSize);
	if (EnqRet == 0)
	{
#ifdef LOG
		wprintf(L"Session ID : %d\tsendRingBuffer Full\t", id);
#endif
		g_DisIdPool.disconnect(id);
		return false;
	}
#ifdef LOG
	wprintf(L"Session ID : %d, sendRB Enqueue Size : %d\tsendRB FreeSize : %d\n", id, EnqRet, sendRB.GetFreeSize());
#endif
	return true;
}

