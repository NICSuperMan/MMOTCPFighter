#pragma once
#include "Network.h"
#include <unordered_map>
// 나중에 자료구조 바뀔수도잇으니 래핑함
class SessionPool
{
public:
	using MyType = std::unordered_map<unsigned, Session*>;
	using MyIter = MyType::iterator;
	enum
	{
		MAX_SESSION = 7200
	};
	SessionPool();
	~SessionPool();
	void RegisterSession(unsigned id, SOCKET socket);
	void removeSession(unsigned id);
	Session* GetFirst();
	Session* GetNext(Session* pSession);
	__forceinline unsigned GetSize()
	{
		return sessionMap.size();
	}
	Session* Find(unsigned id);

	__forceinline bool deleteSession(unsigned id)
	{
		MyIter iter = sessionMap.find(id);
		RetMemoryToPool(sessionMp, iter->second);
		sessionMap.erase(iter);
	}
	MyIter iter_;

private:
	MyType sessionMap;
	MEMORYPOOL sessionMp;
};

