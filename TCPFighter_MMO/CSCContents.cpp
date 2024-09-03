#include "Client.h"
#include "SerializeBuffer.h"
#include "MiddleWare.h"
#include "Logger.h"
#include "Constant.h"
#include "CSCContents.h"
#include "SCContents.h"
#include "Sector.h"
#include "Direction.h"

#include <strstream>


extern int g_iSyncCount;


void SectorUpdateAndNotify(st_Client* pClient, BYTE bySectorMoveDir, SectorPos oldSectorPos, SectorPos newSectorPos, BOOL IsMove);

BOOL CS_MOVE_START(st_Client* pClient, BYTE byMoveDir, Pos clientPos)
{
	Pos serverPos;
	SectorPos oldSector;
	SectorPos newSector;
	BYTE bySectorMoveDir;
	DWORD dwFromId;
	DWORD dwSyncPacketSize;
	DWORD dwMSMSSize;

	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;


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

	dwFromId = pClient->dwID;
	serverPos = pClient->pos;

	CalcSector(&oldSector, serverPos);

	// 이동중에 방향을 바꿔서 STOP이 오지않고 또 다시 START가 왓는데, 이때 서버 프레임이 떨어져서 싱크가 발생하는 경우.
	if (IsSync(serverPos, clientPos))
	{
		++g_iSyncCount;
		dwSyncPacketSize = MAKE_SC_SYNC(dwFromId, serverPos, g_sb1);
		AroundInfo* pAroundInfo = GetAroundValidClient(oldSector, nullptr);
		for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
		{
			EnqPacketRB(pAroundInfo->CI.cArr[i], g_sb1.GetBufferPtr(), dwSyncPacketSize);
		}
		g_sb1.Clear();
		clientPos = serverPos;
	}

	CalcSector(&newSector, clientPos);

	// dfERROR_RANGE보다 오차가 작지만, 섹터가 틀려서 섹터를 클라기준으로 맞출경우 업데이트 
	if (!IsSameSector(oldSector, newSector))
	{
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		// 어차피 곧 움직인다고 보낼거기에 지금은 움직인다고 알릴필요 없다.
		SectorUpdateAndNotify(pClient, bySectorMoveDir, oldSector, newSector, FALSE);
	}

	AroundInfo* pAroundInfo = GetAroundValidClient(newSector, pClient);
	if (pAroundInfo->CI.dwNum <= 0)
		goto lb_apply;

	dwMSMSSize = MAKE_SC_MOVE_START(dwFromId, byMoveDir, clientPos, g_sb1);
	for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
	{
		EnqPacketRB(pAroundInfo->CI.cArr[i], g_sb1.GetBufferPtr(), dwMSMSSize);
	}
	g_sb1.Clear();

lb_apply:
	pClient->byMoveDir = byMoveDir;
	pClient->pos = clientPos;
	return TRUE;
}

void HandleCollsion_HELPER(Pos pos, BYTE byViewDir, Pos AttackRange, st_SECTOR_AROUND* pSectorAround)
{
	int iCount;
	SectorPos originalSector;
	SectorPos temp;
	CalcSector(&originalSector, pos);

	pSectorAround->Around[0].YX = originalSector.YX;
	iCount = 1;

	if (byViewDir == dfPACKET_MOVE_DIR_LL)
	{
		// 왼쪽 방향 공격
		// temp.shX 왼쪽 섹터들의 X값 공통 X값.

		temp.shX = (pos.shX - AttackRange.shX) / df_SECTOR_WIDTH;
		// 바로왼쪽 값이 유효한 섹터임과 동시에 공격범위임
		if (IsValidSector(originalSector.shY, temp.shX) && originalSector.shX != temp.shX)
		{
			pSectorAround->Around[iCount].shX = temp.shX;
			pSectorAround->Around[iCount].shY = originalSector.shY;
			++iCount;

			// 이시점에서 왼쪽은 유효하며 위, 아래 확인해야함
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				// 위
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// 왼쪽 위
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

			// 아래 확인
			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				//아래
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// 왼쪽 아래
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}
		}
		else
		{
			// 왼쪽방향이 다른섹터로 넘어가지 않으면 위 아래만 탐색하면됨
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}


			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}
		}
	}
	else
	{
		// 오른쪽 방향 공격
		// 오른쪽 방향 섹터검색
		temp.shX = (pos.shX + AttackRange.shX) / df_SECTOR_WIDTH;
		if(IsValidSector(originalSector.shY, temp.shX) && originalSector.shX != temp.shX)
		{
			// 오른쪽 확정 위 아래 검사
			pSectorAround->Around[iCount].shX = temp.shX;
			pSectorAround->Around[iCount].shY = originalSector.shY;
			++iCount;

			// 이시점에서 오른쪽은 유효하며 위, 아래 확인해야함
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				// 위
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// 왼쪽 위
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

			// 아래 확인
			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				//아래
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// 왼쪽 아래
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

		}
		else
		{
			// 오른쪽방향이 다른섹터로 넘어가지 않으면 위 아래만 탐색하면됨
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}


			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}
		}
	}
	pSectorAround->byCount = iCount;
}




st_Client* HandleCollision(st_Client* pAttacker, Pos ClientPos, BYTE byViewDir, Pos AttackRange)
{
	extern AroundInfo g_AroundInfo;

	st_SECTOR_AROUND sectorAround;
	int iNum = 0;
	st_Client* pVictim;
	pVictim = nullptr;

	if (byViewDir != dfPACKET_MOVE_DIR_LL && byViewDir != dfPACKET_MOVE_DIR_RR)
		__debugbreak();

	if (byViewDir == dfPACKET_MOVE_DIR_LL)
	{
		HandleCollsion_HELPER(ClientPos, byViewDir, AttackRange, &sectorAround);
		for (int i = 0; i < sectorAround.byCount; ++i)
		{
			GetValidClientFromSector(sectorAround.Around[i], &g_AroundInfo, &iNum, pAttacker);
			for (int j = 0; j < iNum; ++j)
			{
				if (!IsLLColide(ClientPos, g_AroundInfo.CI.cArr[j]->pos, AttackRange.shY, AttackRange.shX))
					continue;

				pVictim = g_AroundInfo.CI.cArr[j];
				goto lb_return;
			}
		}
	}
	else
	{
		HandleCollsion_HELPER(ClientPos, byViewDir, AttackRange, &sectorAround);
		for (int i = 0; i < sectorAround.byCount; ++i)
		{
			GetValidClientFromSector(sectorAround.Around[i], &g_AroundInfo, &iNum, pAttacker);
			for (int j = 0; j < iNum; ++j)
			{
				if (!IsRRColide(ClientPos, g_AroundInfo.CI.cArr[j]->pos, AttackRange.shY, AttackRange.shX))
					continue;

				pVictim = g_AroundInfo.CI.cArr[j];
				goto lb_return;
			}
		}
	}

	lb_return:
	return pVictim;

}

BOOL CS_MOVE_STOP(st_Client* pClient, BYTE byViewDir, Pos ClientPos)
{
	Pos serverPos;
	DWORD dwFromId;
	DWORD dwSyncSize;
	SectorPos oldSector;
	SectorPos newSector;
	BYTE bySectorMoveDir;
	DWORD dwMSMSSize;

	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;

	dwFromId = pClient->dwID;
	serverPos = pClient->pos;

	CalcSector(&oldSector, serverPos);
	if (IsSync(serverPos, ClientPos))
	{
		++g_iSyncCount;
		dwSyncSize = MAKE_SC_SYNC(dwFromId, serverPos, g_sb1);
		AroundInfo* pAroundInfo = GetAroundValidClient(oldSector, nullptr);
		for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
			EnqPacketRB(pAroundInfo->CI.cArr[i], g_sb1.GetBufferPtr(), dwSyncSize);
		g_sb1.Clear();
		ClientPos = serverPos;
	}

	CalcSector(&newSector, ClientPos);
	// dfERROR_RANGE보다 오차가 작지만, 섹터가 틀려서 섹터를 클라기준으로 맞출경우 업데이트 
	if (!IsSameSector(oldSector, newSector))
	{
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		// 아직 움직이고 잇기때문에 움직이고 잇다고 보내야함
		SectorUpdateAndNotify(pClient, bySectorMoveDir, oldSector, newSector, TRUE);
	}

	AroundInfo* pAroundInfo = GetAroundValidClient(newSector, pClient);

	// 근처에 사람없으면 패킷 아예안만들고 끝
	if (pAroundInfo->CI.dwNum <= 0)
		goto lb_apply;

	dwMSMSSize = MAKE_SC_MOVE_STOP(dwFromId, byViewDir, ClientPos, g_sb1);
	for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
	{
		EnqPacketRB(pAroundInfo->CI.cArr[i], g_sb1.GetBufferPtr(), dwMSMSSize);
	}

	g_sb1.Clear();

lb_apply:
	pClient->byMoveDir = dfPACKET_MOVE_DIR_NOMOVE;
	pClient->pos = ClientPos;
	return TRUE;
}

BOOL CS_ATTACK1(st_Client* pClient, BYTE byViewDir, Pos clientPos)
{
	DWORD dwAttackerID;
	SectorPos curSector;
	DWORD dwAttackSize;
	st_Client* pVictim;
	DWORD dwDamagedSize;

	dwAttackerID = pClient->dwID;
	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;

	CalcSector(&curSector, clientPos);

	// 자기자신 제외하고 보낸다
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, pClient->byViewDir, clientPos, 1, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();


	// 충돌처리 진행후 피해자 구해옴, 없으면 NULL
	pVictim = HandleCollision(pClient, clientPos, byViewDir, Pos{ dfATTACK1_RANGE_Y,dfATTACK1_RANGE_X });

	// 피해자가 없음
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK1_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);

	// MAKE_SC_ATTACK보낼때의 ArundInfo를 재활용함. 이전에는 자기자신을 빼고 가져왓기 때문에 자기자신은 for문 끝나고 별도로 보내줘야함.
	// 또한 MAKE_SC_ATTACK을 보내면서 연결이 끊길 클라들이 생길 가능성이 잇으므로 IsNetworkStateInValid로 검사해야함(사실 EnqPacketRB 내부에서 진행하기때문에 안해도 되지만 함수호출을 줄이기위해 수행함)
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		if (IsNetworkStateInValid(pVictim->handle))
			continue;

		// 주위사람에게 보내기
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwDamagedSize);
	}
	// 자기자신에게 보내기
	EnqPacketRB(pClient, g_sb1.GetBufferPtr(), dwDamagedSize);
	g_sb1.Clear();
	return TRUE;
}

BOOL CS_ATTACK2(st_Client* pClient, BYTE byViewDir, Pos clientPos)
{
	DWORD dwAttackerID;
	SectorPos curSector;
	DWORD dwAttackSize;
	st_Client* pVictim;
	DWORD dwDamagedSize;

	dwAttackerID = pClient->dwID;
	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;

	CalcSector(&curSector, clientPos);
	// 자기자신 제외하고 보낸다
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, byViewDir, clientPos, 2, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();


	// 충돌처리 진행후 피해자 구해옴, 없으면 NULL
	pVictim = HandleCollision(pClient, pClient->pos, byViewDir, Pos{ dfATTACK1_RANGE_Y,dfATTACK1_RANGE_X });

	// 피해자가 없음
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK2_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);

	// MAKE_SC_ATTACK보낼때의 ArundInfo를 재활용함. 이전에는 자기자신을 빼고 가져왓기 때문에 자기자신은 for문 끝나고 별도로 보내줘야함.
	// 또한 MAKE_SC_ATTACK을 보내면서 연결이 끊길 클라들이 생길 가능성이 잇으므로 IsNetworkStateInValid로 검사해야함(사실 EnqPacketRB 내부에서 진행하기때문에 안해도 되지만 함수호출을 줄이기위해 수행함)
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		if (IsNetworkStateInValid(pVictim->handle))
			continue;

		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwDamagedSize);
	}
	EnqPacketRB(pClient, g_sb1.GetBufferPtr(), dwDamagedSize);
	g_sb1.Clear();
	return TRUE;
}

BOOL CS_ATTACK3(st_Client* pClient, BYTE byViewDir, Pos clientPos)
{
	DWORD dwAttackerID;
	SectorPos curSector;
	DWORD dwAttackSize;
	st_Client* pVictim;
	DWORD dwDamagedSize;

	dwAttackerID = pClient->dwID;
	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;

	CalcSector(&curSector, clientPos);
	// 자기자신 제외하고 보낸다
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, pClient->byViewDir, clientPos, 3, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();

	// 충돌처리 진행후 피해자 구해옴, 없으면 NULL
	pVictim = HandleCollision(pClient, pClient->pos, byViewDir, Pos{ dfATTACK3_RANGE_Y,dfATTACK3_RANGE_X });

	// 피해자가 없음
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK3_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);
	// MAKE_SC_ATTACK보낼때의 ArundInfo를 재활용함. 이전에는 자기자신을 빼고 가져왓기 때문에 자기자신은 for문 끝나고 별도로 보내줘야함.
	// 또한 MAKE_SC_ATTACK을 보내면서 연결이 끊길 클라들이 생길 가능성이 잇으므로 IsNetworkStateInValid로 검사해야함(사실 EnqPacketRB 내부에서 진행하기때문에 안해도 되지만 함수호출을 줄이기위해 수행함)
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		if (IsNetworkStateInValid(pVictim->handle))
			continue;

		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwDamagedSize);
	}
	EnqPacketRB(pClient, g_sb1.GetBufferPtr(), dwDamagedSize);
	g_sb1.Clear();

	return TRUE;
}

BOOL CS_ECHO(st_Client* pClient, DWORD dwTime)
{
	DWORD dwPacketSize;

	if (IsNetworkStateInValid(pClient->handle))
		return FALSE;

	dwPacketSize = MAKE_SC_ECHO(dwTime, g_sb1);
	EnqPacketRB(pClient, g_sb1.GetBufferPtr(), dwPacketSize);
	g_sb1.Clear();
	return TRUE;
}
