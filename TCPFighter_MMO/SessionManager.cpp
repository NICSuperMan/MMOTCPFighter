#include <Windows.h>
#include <new>
#include "RingBuffer.h"
#include "LinkedList.h"
#include "HashTable.h"
#include "Session.h"
#include "MemoryPool.h"
#include "SessionManager.h"
#include "Network.h"
#include "LinkedList.h"

#pragma warning(disable : 4302)
#pragma warning(disable : 4311)
#pragma warning(disable : 4312)

static DWORD hashFunc(const void* pID)
{
	DWORD dwID = (DWORD)(pID);
	dwID %= SessionManager::MAX_SESSION;
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
SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
	st_Session* pSessionArr[MAX_SESSION];
	DWORD dwCurSession;
	BOOL pbOutInSuf;
	int iLeakRet;

	dwCurSession = hash_.GetItemNum();
	hash_.GetAllItems((void**)&pSessionArr, dwCurSession, &pbOutInSuf);
	for (DWORD i = 0; i < dwCurSession; ++i)
	{
		RetMemoryToPool(mp_, pSessionArr[i]);
	}

	iLeakRet = ReportLeak(mp_);
	if (iLeakRet > 0)
		__debugbreak();

	ReleaseMemoryPool(mp_);
}

void SessionManager::RegisterSession(unsigned id, SOCKET socket, st_Session** ppOut)
{
	st_Session* pNewSession = (st_Session*)AllocMemoryFromPool(mp_);
	new(pNewSession)st_Session{ socket,id };
	*ppOut = pNewSession;
	hash_.Insert((const void*)pNewSession, (const void*)id, sizeof(st_Session::id));
	++dwUserNum_;
}

void SessionManager::removeSession(unsigned id)
{
	st_Session* pSession;
	DWORD dwFindRet;
	dwFindRet = hash_.Find((void**)&pSession, 1, (const void*)id, sizeof(id));
	if (!dwFindRet)
	{
		__debugbreak();
		return;
	}
	removeSession(pSession);
}

void SessionManager::removeSession(st_Session* pSession)
{
	hash_.Delete((const void*)pSession);
	--dwUserNum_;
	closesocket(pSession->id);
	RetMemoryToPool(mp_, pSession);
}

BOOL SessionManager::Initialize()
{
	hash_.Initialize(MAX_SESSION, sizeof(st_Session::id), hashFunc, keyAssignFunc, keyCompareFunc, offsetof(st_Session, shm));
	mp_ = CreateMemoryPool(sizeof(st_Session), MAX_SESSION);
	return TRUE;
}

st_Session* SessionManager::GetFirst()
{
	st_Session* pRet;
	pRet = (st_Session*)hash_.GetFirst();
	return pRet;
}

st_Session* SessionManager::GetNext(const st_Session* pSession)
{
	st_Session* pRet;
	pRet = (st_Session*)hash_.GetNext((const void*)pSession);
	return pRet;
}

st_Session* SessionManager::Find(unsigned id)
{
	st_Session* pRet;
	DWORD dwFindRet;
	
	dwFindRet = hash_.Find((void**)&pRet, 1, (const void**)(id), sizeof(st_Session::id));
	if (!dwFindRet)
	{
		return nullptr;
	}
	return pRet;
}

#pragma warning(disable : 4700)
BOOL SessionManager::Disconnect(unsigned id)
{
	st_Session* pSession;
	DWORD dwFindRet;
	dwFindRet = hash_.Find((void**)&pSession, 1, (const void**)(id), sizeof(pSession->id));
	if (!dwFindRet)
	{
		__debugbreak();
		return FALSE;
	}
	hash_.Delete(pSession);
	closesocket(pSession->clientSock);
	RetMemoryToPool(mp_, pSession);
	return TRUE;
}
#pragma warning(default : 4700)

BOOL SessionManager::IsFull()
{
	BOOL bRet;
	bRet = (dwUserNum_ >= MAX_SESSION);
	return bRet;
}
