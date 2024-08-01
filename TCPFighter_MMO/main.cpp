#define _CRTDBG_MAP_ALLOC
#include <time.h>
#include <crtdbg.h>
#include <windows.h>
#include "Network.h"
#pragma comment(lib,"Winmm.lib")
#ifdef _M_IX86
#ifdef _DEBUG
#pragma comment(lib,"MemoryPoolD.lib")
#else
#pragma comment(lib,"MemoryPool.lib")
#endif
#endif

#ifdef _M_X64
#ifdef _DEBUG
#pragma comment(lib,"MemoryPoolD64.lib")
#else
#pragma comment(lib,"MemoryPool64.lib")
#endif
#endif


#ifdef _DEBUG
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#endif

constexpr int TICK_PER_FRAME = 20;
constexpr int FRAME_PER_SECONDS = (1000) / TICK_PER_FRAME;
void Update();

int main()
{
	int iOldFrameTick;
	int iTime;
	BOOL bShutDown;
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, GetStdHandle(STD_OUTPUT_HANDLE));
#endif
	srand((unsigned)time(nullptr));
	timeBeginPeriod(1);
	if (!NetworkInitAndListen())
	{
		return 0;
	}
	iOldFrameTick = timeGetTime();
	bShutDown = FALSE;
	while (!bShutDown)
	{
		NetworkProc();
		iTime = timeGetTime();
		if (iTime - iOldFrameTick >= TICK_PER_FRAME)
		{
			Update();
			iOldFrameTick += TICK_PER_FRAME;
		}
		if (GetAsyncKeyState(0x35) & 0x8001)
		{
			bShutDown = TRUE;
		}
	}
	//ClearSessionInfo();
	timeEndPeriod(1);
	return 0;
}
