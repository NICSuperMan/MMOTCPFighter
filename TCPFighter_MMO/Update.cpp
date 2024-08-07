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

// X64 Calling Convetion �� ���� RTL 4������ �������Ϳ� �־��༭ ���� ��
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

	// ���� ����Ʈ�� ����
	RemoveClientAtSector(pClient, shOldSectorY, shOldSectorX);
	AddClientAtSector(pClient, shNewSectorY, shNewSectorX);

	// ����ǿ��� ������ ���Ϳ� ����ǿ� ������ ���͸� ����
	byDeltaSectorNum = (byMoveDir % 2 == 0) ? 3 : 5;
	GetDeltaSector(removeDirArr[byMoveDir], &removeSectorAround, byDeltaSectorNum, shOldSectorY, shOldSectorX);
	
	st_SECTOR_CLIENT_INFO* pSCI;
	LINKED_NODE* pCurLink;

	// ���� �þ߿��� ������ ���Ϳ� ���� ����ٰ� �˸�
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

			// �����̴� ĳ������ �þ߿��� ������ ���Ϳ� �����ϴ� ĳ���͵鵵 ��������.
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
	// ���� �þ߿��� ���� ���Ϳ� ���� ����ٰ� �˸�
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
		// ������ ���� ������ ��� Ȯ��
		if (g_pDisconnectManager->IsDeleted(pClient->dwID))
			goto lb_next;

		// Ÿ�Ӿƿ� ó��
		dwTime = timeGetTime();
		if (dwTime - pClient->dwLastRecvTime > dwTimeOut)
		{
			++g_iDisConByTimeOut;
			g_pDisconnectManager->RegisterId(pClient->dwID);
			goto lb_next;
		}

		// ������ �Ȼ�� ���̱�
		if (pClient->chHp <= 0)
		{
			g_pDisconnectManager->RegisterId(pClient->dwID);
			//_LOG(dwLog_LEVEL_DEBUG,L"Delete Client : %u By Dead", pClient->dwID);
			goto lb_next;
		}

		// �� �����̴� ��� �ǳʶٱ�
		if (pClient->dwAction != MOVE)
			goto lb_next;

		// �̵��� ��ġ���
		dirVector = vArr[pClient->byMoveDir];
		shOldY = pClient->shY;
		shOldX = pClient->shX;
		shNewY = shOldY + dirVector.shY * dfSPEED_PLAYER_Y;
		shNewX = shOldX + dirVector.shX * dfSPEED_PLAYER_X;


		// ���� �̵��� ��ġ����
		if (!IsValidPos(shNewY, shNewX))
			goto lb_next;

		//_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", pClient->dwID, shOldX, shOldY, shNewX, shNewY);

		// �̵� �� ���Ϳ� ���� ���� ���
		shOldSectorY = shOldY / df_SECTOR_HEIGHT;
		shOldSectorX = shOldX / df_SECTOR_WIDTH;
		shNewSectorY = shNewY / df_SECTOR_HEIGHT;
		shNewSectorX = shNewX / df_SECTOR_WIDTH;

		// ���Ͱ� �̵��� ���
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

	// ����� �� ��� ���� ����
	//pClosedId = g_pDisconnectManager->GetFirst();
	//while (pClosedId)
	//{
	//	pClient = g_pClientManager->Find(*pClosedId);
	//	GetSectorAround(pClient->shY, pClient->shX, &removeSectorAround);
	//	RemoveClientAtSector(pClient, pClient->CurSector.shY, pClient->CurSector.shX);

	//	// ������ ĳ���� ���� ���Ϳ� ĳ���� ���� �޽��� �Ѹ���
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