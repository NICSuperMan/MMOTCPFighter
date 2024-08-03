#pragma once
#include "SerializeBuffer.h"
extern SerializeBuffer g_sb;
#pragma warning(disable : 4700)

class CStub
{
private:
	enum PacketNum
	{
		dfPACKET_CS_MOVE_START = 10,
		dfPACKET_CS_MOVE_STOP = 12,
		dfPACKET_CS_ATTACK1 = 20,
		dfPACKET_CS_ATTACK2 = 22,
		dfPACKET_CS_ATTACK3 = 24,
		dfPACKET_CS_ECHO = 252,
	};

public:
	BOOL PacketProc(DWORD dwFromId, BYTE byPacketType)
	{
		switch (byPacketType)
		{
		case dfPACKET_CS_MOVE_START:
			return CS_MOVE_START_RECV(dwFromId);
		case dfPACKET_CS_MOVE_STOP:
			return CS_MOVE_STOP_RECV(dwFromId);
		case dfPACKET_CS_ATTACK1:
			return CS_ATTACK1_RECV(dwFromId);
		case dfPACKET_CS_ATTACK2:
			return CS_ATTACK2_RECV(dwFromId);
		case dfPACKET_CS_ATTACK3:
			return CS_ATTACK3_RECV(dwFromId);
		case dfPACKET_CS_ECHO:
			return CS_ECHO_RECV(dwFromId);
		default:
			__debugbreak();
			return FALSE;
		}
	}

	BOOL CS_MOVE_START_RECV(DWORD dwFromId)
	{
		BYTE byMoveDir;
		SHORT shX;
		SHORT shY;
		g_sb >> byMoveDir >> shX >> shY;
		g_sb.Clear();
		CS_MOVE_START(dwFromId, byMoveDir, shX, shY);
		return TRUE;
	}

	virtual BOOL CS_MOVE_START(DWORD dwFromId, BYTE byMoveDir, SHORT shX, SHORT shY)
	{
		return FALSE;
	}

	BOOL CS_MOVE_STOP_RECV(DWORD dwFromId)
	{
		BYTE byViewDir;
		SHORT shX;
		SHORT shY;
		g_sb >> byViewDir >> shX >> shY;
		g_sb.Clear();
		CS_MOVE_STOP(dwFromId, byViewDir, shX, shY);
		return TRUE;
	}

	virtual BOOL CS_MOVE_STOP(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
	{
		return FALSE;
	}

	BOOL CS_ATTACK1_RECV(DWORD dwFromId)
	{
		BYTE byViewDir;
		SHORT shX;
		SHORT shY;
		g_sb >> byViewDir >> shX >> shY;
		g_sb.Clear();
		CS_ATTACK1(dwFromId, byViewDir, shX, shY);
		return TRUE;
	}

	virtual BOOL CS_ATTACK1(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
	{
		return FALSE;
	}

	BOOL CS_ATTACK2_RECV(DWORD dwFromId)
	{
		BYTE byViewDir;
		SHORT shX;
		SHORT shY;
		g_sb >> byViewDir >> shX >> shY;
		g_sb.Clear();
		CS_ATTACK2(dwFromId, byViewDir, shX, shY);
		return TRUE;
	}

	virtual BOOL CS_ATTACK2(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
	{
		return FALSE;
	}

	BOOL CS_ATTACK3_RECV(DWORD dwFromId)
	{
		BYTE byViewDir;
		SHORT shX;
		SHORT shY;
		g_sb >> byViewDir >> shX >> shY;
		g_sb.Clear();
		CS_ATTACK3(dwFromId, byViewDir, shX, shY);
		return TRUE;
	}

	virtual BOOL CS_ATTACK3(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY)
	{
		return FALSE;
	}


	BOOL CS_ECHO_RECV(DWORD dwFromId)
	{
		DWORD dwTime;
		g_sb >> dwTime;
		g_sb.Clear();
		CS_ECHO(dwFromId, dwTime);
		return TRUE;
	}

	virtual BOOL CS_ECHO(DWORD dwFromId,DWORD dwTime)
	{
		return FALSE;
	}
};

class CSProc : public CStub
{
	virtual BOOL CS_MOVE_START(DWORD dwFromId, BYTE byMoveDir, SHORT shX, SHORT shY);
	virtual BOOL CS_MOVE_STOP(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY);
	virtual BOOL CS_ATTACK1(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY);
	virtual BOOL CS_ATTACK2(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY);
	virtual BOOL CS_ATTACK3(DWORD dwFromId, BYTE byViewDir, SHORT shX, SHORT shY);
	virtual BOOL CS_ECHO(DWORD dwFromId, DWORD dwTime);
};

extern CStub* g_pCSProc;

#pragma warning(default : 4700)
