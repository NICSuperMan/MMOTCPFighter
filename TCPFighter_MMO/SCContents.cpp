#include <windows.h>
#include "SerializeBuffer.h"
#include "Constant.h"
#include "LinkedList.h"
#include "SCContents.h"
#include "Sector.h"
extern SerializeBuffer g_sb;

BOOL EnqPacketUnicast(const DWORD dwID, char* pPacket, const size_t packetSize);

__forceinline DWORD MAKE_HEADER(BYTE byPacketSize, BYTE byPacketType)
{
	g_sb << (BYTE)dfPACKET_CODE << byPacketSize << byPacketType;
	return sizeof((BYTE)dfPACKET_CODE) + sizeof(byPacketSize) + sizeof(byPacketType);
}

DWORD MAKE_SC_CREATE_MY_CHARACTER(DWORD dwDestID, BYTE byDirection, SHORT shX, SHORT shY, CHAR chHP)
{
	constexpr DWORD dwSize = sizeof(dwDestID) + sizeof(byDirection) + sizeof(shX) + sizeof(shY) + sizeof(chHP);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_CREATE_MY_CHARACTER);
	g_sb << dwDestID << byDirection << shX << shY << chHP;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_CREATE_OTHER_CHARACTER(DWORD dwCreateId, BYTE byDirection, SHORT shX, SHORT shY, CHAR chHP)
{
	constexpr DWORD dwSize = sizeof(dwCreateId) + sizeof(byDirection) + sizeof(shX) + sizeof(shY) + sizeof(chHP);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_CREATE_OTHER_CHARACTER);
	g_sb << dwCreateId << byDirection << shX << shY << chHP;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_DELETE_CHARACTER(DWORD dwDeleteId)
{
	constexpr DWORD dwSize = sizeof(dwDeleteId);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_DELETE_CHARACTER);
	g_sb << dwDeleteId;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_MOVE_START(DWORD dwStartId, BYTE byMoveDir, SHORT shX, SHORT shY)
{
	constexpr DWORD dwSize = sizeof(dwStartId) + sizeof(byMoveDir) + sizeof(shX) + sizeof(shY);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_MOVE_START);
	g_sb << dwStartId << byMoveDir << shX << shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_MOVE_STOP(DWORD dwStopId, BYTE byViewDir, SHORT shX, SHORT shY)
{
	constexpr DWORD dwSize = sizeof(dwStopId) + sizeof(byViewDir) + sizeof(shX) + sizeof(shY);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_MOVE_STOP);
	g_sb << dwStopId << byViewDir << shX << shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_ATTACK(DWORD dwAttackerId, BYTE byViewDir, SHORT shX, SHORT shY, DWORD dwAttackNum)
{
	constexpr DWORD dwSize = sizeof(dwAttackerId) + sizeof(byViewDir) + sizeof(shX) + sizeof(shY);
	DWORD dwHeaderSize;
	switch (dwAttackNum)
	{
	case 1:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK1);
		break;
	case 2:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK2);
		break;
	case 3:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK3);
		break;
	default:
		__debugbreak();
		break;
	}
	g_sb << dwAttackerId << byViewDir << shX << shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_DAMAGE(DWORD dwAttackerId, DWORD dwVictimId, BYTE byVimctimHp)
{
	constexpr DWORD dwSize = sizeof(dwAttackerId) + sizeof(dwVictimId) + sizeof(byVimctimHp);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_DAMAGE);
	g_sb << dwAttackerId << dwVictimId << byVimctimHp;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_SYNC(DWORD dwSyncId, SHORT shX, SHORT shY)
{
	constexpr DWORD dwSize = sizeof(dwSyncId) + sizeof(shX) + sizeof(shY);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_SYNC);
	g_sb << dwSyncId << shX << shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_ECHO(DWORD dwTime)
{
	constexpr DWORD dwSize = sizeof(dwTime);
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ECHO);
	g_sb << dwTime;
	return dwSize + dwHeaderSize;
}


