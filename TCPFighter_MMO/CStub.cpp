#include <windows.h>
#include <math.h>
#include "Constant.h"
#include "RingBuffer.h"
#include "CStub.h"
#include "LinkedList.h"
#include "HashTable.h"
#include "Session.h"
#include "MemoryPool.h"
#include "Constant.h"
#include "Sector.h"
#include "Client.h"
#include "SCContents.h"
#include "ClientManager.h"
#include "DisconnectManager.h"
#include "Logger.h"

CStub* g_pCSProc;
BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize);
void ClientSectorUpdateAndPacket(st_Client* pClient, SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX);


BOOL CSProc::CS_MOVE_START(DWORD dwFromId, BYTE byMoveDir, SHORT shX, SHORT shY)
{
	st_Client* pClient;
	st_SECTOR_AROUND sectorAround;
	DWORD dwSyncPacketSize;
	SHORT shServerY;
	SHORT shServerX;
	SHORT shOldSectorY;
	SHORT shOldSectorX;
	SHORT shNewSectorY;
	SHORT shNewSectorX;

	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	// 클라이언트 찾기
	pClient = g_pClientManager->Find(dwFromId);
	shServerY = pClient->shY;
	shServerX = pClient->shX;

	// 클라이언트 시선처리
	switch (byMoveDir)
	{
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RD:
		pClient->byViewDir = dfPACKET_MOVE_DIR_RR;
		break;
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pClient->byViewDir = dfPACKET_MOVE_DIR_LL;
		break;
	}

	// 오차가 dfERROR_RANGE 이상이면 서버가 알던 좌표로 싱크 맞추기
	if (abs(shServerX - shX) > dfERROR_RANGE || abs(shServerY - shY) > dfERROR_RANGE)
	{
		dwSyncPacketSize = MAKE_SC_SYNC(dwFromId, shServerX, shServerY);
		GetSectorAround(shServerY, shServerX, &sectorAround);
		SendPacket_Around(pClient, &sectorAround, g_sb.GetBufferPtr(), dwSyncPacketSize, TRUE);
		shY = shServerY;
		shX = shServerX;
	}

	shOldSectorY = shServerY / df_SECTOR_HEIGHT;
	shOldSectorX = shServerX / df_SECTOR_WIDTH;
	shNewSectorY = shY / df_SECTOR_HEIGHT;
	shNewSectorX = shX / df_SECTOR_WIDTH;
	
	// dfERROR_RANGE보다 오차가 작지만, 섹터가 틀려서 섹터를 클라기준으로 맞출경우 업데이트 
	if (!IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
		ClientSectorUpdateAndPacket(pClient, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);

	// 서버에서 보관하는 클라이언트의 이동상태, 좌표 , 이동방향 저장
	pClient->dwAction = MOVE;
	pClient->shX = shX;
	pClient->shY = shY;
	pClient->byMoveDir = byMoveDir;
	return TRUE;
}
BOOL CSProc::CS_MOVE_STOP(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	/*register*/st_Client* pClient;
	/*register*/SHORT shServerY;
	/*register*/SHORT shServerX;
	/*register*/ DWORD dwSyncPacketSize;
	/*register*/ SHORT shOldSectorY;
	/*register*/SHORT shOldSectorX;
	/*register*/SHORT shNewSectorY;
	/*register*/SHORT shNewSectorX;
	st_SECTOR_AROUND sectorAround;

	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	pClient = g_pClientManager->Find(dwFromId);
	shServerY = pClient->shY;
	shServerX = pClient->shX;

	// 오차가 dfERROR_RANGE 이상이면 서버가 알던 좌표로 싱크 맞추기
	if (abs(shServerX - shX) > dfERROR_RANGE || abs(shServerY - shY) > dfERROR_RANGE)
	{
		dwSyncPacketSize = MAKE_SC_SYNC(dwFromId, shServerX, shServerY);
		GetSectorAround(shServerY, shServerX, &sectorAround);
		SendPacket_Around(pClient, &sectorAround, g_sb.GetBufferPtr(), dwSyncPacketSize, TRUE);
		shY = shServerY;
		shX = shServerX;
	}

	// 섹터 구하기
	shOldSectorY = shServerY / df_SECTOR_HEIGHT;
	shOldSectorX = shServerX / df_SECTOR_WIDTH;
	shNewSectorY = shY / df_SECTOR_HEIGHT;
	shNewSectorX = shX / df_SECTOR_WIDTH;

	// dfERROR_RANGE보다 오차가 작지만, 섹터가 틀려서 섹터를 클라기준으로 맞출경우 업데이트 
	if (!IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
		ClientSectorUpdateAndPacket(pClient, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);

	pClient->dwAction = NOMOVE;
	pClient->shX = shX;
	pClient->shY = shY;
	return TRUE;
}

#define IsRRColide(shAttackerY,shAttackerX,shVictimY,shVictimX,n) ((((shAttackerY - (dfATTACK##n##_RANGE_Y/2)) < shVictimY) && ((shAttackerY + (dfATTACK##n##_RANGE_Y/2)) > shVictimY)) && ((shAttackerX < shVictimX) && ((shAttackerX + dfATTACK##n##_RANGE_X) > shVictimX)))
#define IsLLColide(shAttackerY,shAttackerX,shVictimY,shVictimX,n) ((((shAttackerY - (dfATTACK##n##_RANGE_Y/2)) < shVictimY) && (((shAttackerY + (dfATTACK##n##_RANGE_Y/2)) > shVictimY)) && (shAttackerX > shVictimX) && ((shAttackerX - dfATTACK##n##_RANGE_X) < shVictimX)))

BOOL CSProc::CS_ATTACK1(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	st_Client* pAttacker;
	st_SECTOR_AROUND attackerSectorAround;
	st_SECTOR_AROUND victimSectorAround;
	DWORD dwPacketSize;
	st_SECTOR_CLIENT_INFO* pSCI;
	st_Client* pVictim;
	LINKED_NODE* pCurLink;

	// 끊길 유저 걸러내기
	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	// 공격자 클라이언트 찾고, 근처 섹터찾아서 ATTACK 패킷 보내기
	pAttacker = g_pClientManager->Find(dwFromId);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 1);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);

	// 공격자 주위 섹터에 있는 클라이언트로부터 피격판정 시작
	for (DWORD i = 0; i < attackerSectorAround.byCount; ++i)
	{
		pSCI = &g_Sector[attackerSectorAround.Around[i].shY][attackerSectorAround.Around[i].shX];

		// 해당 섹터에 사람 없으면 패스
		pCurLink = pSCI->pClientLinkHead;
		for (DWORD j = 0; j < pSCI->dwNumOfClient; ++j)
		{
			pVictim = LinkToClient(pCurLink);

			// 피격 후보자가 끊길 유저면 걸러냄
			if (g_pDisconnectManager->IsDeleted(pVictim->dwID))
				goto lb_next;

			// 공격자와 피격 후보자가 같으면 넘김
			if (pAttacker == pVictim)
				goto lb_next;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 1))
				goto lb_find;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 1))
				goto lb_find;

			lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	if (pVictim->chHp <= 0)
	{
		g_pDisconnectManager->RegisterId(pVictim->dwID);
		RemoveClientAtSector(pVictim, pVictim->CurSector.shY, pVictim->CurSector.shX);
	}
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
lb_return:
	return TRUE;
}

BOOL CSProc::CS_ATTACK2(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	st_Client* pAttacker;
	st_SECTOR_AROUND attackerSectorAround;
	st_SECTOR_AROUND victimSectorAround;
	DWORD dwPacketSize;
	st_SECTOR_CLIENT_INFO* pSCI;
	st_Client* pVictim;
	LINKED_NODE* pCurLink;

	// 끊길 유저 걸러내기
	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	// 공격자 클라이언트 찾고, 근처 섹터찾아서 ATTACK 패킷 보내기
	pAttacker = g_pClientManager->Find(dwFromId);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 2);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);

	// 공격자 주위 섹터에 있는 클라이언트로부터 피격판정 시작
	for (DWORD i = 0; i < attackerSectorAround.byCount; ++i)
	{
		pSCI = &g_Sector[attackerSectorAround.Around[i].shY][attackerSectorAround.Around[i].shX];

		// 해당 섹터에 사람 없으면 패스
		pCurLink = pSCI->pClientLinkHead;
		for (DWORD j = 0; j < pSCI->dwNumOfClient; ++j)
		{
			pVictim = LinkToClient(pCurLink);

			// 피격 후보자가 끊길 유저면 걸러냄
			if (g_pDisconnectManager->IsDeleted(pVictim->dwID))
				goto lb_next;

			// 공격자와 피격 후보자가 같으면 넘김
			if (pAttacker == pVictim)
				goto lb_next;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 2))
				goto lb_find;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 2))
				goto lb_find;

		lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
lb_return:
	return TRUE;
}

BOOL CSProc::CS_ATTACK3(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	st_Client* pAttacker;
	st_SECTOR_AROUND attackerSectorAround;
	st_SECTOR_AROUND victimSectorAround;
	DWORD dwPacketSize;
	st_SECTOR_CLIENT_INFO* pSCI;
	st_Client* pVictim;
	LINKED_NODE* pCurLink;

	// 끊길 유저 걸러내기
	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	// 공격자 클라이언트 찾고, 근처 섹터찾아서 ATTACK 패킷 보내기
	pAttacker = g_pClientManager->Find(dwFromId);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 3);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);

	// 공격자 주위 섹터에 있는 클라이언트로부터 피격판정 시작
	for (DWORD i = 0; i < attackerSectorAround.byCount; ++i)
	{
		pSCI = &g_Sector[attackerSectorAround.Around[i].shY][attackerSectorAround.Around[i].shX];

		// 해당 섹터에 사람 없으면 패스
		pCurLink = pSCI->pClientLinkHead;
		for (DWORD j = 0; j < pSCI->dwNumOfClient; ++j)
		{
			pVictim = LinkToClient(pCurLink);

			// 피격 후보자가 끊길 유저면 걸러냄
			if (g_pDisconnectManager->IsDeleted(pVictim->dwID))
				goto lb_next;

			// 공격자와 피격 후보자가 같으면 넘김
			if (pAttacker == pVictim)
				goto lb_next;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 3))
				goto lb_find;

			if (pVictim->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 3))
				goto lb_find;

		lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
lb_return:
	return TRUE;
}

BOOL CSProc::CS_ECHO(DWORD dwFromId, DWORD dwTime)
{
	DWORD dwPacketSize;
	dwPacketSize = MAKE_SC_ECHO(dwTime);
	EnqPacketUnicast(dwFromId, g_sb.GetBufferPtr(), dwPacketSize);
	return TRUE;
}

