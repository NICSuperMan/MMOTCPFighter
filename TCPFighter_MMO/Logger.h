#pragma once
#include <Windows.h>
enum LOG_LEVEL { DEBUG, SYSTEM, ERR };
#define CONSOLE 3
#define TEXTFILE 4 

void InitLogger(const WCHAR* szPath);
void ClearLogger();

LOG_LEVEL INCREASE_LOG_LEVEL();
LOG_LEVEL DECREASE_LOG_LEVEL();
void SET_LOG_LEVEL(LOG_LEVEL level);
void LOG(const WCHAR* szHead, LOG_LEVEL LogLevel, CHAR OUTPUT, const WCHAR* szStringFormat, ...);


