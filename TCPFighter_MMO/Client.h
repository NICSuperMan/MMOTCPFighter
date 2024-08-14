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
#ifdef SYNC
	BOOL IsAlreadyStart = FALSE;
	BOOL IsFirstUpdate = FALSE;
	DWORD Start1ArrivedTime;
	DWORD Start2ArrivedTime;
	DWORD StopArrivedTime;
	DWORD Start1MashalingTime;
	DWORD Start2MashalingTime;
	DWORD StopMashallingTime;
	ULONGLONG Start1ArrivedFPS;
	ULONGLONG Start2ArrivedFPS;
	ULONGLONG StopArrivedFPS;
	ULONGLONG Start1MashalingFPS;
	ULONGLONG Start2MashalingFPS;
	ULONGLONG StopMashalingFPS;
	ULONGLONG FirstUpdateTime;
	ULONGLONG FirstUpdateFPS;
	std::wstring DebugSync = L"";
	Pos PrevPos;
	BYTE RBArr[30];
	BYTE SerializeArr[30];
#endif
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
