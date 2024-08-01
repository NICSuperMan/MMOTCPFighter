#pragma once
#include "Session.h"
#include "LinkedList.h"
#include "HashTable.h"
#include "MemoryPool.h"
#include "Client.h"
class ClientManager
{
public:
	enum
	{
		MAX_CLIENT = 7200
	};
	ClientManager();
	~ClientManager();
	BOOL Initialize();
	void RegisterClient(DWORD dwID, st_Session* pNewSession,st_Client** ppOut);
	void removeClient(DWORD dwID);
	void removeClient(st_Client* pClientInfo);
	st_Client* GetFirst();
	st_Client* GetNext(const st_Client* pClient);
	st_Client* Find(DWORD dwID);

private:
	CStaticHashTable hash_;
	DWORD dwClientNum_ = 0;
	MEMORYPOOL mp_;
};

extern ClientManager* g_pClientManager;


