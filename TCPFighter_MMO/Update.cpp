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

// �̵��� OldSector���� ���� ĳ������ �̵������� �ش� �迭�� �ε����� �����ؼ� ���� ������ GetDeltaSector�� ����
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

// �̵��� NewSecotr���� ���� ĳ������ �̵������� �ش� �迭�� �ε����� �����ؼ� ���� ������ GetDeltaSctor�� ����
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

// ���ŵ� ���͸� �������� BaseSector�� OldSector����
// ���ο� ���͸� �������� BaseSector�� NewSector����
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

// X64 Calling Convetion �� ���� RTL 4������ �������Ϳ� �־��༭ ���� ��
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

	// ���� ����Ʈ�� ����
	AddClientAtSector(pClient, shNewSectorY, shNewSectorX);
	RemoveClientAtSector(pClient, shOldSectorY, shOldSectorX);

	// ����ǿ��� ������ ���Ϳ� ����ǿ� ������ ���͸� ����
	byDeltaSectorNum = (byMoveDir % 2 == 0) ? 3 : 5;
	GetDeltaSector(removeDirArr[byMoveDir], &removeSectorAround, byDeltaSectorNum, shOldSectorY, shOldSectorX);
	GetDeltaSector(AddDirArr[byMoveDir], &newSectorAround, byDeltaSectorNum, shNewSectorY, shNewSectorX);

	// ���� �þ߿��� ������ ���Ϳ� ���� ����ٰ� �˸�
	dwPacketSize = MAKE_SC_DELETE_CHARACTER(dwID);
	for (BYTE i = 0; i < removeSectorAround.byCount; ++i)
		SendPacket_SectorOne(&removeSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	g_sb.Clear();

	// ���� �þ߿��� ���� ���Ϳ� ���� ����ٰ� �˸�
	dwPacketSize = MAKE_SC_CREATE_OTHER_CHARACTER(dwID, pClient->byViewDir, shX, shY, pClient->chHp);
	for (BYTE i = 0; i < newSectorAround.byCount; ++i)
		SendPacket_SectorOne(&newSectorAround.Around[i], g_sb.GetBufferPtr(), dwPacketSize, nullptr);
	g_sb.Clear();

	// �ش� �÷��̾ �����̴� ���� �����̱� ������ ���� �þ߿��� ���� ���Ϳ� ���� �����δٴ� ����� �����ؾ���.
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
		// ������ ���� ������ ��� Ȯ��
		if (g_pDisconnectManager->IsDeleted(pClient->dwID))
			goto lb_next;

		// ������ �Ȼ�� ���̱�
		if (pClient->chHp <= 0)
		{
			g_pDisconnectManager->RegisterId(pClient->dwID);
			_LOG(dwLog_LEVEL_DEBUG,L"Delete Client : %u By Dead", pClient->dwID);
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

		_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u X : %d, Y : %d -> X : %d, Y : %d ", pClient->dwID, shOldX, shOldY, shNewX, shNewY);

		// ���� �̵��� ��ġ����
		if (!IsValidPos(shNewY, shNewY))
			goto lb_next;

		// �̵� �� ���Ϳ� ���� ���� ���
		shOldSectorY = shOldY / df_SECTOR_HEIGHT;
		shOldSectorX = shOldX / df_SECTOR_WIDTH;
		shNewSectorY = shNewY / df_SECTOR_HEIGHT;
		shNewSectorX = shNewX / df_SECTOR_WIDTH;

		// �̵��ް� ��ȿ�� ��ġ�� ������ ������ġ�� ���� ���Ϳ� �����ϴ� ���
		if (IsSameSector(shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX))
			goto lb_next;

		ClientSectorUpdateAndPacket(pClient, shOldSectorY, shOldSectorX, shNewSectorY, shNewSectorX);
	lb_next:
		pClient = g_pClientManager->GetNext(pClient);
	}

	// ����� �� ��� ���� ����
	pClosedId = g_pDisconnectManager->GetFirst();
	while (pClosedId)
	{
		pClient = g_pClientManager->Find(*pClosedId);
		GetSectorAround(pClient->shY, pClient->shX, &removeSectorAround);

		// ������ ĳ���� ���� ���Ϳ� ĳ���� ���� �޽��� �Ѹ���
		dwPacketSize = MAKE_SC_DELETE_CHARACTER(*pClosedId);
		SendPacket_Around(pClient, &removeSectorAround, g_sb.GetBufferPtr(), dwPacketSize, FALSE);
		g_sb.Clear();

		// ���� ����Ʈ���� ����
		RemoveClientAtSector(pClient, pClient->CurSector.shY, pClient->CurSector.shX);
		
		g_pClientManager->removeClient(pClient);
		g_pSessionManager->Disconnect(*pClosedId);
		g_pDisconnectManager->removeID(pClosedId);
		pClosedId = g_pDisconnectManager->GetFirst();
	}
	

}