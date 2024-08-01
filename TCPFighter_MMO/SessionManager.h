#pragma once
// 나중에 자료구조 바뀔수도잇으니 래핑함
class SessionManager
{
public:
	enum
	{
		MAX_SESSION = 7200
	};
	SessionManager();
	~SessionManager();
	BOOL Initialize();
	void RegisterSession(unsigned id, SOCKET socket, st_Session** ppOut);
	void removeSession(unsigned id);
	void removeSession(st_Session* pSession);
	st_Session* GetFirst();
	st_Session* GetNext(const st_Session* pSession);
	st_Session* Find(unsigned id);
	BOOL Disconnect(unsigned id);
	BOOL IsFull();

private:
	CStaticHashTable hash_;
	SHORT dwUserNum_ = 0;
	MEMORYPOOL mp_;
};

extern SessionManager* g_pSessionManager;

