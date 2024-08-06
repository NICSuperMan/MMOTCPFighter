#include <Windows.h>
#include <new>
#include "Constant.h"
#include "RingBuffer.h"
#include "ClientManager.h"
#include <time.h>

#pragma warning(disable : 4302)
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)
static DWORD hashFunc(const void* pID)
{
	DWORD dwID = (DWORD)(pID);
	dwID %= ClientManager::MAX_CLIENT;
	return dwID;
}

static void keyAssignFunc(void** ppKeyInHash, const void* pKey)
{
	DWORD dwKey;
	DWORD* pKeyInHash;
	dwKey = (DWORD)pKey;
	pKeyInHash = (DWORD*)ppKeyInHash;
	*pKeyInHash = dwKey;
}

static BOOL keyCompareFunc(const void* pKeyInHash, const void* pKey)
{
	DWORD dwKey;
	DWORD dwKeyInHash;
	BOOL bRet;
	dwKey = (DWORD)pKey;
	dwKeyInHash = (DWORD)pKeyInHash;
	bRet = (pKey == pKeyInHash);
	return bRet;
}

ClientManager::ClientManager()
{
	srand((unsigned)time(nullptr));
}

ClientManager::~ClientManager()
{
	st_Client* pClientArr;
	DWORD dwItemNum;
	BOOL pbOutInSuf;
	int iLeakRet;

	dwItemNum = hash_.GetItemNum();
	pClientArr = new st_Client[dwItemNum];

	// 소멸자 호출 이전에 존재하는 모든 클라이언트를 순회하며 removeSession을 호출햇어야 한다.
	if (dwClientNum_ > 0)
	{
		__debugbreak();
	}
	hash_.GetAllItems((void**)&pClientArr, dwItemNum, &pbOutInSuf);
	for (DWORD i = 0; i < dwItemNum; ++i)
		RetMemoryToPool(mp_, pClientArr + i);

	iLeakRet = ReportLeak(mp_);
	if (iLeakRet > 0)
	{
		__debugbreak();
	}
	delete[] pClientArr;
	ReleaseMemoryPool(mp_);
}

BOOL ClientManager::Initialize()
{
	hash_.Initialize(MAX_CLIENT, sizeof(st_Client::dwID), hashFunc, keyAssignFunc, keyCompareFunc, offsetof(st_Client, shm));
	mp_ = CreateMemoryPool(sizeof(st_Client), MAX_CLIENT);
	return TRUE;
}

void ClientManager::RegisterClient(DWORD id, st_Session* pNewSession, st_Client** ppOut)
{
	st_Client* pNewClient = (st_Client*)AllocMemoryFromPool(mp_);
	new(pNewClient)st_Client{};
	
	pNewClient->dwID = id;
	pNewClient->dwAction = NOMOVE;
	pNewClient->dwLastRecvTime = timeGetTime();
	//pNewClient->shX = INIT_POS_X;
	//pNewClient->shY = INIT_POS_Y;
	pNewClient->shX = (rand() % (dfRANGE_MOVE_RIGHT - 1)) + 1; // 1 ~ 6399
	pNewClient->shY = (rand() % (dfRANGE_MOVE_BOTTOM- 1)) + 1;
	pNewClient->byViewDir = INIT_DIR;
	pNewClient->byMoveDir = dfPACKET_MOVE_DIR_NOMOVE;
	pNewClient->chHp = INIT_HP;

	*ppOut = pNewClient;

	st_Client* pDebug;
	DWORD dwFindRet;
	dwFindRet = hash_.Find((void**)&pDebug, 1, (const void*)id, sizeof(id));
	//이미잇음
	if (dwFindRet > 0)
		__debugbreak();

	hash_.Insert((const void*)pNewClient, (const void*)id, sizeof(st_Client::dwID));
	++dwClientNum_;
}

void ClientManager::removeClient(DWORD id)
{
	st_Client* pClient;
	DWORD dwFindRet;
	dwFindRet = hash_.Find((void**)&pClient, 1, (const void*)id, sizeof(st_Client::dwID));
	// 여러개잇으며
	if (dwFindRet > 2)
		__debugbreak();
	// 제거해야하는데 이미 제거되엇으면, 즉 중복제거이면
	if (dwFindRet <= 0)
	{
		__debugbreak();
		return;
	}
	removeClient(pClient);
}

void ClientManager::removeClient(st_Client* pClient)
{
	hash_.Delete((const void*)pClient);
	--dwClientNum_;
	RetMemoryToPool(mp_, pClient);
}

st_Client* ClientManager::GetFirst()
{
	st_Client* pRet;
	pRet = (st_Client*)hash_.GetFirst();
	return pRet;
}

st_Client* ClientManager::GetNext(const st_Client* pClient)
{
	st_Client* pRet;
	pRet = (st_Client*)hash_.GetNext((const void*)pClient);
	return pRet;
}

st_Client* ClientManager::Find(DWORD dwID)
{
	st_Client* pRet;
	DWORD dwFindRet;

	dwFindRet = hash_.Find((void**)&pRet, 2, (const void**)dwID, sizeof(st_Client::dwID));
	// 여러개잇음
	if (dwFindRet > 1)
		__debugbreak();
	if (!dwFindRet)
	{
		__debugbreak();
		return nullptr;
	}

	return pRet;
}
#pragma warning(default : 4302)
#pragma warning(default : 4311)
#pragma warning(default : 4312)
