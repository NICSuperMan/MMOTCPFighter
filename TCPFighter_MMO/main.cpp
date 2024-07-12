#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <cstdlib>
#include <crtdbg.h>
#include <WS2spi.h>
#include <windows.h>
#include <time.h>
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

int main()
{
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
	int oldFrameTick = timeGetTime();
	bool isShutDown = false;
	while (!isShutDown)
	{
		//NetworkProc();
		//time = timeGetTime();
		//if (time - oldFrameTick >= TICK_PER_FRAME)
		//{
		//	Update();
		//	oldFrameTick += TICK_PER_FRAME;
		//}
		if (GetAsyncKeyState(0x35) & 0x8001)
		{
			isShutDown = true;
		}
	}
	//ClearSessionInfo();
	timeEndPeriod(1);
	return 0;
}
