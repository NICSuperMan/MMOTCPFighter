#include <windows.h>
#include "Constant.h"
#include "SCContents.h"


extern SerializeBuffer g_sb1;
extern SerializeBuffer g_sb2;


__forceinline DWORD MAKE_HEADER(BYTE byPacketSize, BYTE byPacketType, SerializeBuffer& sb)
{
	sb << (BYTE)dfPACKET_CODE << byPacketSize << byPacketType;
	return sizeof((BYTE)dfPACKET_CODE) + sizeof(byPacketSize) + sizeof(byPacketType);
}

DWORD MAKE_SC_CREATE_MY_CHARACTER(DWORD dwDestID, BYTE byDirection, Pos pos, CHAR chHP, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwDestID) + sizeof(byDirection) + sizeof(Pos::shX) + sizeof(Pos::shY) + sizeof(chHP);
#ifdef _DEBUG
	if ((int)dwDestID < 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_CREATE_MY_CHARACTER, sb);
	sb << dwDestID << byDirection << pos.shX << pos.shY << chHP;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_CREATE_OTHER_CHARACTER(DWORD dwCreateId, BYTE byDirection, Pos pos, CHAR chHP, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwCreateId) + sizeof(byDirection) + sizeof(Pos::shX) + sizeof(Pos::shY) + sizeof(chHP);
#ifdef _DEBUG
	if ((int)dwCreateId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_CREATE_OTHER_CHARACTER, sb);
	sb << dwCreateId << byDirection << pos.shX << pos.shY << chHP;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_DELETE_CHARACTER(DWORD dwDeleteId,SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwDeleteId);
#ifdef _DEBUG
	if ((int)dwDeleteId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_DELETE_CHARACTER, sb);
	sb << dwDeleteId;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_MOVE_START(DWORD dwStartId, BYTE byMoveDir, Pos pos, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwStartId) + sizeof(byMoveDir) + sizeof(pos.shX) + sizeof(pos.shY);
#ifdef _DEBUG
	if ((int)dwStartId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_MOVE_START, sb);
	sb << dwStartId << byMoveDir << pos.shX << pos.shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_MOVE_STOP(DWORD dwStopId, BYTE byViewDir,Pos pos,SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwStopId) + sizeof(byViewDir) + sizeof(pos.shX) + sizeof(pos.shY);
#ifdef _DEBUG
	if ((int)dwStopId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_MOVE_STOP, sb);
	sb << dwStopId << byViewDir << pos.shX << pos.shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_ATTACK(DWORD dwAttackerId, BYTE byViewDir, Pos AttackerPos, DWORD dwAttackNum, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwAttackerId) + sizeof(byViewDir) + sizeof(AttackerPos.shX) + sizeof(AttackerPos.shY);
#ifdef _DEBUG
	if ((int)dwAttackerId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize;
	switch (dwAttackNum)
	{
	case 1:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK1, sb);
		break;
	case 2:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK2, sb);
		break;
	case 3:
		dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ATTACK3, sb);
		break;
	default:
		__debugbreak();
		break;
	}
	sb << dwAttackerId << byViewDir << AttackerPos.shX << AttackerPos.shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_DAMAGE(DWORD dwAttackerId, DWORD dwVictimId, BYTE byVimctimHp, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwAttackerId) + sizeof(dwVictimId) + sizeof(byVimctimHp);
#ifdef _DEBUG
	if ((int)dwAttackerId < 0 || (int)dwVictimId < 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_DAMAGE, sb);
	sb << dwAttackerId << dwVictimId << byVimctimHp;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_SYNC(DWORD dwSyncId, Pos pos, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwSyncId) + sizeof(pos.shX) + sizeof(pos.shY);
#ifdef _DEBUG
	if ((int)dwSyncId< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_SYNC, sb);
	sb << dwSyncId << pos.shX << pos.shY;
	return dwSize + dwHeaderSize;
}

DWORD MAKE_SC_ECHO(DWORD dwTime, SerializeBuffer& sb)
{
	constexpr DWORD dwSize = sizeof(dwTime);
#ifdef _DEBUG
	if ((int)dwSize< 0)
		__debugbreak();
#endif
	DWORD dwHeaderSize = MAKE_HEADER(dwSize, dfPACKET_SC_ECHO, sb);
	sb << dwTime;
	return dwSize + dwHeaderSize;
}
