#include "Sector.h"
#include "Logger.h"
#include <emmintrin.h>


st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];
AroundInfo g_AroundInfo;


void GetSectorAround(SectorPos CurSector, st_SECTOR_AROUND* pOutSectorAround)
{
	SHORT* pCount;
	pCount = (SHORT*)((char*)pOutSectorAround + offsetof(st_SECTOR_AROUND, byCount));
	*pCount = 0;

	// posX, posY에 pos의 좌표값 담기
	__m128i posY = _mm_set1_epi16(CurSector.shY);
	__m128i posX = _mm_set1_epi16(CurSector.shX);

	// 8방향 방향벡터 X성분 Y성분 준비
	__m128i DirVY = _mm_set_epi16(+1, +1, +1, +0, -1, -1, -1, +0);
	__m128i DirVX = _mm_set_epi16(-1, +0, +1, +1, +1, +0, -1, -1);

	// 더한다
	posX = _mm_add_epi16(posX, DirVX);
	posY = _mm_add_epi16(posY, DirVY);

	__m128i min = _mm_set1_epi16(-1);
	__m128i max = _mm_set1_epi16(df_SECTOR_HEIGHT);

	__m128i cmpMin = _mm_cmpgt_epi16(posX, min);
	__m128i cmpMax = _mm_cmplt_epi16(posX, max);
	__m128i result = _mm_and_si128(cmpMin, cmpMax);
	SHORT maskX = _mm_movemask_epi8(result);

	SHORT X[8];
	_mm_storeu_si128((__m128i*)X, posX);

	SHORT Y[8];
	cmpMin = _mm_cmpgt_epi16(posY, min);
	cmpMax = _mm_cmplt_epi16(posY, max);
	result = _mm_and_si128(cmpMin, cmpMax);
	SHORT maskY = _mm_movemask_epi8(result);
	_mm_storeu_si128((__m128i*)Y, posY);

	SHORT temp = 2;
	SHORT ret = maskX & maskY;
	pOutSectorAround->Around[0].shY = CurSector.shY;
	pOutSectorAround->Around[0].shX = CurSector.shX;
	++(pOutSectorAround->byCount);

	for (int i = 0; i < 8; ++i)
	{
		if (ret & (temp << i))
		{
			pOutSectorAround->Around[i+1].shY = Y[i+1];
			pOutSectorAround->Around[i+1].shX = X[i+1];
			++(pOutSectorAround->byCount);
		}
	}

}

AroundInfo* GetAroundValidClient(SectorPos sp, st_Client* pExcept)
{
	SectorPos temp;
	int iNum = 0;
	for (int i = 0; i < 9; ++i)
	{
		temp.shY = sp.shY + spArrDir[i].shY;
		temp.shX = sp.shX + spArrDir[i].shX;
		if (IsValidSector(temp))
			GetValidClientFromSector(temp, &g_AroundInfo, &iNum, pExcept);
	}
	g_AroundInfo.CI.dwNum = iNum;
	return &g_AroundInfo;
}

AroundInfo* GetDeltaValidClient(BYTE byBaseDir, BYTE byDeltaSectorNum, SectorPos SectorBasePos)
{
	SectorPos GetSector;
	int iNum = 0;
	for (int i = 0; i < byDeltaSectorNum; ++i)
	{
		GetSector.shY = SectorBasePos.shY + vArr[byBaseDir].shY;
		GetSector.shX = SectorBasePos.shX + vArr[byBaseDir].shX;
		byBaseDir = (++byBaseDir) % 8;
		if (IsValidSector(GetSector))
			GetValidClientFromSector(GetSector, &g_AroundInfo, &iNum, nullptr);
	}
	g_AroundInfo.CI.dwNum = iNum;
	return &g_AroundInfo;
}


void AddClientAtSector(st_Client* pClient,SectorPos newSectorPos)
{
	LinkToLinkedListLast(&(g_Sector[newSectorPos.shY][newSectorPos.shX].pClientLinkHead), &(g_Sector[newSectorPos.shY][newSectorPos.shX].pClientLinkTail), &(pClient->SectorLink));
	++(g_Sector[newSectorPos.shY][newSectorPos.shX].dwNumOfClient);
	//LOG(L"SECTOR",ERR,CONSOLE, L"NewClient ID : %u \nIn Sector X : %d, Y : %d, ClientNum In Sector : %u",pClient->dwID,newSectorPos.shX, newSectorPos.shY, g_Sector[newSectorPos.shY][newSectorPos.shX].dwNumOfClient);
}

void RemoveClientAtSector(st_Client* pClient, SectorPos oldSectorPos)
{
	UnLinkFromLinkedList(&(g_Sector[oldSectorPos.shY][oldSectorPos.shX].pClientLinkHead), &(g_Sector[oldSectorPos.shY][oldSectorPos.shX].pClientLinkTail), &(pClient->SectorLink));
	--(g_Sector[oldSectorPos.shY][oldSectorPos.shX].dwNumOfClient);
	//LOG(L"SECTOR", ERR, CONSOLE, L"Client ID : %u \nremoved from Sector X : %d, Y : %d, ClientNum In Sector : %u", pClient->dwID, oldSectorPos.shX, oldSectorPos.shY, g_Sector[oldSectorPos.shY][oldSectorPos.shX].dwNumOfClient);
}
