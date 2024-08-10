#pragma once


struct Header
{
	unsigned char byCode;			// 패킷코드 0x89 고정.
	unsigned char bySize;			// 패킷 사이즈.
	unsigned char byType;			// 패킷타입.
};

BOOL NetworkInitAndListen();
BOOL AcceptProc();
BOOL NetworkProc();


