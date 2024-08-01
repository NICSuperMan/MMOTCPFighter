#pragma once
// 나중에 자료구조 바뀔수도잇으니 래핑함


struct Header
{
	unsigned char byCode;			// 패킷코드 0x89 고정.
	unsigned char bySize;			// 패킷 사이즈.
	unsigned char byType;			// 패킷타입.
};

BOOL NetworkInitAndListen();
BOOL AcceptProc();
//void SendProc(Session* pSession);
BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize);
BOOL NetworkProc();


