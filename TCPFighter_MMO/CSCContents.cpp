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


	// Ŭ���̾�Ʈ �ü�ó��
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

	// �̵��߿� ������ �ٲ㼭 STOP�� �����ʰ� �� �ٽ� START�� �Ӵµ�, �̶� ���� �������� �������� ��ũ�� �߻��ϴ� ���.
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

	// dfERROR_RANGE���� ������ ������, ���Ͱ� Ʋ���� ���͸� Ŭ��������� ������ ������Ʈ 
	if (!IsSameSector(oldSector, newSector))
	{
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		// ������ �� �����δٰ� �����ű⿡ ������ �����δٰ� �˸��ʿ� ����.
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
		// ���� ���� ����
		// temp.shX ���� ���͵��� X�� ���� X��.

		temp.shX = (pos.shX - AttackRange.shX) / df_SECTOR_WIDTH;
		// �ٷο��� ���� ��ȿ�� �����Ӱ� ���ÿ� ���ݹ�����
		if (IsValidSector(originalSector.shY, temp.shX) && originalSector.shX != temp.shX)
		{
			pSectorAround->Around[iCount].shX = temp.shX;
			pSectorAround->Around[iCount].shY = originalSector.shY;
			++iCount;

			// �̽������� ������ ��ȿ�ϸ� ��, �Ʒ� Ȯ���ؾ���
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				// ��
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// ���� ��
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

			// �Ʒ� Ȯ��
			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				//�Ʒ�
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// ���� �Ʒ�
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}
		}
		else
		{
			// ���ʹ����� �ٸ����ͷ� �Ѿ�� ������ �� �Ʒ��� Ž���ϸ��
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
		// ������ ���� ����
		// ������ ���� ���Ͱ˻�
		temp.shX = (pos.shX + AttackRange.shX) / df_SECTOR_WIDTH;
		if(IsValidSector(originalSector.shY, temp.shX) && originalSector.shX != temp.shX)
		{
			// ������ Ȯ�� �� �Ʒ� �˻�
			pSectorAround->Around[iCount].shX = temp.shX;
			pSectorAround->Around[iCount].shY = originalSector.shY;
			++iCount;

			// �̽������� �������� ��ȿ�ϸ� ��, �Ʒ� Ȯ���ؾ���
			temp.shY = (pos.shY + AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				// ��
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// ���� ��
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

			// �Ʒ� Ȯ��
			temp.shY = (pos.shY - AttackRange.shY) / df_SECTOR_HEIGHT;
			if (IsValidSector(temp.shY, originalSector.shX) && originalSector.shY != temp.shY)
			{
				//�Ʒ�
				pSectorAround->Around[iCount].shX = originalSector.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;

				// ���� �Ʒ�
				pSectorAround->Around[iCount].shX = temp.shX;
				pSectorAround->Around[iCount].shY = temp.shY;
				++iCount;
			}

		}
		else
		{
			// �����ʹ����� �ٸ����ͷ� �Ѿ�� ������ �� �Ʒ��� Ž���ϸ��
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
	// dfERROR_RANGE���� ������ ������, ���Ͱ� Ʋ���� ���͸� Ŭ��������� ������ ������Ʈ 
	if (!IsSameSector(oldSector, newSector))
	{
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		// ���� �����̰� �ձ⶧���� �����̰� �մٰ� ��������
		SectorUpdateAndNotify(pClient, bySectorMoveDir, oldSector, newSector, TRUE);
	}

	AroundInfo* pAroundInfo = GetAroundValidClient(newSector, pClient);

	// ��ó�� ��������� ��Ŷ �ƿ��ȸ���� ��
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

	// �ڱ��ڽ� �����ϰ� ������
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, pClient->byViewDir, clientPos, 1, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();


	// �浹ó�� ������ ������ ���ؿ�, ������ NULL
	pVictim = HandleCollision(pClient, clientPos, byViewDir, Pos{ dfATTACK1_RANGE_Y,dfATTACK1_RANGE_X });

	// �����ڰ� ����
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK1_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);

	// MAKE_SC_ATTACK�������� ArundInfo�� ��Ȱ����. �������� �ڱ��ڽ��� ���� �����ӱ� ������ �ڱ��ڽ��� for�� ������ ������ ���������.
	// ���� MAKE_SC_ATTACK�� �����鼭 ������ ���� Ŭ����� ���� ���ɼ��� �����Ƿ� IsNetworkStateInValid�� �˻��ؾ���(��� EnqPacketRB ���ο��� �����ϱ⶧���� ���ص� ������ �Լ�ȣ���� ���̱����� ������)
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		if (IsNetworkStateInValid(pVictim->handle))
			continue;

		// ����������� ������
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwDamagedSize);
	}
	// �ڱ��ڽſ��� ������
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
	// �ڱ��ڽ� �����ϰ� ������
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, byViewDir, clientPos, 2, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();


	// �浹ó�� ������ ������ ���ؿ�, ������ NULL
	pVictim = HandleCollision(pClient, pClient->pos, byViewDir, Pos{ dfATTACK1_RANGE_Y,dfATTACK1_RANGE_X });

	// �����ڰ� ����
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK2_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);

	// MAKE_SC_ATTACK�������� ArundInfo�� ��Ȱ����. �������� �ڱ��ڽ��� ���� �����ӱ� ������ �ڱ��ڽ��� for�� ������ ������ ���������.
	// ���� MAKE_SC_ATTACK�� �����鼭 ������ ���� Ŭ����� ���� ���ɼ��� �����Ƿ� IsNetworkStateInValid�� �˻��ؾ���(��� EnqPacketRB ���ο��� �����ϱ⶧���� ���ص� ������ �Լ�ȣ���� ���̱����� ������)
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
	// �ڱ��ڽ� �����ϰ� ������
	AroundInfo* pAround = GetAroundValidClient(curSector, pClient);
	dwAttackSize = MAKE_SC_ATTACK(dwAttackerID, pClient->byViewDir, clientPos, 3, g_sb1);
	for (DWORD i = 0; i < pAround->CI.dwNum; ++i)
	{
		EnqPacketRB(pAround->CI.cArr[i], g_sb1.GetBufferPtr(), dwAttackSize);
	}
	g_sb1.Clear();

	// �浹ó�� ������ ������ ���ؿ�, ������ NULL
	pVictim = HandleCollision(pClient, pClient->pos, byViewDir, Pos{ dfATTACK3_RANGE_Y,dfATTACK3_RANGE_X });

	// �����ڰ� ����
	if (!pVictim)
		return FALSE;

	pVictim->chHp -= dfATTACK3_DAMAGE;
	dwDamagedSize = MAKE_SC_DAMAGE(dwAttackerID, pVictim->dwID, pVictim->chHp, g_sb1);
	// MAKE_SC_ATTACK�������� ArundInfo�� ��Ȱ����. �������� �ڱ��ڽ��� ���� �����ӱ� ������ �ڱ��ڽ��� for�� ������ ������ ���������.
	// ���� MAKE_SC_ATTACK�� �����鼭 ������ ���� Ŭ����� ���� ���ɼ��� �����Ƿ� IsNetworkStateInValid�� �˻��ؾ���(��� EnqPacketRB ���ο��� �����ϱ⶧���� ���ص� ������ �Լ�ȣ���� ���̱����� ������)
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
