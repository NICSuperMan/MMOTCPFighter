#pragma once
// ���߿� �ڷᱸ�� �ٲ���������� ������


struct Header
{
	unsigned char byCode;			// ��Ŷ�ڵ� 0x89 ����.
	unsigned char bySize;			// ��Ŷ ������.
	unsigned char byType;			// ��ŶŸ��.
};

BOOL NetworkInitAndListen();
BOOL AcceptProc();
//void SendProc(Session* pSession);
BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize);
BOOL NetworkProc();


