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
#include "Update.h"
extern int g_iSyncCount;

CStub* g_pCSProc;
BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize);

void GetDirStr(BYTE byMoveDir,WCHAR* pDir,int len)
{
	switch (byMoveDir)
	{
	case dfPACKET_MOVE_DIR_LL:
		wcscpy_s(pDir,len, L"LL");
		break;
	case dfPACKET_MOVE_DIR_LU:
		wcscpy_s(pDir,len, L"LU");
		break;
	case dfPACKET_MOVE_DIR_UU:
		wcscpy_s(pDir,len, L"UU");
		break;
	case dfPACKET_MOVE_DIR_RU:
		wcscpy_s(pDir,len, L"RU");
		break;
	case dfPACKET_MOVE_DIR_RR:
		wcscpy_s(pDir,len, L"RR");
		break;
	case dfPACKET_MOVE_DIR_RD:
		wcscpy_s(pDir,len, L"RD");
		break;
	case dfPACKET_MOVE_DIR_DD:
		wcscpy_s(pDir,len, L"DD");
		break;
	case dfPACKET_MOVE_DIR_LD:
		wcscpy_s(pDir,len, L"LD");
		break;	
	default:
		break;
	}
}

BYTE GetSectorMoveDir(SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX)
{
	SHORT shCompY;
	SHORT shCompX;

	shCompX = shNewSectorX - shOldSectorX;
	shCompY = shNewSectorY - shOldSectorY;

	switch (shCompY)
	{
	default:
	case -1:
		switch (shCompX)
		{
		case -1:
			return dfPACKET_MOVE_DIR_LU;
		case 0:
			return dfPACKET_MOVE_DIR_UU;
		case 1:
			return dfPACKET_MOVE_DIR_RU;
		}
	case 0:
		switch (shCompX)
		{
		case -1:
			return dfPACKET_MOVE_DIR_LL;
		case 1:
			return dfPACKET_MOVE_DIR_RR;
		}
	case 1:
		switch (shCompX)
		{
		case -1:
			return dfPACKET_MOVE_DIR_LD;
		case 0:
			return dfPACKET_MOVE_DIR_DD;
		case 1:
			return dfPACKET_MOVE_DIR_RD;
		}
	}
}

BOOL CSProc::CS_MOVE_START(DWORD dwFromId, BYTE byMoveDir, SHORT shX, SHORT shY)
{
	st_Client* pClient;
	st_SECTOR_AROUND sectorAround;
	DWORD dwPacketSize;
	SHORT shServerY;
	SHORT shServerX;
	SHORT shOldSectorY;
	SHORT shOldSectorX;
	SHORT shNewSectorY;
	SHORT shNewSectorX;
	BYTE bySectorMoveDir;

	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	// 클라이언트 찾기
	pClient = g_pClientManager->Find(dwFromId);
	shServerY = pClient->shY;
	shServerX = pClient->shX;

	// 임시방편
	pClient->byMoveDir = byMoveDir;
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

	WCHAR ClientOwnDir[512];
	WCHAR ServerOwnDir[512];
	GetDirStr(byMoveDir, ClientOwnDir,512);
	GetDirStr(pClient->byMoveDir, ServerOwnDir,512);
	_LOG(dwLog_LEVEL_ERROR, L"CS_MOVE_START ID : %u, STOP POS X : %d, Y : %d", dwFromId, shX, shY);
	//_LOG(dwLog_LEVEL_ERROR, L"Server Dir : %s, Client Dir : %s", ServerOwnDir, ClientOwnDir);

	// 오차가 dfERROR_RANGE 이상이면 서버가 알던 좌표로 싱크 맞추기
	if (abs(shServerX - shX) > dfERROR_RANGE || abs(shServerY - shY) > dfERROR_RANGE)
	{
		//_LOG(dwLog_LEVEL_SYSTEM, L"Send Sync To Client : %u X : %d, Y : %d -> X : %d, Y : %d", dwFromId, shX, shY, shServerX, shServerY);
		++g_iSyncCount;
		dwPacketSize = MAKE_SC_SYNC(dwFromId, shServerX, shServerY);
		GetSectorAround(shServerY, shServerX, &sectorAround);
		SendPacket_Around(pClient, &sectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
		g_sb.Clear();
		shY = shServerY;
		shX = shServerX;
	}

	shOldSectorY = shServerY / df_SECTOR_HEIGHT;
	shOldSectorX = shServerX / df_SECTOR_WIDTH;
	shNewSectorY = shY / df_SECTOR_HEIGHT;
	shNewSectorX = shX / df_SECTOR_WIDTH;

	
	// dfERROR_RANGE보다 오차가 작지만, 섹터가 틀려서 섹터를 클라기준으로 맞출경우 업데이트 
	if (!IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
	{
		bySectorMoveDir = GetSectorMoveDir(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);
		SectorUpdateAndNotify(pClient, bySectorMoveDir, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX, &sectorAround);
	}

	GetSectorAround(shY, shX, &sectorAround);
	dwPacketSize = MAKE_SC_MOVE_START(dwFromId, byMoveDir, shX, shY);
	for (BYTE i = 0; i < sectorAround.byCount; ++i)
	{
		//_LOG(dwLog_LEVEL_DEBUG, L"Notify MOVE START Character ID : %u To SECTOR X : %d, Y : %d", dwFromId, sectorAround.Around[i].shX, sectorAround.Around[i].shY);
		SendPacket_SectorOne(&sectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	}
	g_sb.Clear();

	// 서버에서 보관하는 클라이언트의 이동상태, 좌표 , 이동방향 저장
	pClient->dwAction = MOVE;
	pClient->shX = shX;
	pClient->shY = shY;
	return TRUE;
}

BOOL CSProc::CS_MOVE_STOP(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	/*register*/st_Client* pClient;
	/*register*/SHORT shServerY;
	/*register*/SHORT shServerX;
	/*register*/ DWORD dwPacketSize;
	/*register*/ SHORT shOldSectorY;
	/*register*/SHORT shOldSectorX;
	/*register*/SHORT shNewSectorY;
	/*register*/SHORT shNewSectorX;
	st_SECTOR_AROUND sectorAround;
	BYTE bySectorMoveDir;
	//BYTE byReverseDir;

	if (g_pDisconnectManager->IsDeleted(dwFromId))
		return FALSE;

	pClient = g_pClientManager->Find(dwFromId);
	shServerY = pClient->shY;
	shServerX = pClient->shX;

	// 임시방편
	_LOG(dwLog_LEVEL_ERROR, L"CS_MOVE_STOP ID : %u, STOP POS X : %d, Y : %d", dwFromId, shX, shY);

	// 오차가 dfERROR_RANGE 이상이면 서버가 알던 좌표로 싱크 맞추기
	if (abs(shServerX - shX) > dfERROR_RANGE || abs(shServerY - shY) > dfERROR_RANGE)
	{
		//_LOG(dwLog_LEVEL_SYSTEM, L"Sync POS ID : %u STOP POS X : %d, Y : %d -> Sync POS X : %d, Y : %d", dwFromId, shX, shY,shServerX,shServerY);
		++g_iSyncCount;
		dwPacketSize = MAKE_SC_SYNC(dwFromId, shServerX, shServerY);
		GetSectorAround(shServerY, shServerX, &sectorAround);
		SendPacket_Around(pClient, &sectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
		g_sb.Clear();
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
	{
		bySectorMoveDir = GetSectorMoveDir(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);
		SectorUpdateAndNotify(pClient, bySectorMoveDir, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX, &sectorAround);
	}

	GetSectorAround(shY, shX, &sectorAround);
	dwPacketSize = MAKE_SC_MOVE_STOP(dwFromId, byViewDir, shX, shY);
	//_LOG(dwLog_LEVEL_DEBUG, L"Notify MOVE STOP Character ID To SECTOR");
	SendPacket_Around(pClient, &sectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	g_sb.Clear();

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
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u CS_ATTACK1", pAttacker->dwID);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 1);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	g_sb.Clear();

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

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 1))
				goto lb_find;

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 1))
				goto lb_find;

			lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u Damaged By Client Id : %u", pVictim->dwID, pAttacker->dwID);
	if (pVictim->chHp <= 0)
	{
		g_pDisconnectManager->RegisterId(pVictim->dwID);
		RemoveClientAtSector(pVictim, pVictim->CurSector.shY, pVictim->CurSector.shX);
	}
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
	g_sb.Clear();
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
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u CS_ATTACK2", pAttacker->dwID);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 2);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	g_sb.Clear();

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

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 2))
				goto lb_find;

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 2))
				goto lb_find;

		lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u Damaged By Client Id : %u", pVictim->dwID, pAttacker->dwID);
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
	g_sb.Clear();
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
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u CS_ATTACK3", pAttacker->dwID);
	GetSectorAround(shY, shX, &attackerSectorAround);
	dwPacketSize = MAKE_SC_ATTACK(dwFromId, pAttacker->byViewDir, shX, shY, 3);
	SendPacket_Around(pAttacker, &attackerSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
	g_sb.Clear();

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

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_LL && IsLLColide(shY, shX, pVictim->shY, pVictim->shX, 3))
				goto lb_find;

			if (pAttacker->byViewDir == dfPACKET_MOVE_DIR_RR && IsRRColide(shY, shX, pVictim->shY, pVictim->shX, 3))
				goto lb_find;

		lb_next:
			pCurLink = pCurLink->pNext;
		}
	}
	goto lb_return;
lb_find: // 피격대상자 HP깎고 피격자 근처 섹터에 데미지 메시지 날리기
	pVictim->chHp -= dfATTACK1_DAMAGE;
	_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u Damaged By Client Id : %u", pVictim->dwID, pAttacker->dwID);
	GetSectorAround(pVictim->shY, pVictim->shX, &victimSectorAround);
	dwPacketSize = MAKE_SC_DAMAGE(dwFromId, pVictim->dwID, pVictim->chHp);
	SendPacket_Around(pVictim, &victimSectorAround, g_sb.GetBufferPtr(), dwPacketSize, TRUE);
	g_sb.Clear();
lb_return:
	return TRUE;
}

BOOL CSProc::CS_ECHO(DWORD dwFromId, DWORD dwTime)
{
	DWORD dwPacketSize;
	dwPacketSize = MAKE_SC_ECHO(dwTime);
	EnqPacketUnicast(dwFromId, g_sb.GetBufferPtr(), dwPacketSize);
	g_sb.Clear();
	return TRUE;
}

