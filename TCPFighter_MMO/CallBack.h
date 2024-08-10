#pragma once
#include "MiddleWare.h"
#include <Windows.h>
void CALLBACK CreatePlayer(void** ppOutClient, NETWORK_HANDLE handle, DWORD dwID);
void CALLBACK RemoveClient_IMPL(void* pClient);
BOOL CALLBACK PacketProc(void* pClient, BYTE byPacketType);


