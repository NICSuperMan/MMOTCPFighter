#include "SessionPool.h"

SessionPool::SessionPool()
{
	sessionMp = CreateMemoryPool(sizeof(Session), MAX_SESSION);
}

SessionPool::~SessionPool()
{
	ReleaseMemoryPool(sessionMp);
}

void SessionPool::RegisterSession(unsigned id, SOCKET socket)
{
	Session* pNewSession = (Session*)AllocMemoryFromPool(sessionMp);
	new(pNewSession)Session{ socket,id };
	std::pair<unsigned, Session*> tempPair{ id,pNewSession };
	sessionMap.insert(tempPair);
}

void SessionPool::removeSession(unsigned id)
{
	auto&& iter = sessionMap.find(id);
	RetMemoryToPool(sessionMp, iter->second);
	sessionMap.erase(iter);
}

Session* SessionPool::GetFirst()
{
	MyIter iter = sessionMap.begin();
	if (iter == sessionMap.end())
	{
		return nullptr;
	}
	return iter->second;
}

Session* SessionPool::GetNext(Session* pSession)
{
	MyIter iter = sessionMap.find(pSession->id);
	++iter;
	if (iter == sessionMap.end())
	{
		return nullptr;
	}
	return iter->second;
}

Session* SessionPool::Find(unsigned id)
{
	MyIter iter = sessionMap.find(id);
	if (iter == sessionMap.end())
	{
		return nullptr;
	}
	return iter->second;
}
