#ifdef MEMORYPOOLAPI

#else
#define MEMORYPOOLAPI extern "C" __declspec(dllimport)
#endif

#include <Windows.h>
using MEMORYPOOL = PVOID;

MEMORYPOOLAPI MEMORYPOOL CreateMemoryPool(int objectSize, int objectNum);
MEMORYPOOLAPI PVOID AllocMemoryFromPool(MEMORYPOOL MP);
MEMORYPOOLAPI VOID RetMemoryToPool(MEMORYPOOL MP, PVOID freeAddr);
MEMORYPOOLAPI int ReportLeak(MEMORYPOOL MP);
MEMORYPOOLAPI VOID ReleaseMemoryPool(MEMORYPOOL MP);

