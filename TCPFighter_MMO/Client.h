#pragma once
#include "Sector.h"
#include "Session.h"
struct st_Client
{
	st_Session* pSession;
	DWORD dwID;
	DWORD dwAction;
	SHORT shX;
	SHORT shY;
	BYTE byViewDir;
	BYTE byMoveDir;
	CHAR chHp;
	st_SECTOR_POS CurSector;
	st_SECTOR_POS OldSector;
	STATIC_HASH_METADATA shm;
	LINKED_NODE SectorLink;
};

__forceinline st_Client* LinkToClient(LINKED_NODE* pLink)
{
	st_Client* pRet;
	pRet = (st_Client*)((char*)pLink - offsetof(st_Client, SectorLink));
	return pRet;
}
