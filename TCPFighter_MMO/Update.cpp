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

extern SerializeBuffer g_sb;

extern st_SECTOR_CLIENT_INFO g_Sector[dwNumOfSectorVertical][dwNumOfSectorHorizon];

struct st_DirVector
{
	SHORT shY;
	SHORT shX;
};

constexpr st_DirVector vArr[8] = {
	st_DirVector{0,-1},//LL
	st_DirVector{-1,-1},//LU
	st_DirVector{-1,0},//UU
	st_DirVector{-1,1}, //RU
	st_DirVector{0,1}, //RR
	st_DirVector{1,1}, //RD
	st_DirVector{1,0}, // DD
	st_DirVector{1,-1} //LD
};

// 이동후 OldSector에서 현재 캐릭터의 이동방향을 해당 배열의 인덱스로 대입해서 얻은 방향을 GetDeltaSector에 대입
constexpr BYTE removeDirArr[8] = 
{
	dfPACKET_MOVE_DIR_RU, //LL
	dfPACKET_MOVE_DIR_RU, //LU
	dfPACKET_MOVE_DIR_RD, //UU
	dfPACKET_MOVE_DIR_RD, //RU
	dfPACKET_MOVE_DIR_LD, //RR
	dfPACKET_MOVE_DIR_LD, //RD
	dfPACKET_MOVE_DIR_LU, //DD
	dfPACKET_MOVE_DIR_LU, //LD
};

// 이동후 NewSecotr에서 현재 캐릭터의 이동방향을 해당 배열의 인덱스로 대입해서 얻은 방향을 GetDeltaSctor에 대입
constexpr BYTE AddDirArr[8] =
{
	dfPACKET_MOVE_DIR_LD, //LL
	dfPACKET_MOVE_DIR_LD, //LU
	dfPACKET_MOVE_DIR_LU, //UU
	dfPACKET_MOVE_DIR_LU, //RU,
	dfPACKET_MOVE_DIR_RU, //RR
	dfPACKET_MOVE_DIR_RU, //RD
	dfPACKET_MOVE_DIR_RD, //DD
	dfPACKET_MOVE_DIR_RD, //LD
};

// 제거된 섹터를 얻을때는 BaseSector에 OldSector대입
// 새로운 섹터를 얻을때는 BaseSector에 NewSector대입
static __forceinline void GetDeltaSector(BYTE byBaseDir, st_SECTOR_AROUND* pSectorAround, BYTE byDeltaSectorNum, SHORT shBaseSectorPointY, SHORT shBaseSectorPointX)
{
	SHORT shGetSectorY;
	SHORT shGetSectorX;

	pSectorAround->byCount = 0;
	for (int i = 0; i < byDeltaSectorNum; ++i)
	{
		shGetSectorY = shBaseSectorPointY + vArr[byBaseDir].shY;
		shGetSectorX = shBaseSectorPointX + vArr[byBaseDir].shX;
		byBaseDir = (++byBaseDir) % 8;
		if (IsValidSector(shGetSectorY, shGetSectorX))
		{
			pSectorAround->Around[pSectorAround->byCount].shY = shGetSectorY;
			pSectorAround->Around[pSectorAround->byCount].shX = shGetSectorX;
			++(pSectorAround->byCount);
		}
	}
}

// X64 Calling Convetion 에 따라서 RTL 4개까지 레지스터에 넣어줘서 굳이 씀
void ClientSectorUpdateAndPacket(st_Client* pClient,SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX)
{
	BYTE byDeltaSectorNum;
	st_SECTOR_AROUND removeSectorAround;
	st_SECTOR_AROUND newSectorAround;
	DWORD dwPacketSize;
	DWORD dwID;
	BYTE byMoveDir;
	SHORT shY;
	SHORT shX;

	dwID = pClient->dwID;
	byMoveDir = pClient->byMoveDir;
	shY = pClient->shY;
	shX = pClient->shX;

	// 섹터 리스트에 갱신
	AddClientAtSector(pClient, shNewSectorY, shNewSectorX);
	RemoveClientAtSector(pClient, shOldSectorY, shOldSectorX);

	// 영향권에서 없어진 섹터와 영향권에 들어오는 섹터를 구함
	byDeltaSectorNum = (byMoveDir % 2 == 0) ? 3 : 5;
	GetDeltaSector(removeDirArr[byMoveDir], &removeSectorAround, byDeltaSectorNum, shOldSectorY, shOldSectorX);
	GetDeltaSector(AddDirArr[byMoveDir], &newSectorAround, byDeltaSectorNum, shNewSectorY, shNewSectorX);

	// 나의 시야에서 없어진 섹터에 내가 없어졋다고 알림
	dwPacketSize = MAKE_SC_DELETE_CHARACTER(dwID);
	for (BYTE i = 0; i < removeSectorAround.byCount; ++i)
		SendPacket_SectorOne(&removeSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	g_sb.Clear();

	// 나의 시야에서 생긴 섹터에 내가 생겻다고 알림
	dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(dwID, pClient->byViewDir, shX, shY, pClient->chHp);
	for (BYTE i = 0; i < newSectorAround.byCount; ++i)
		SendPacket_SectorOne(&newSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	g_sb.Clear();

	// 해당 플레이어가 움직이는 것이 전제이기 때문에 나의 시야에서 생긴 섹터에 내가 움직인다는 사실을 통지해야함.
	dwPacketSize = MAKE_SC_MOVE_START(dwID, byMoveDir, shX, shY);
	for (BYTE i = 0; i < newSectorAround.byCount; ++i)
		SendPacket_SectorOne(&newSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	g_sb.Clear();

	pClient->OldSector.shY = shOldSectorY;
	pClient->OldSector.shX = shOldSectorX;
	pClient->CurSector.shY = shNewSectorY;
	pClient->CurSector.shX = shNewSectorX;
}
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

	st_DirVector dirVector;
	st_Client* pClient;
	st_SECTOR_AROUND removeSectorAround;
	DWORD dwPacketSize;
	unsigned int* pClosedId;

	pClient = g_pClientManager->GetFirst();
	while (pClient)
	{
		// 접속이 끊길 예정인 사람 확인
		if (g_pDisconnectManager->IsDeleted(pClient->dwID))
			goto lb_next;

		// 죽을때 된사람 죽이기
		if (pClient->chHp <= 0)
		{
			g_pDisconnectManager->RegisterId(pClient->dwID);
			_LOG(dwLog_LEVEL_DEBUG,L"Delete Client : %u By Dead", pClient->dwID);
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

		_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", pClient->dwID, shOldX, shOldY, shNewX, shNewY);

		// 새로 이동한 위치검증
		if (!IsValidPos(shNewY, shNewY))
			goto lb_next;

		// 이동 후 섹터와 이전 섹터 계산
		shOldSectorY = shOldY / df_SECTOR_HEIGHT;
		shOldSectorX = shOldX / df_SECTOR_WIDTH;
		shNewSectorY = shNewY / df_SECTOR_HEIGHT;
		shNewSectorX = shNewX / df_SECTOR_WIDTH;

		// 이동햇고 유효한 위치에 있으나 이전위치와 같은 섹터에 존재하는 경우
		if (IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
			goto lb_next;

		ClientSectorUpdateAndPacket(pClient, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);
	lb_next:
		pClient = g_pClientManager->GetNext(pClient);
	}

	// 끊어야 할 사람 전부 끊기
	pClosedId = g_pDisconnectManager->GetFirst();
	while (pClosedId)
	{
		pClient = g_pClientManager->Find(*pClosedId);
		GetSectorAround(pClient->shY, pClient->shX, &removeSectorAround);

		// 삭제될 캐릭터 주위 섹터에 캐릭터 삭제 메시지 뿌리기
		dwPacketSize = MAKE_SC_DELETE_CHARACTER(*pClosedId);
		SendPacket_Around(pClient, &removeSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
		g_sb.Clear();

		// 섹터 리스트에서 삭제
		RemoveClientAtSector(pClient, pClient->CurSector.shY, pClient->CurSector.shX);
		
		g_pClientManager->removeClient(pClient);
		g_pSessionManager->Disconnect(*pClosedId);
		g_pDisconnectManager->removeID(pClosedId);
		pClosedId = g_pDisconnectManager->GetFirst();
	}
	

}