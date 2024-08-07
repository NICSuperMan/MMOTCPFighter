#include <windows.h>
#include "Update.h"
#include "ClientManager.h"
#include "DisconnectManager.h"
#include "SessionManager.h"
#include "Sector.h"
#include "SCContents.h"
#include "SerializeBuffer.h"
#include "Constant.h"
#include "Logger.h"
#include "sector.h"
#include "Network.h"

extern SerializeBuffer g_sb;

extern st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];

extern int g_iDisConByTimeOut;


void GetDirStr(BYTE byMoveDir, WCHAR* pDir, int len);

// X64 Calling Convetion 에 따라서 RTL 4개까지 레지스터에 넣어줘서 굳이 씀
void SectorUpdateAndNotify(st_Client* pClient, BYTE byMoveDir, SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX, st_SECTOR_AROUND* pOutNewAround)
{
	BYTE byDeltaSectorNum;
	st_SECTOR_AROUND removeSectorAround;
	DWORD dwPacketSize;
	DWORD dwID;
	SHORT shY;
	SHORT shX;

	if (!pOutNewAround)
		__debugbreak();

	dwID = pClient->dwID;
	shY = pClient->shY;
	shX = pClient->shX;

	// 섹터 리스트에 갱신
	RemoveClientAtSector(pClient, shOldSectorY, shOldSectorX);
	AddClientAtSector(pClient, shNewSectorY, shNewSectorX);

	// 영향권에서 없어진 섹터와 영향권에 들어오는 섹터를 구함
	byDeltaSectorNum = (byMoveDir % 2 == 0) ? 3 : 5;
	GetDeltaSector(removeDirArr[byMoveDir], &removeSectorAround, byDeltaSectorNum, shOldSectorY, shOldSectorX);
	
	st_SECTOR_CLIENT_INFO* pSCI;
	LINKED_NODE* pCurLink;

	// 나의 시야에서 없어진 섹터에 내가 없어졋다고 알림
	if (removeSectorAround.byCount)
	{
		st_Client* pRemovedClient;

		for (BYTE i = 0; i < removeSectorAround.byCount; ++i)
		{
			dwPacketSize = MAKE_SC_DELETE_CHARACTER(dwID); 
			_LOG(dwLog_LEVEL_DEBUG, L"Notify Remove Character ID : %u To SECTOR X : %d, Y : %d", dwID, removeSectorAround.Around[i].shX, removeSectorAround.Around[i].shY);
			SendPacket_SectorOne(&removeSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
			g_sb.Clear();
			pSCI = &g_Sector[removeSectorAround.Around[i].shY][removeSectorAround.Around[i].shX];

			// 움직이는 캐릭터의 시야에서 없어진 섹터에 존재하는 캐릭터들도 지워야함.
			pCurLink = pSCI->pClientLinkHead;
			for (DWORD i = 0; i < pSCI->dwNumOfClient; ++i)
			{
				pRemovedClient = LinkToClient(pCurLink);
				_LOG(dwLog_LEVEL_ERROR, L"Notify Remove Character ID : %u -> ID : %u ", pRemovedClient->dwID, dwID);
				dwPacketSize = MAKE_SC_DELETE_CHARACTER(pRemovedClient->dwID);
				EnqPacketUnicast(dwID, g_sb.GetBufferPtr(), dwPacketSize);
				g_sb.Clear();
				pCurLink = pCurLink->pNext;
			}
		}
	}

	GetDeltaSector(AddDirArr[byMoveDir], pOutNewAround, byDeltaSectorNum, shNewSectorY, shNewSectorX);
	// 나의 시야에서 생긴 섹터에 내가 생겻다고 알림
	if (pOutNewAround->byCount)
	{
		st_Client* pNewClient;
		for (BYTE i = 0; i < pOutNewAround->byCount; ++i)
		{
			dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(dwID, pClient->byViewDir, shX, shY, pClient->chHp);
			_LOG(dwLog_LEVEL_DEBUG, L"Notify Create Character ID : %u To SECTOR X : %d, Y : %d", dwID, pOutNewAround->Around[i].shX, pOutNewAround->Around[i].shY);
			SendPacket_SectorOne(&pOutNewAround->Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
			g_sb.Clear();
			pSCI = &g_Sector[pOutNewAround->Around[i].shY][pOutNewAround->Around[i].shX];
			pCurLink = pSCI->pClientLinkHead;
			for (DWORD i = 0; i < pSCI->dwNumOfClient; ++i)
			{
				pNewClient = LinkToClient(pCurLink);
				_LOG(dwLog_LEVEL_ERROR, L"Notify New Already Character ID : %u -> ID : %u ", pNewClient->dwID, dwID);
				dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(pNewClient->dwID, pNewClient->byViewDir, pNewClient->shX, pNewClient->shY, pNewClient->chHp);
				EnqPacketUnicast(pClient->dwID, g_sb.GetBufferPtr(), dwPacketSize);
				g_sb.Clear();
				if (pNewClient->dwAction == MOVE)
				{
					dwPacketSize = MAKE_SC_MOVE_START(pNewClient->dwID, pNewClient->byMoveDir, pNewClient->shX, pNewClient->shY);
					EnqPacketUnicast(pNewClient->dwID, g_sb.GetBufferPtr(), dwPacketSize);
					g_sb.Clear();
				}
				pCurLink = pCurLink->pNext;
			}
		}
	}

	pClient->OldSector.shY = shOldSectorY;
	pClient->OldSector.shX = shOldSectorX;
	pClient->CurSector.shY = shNewSectorY;
	pClient->CurSector.shX = shNewSectorX;

}

BYTE GetSectorMoveDir(SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX);

void Update()
{
	SHORT shOldY;
	SHORT shOldX;
	SHORT shNewY;
	SHORT shNewX;
	SHORT shOldSectorY;
	SHORT shOldSectorX;
	SHORT shNewSectorY;
	SHORT shNewSectorX;
	BYTE bySectorMoveDir;
	DWORD dwTime;


	st_DirVector dirVector;
	st_Client* pClient;
	st_SECTOR_AROUND newSectorAround;
	DWORD dwPacketSize;

	pClient = g_pClientManager->GetFirst();
	while (pClient)
	{
		// 접속이 끊길 예정인 사람 확인
		if (g_pDisconnectManager->IsDeleted(pClient->dwID))
			goto lb_next;

		// 타임아웃 처리
		dwTime = timeGetTime();
		if (dwTime - pClient->dwLastRecvTime > dwTimeOut)
		{
			++g_iDisConByTimeOut;
			g_pDisconnectManager->RegisterId(pClient->dwID);
			goto lb_next;
		}

		// 죽을때 된사람 죽이기
		if (pClient->chHp <= 0)
		{
			g_pDisconnectManager->RegisterId(pClient->dwID);
			//_LOG(dwLog_LEVEL_DEBUG,L"Delete Client : %u By Dead", pClient->dwID);
			goto lb_next;
		}

		// 안 움직이는 사람 건너뛰기
		if (pClient->dwAction != MOVE)
			goto lb_next;

		// 이동후 위치계산
		dirVector = vArr[pClient->byMoveDir];
		shOldY = pClient->shY;
		shOldX = pClient->shX;
		shNewY = shOldY + dirVector.shY * dfSPEED_PLAYER_Y;
		shNewX = shOldX + dirVector.shX * dfSPEED_PLAYER_X;


		// 새로 이동한 위치검증
		if (!IsValidPos(shNewY, shNewX))
			goto lb_next;

		//_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", pClient->dwID, shOldX, shOldY, shNewX, shNewY);

		// 이동 후 섹터와 이전 섹터 계산
		shOldSectorY = shOldY / df_SECTOR_HEIGHT;
		shOldSectorX = shOldX / df_SECTOR_WIDTH;
		shNewSectorY = shNewY / df_SECTOR_HEIGHT;
		shNewSectorX = shNewX / df_SECTOR_WIDTH;

		// 섹터가 이동한 경우
		if (!IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
		{
			bySectorMoveDir = GetSectorMoveDir(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);
			SectorUpdateAndNotify(pClient, bySectorMoveDir, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX, &newSectorAround);
			if (newSectorAround.byCount > 0)
			{
				dwPacketSize = MAKE_SC_MOVE_START(pClient->dwID, pClient->byMoveDir, shNewX, shNewY);
				for (int i = 0; i < newSectorAround.byCount; ++i)
				{
					//_LOG(dwLog_LEVEL_DEBUG, L"Notify MOVE START Character ID : %u To SECTOR X : %d, Y : %d", pClient->dwID, newSectorAround.Around[i].shX, newSectorAround.Around[i].shY);
					SendPacket_SectorOne(&newSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
				}
				g_sb.Clear();
			}
		}

		pClient->shY = shNewY;
		pClient->shX = shNewX;
	lb_next:
		pClient = g_pClientManager->GetNext(pClient);
	}

	// 끊어야 할 사람 전부 끊기
	//pClosedId = g_pDisconnectManager->GetFirst();
	//while (pClosedId)
	//{
	//	pClient = g_pClientManager->Find(*pClosedId);
	//	GetSectorAround(pClient->shY, pClient->shX, &removeSectorAround);
	//	RemoveClientAtSector(pClient, pClient->CurSector.shY, pClient->CurSector.shX);

	//	// 삭제될 캐릭터 주위 섹터에 캐릭터 삭제 메시지 뿌리기
	//	dwPacketSize = MAKE_SC_DELETE_CHARACTER(*pClosedId);
	//	SendPacket_Around(pClient, &removeSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	//	g_sb.Clear();
	//	_LOG(dwLog_LEVEL_SYSTEM, L"Client ID : %u Disconnected", *pClosedId);

	//	g_pClientManager->removeClient(pClient);
	//	g_pSessionManager->removeSession(*pClosedId);
	//	g_pDisconnectManager->removeID(pClosedId);
	//	pClosedId = g_pDisconnectManager->GetFirst();
	//}
}