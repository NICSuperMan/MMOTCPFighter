#include <windows.h>
#include <stdio.h>
int g_iLogLevel;
WCHAR g_szLogBuff[1024];

void Log(const WCHAR* pszString, int iLogLevel)
{
	wprintf(L"%s\n",pszString);
}