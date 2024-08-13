#include "Client.h"
#include "Constant.h"
#include "Logger.h"
#include "MemoryPool.h"
#include "SCContents.h"
#include "Sector.h"
#include "CSCContents.h"



MEMORYPOOL g_ClientMemoryPool;

extern SerializeBuffer g_sb1;
extern SerializeBuffer g_sb2;

void CALLBACK CreatePlayer(void** ppOutClient, NETWORK_HANDLE handle, DWORD dwID)
{
	Pos ClientPos;
	SectorPos sector;
	DWORD dwMSCMCSize;
	AroundInfo* pAroundInfo;
	DWORD dwMSCOCNewToOtherSize;
	DWORD dwMSCOCOtherToNewSize;
	DWORD dwMSMSSize;
	st_Client* pClient;

	ClientPos.shY = (rand() % (dfRANGE_MOVE_BOTTOM - 1)) + 1;
	ClientPos.shX = (rand() % (dfRANGE_MOVE_RIGHT - 1)) + 1;
	//ClientPos = { 300,300 };
	sector.shY = ClientPos.shY / df_SECTOR_HEIGHT;
	sector.shX = ClientPos.shX / df_SECTOR_WIDTH;

	pClient = (st_Client*)AllocMemoryFromPool(g_ClientMemoryPool);
	pClient->dwID = dwID;
	pClient->handle = handle;
	pClient->pos = ClientPos;
	pClient->byViewDir = dfPACKET_MOVE_DIR_LL;
	pClient->byMoveDir = dfPACKET_MOVE_DIR_NOMOVE;
	pClient->chHp = INIT_HP;
	pClient->CurSector = sector;

#ifdef SYNC
	pClient->IsAlreadyStart = FALSE;
#endif

	// ������ Ŭ���̾�Ʈ���� �ڱ� �ڽ��� �����Ǿ����� �˸���
	//_LOG(dwLog_LEVEL_DEBUG, L"Notify New Characters of his Location");
	dwMSCMCSize = MAKE_SC_CREATE_MY_CHARACTER(dwID, dfPACKET_MOVE_DIR_LL, ClientPos, INIT_HP, g_sb1);
	EnqPacketRB(pClient, g_sb1.GetBufferPtr(), dwMSCMCSize);
	g_sb1.Clear();

	// Ŭ���̾�Ʈ�� ������ ��ġ�� ���� 9�� ���Ϳ� �����ϴ� ������������ ���� Ŭ���̾�Ʈ���� ������������
	// ���� ���͹迭�� ����Ʈ�� �߰����� �ʾѱ⿡ GAVC�� �ι�°���ڴ� NULL�̵� �ƴϵ� �������
	pAroundInfo = GetAroundValidClient(sector, pClient);

	// ���� 9������ Ŭ���̾�Ʈ�鿡�� �ڱ� �ڽ��� �����Ǿ����� �˸��� �޽��� �����

	if (pAroundInfo->CI.dwNum <= 0)
		goto lb_Add;

	//_LOG(dwLog_LEVEL_DEBUG, L"Notify clients in the surrounding 9 sectors of the creation of a new client");
	dwMSCOCNewToOtherSize = MAKE_SC_CREATE_OTHER_CHARACTER(dwID, dfPACKET_MOVE_DIR_LL, ClientPos, INIT_HP, g_sb1);
	dwMSCOCOtherToNewSize = 0;
	dwMSMSSize = 0;

	// ���� 9�� ������ Ŭ���̾�Ʈ���� �ڽ��� ��ġ�� ���λ��� Ŭ���̾�Ʈ���� �˸��� ���߿� �����̴� Ŭ���̾�Ʈ�� �����δٰ� ����
	for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
	{
		st_Client* pOtherClient = pAroundInfo->CI.cArr[i];
		// ������ ����� ��� �ٲ�� ������ ���������� Clear��
		EnqPacketRB(pOtherClient, g_sb1.GetBufferPtr(), dwMSCOCNewToOtherSize);
		g_sb1.Clear();

		// �Ѹ����� ����ؼ� �����⶧���� Clear����.
		dwMSCOCOtherToNewSize += MAKE_SC_CREATE_OTHER_CHARACTER(pOtherClient->dwID, pOtherClient->byViewDir, pOtherClient->pos, pOtherClient->chHp, g_sb2);
		if (pOtherClient->byMoveDir != dfPACKET_MOVE_DIR_NOMOVE)
			dwMSMSSize += MAKE_SC_MOVE_START(pOtherClient->dwID, pOtherClient->byMoveDir, pOtherClient->pos, g_sb2);
	}

	EnqPacketRB(pClient, g_sb2.GetBufferPtr(), dwMSCOCOtherToNewSize + dwMSMSSize);
	g_sb2.Clear();

lb_Add:
	// Sector�迭���մ� ����Ʈ�� ���
	AddClientAtSector(pClient, sector);


	// Network Layer���� Client����ü�� �ּ� �˷��ֱ�
	*ppOutClient = (void*)pClient;
}

void CALLBACK RemoveClient_IMPL(void* pClient)
{
	DWORD dwMSDCSize;
	st_Client* pRemoveClient = (st_Client*)pClient;
	SectorPos curSector;
	CalcSector(&curSector, pRemoveClient->pos);

	AroundInfo* pAroundInfo = GetAroundValidClient(curSector, nullptr);
	dwMSDCSize = MAKE_SC_DELETE_CHARACTER(pRemoveClient->dwID, g_sb1);
	for (DWORD i = 0; i < pAroundInfo->CI.dwNum; ++i)
	{
		EnqPacketRB(pAroundInfo->CI.cArr[i], g_sb1.GetBufferPtr(), dwMSDCSize);
		//_LOG(dwLog_LEVEL_DEBUG, L"Client ID %u Send Delete Character To %u  ( X : %u, Y : %u)", pRemoveClient->dwID, pAroundInfo->CI.cArr[i]->dwID,
		//	pAroundInfo->CI.cArr[i]->CurSector.shX, pAroundInfo->CI.cArr[i]->CurSector.shY);
	}
	g_sb1.Clear();

	RemoveClientAtSector(pRemoveClient, curSector);

	RetMemoryToPool(g_ClientMemoryPool, pClient);
	//_LOG(dwLog_LEVEL_DEBUG, L"Client ID : %u Disconnected", pRemoveClient->dwID);
}

BOOL CALLBACK PacketProc(void* pClient, BYTE byPacketType)
{
	switch (byPacketType)
	{
	case dfPACKET_CS_MOVE_START:
		return CS_MOVE_START_RECV((st_Client*)pClient);
	case dfPACKET_CS_MOVE_STOP:
		return CS_MOVE_STOP_RECV((st_Client*)pClient);
	case dfPACKET_CS_ATTACK1:
		return CS_ATTACK1_RECV((st_Client*)pClient);
	case dfPACKET_CS_ATTACK2:
		return CS_ATTACK2_RECV((st_Client*)pClient);
	case dfPACKET_CS_ATTACK3:
		return CS_ATTACK3_RECV((st_Client*)pClient);
	case dfPACKET_CS_ECHO:
		return CS_ECHO_RECV((st_Client*)pClient);
	default:
		__debugbreak();
		return FALSE;
	}
}

