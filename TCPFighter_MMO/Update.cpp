#include <windows.h>
#include "Constant.h"
#include "Direction.h"
#include "Logger.h"
#include "MiddleWare.h"
#include "SCContents.h"
#include "Sector.h"
#include "SerializeBuffer.h"

extern SerializeBuffer g_sb1;
extern SerializeBuffer g_sb2;
extern SerializeBuffer g_sb3;

extern st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];

st_Client* g_pClientArr[MAX_SESSION];

extern int g_iDisConByTimeOut;


//void GetDirStr(BYTE byMoveDir, WCHAR* pDir, int len);

// X64 Calling Convetion �� ���� RTL 4������ �������Ϳ� �־��༭ ���� ��
void SectorUpdateAndNotify(st_Client* pClient, BYTE bySectorMoveDir, SectorPos oldSectorPos, SectorPos newSectorPos, BOOL IsMove)
{
	DWORD dwID;
	Pos CurPos;
	CHAR chHP;
	BYTE byDeltaSectorNum;
	BYTE byViewDir;
	DWORD dwMSDC_OneToAllSize;
	DWORD dwMSDC_AllToOneSize;
	DWORD dwMSCOC_OneToAllSize;
	DWORD dwMSCOC_AllToOneSize;
	DWORD dwMSMS_OneToAllSize;
	DWORD dwMSMS_AllToOneSize;

	dwID = pClient->dwID;
	CurPos = pClient->pos;
	chHP = pClient->chHp;
	byViewDir = pClient->byViewDir;

	// ���� ����Ʈ�� ����
	RemoveClientAtSector(pClient, oldSectorPos);
	AddClientAtSector(pClient, newSectorPos);

	// ����ǿ��� ������ ���Ϳ� ����ǿ� ������ ���͸� ����
	byDeltaSectorNum = (bySectorMoveDir % 2 == 0) ? 3 : 5;
	AroundInfo* pRemoveAroundInfo = GetDeltaValidClient(removeDirArr[bySectorMoveDir], byDeltaSectorNum, oldSectorPos);
	if (pRemoveAroundInfo->CI.dwNum <= 0)
		goto lb_new;

	dwMSDC_OneToAllSize = MAKE_SC_DELETE_CHARACTER(dwID, g_sb1);
	dwMSDC_AllToOneSize = 0;
	for (DWORD i = 0; i < pRemoveAroundInfo->CI.dwNum; ++i)
	{
		st_Client* pOtherClient = pRemoveAroundInfo->CI.cArr[i];
		EnqPacketRB(pOtherClient, g_sb1.GetBufferPtr(), dwMSDC_OneToAllSize);
		dwMSDC_AllToOneSize += MAKE_SC_DELETE_CHARACTER(pOtherClient->dwID, g_sb2);
	}
	EnqPacketRB(pClient, g_sb2.GetBufferPtr(), dwMSDC_AllToOneSize);
	g_sb1.Clear();
	g_sb2.Clear();


lb_new:
	AroundInfo* pNewAroundInfo = GetDeltaValidClient(AddDirArr[bySectorMoveDir], byDeltaSectorNum, newSectorPos);
	if (pNewAroundInfo->CI.dwNum <= 0)
		return;

	dwMSCOC_OneToAllSize = MAKE_SC_CREATE_OTHER_CHARACTER(dwID, byViewDir, CurPos, chHP, g_sb1);
	dwMSMS_OneToAllSize = 0;

	if (IsMove)
		dwMSMS_OneToAllSize = MAKE_SC_MOVE_START(dwID, pClient->byMoveDir, CurPos, g_sb1);

	dwMSCOC_AllToOneSize = 0;
	dwMSMS_AllToOneSize = 0;
	for (DWORD i = 0; i < pNewAroundInfo->CI.dwNum; ++i)
	{
		st_Client* pOtherClient = pNewAroundInfo->CI.cArr[i];
		EnqPacketRB(pOtherClient, g_sb1.GetBufferPtr(), dwMSCOC_OneToAllSize + dwMSMS_OneToAllSize);
		dwMSCOC_AllToOneSize += MAKE_SC_CREATE_OTHER_CHARACTER(pOtherClient->dwID, pOtherClient->byViewDir, pOtherClient->pos, pOtherClient->chHp, g_sb2);
		if (pOtherClient->byMoveDir != dfPACKET_MOVE_DIR_NOMOVE)
			dwMSMS_AllToOneSize += MAKE_SC_MOVE_START(pOtherClient->dwID, pOtherClient->byMoveDir, pOtherClient->pos, g_sb2);
	}
	EnqPacketRB(pClient, g_sb2.GetBufferPtr(), dwMSCOC_AllToOneSize + dwMSMS_AllToOneSize);
	g_sb1.Clear();
	g_sb2.Clear();


	pClient->OldSector = oldSectorPos;
	pClient->CurSector = newSectorPos;
}


constexpr DirVector speedVector{ dfSPEED_PLAYER_Y,dfSPEED_PLAYER_X };

void Update()
{
	Pos oldPos;
	Pos newPos;
	SectorPos oldSector;
	SectorPos newSector;
	BYTE bySectorMoveDir;
	DWORD dwTime;
	DirVector dirVector;
	DWORD dwClientNum;


	GetAllValidClient(&dwClientNum, (void**)g_pClientArr);

	for (DWORD i = 0; i < dwClientNum; ++i)
	{
		// ������ ���� ������ ��� Ȯ��, Valid �� ������ ���ؿ����� �߰��߰� �����̵������� ���� �޽��� ���۰������� ��Ʈ��ũ ���°� INVALID�� �ٲ�� ��찡 ����.
		if (IsNetworkStateInValid(g_pClientArr[i]->handle))
			continue;

		//Ÿ�Ӿƿ� ó��
		dwTime = timeGetTime();
		if (dwTime - GetLastRecvTime(g_pClientArr[i]->handle) > dwTimeOut + 100)
		{
			++g_iDisConByTimeOut;
			AlertSessionOfClientInvalid(g_pClientArr[i]->handle);
			continue;
		}

		// ������ �Ȼ�� ���̱�
		if (g_pClientArr[i]->chHp <= 0)
		{
			AlertSessionOfClientInvalid(g_pClientArr[i]->handle);
			_LOG(dwLog_LEVEL_DEBUG, L"Delete Client : %u By Dead", g_pClientArr[i]->dwID);
			continue;
		}

		// �� �����̴� ��� �ǳʶٱ�
		if (g_pClientArr[i]->byMoveDir == dfPACKET_MOVE_DIR_NOMOVE)
			continue;

		// �̵��� ��ġ���
		dirVector = vArr[g_pClientArr[i]->byMoveDir];
		oldPos = g_pClientArr[i]->pos;
		newPos.shY = oldPos.shY + vArr[g_pClientArr[i]->byMoveDir].shY * speedVector.shY;
		newPos.shX = oldPos.shX + vArr[g_pClientArr[i]->byMoveDir].shX * speedVector.shX;

		// ���� �̵��� ��ġ����, ���⼭ ����ؾ� ���Ͱ� ����� ��쿡 ���� ������ ��ģ��
		if (!IsValidPos(newPos))
			continue;

		_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", g_pClientArr[i]->dwID, oldPos.shX, oldPos.shY, newPos.shX, newPos.shY);

		// �̵� �� ���Ϳ� ���� ���� ���
		CalcSector(&oldSector, oldPos);
		CalcSector(&newSector, newPos);

		if (IsSameSector(oldSector, newSector))
			goto lb_same;

		// ���Ͱ� �̵��� ���
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		SectorUpdateAndNotify(g_pClientArr[i], bySectorMoveDir, oldSector, newSector, TRUE);

	lb_same:
		g_pClientArr[i]->pos = newPos;
	}
}