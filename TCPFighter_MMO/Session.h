#pragma once
#include "RingBuffer.h"
#include "LinkedList.h"
#include "HashTable.h"

class RingBuffer;
struct STATIC_HASH_METADATA;
struct st_Session
{
	unsigned id;
	SOCKET clientSock;
	RingBuffer recvBuffer;
	RingBuffer sendBuffer;
	DWORD dwLastRecvTime;
	STATIC_HASH_METADATA shm;
	st_Session(SOCKET sock, unsigned ID)
		:clientSock{ sock }, id{ ID }
	{}
};

