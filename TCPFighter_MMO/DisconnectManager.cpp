#include <windows.h>
#include <new>
#include "LinkedList.h"
#include "HashTable.h"
#include "MemoryPool.h"
#include "DisconnectManager.h"
#pragma warning (disable : 4302)
#pragma warning (disable : 4311)
#pragma warning (disable : 4312)
static DWORD hashFunc(const void* pID)
{
	DWORD dwID = (DWORD)(pID);
	dwID %= DisconnectManager::MAX_ID;
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

DisconnectManager::DisconnectManager()
{
}

DisconnectManager::~DisconnectManager()
{
	st_ID* pSt_IDArr;
	DWORD dwCurIdNum;
	BOOL pbOutInSuf;
	int iLeakRet;

	pSt_IDArr = new st_ID[MAX_ID];
	dwCurIdNum = dwIdNum_;
	hash_.GetAllItems((void**)&pSt_IDArr, dwCurIdNum, &pbOutInSuf);
	for (DWORD i = 0; i < dwCurIdNum; ++i)
		RetMemoryToPool(mp_, pSt_IDArr + i);

	delete[] pSt_IDArr;

	iLeakRet = ReportLeak(mp_);
	if (iLeakRet > 0)
		__debugbreak();

	ReleaseMemoryPool(mp_);
}

BOOL DisconnectManager::Initialize()
{
	hash_.Initialize(MAX_ID, sizeof(unsigned), hashFunc, keyAssignFunc, keyCompareFunc, offsetof(st_ID, shm));
	mp_ = CreateMemoryPool(sizeof(st_ID), MAX_ID);
	return TRUE;
}

BOOL DisconnectManager::IsDeleted(unsigned id)
{
	st_ID* pFindRet;
	DWORD dwFindRet;
	BOOL bRet;
	dwFindRet = hash_.Find((void**)&pFindRet, 1, (const void*)id, sizeof(id));
	if (dwFindRet > 1)
	{
		__debugbreak();
		return FALSE;
	}
	else if (dwFindRet == 1)
		return TRUE;
	else
		return FALSE;
}
void DisconnectManager::RegisterId(unsigned id)
{
	st_ID* pID = (st_ID*)AllocMemoryFromPool(mp_);
	new(pID)st_ID{};
	st_ID* pDebug;
	DWORD dwFindRet;
	pID->id = id;
	dwFindRet = hash_.Find((void**)&pDebug, 1, (const void*)id, sizeof(id));
	if (dwFindRet != 0)
		__debugbreak();
	hash_.Insert((const void*)pID, (const void*)id, sizeof(st_ID::id));
	++dwIdNum_;
}

void DisconnectManager::removeID(unsigned* pID)
{
	st_ID* pSt_ID;
	pSt_ID = (st_ID*)(pID - offsetof(st_ID, id));
	hash_.Delete((const void*)pSt_ID);
	--dwIdNum_;
	RetMemoryToPool(mp_, pSt_ID);
}

unsigned* DisconnectManager::GetFirst()
{
	st_ID* pFindRet;
	pFindRet = (st_ID*)hash_.GetFirst();
	if (!pFindRet)
	{
		return nullptr;
	}
	return &(pFindRet->id);
}

unsigned* DisconnectManager::GetNext(unsigned* pID)
{
	st_ID* pCur;
	st_ID* pNext;
	pCur = (st_ID*)(pID - offsetof(st_ID, id));
	pNext = (st_ID*)hash_.GetNext((const void*)pCur);
	if (!pNext)
	{
		return nullptr;
	}
	return &(pNext->id);
}

#pragma warning (default : 4312)
#pragma warning (default : 4311)
#pragma warning (default : 4302)

