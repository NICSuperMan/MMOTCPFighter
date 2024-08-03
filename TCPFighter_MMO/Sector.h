#pragma once
#include "LinkedList.h"
#include "Constant.h"
#include "Sector.h"
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

struct st_DirVector
{
	SHORT shY;
	SHORT shX;
};


struct st_MoveInfo
{
	SHORT shOldY;
	SHORT shOldX;
	SHORT shNewY;
	SHORT shNewX;
};


constexpr st_DirVector vArr[8] = {
	st_DirVector{0,-1},//LL
	st_DirVector{-1,-1},//LU
	st_DirVector{-1,0},//UU
	st_DirVector{-1,1}, //RU
	st_DirVector{0,1}, //RR
	st_DirVector{1,1}, //RD
	st_DirVector{1,0}, // DD
	st_DirVector{1,-1} //LD
};

// 이동후 OldSector에서 현재 캐릭터의 이동방향을 해당 배열의 인덱스로 대입해서 얻은 방향을 GetDeltaSector에 대입
constexpr BYTE removeDirArr[8] =
{
	dfPACKET_MOVE_DIR_RU, //LL
	dfPACKET_MOVE_DIR_RU, //LU
	dfPACKET_MOVE_DIR_RD, //UU
	dfPACKET_MOVE_DIR_RD, //RU
	dfPACKET_MOVE_DIR_LD, //RR
	dfPACKET_MOVE_DIR_LD, //RD
	dfPACKET_MOVE_DIR_LU, //DD
	dfPACKET_MOVE_DIR_LU, //LD
};

// 이동후 NewSecotr에서 현재 캐릭터의 이동방향을 해당 배열의 인덱스로 대입해서 얻은 방향을 GetDeltaSctor에 대입
constexpr BYTE AddDirArr[8] =
{
	dfPACKET_MOVE_DIR_LD, //LL
	dfPACKET_MOVE_DIR_LD, //LU
	dfPACKET_MOVE_DIR_LU, //UU
	dfPACKET_MOVE_DIR_LU, //RU,
	dfPACKET_MOVE_DIR_RU, //RR
	dfPACKET_MOVE_DIR_RU, //RD
	dfPACKET_MOVE_DIR_RD, //DD
	dfPACKET_MOVE_DIR_RD, //LD
};

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

static __forceinline BOOL IsValidPos(SHORT shY, SHORT shX)
{
	BOOL bRet;
	BOOL bValidY;
	BOOL bValidX;
	bValidY = (dfRANGE_MOVE_TOP <= shY && shY <= dfRANGE_MOVE_BOTTOM);
	bValidX = (dfRANGE_MOVE_LEFT <= shX && shX <= dfRANGE_MOVE_RIGHT);
	bRet = bValidY && bValidX;
	return bRet;
}

// 제거된 섹터를 얻을때는 BaseSector에 OldSector대입
// 새로운 섹터를 얻을때는 BaseSector에 NewSector대입
__forceinline void GetDeltaSector(BYTE byBaseDir, st_SECTOR_AROUND* pSectorAround, BYTE byDeltaSectorNum, SHORT shBaseSectorPointY, SHORT shBaseSectorPointX)
{
	SHORT shGetSectorY;
	SHORT shGetSectorX;

	pSectorAround->byCount = 0;
	for (int i = 0; i < byDeltaSectorNum; ++i)
	{
		shGetSectorY = shBaseSectorPointY + vArr[byBaseDir].shY;
		shGetSectorX = shBaseSectorPointX + vArr[byBaseDir].shX;
		byBaseDir = (++byBaseDir) % 8;
		if (IsValidSector(shGetSectorY, shGetSectorX))
		{
			pSectorAround->Around[pSectorAround->byCount].shY = shGetSectorY;
			pSectorAround->Around[pSectorAround->byCount].shX = shGetSectorX;
			++(pSectorAround->byCount);
		}
	}
}

__forceinline BYTE GetReverseDir(BYTE byOriginalDir)
{
	switch (byOriginalDir)
	{
	case dfPACKET_MOVE_DIR_LL:
		return dfPACKET_MOVE_DIR_RR;
	case dfPACKET_MOVE_DIR_LU:
		return dfPACKET_MOVE_DIR_RD;
	case dfPACKET_MOVE_DIR_UU:
		return dfPACKET_MOVE_DIR_DD;
	case dfPACKET_MOVE_DIR_RU:
		return dfPACKET_MOVE_DIR_LD;
	case dfPACKET_MOVE_DIR_RR:
		return dfPACKET_MOVE_DIR_LL;
	case dfPACKET_MOVE_DIR_RD:
		return dfPACKET_MOVE_DIR_LU;
	case dfPACKET_MOVE_DIR_DD:
		return dfPACKET_MOVE_DIR_UU;
	case dfPACKET_MOVE_DIR_LD:
		return dfPACKET_MOVE_DIR_RU;
	default:
		__debugbreak();
	}
}

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


