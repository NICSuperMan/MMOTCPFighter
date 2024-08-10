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

// X64 Calling Convetion 에 따라서 RTL 4개까지 레지스터에 넣어줘서 굳이 씀
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

	// 섹터 리스트에 갱신
	RemoveClientAtSector(pClient, oldSectorPos);
	AddClientAtSector(pClient, newSectorPos);

	// 영향권에서 없어진 섹터와 영향권에 들어오는 섹터를 구함
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
		// 접속이 끊길 예정인 사람 확인, Valid 한 새끼만 구해왓으나 중간중간 섹터이동등으로 인한 메시지 전송과정에서 네트워크 상태가 INVALID로 바뀌는 경우가 잇음.
		if (IsNetworkStateInValid(g_pClientArr[i]->handle))
			continue;

		//타임아웃 처리
		dwTime = timeGetTime();
		if (dwTime - GetLastRecvTime(g_pClientArr[i]->handle) > dwTimeOut + 100)
		{
			++g_iDisConByTimeOut;
			AlertSessionOfClientInvalid(g_pClientArr[i]->handle);
			continue;
		}

		// 죽을때 된사람 죽이기
		if (g_pClientArr[i]->chHp <= 0)
		{
			AlertSessionOfClientInvalid(g_pClientArr[i]->handle);
			_LOG(dwLog_LEVEL_DEBUG, L"Delete Client : %u By Dead", g_pClientArr[i]->dwID);
			continue;
		}

		// 안 움직이는 사람 건너뛰기
		if (g_pClientArr[i]->byMoveDir == dfPACKET_MOVE_DIR_NOMOVE)
			continue;

		// 이동후 위치계산
		dirVector = vArr[g_pClientArr[i]->byMoveDir];
		oldPos = g_pClientArr[i]->pos;
		newPos.shY = oldPos.shY + vArr[g_pClientArr[i]->byMoveDir].shY * speedVector.shY;
		newPos.shX = oldPos.shX + vArr[g_pClientArr[i]->byMoveDir].shX * speedVector.shX;

		// 새로 이동한 위치검증, 여기서 통과해야 섹터가 변경된 경우에 대한 절차를 거친다
		if (!IsValidPos(newPos))
			continue;

		_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", g_pClientArr[i]->dwID, oldPos.shX, oldPos.shY, newPos.shX, newPos.shY);

		// 이동 후 섹터와 이전 섹터 계산
		CalcSector(&oldSector, oldPos);
		CalcSector(&newSector, newPos);

		if (IsSameSector(oldSector, newSector))
			goto lb_same;

		// 섹터가 이동한 경우
		bySectorMoveDir = GetSectorMoveDir(oldSector, newSector);
		SectorUpdateAndNotify(g_pClientArr[i], bySectorMoveDir, oldSector, newSector, TRUE);

	lb_same:
		g_pClientArr[i]->pos = newPos;
	}
}