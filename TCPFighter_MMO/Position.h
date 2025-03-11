#pragma once
#include <windows.h>
union SectorPos
{
	int YX;
	struct
	{
		SHORT shY;
		SHORT shX;
	};
};

union Pos
{
	struct
	{
		SHORT shY;
		SHORT shX;
	};
	int YX;
}; 

