#pragma once


struct Header
{
	unsigned char byCode;			// ��Ŷ�ڵ� 0x89 ����.
	unsigned char bySize;			// ��Ŷ ������.
	unsigned char byType;			// ��ŶŸ��.
};

BOOL NetworkInitAndListen();
BOOL AcceptProc();
BOOL NetworkProc();


