#include <strsafe.h>
#include <windows.h>
#include <memory.h>

#include <crtdbg.h>

enum LOG_LEVEL { DEBUG, SYSTEM, ERR };


#define LogTimeLen 24
static DWORD g_dwLogCount = 0;
static LOG_LEVEL g_logLevel = DEBUG;

#define LOGSTR 10000;
#define CONSOLE 3
#define TEXTFILE 4 

__declspec(thread) WCHAR* g_pszFolerPath;
__declspec(thread) WCHAR* g_pLogBufForFileConsole;
__declspec(thread) WCHAR* g_pLogBufForTempBuffer;

#define LOCAL_LOG_BUF_SIZE 3000
int g_CurSize = LOCAL_LOG_BUF_SIZE;
SRWLOCK g_srwForFILEIO;


void LOG(const WCHAR* szHead, LOG_LEVEL LogLevel, CHAR OUTPUT, const WCHAR* szStringFormat, ...);

__forceinline void ExceptLogBufInSuf()
{
	HeapFree(GetProcessHeap(), 0, g_pLogBufForFileConsole);
	g_pLogBufForFileConsole = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_CurSize * 2 * sizeof(WCHAR));
	g_CurSize *= 2;
	LOG(L"SYSTEM", ERR, TEXTFILE, L"LogBUFSIZE INSUFFICIENT ALLOCATE %d -> %dBytes", g_CurSize / 2, g_CurSize);
}


__forceinline void ExceptLogBufInSufForTempBuffer()
{
	g_pLogBufForFileConsole = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_CurSize * 2 * sizeof(WCHAR));
	g_CurSize *= 2;
	LOG(L"SYSTEM", ERR, TEXTFILE, L"LogBUFSIZE INSUFFICIENT ALLOCATE %d -> %dBytes", g_CurSize / 2, g_CurSize);
}

void TempBufferToFile(const WCHAR* szHead, const WCHAR* Buffer)
{
	// ���� �� ����
	SYSTEMTIME st;
	GetLocalTime(&st);

	AcquireSRWLockExclusive(&g_srwForFILEIO);
	FILE* pFile;
	WCHAR FilePath[MAX_PATH];
	StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolerPath, st.wYear, st.wMonth, szHead);
	_wfopen_s(&pFile, FilePath, L"a");
	fputws(Buffer, pFile);
	fclose(pFile);
	ReleaseSRWLockExclusive(&g_srwForFILEIO);
}

void TempBufferToFileBinary(const WCHAR* szHead, const CHAR* Buffer, SIZE_T size)
{
	// ���� �� ����
	SYSTEMTIME st;
	GetLocalTime(&st);

	AcquireSRWLockExclusive(&g_srwForFILEIO);
	FILE* pFile;
	WCHAR FilePath[MAX_PATH];
	StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolerPath, st.wYear, st.wMonth, szHead);
	_wfopen_s(&pFile, FilePath, L"a");
	fwprintf(pFile, L"%02X %02X %02X %02X %02X %02X %02X %02X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7]);
	fclose(pFile);
	ReleaseSRWLockExclusive(&g_srwForFILEIO);
}

void LOG(const WCHAR* szHead, LOG_LEVEL LogLevel, CHAR OUTPUT, const WCHAR* szStringFormat, ...)
{
	LOG_LEVEL level = (LOG_LEVEL)InterlockedExchange((LONG*)&g_logLevel, g_logLevel);
	if (level > LogLevel)
		return;

	while (1)
	{
		//[BATTLE]  ���� ���ڿ� ���̱�
		StringCchPrintf(g_pLogBufForFileConsole, g_CurSize, L"[%s]  ", szHead);

		SYSTEMTIME st;
		GetLocalTime(&st);

		// �����Ͻú��� / 
		size_t len = wcslen(g_pLogBufForFileConsole);
		HRESULT hResult = StringCchPrintf(g_pLogBufForFileConsole + len, g_CurSize - len, L"[%04d-%02d-%02d %02d:%02d:%02d / ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}

		// �α׷��� ���ϱ�
		WCHAR LogLevelStr[10];
		switch (LogLevel)
		{
		case LOG_LEVEL::DEBUG:
			memcpy(LogLevelStr, L"DEBUG", sizeof(L"DEBUG"));
			break;
		case LOG_LEVEL::SYSTEM:
			memcpy(LogLevelStr, L"SYSTEM", sizeof(L"SYSTEM"));
			break;
		case LOG_LEVEL::ERR:
			memcpy(LogLevelStr, L"ERROR", sizeof(L"ERROR"));
			break;
		default:
			__debugbreak();
			break;
		}

		len = wcslen(g_pLogBufForFileConsole);
		// �αװ� ���� ������ ��Ƽ������ �������հ� �ľ��ϱ� ���ؼ� ����
		DWORD dwLogCount = InterlockedIncrement((LONG*)&g_dwLogCount);

		hResult = StringCchPrintf(g_pLogBufForFileConsole + len, g_CurSize - len, L"%-6s / %09d]  ", LogLevelStr, dwLogCount);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}

		// ��¥ �α��ϰ��� �ϴ°� �������ڷ� �ֱ�
		len = wcslen(g_pLogBufForFileConsole);
		va_list va;
		va_start(va, szStringFormat);
		_ASSERTE(_CrtCheckMemory());
		hResult = StringCchVPrintf(g_pLogBufForFileConsole + len, g_CurSize - len, szStringFormat, va);
		va_end(va);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}

		if (OUTPUT == CONSOLE)
		{
			wprintf(L"%s\n", g_pLogBufForFileConsole);
			return;
		}

		// ���� �� ����
		FILE* pFile;
		WCHAR FilePath[MAX_PATH];
		StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolerPath, st.wYear, st.wMonth, szHead);

		// ���� ���� ������ �ݱ�
		// �������� ��������� ���ÿ� �������Ͽ� �����ϸ� ���Ͽ��⿡ �����Ҽ�������
		AcquireSRWLockExclusive(&g_srwForFILEIO);
		_wfopen_s(&pFile, FilePath, L"a");
		fputws(g_pLogBufForFileConsole, pFile);
		fputc(L'\n', pFile);
		fclose(pFile);
		ReleaseSRWLockExclusive(&g_srwForFILEIO);
		break;
	}
}

static void GetParentDir(WCHAR* szTargetPath)
{
	WCHAR* lastSlash = wcsrchr(szTargetPath, L'\\');
	if (lastSlash)
		*lastSlash = L'\0';
}

void SET_LOG_LEVEL(LOG_LEVEL level)
{
	if (level >= DEBUG && level <= ERR)
		InterlockedExchange((LONG*)&g_logLevel, level);
}

LOG_LEVEL INCREASE_LOG_LEVEL()
{
	LOG_LEVEL Ret;
	LOG_LEVEL level;

	level = (LOG_LEVEL)InterlockedExchange((LONG*)&g_logLevel, g_logLevel);
	if (level >= DEBUG && level <= SYSTEM)
		Ret = (LOG_LEVEL)InterlockedIncrement((LONG*)&g_logLevel);
	else
		Ret = level;
	return Ret;
}

LOG_LEVEL DECREASE_LOG_LEVEL()
{
	LOG_LEVEL Ret;
	LOG_LEVEL level;
	level = (LOG_LEVEL)InterlockedExchange((LONG*)&g_logLevel, g_logLevel);
	if (level >= SYSTEM && level <= ERR)
		Ret = (LOG_LEVEL)InterlockedDecrement((LONG*)&g_logLevel);
	else
		Ret = level;
	return Ret;
}

static void SYSLOG_DIRECTORY(const WCHAR* szPath)
{
	WCHAR exePath[MAX_PATH];
	WCHAR ParentDir[MAX_PATH];

	// �������� �̸� ������
	GetModuleFileName(NULL, exePath, MAX_PATH);
	StringCchCopy(ParentDir, MAX_PATH, exePath);

	// �δܰ� ����� ������
	GetParentDir(ParentDir);
	GetParentDir(ParentDir);

	// �����̸� �����
	StringCchPrintf(g_pszFolerPath, MAX_PATH, L"%s\\%s", ParentDir, szPath);

	// ���������ϸ� �Ѿ�� ������ �����
	DWORD dwAttr = GetFileAttributes(g_pszFolerPath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
	{
		if (!CreateDirectory(g_pszFolerPath, NULL))
		{
			DWORD dwErrCode = GetLastError();
			__debugbreak();
		}
	}
}

void InitLogger(const WCHAR* szPath)
{
	InitializeSRWLock(&g_srwForFILEIO);
	g_pLogBufForFileConsole = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_CurSize * sizeof(WCHAR));
	g_pszFolerPath = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, MAX_PATH * sizeof(WCHAR));
	SYSLOG_DIRECTORY(szPath);
}

void ClearLogger()
{
	HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_pLogBufForFileConsole);
	HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_pszFolerPath);
}

