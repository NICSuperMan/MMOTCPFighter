#pragma once
#include "Position.h"
#include "LinkedList.h"

#include "MiddleWare.h"

#include "string"

//#define SYNC

struct st_Client
{
	DWORD dwID;
	NETWORK_HANDLE handle;
	Pos pos;
	BYTE byViewDir;
	BYTE byMoveDir;
	CHAR chHp;
	SectorPos CurSector;
	SectorPos OldSector;
	LINKED_NODE SectorLink;
};

#pragma optimize("",on)
__forceinline st_Client* LinkToClient(LINKED_NODE* pLink)
{
	st_Client* pRet;
	pRet = (st_Client*)((char*)pLink - offsetof(st_Client, SectorLink));
	return pRet;
}
#pragma optimize("",off)
