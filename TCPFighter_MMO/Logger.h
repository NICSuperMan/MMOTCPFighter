#pragma once
constexpr int dwLog_LEVEL_DEBUG = 0;
constexpr int dwLog_LEVEL_ERROR = 1;
constexpr int dwLog_LEVEL_SYSTEM = 2;
constexpr int dwLOG_LEVEL_DEVELOP = 3;

extern int g_iLogLevel;
extern WCHAR g_szLogBuff[1024];

void Log(const WCHAR* pszString, int iLogLevel);

#define _LOG(LogLevel, fmt, ...)                  \
do{                                          \
   if(g_iLogLevel <= LogLevel)                     \
   {                                       \
      wsprintf(g_szLogBuff, fmt,##__VA_ARGS__);      \
      Log(g_szLogBuff, LogLevel);                  \
   }                                       \
} while (0) 
