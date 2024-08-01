#pragma once
#include "LinkedList.h"
#include "Constant.h"
struct st_Client;

struct st_POS
{
	SHORT shY;
	SHORT shX;
};

struct st_SECTOR_POS
{
	SHORT shY;
	SHORT shX;
};

struct st_SECTOR_AROUND
{
	st_SECTOR_POS Around[9];
	BYTE byCount; // 0 ~ 9
};

void GetSectorAround(SHORT dwPosY, SHORT dwPosX, st_SECTOR_AROUND* pOutSectorAround);
void SendPacket_SectorOne(const st_SECTOR_POS* pSectorPos, char* pPacket, DWORD dwPacketSize, const st_Client* pExceptClient);
void GetUpdateSectorAround(st_Client* pClient, st_SECTOR_AROUND* pRemoveSector, st_SECTOR_AROUND* pOutAddSector);
void SendPacket_Around(const st_Client* pClient, const st_SECTOR_AROUND* pSectorAround, char* pPacket, DWORD dwPacketSize, BOOL bSendMe);
void AddClientAtSector(st_Client* pClient, SHORT shNewSectorY, SHORT shNewSectorX);
void RemoveClientAtSector(st_Client* pClient, SHORT shOldSectorY, SHORT shOldSectorX);



struct st_SECTOR_CLIENT_INFO
{
	LINKED_NODE* pClientLinkHead = nullptr;
	LINKED_NODE* pClientLinkTail = nullptr;
	DWORD dwNumOfClient = 0;
};

extern st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];

__forceinline BOOL IsValidSector(int iSectorY, int iSectorX)
{
	BOOL bValidVertical;
	BOOL bValidHorizon;
	bValidVertical = (iSectorY >= 0) && (iSectorY < dwNumOfSectorVertical); // Y축 정상
	bValidHorizon = (iSectorX >= 0) && (iSectorX < dwNumOfSectorHorizon); // X축 정상
	return bValidVertical && bValidHorizon; // 둘다 정상이면 TRUE
}

static __forceinline BOOL IsSameSector(SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX)
{
	BOOL bRet;
	BOOL bSameY;
	BOOL bSameX;
	bSameY = shOldSectorY == shNewSectorY;
	bSameX = shOldSectorX == shNewSectorX;
	bRet = bSameY && bSameX;
	return bRet;
}

static __forceinline BOOL IsValidPos(SHORT shX, SHORT shY)
{
	BOOL bRet;
	BOOL bValidY;
	BOOL bValidX;
	bValidY = (dfRANGE_MOVE_TOP >= shX && shX <= dfRANGE_MOVE_BOTTOM);
	bValidX = (dfRANGE_MOVE_LEFT >= shY && shY <= dfRANGE_MOVE_RIGHT);
	bRet = bValidY && bValidX;
	return bRet;
}

