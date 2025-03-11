#pragma once
#include <Windows.h>

constexpr DWORD VALID = 1;
constexpr DWORD INVALID = 0;

extern const void* pReserved;
extern void* pReservedForDiscon1;
extern const void* pReservedForDiscon2;

extern const DWORD dwMagicNumber;
extern const DWORD dwReservedForState;
extern const DWORD dwReservedForLastRecvTime;


typedef DWORD NETWORK_HANDLE;

typedef void(CALLBACK* REGISTER_CLIENT)(void** ppOutClient, NETWORK_HANDLE handle, DWORD dwID);
typedef void(CALLBACK* REMOVE_CLIENT)(void* pClient);
typedef BOOL(CALLBACK* PACKET_PROC)(void* pClient, BYTE byPacketType);

 
extern REGISTER_CLIENT RegisterClient;
extern REMOVE_CLIENT  RemoveClient;
extern PACKET_PROC packetProc;



void InitializeMiddleWare(REGISTER_CLIENT RegisterPlayer, REMOVE_CLIENT RemovePlayer, PACKET_PROC packetProcedure, DWORD HandleOffsetOwnByClient);
BOOL EnqPacketRB(const void* pClient, char* pPacket, const DWORD packetSize);
BOOL GetAllValidClient(DWORD* pOutUserNum, void** ppOutClientArr);


__forceinline DWORD GetLastRecvTime(NETWORK_HANDLE handle)
{
	DWORD dwRet = *(DWORD*)(*((char**)pReserved + handle) + dwReservedForLastRecvTime);
	return dwRet;
}


__forceinline BOOL IsNetworkStateInValid(NETWORK_HANDLE handle)
{
	return *(*((char**)pReserved + handle) + dwReservedForState) == INVALID;
}

__forceinline void AlertSessionOfClientInvalid(NETWORK_HANDLE handle)
{
	*(*((char**)pReserved + handle) + dwReservedForState) = INVALID;
	*((char**)pReservedForDiscon2 + (*(DWORD*)pReservedForDiscon1)) = *((char**)pReserved + handle);
	++(*(DWORD*)pReservedForDiscon1);
}





