#include <Windows.h>
#include "Constant.h"
#include "LinkedList.h"
#include "HashTable.h"
#include "RingBuffer.h"
#include "MemoryPool.h"
#include "Session.h"
#include "Client.h"
#include "Sector.h"
#include "ClientManager.h"
#include "Logger.h"
#include "Network.h"

extern ClientManager* g_pClientManager;

st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];



void GetSectorAround(SHORT dwPosY, SHORT dwPosX, st_SECTOR_AROUND* pOutSectorAround)
{
	int iSectorY;
	int iSectorX;
	int iAroundSectorY;
	int iAroundSectorX;
	SHORT* pdwCount;
	
	iSectorY = dwPosY / df_SECTOR_HEIGHT;
	iSectorX = dwPosX / df_SECTOR_WIDTH;
	pdwCount = (SHORT*)((char*)pOutSectorAround + offsetof(st_SECTOR_AROUND, byCount));
	*pdwCount = 0;

	for (int dy = -1; dy <= 1; ++dy)
	{
		for (int dx = -1; dx <= 1; ++dx)
		{
			iAroundSectorY = iSectorY + dy;
			iAroundSectorX = iSectorX + dx;
			if (IsValidSector(iAroundSectorY, iAroundSectorX))
			{
				pOutSectorAround->Around[*pdwCount].shY = iAroundSectorY;
				pOutSectorAround->Around[*pdwCount].shX = iAroundSectorX;
				++(*pdwCount);
			}
		}
	}
}


void GetUpdateSectorAround(st_Client* pClient, st_SECTOR_AROUND* pRemoveSector, st_SECTOR_AROUND* pOutAddSector)
{
	GetSectorAround(pClient->OldSector.shY, pClient->OldSector.shX, pOutAddSector);
	GetSectorAround(pClient->CurSector.shY, pClient->CurSector.shX, pOutAddSector);
}


void SendPacket_SectorOne(const st_SECTOR_POS* pSectorPos, char* pPacket, DWORD dwPacketSize, const st_Client* pExceptClient)
{
	LINKED_NODE* pCurLink;
	DWORD dwNum;
	st_Client* pClient;

	if (!IsValidSector(pSectorPos->shY, pSectorPos->shX))
		return;

	pCurLink = g_Sector[pSectorPos->shY][pSectorPos->shX].pClientLinkHead;
	dwNum = g_Sector[pSectorPos->shY][pSectorPos->shX].dwNumOfClient;

	for (DWORD i = 0; i < dwNum; ++i)
	{
		if (!pCurLink)
			break;
		
		pClient = LinkToClient(pCurLink);
		if (pClient != pExceptClient)
		{
			_LOG(dwLog_LEVEL_DEBUG, L"SendPacket To Client %d", pClient->dwID);
			EnqPacketUnicast(pClient->dwID, pPacket, dwPacketSize);
		}
		pCurLink = pClient->SectorLink.pNext;
	}
}

void SendPacket_Around(const st_Client* pClient, const st_SECTOR_AROUND* pSectorAround, char* pPacket, DWORD dwPacketSize, BOOL bSendMe)
{
	const st_Client* pExceptClient;
	if (bSendMe)
		pExceptClient = nullptr;
	else
		pExceptClient = pClient;

	for (SHORT i = 0; i < pSectorAround->byCount; ++i)
		SendPacket_SectorOne(pSectorAround->Around + i, pPacket, dwPacketSize, pExceptClient);
}

void AddClientAtSector(st_Client* pClient,SHORT shNewSectorY, SHORT shNewSectorX)
{
	LinkToLinkedListLast(&(g_Sector[shNewSectorY][shNewSectorX].pClientLinkHead), &(g_Sector[shNewSectorY][shNewSectorX].pClientLinkTail), &(pClient->SectorLink));
	++(g_Sector[shNewSectorY][shNewSectorX].dwNumOfClient);
	_LOG(dwLog_LEVEL_DEBUG, L"NewClient In Sector X : %d, Y : %d, ClientNum In Sector : %d", shNewSectorX, shNewSectorY, g_Sector[shNewSectorY][shNewSectorX].dwNumOfClient);
}

void RemoveClientAtSector(st_Client* pClient, SHORT shOldSectorY, SHORT shOldSectorX)
{
	UnLinkFromLinkedList(&(g_Sector[shOldSectorY][shOldSectorX].pClientLinkHead), &(g_Sector[shOldSectorY][shOldSectorX].pClientLinkTail), &(pClient->SectorLink));
	--(g_Sector[shOldSectorY][shOldSectorX].dwNumOfClient);
	_LOG(dwLog_LEVEL_DEBUG, L"Client removed from Sector X : %d, Y : %d, ClientNum In Sector : %d", shOldSectorX, shOldSectorY, g_Sector[shOldSectorY][shOldSectorX].dwNumOfClient);
}
