#define _CRTDBG_MAP_ALLOC
#include <time.h>
#include <crtdbg.h>
#include <windows.h>
#include <conio.h>
#include "Network.h"
#include "Logger.h"
#include <stdio.h>
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

BOOL g_bShutDown = FALSE;

void ServerControl(void)
{
	static bool bControlMode = FALSE;
	WCHAR ControlKey;

	// L : 컨트롤 Lock / U : 컨트롤 Unlock / Q : 서버종료

	if (_kbhit())
	{
		ControlKey = _getwch();

		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			bControlMode = TRUE;
			wprintf(L"Control Mode : Press Q - Quit \n");
			wprintf(L"Control Mode : Press L - Key Lock \n");
			wprintf(L"Increase Log Level : Press K\n");
			wprintf(L"Decrease Log Level : Press J\n");
		}

		// 키보드 제어 잠그기
		if ((L'l' == ControlKey || L'L' == ControlKey) && bControlMode)
		{
			wprintf(L"Control Lock..! Press U - Control Unlock\n");
			bControlMode = FALSE;
		}

		if ((L'q' == ControlKey || L'Q' == ControlKey) && bControlMode)
			g_bShutDown = TRUE;

		if (L'k' == ControlKey || L'K' == ControlKey && bControlMode)
			++g_iLogLevel;

		if (L'j' == ControlKey || L'J' == ControlKey && bControlMode)
			--g_iLogLevel;
	}

}

int main()
{
	int iOldFrameTick;
	int iFpsCheck;
	int iTime;
	int iFPS;
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
	iFPS = 0;
	iOldFrameTick = timeGetTime();
	iTime = iOldFrameTick;
	iFpsCheck = iOldFrameTick;

	while (!g_bShutDown)
	{
		NetworkProc();

		iTime = timeGetTime();
		if (iTime - iOldFrameTick >= TICK_PER_FRAME)
		{
			Update();
			iOldFrameTick += TICK_PER_FRAME;
			++iFPS;
		}
		if (iTime - iFpsCheck >= 1000)
		{
			iFpsCheck += 1000;
			//_LOG(dwLog_LEVEL_DEBUG, L"FPS : %d", iFPS);
			iFPS = 0;
		}
		ServerControl();
	}
	//ClearSessionInfo();
	timeEndPeriod(1);
	return 0;
}
