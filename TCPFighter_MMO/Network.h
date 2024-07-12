#pragma once
#include <new>
#include "MemoryPool.h"
#include "RingBuffer.h"
// 나중에 자료구조 바뀔수도잇으니 래핑함
struct Session
{
	unsigned id;
	SOCKET clientSock;
	RingBuffer recvBuffer;
	RingBuffer sendBuffer;
	DWORD dwLastRecvTime;
	Session(SOCKET sock, unsigned ID)
		:clientSock{ sock }, id{ ID }
	{}
};


struct Header
{
	unsigned char byCode;			// 패킷코드 0x89 고정.
	unsigned char bySize;			// 패킷 사이즈.
	unsigned char byType;			// 패킷타입.
};
#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7
#define dfPACKET_MOVE_DIR_NOMOVE				8





bool NetworkInitAndListen();
bool AcceptProc();
void SendProc(Session* pSession);
bool EnqPacketUnicast(const int id, char* pPacket, const size_t packetSize);

