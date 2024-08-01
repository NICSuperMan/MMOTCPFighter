#pragma once
#include "MemoryPool.h"
#include "LinkedList.h"
#include "HashTable.h"


class DisconnectManager
{
public:
	enum
	{
		MAX_ID = 7200
	};

	DisconnectManager();
	~DisconnectManager();
	BOOL Initialize();
	BOOL IsDeleted(unsigned id);
	void RegisterId(unsigned id);
	void removeID(unsigned* pID);
	unsigned* GetFirst();
	unsigned* GetNext(unsigned* pID);
private:
	struct st_ID
	{
		unsigned id;
		STATIC_HASH_METADATA shm;
	};
	CStaticHashTable hash_;
	SHORT dwIdNum_ = 0;
	MEMORYPOOL mp_;
};


extern DisconnectManager* g_pDisconnectManager;