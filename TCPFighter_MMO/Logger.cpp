#include <strsafe.h>
#include <windows.h>


enum LOG_LEVEL { DEBUG, SYSTEM, ERR };

#define LogTimeLen 24

struct LogBufInfo
{
	WCHAR* pLogBuf;
	int iCurrentSize;
};

static DWORD g_dwLogCount = 0;
static LOG_LEVEL g_logLevel = DEBUG;

#define LOGSTR 10000;
#define CONSOLE 3
#define TEXTFILE 4 
#define LOCAL_LOG_BUF_SIZE 3000

__declspec(thread) WCHAR* g_pszFolderPath;
__declspec(thread) WCHAR* g_pLogBuf;
__declspec(thread) int g_CurSize = LOCAL_LOG_BUF_SIZE;

SRWLOCK g_srwForFILEIO;
SRWLOCK g_srwForLogLevel;


void LOG(const WCHAR* szHead, LOG_LEVEL LogLevel, CHAR OUTPUT, const WCHAR* szStringFormat, ...);

__forceinline void ExceptLogBufInSuf()
{
	HeapFree(GetProcessHeap(), 0, g_pLogBuf);
	g_pLogBuf = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_CurSize * 2 * sizeof(WCHAR));
	g_CurSize *= 2;
	LOG(L"SYSTEM", ERR, TEXTFILE, L"LogBUFSIZE INSUFFICIENT ALLOCATE %d -> %dBytes", g_CurSize / 2, g_CurSize);
}

void TempBufferToFile(const WCHAR* szHead, const WCHAR* Buffer)
{
	// 파일 명 생성
	SYSTEMTIME st;
	GetLocalTime(&st);

	AcquireSRWLockExclusive(&g_srwForFILEIO);
	FILE* pFile;
	WCHAR FilePath[MAX_PATH];
	StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolderPath, st.wYear, st.wMonth, szHead);
	_wfopen_s(&pFile, FilePath, L"a");
	fputws(Buffer, pFile);
	fclose(pFile);
	ReleaseSRWLockExclusive(&g_srwForFILEIO);
}

void TempBufferToFileBinary(const WCHAR* szHead, const CHAR* Buffer, SIZE_T size)
{
	// 파일 명 생성
	SYSTEMTIME st;
	GetLocalTime(&st);

	AcquireSRWLockExclusive(&g_srwForFILEIO);
	FILE* pFile;
	WCHAR FilePath[MAX_PATH];
	StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolderPath, st.wYear, st.wMonth, szHead);
	_wfopen_s(&pFile, FilePath, L"a");
	fwprintf(pFile, L"%02X %02X %02X %02X %02X %02X %02X %02X\n", Buffer[0], Buffer[1], Buffer[2], Buffer[3], Buffer[4], Buffer[5], Buffer[6], Buffer[7]);
	fclose(pFile);
	ReleaseSRWLockExclusive(&g_srwForFILEIO);
}

void LOG(const WCHAR* szHead, LOG_LEVEL LogLevel, CHAR OUTPUT, const WCHAR* szStringFormat, ...)
{
	LOG_LEVEL level;
	AcquireSRWLockShared(&g_srwForLogLevel);
	level = g_logLevel;
	ReleaseSRWLockShared(&g_srwForLogLevel);
	if (level > LogLevel)
		return;

	while (1)
	{
		//[BATTLE]  같은 문자열 붙이기
		StringCchPrintf(g_pLogBuf, g_CurSize, L"[%s]  ", szHead);

		SYSTEMTIME st;
		GetLocalTime(&st);

		// 연월일시분초 / 
		SIZE_T len = wcslen(g_pLogBuf);
		HRESULT hResult = StringCchPrintf(g_pLogBuf + len, g_CurSize - len, L"[%04d-%02d-%02d %02d:%02d:%02d / ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}

		// 로그레벨 구하기
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

		len = wcslen(g_pLogBuf);
		// 로그가 찍힌 순서를 멀티스레드 안전성잇게 파악하기 위해서 설정
		DWORD dwLogCount = InterlockedIncrement((LONG*)&g_dwLogCount);

		hResult = StringCchPrintf(g_pLogBuf + len, g_CurSize - len, L"%-6s / %09d]  ", LogLevelStr, dwLogCount);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}

		// 진짜 로깅하고자 하는것 가변인자로 넣기
		len = wcslen(g_pLogBuf);
		va_list va;
		va_start(va, szStringFormat);
		hResult = StringCchVPrintf(g_pLogBuf + len, g_CurSize - len, szStringFormat, va);
		va_end(va);
		if (hResult == STRSAFE_E_INSUFFICIENT_BUFFER)
		{
			ExceptLogBufInSuf();
			continue;
		}
		if (OUTPUT == CONSOLE)
		{
			wprintf(L"%s\n", g_pLogBuf);
			return;
		}

		// 파일 명 생성
		FILE* pFile;
		WCHAR FilePath[MAX_PATH];
		StringCchPrintf(FilePath, MAX_PATH, L"%s\\%04d%02d_%s.txt", g_pszFolderPath, st.wYear, st.wMonth, szHead);

		// 파일 열고 쓴다음 닫기
		// 여러개의 스레드들이 동시에 같은파일에 접근하면 파일열기에 실패할수도잇음
		AcquireSRWLockExclusive(&g_srwForFILEIO);
		_wfopen_s(&pFile, FilePath, L"a");
		fputws(g_pLogBuf, pFile);
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
	AcquireSRWLockExclusive(&g_srwForLogLevel);
	if (level >= DEBUG && level <= ERR)
		g_logLevel = level;
	ReleaseSRWLockExclusive(&g_srwForLogLevel);
}

LOG_LEVEL INCREASE_LOG_LEVEL()
{
	LOG_LEVEL Ret;

	AcquireSRWLockExclusive(&g_srwForLogLevel);
	if (g_logLevel >= DEBUG && g_logLevel <= SYSTEM)
	{
		++*(DWORD*)g_logLevel;
		Ret = g_logLevel;
	}
	else
	{
		Ret = g_logLevel;
	}
	ReleaseSRWLockExclusive(&g_srwForLogLevel);
	return Ret;
}

LOG_LEVEL DECREASE_LOG_LEVEL()
{
	LOG_LEVEL Ret;
	AcquireSRWLockExclusive(&g_srwForLogLevel);
	if (g_logLevel >= SYSTEM && g_logLevel <= ERR)
	{
		--*(DWORD*)(&g_logLevel);
		Ret = g_logLevel;
	}
	else
	{
		Ret = g_logLevel;
	}
	ReleaseSRWLockExclusive(&g_srwForLogLevel);
	return Ret;
}

static void SYSLOG_DIRECTORY(const WCHAR* szPath)
{
	WCHAR exePath[MAX_PATH];
	WCHAR ParentDir[MAX_PATH];

	// 실행파일 이름 얻어오기
	GetModuleFileName(NULL, exePath, MAX_PATH);
	StringCchCopy(ParentDir, MAX_PATH, exePath);

	// 두단계 윗경로 얻어오기
	GetParentDir(ParentDir);
	GetParentDir(ParentDir);

	// 폴더이름 만들기
	StringCchPrintf(g_pszFolderPath, MAX_PATH, L"%s\\%s", ParentDir, szPath);

	// 폴더존재하면 넘어가고 없으면 만들기
	DWORD dwAttr = GetFileAttributes(g_pszFolderPath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES)
	{
		if (!CreateDirectory(g_pszFolderPath, NULL))
		{
			DWORD dwErrCode = GetLastError();
			__debugbreak();
		}
	}
}

void InitLogger(const WCHAR* szPath)
{
	InitializeSRWLock(&g_srwForFILEIO);
	InitializeSRWLock(&g_srwForLogLevel);
	g_pLogBuf = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_CurSize * sizeof(WCHAR));
	g_pszFolderPath = (WCHAR*)HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, MAX_PATH * sizeof(WCHAR));
	SYSLOG_DIRECTORY(szPath);
}

void ClearLogger()
{
	HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_pLogBuf);
	HeapFree(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, g_pszFolderPath);
}

