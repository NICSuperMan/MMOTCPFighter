#define _CRTDBG_MAP_ALLOC
#include <time.h>
#include <crtdbg.h>
#include <windows.h>
#include <conio.h>
#include "Network.h"
#include "Logger.h"
#include <stdio.h>


#include "MiddleWare.h"
#include "Callback.h"

#include "Client.h"
#include "MemoryPool.h"

#include "Constant.h"

#include "process.h"



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

constexpr int TICK_PER_FRAME = 40;
constexpr int FRAME_PER_SECONDS = (1000) / TICK_PER_FRAME;
void Update();
void ClearSessionInfo();

BOOL g_bShutDown = FALSE;
int g_iSyncCount = 0;
int g_iDisconCount = 0;
int g_iDisConByRBFool = 0;
int g_iDisConByTimeOut = 0;

extern MEMORYPOOL g_ClientMemoryPool;
extern DWORD g_dwSessionNum;

int g_iOldFrameTick;
int g_iFpsCheck;
int g_iTime;
int g_iFPS;
int g_iNetworkLoop;
int g_iFirst;

ULONGLONG g_fpsCheck;


unsigned __stdcall ServerControl(void* pParam)
{
	static bool bControlMode = TRUE;
	WCHAR ControlKey;

	// L : 컨트롤 Lock / U : 컨트롤 Unlock / Q : 서버종료
	while (!g_bShutDown)
	{
		if (!_kbhit())
			continue;

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
		{
			LOG_LEVEL level = INCREASE_LOG_LEVEL();
			wprintf(L"Log Level : %d\n", level);
		}

		if (L'j' == ControlKey || L'J' == ControlKey && bControlMode)
		{
			LOG_LEVEL level = DECREASE_LOG_LEVEL();
			wprintf(L"Log Level : %d\n", level);
		}

		if (L'D' == ControlKey || L'd' == ControlKey && bControlMode)
		{
			FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUsertTime;
			FILETIME ftCurTime;
			GetProcessTimes(GetCurrentProcess(), &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUsertTime);
			GetSystemTimeAsFileTime(&ftCurTime);

			ULARGE_INTEGER start, now;
			start.LowPart = ftCreationTime.dwLowDateTime;
			start.HighPart = ftCreationTime.dwHighDateTime;
			now.LowPart = ftCurTime.dwLowDateTime;
			now.HighPart = ftCurTime.dwHighDateTime;
 
			ULONGLONG ullElapsedSecond = (now.QuadPart - start.QuadPart) / 10000 / 1000;
			ULONGLONG ullElapsedMin = ullElapsedSecond / 60;
			ullElapsedSecond %= 60;

			ULONGLONG ullElapsedHour = ullElapsedMin / 60;
			ullElapsedMin %= 60;

			ULONGLONG ullElapsedDay = ullElapsedHour / 24;
			ullElapsedHour %= 24;

			printf("-----------------------------------------------------\n");
			printf("Elapsed Time : %02lluD-%02lluH-%02lluMin-%02lluSec\n", ullElapsedDay, ullElapsedHour, ullElapsedMin, ullElapsedSecond);
			printf("Client Number : %u\n", g_dwSessionNum);
			printf("Disconnect Count : %d\n", g_iDisconCount);
			printf("Disconnect By TimeOut : %d\n", g_iDisConByTimeOut);
			printf("Didsconnected By RingBuffer FOOL : %u\n", g_iDisConByRBFool);
			printf("-----------------------------------------------------\n");
		}

	}
	return 0;
}


int main()
{
	HANDLE hMonitoring;
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, GetStdHandle(STD_OUTPUT_HANDLE));
	srand((unsigned)time(nullptr));

	InitLogger(L"LOG");
	timeBeginPeriod(1);

	LOG(L"SERVERINIT", ERR, TEXTFILE, L"\n\nSERVER Start!!!");
	if (!NetworkInitAndListen())
		goto lb_NetworkInitAndListenFail;

	hMonitoring = (HANDLE)_beginthreadex(nullptr, 0, ServerControl, nullptr, 0, nullptr);
	if (hMonitoring == INVALID_HANDLE_VALUE)
	{
		LOG(L"SERVERINIT", ERR, TEXTFILE, L"Monitoring Thread _beginthreadex() func failed");
		goto lb_monitorThreadFail;
	}
	LOG(L"SERVERINIT", ERR, TEXTFILE, L"Monitoring Thread Created!");

	InitializeMiddleWare(CreatePlayer, RemoveClient_IMPL, PacketProc, offsetof(st_Client, handle));
	g_ClientMemoryPool = CreateMemoryPool(sizeof(st_Client), MAX_SESSION);

	g_iNetworkLoop = 0;
	g_iFirst = timeGetTime();
	g_iOldFrameTick = g_iFirst;
	g_iTime = g_iOldFrameTick;
	g_iFPS = 0;
	g_iFpsCheck = g_iOldFrameTick;

	while (!g_bShutDown)
	{
		NetworkProc();
		++g_iNetworkLoop;
		g_iTime = timeGetTime();
		if (g_iTime - g_iOldFrameTick >= TICK_PER_FRAME)
		{
			Update();
			g_iOldFrameTick = g_iTime - ((g_iTime - g_iFirst) % TICK_PER_FRAME);
			++g_iFPS;
		}
		if (g_iTime - g_iFpsCheck >= 1000)
		{
			g_iFpsCheck += 1000;
			printf("-----------------------------------------------------\n");
			printf("FPS : %d\n", g_iFPS);
			printf("Network Loop Num: %u\n", g_iNetworkLoop);
			printf("SyncCount : %d\n", g_iSyncCount);
			printf("-----------------------------------------------------\n");
			g_iNetworkLoop = 0;
			g_iFPS = 0;
		}
	}
	WaitForSingleObject(hMonitoring, INFINITE);
	ClearSessionInfo();
	lb_monitorThreadFail:
	lb_NetworkInitAndListenFail:
	LOG(L"TERMINATE", SYSTEM, TEXTFILE, L"Server Terminate SyncCount : %d\n\n", g_iSyncCount);
	ClearLogger();
	return 0;
}
