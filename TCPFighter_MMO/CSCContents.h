#pragma once
#pragma once
#include "Network.h"
//#include "CStub.h"
constexpr int ERROR_RANGE = 50;
constexpr int MOVE_UNIT_X = 3;
constexpr int MOVE_UNIT_Y = 2;

constexpr char INIT_DIR = dfPACKET_MOVE_DIR_LL;
constexpr int INIT_POS_X = 300;
constexpr int INIT_POS_Y = 300;
constexpr int INIT_HP = 100;
constexpr int dfRANGE_MOVE_TOP = 50;
constexpr int dfRANGE_MOVE_LEFT = 10;
constexpr int dfRANGE_MOVE_RIGHT = 630;
constexpr int dfRANGE_MOVE_BOTTOM = 470;

constexpr int dfATTACK1_RANGE_X = 80;
constexpr int dfATTACK2_RANGE_X = 90;
constexpr int dfATTACK3_RANGE_X = 100;
constexpr int dfATTACK1_RANGE_Y = 10;
constexpr int dfATTACK2_RANGE_Y = 10;
constexpr int dfATTACK3_RANGE_Y = 20;
struct ClientInfo
{
	unsigned ID;
	int hp;
	int x;
	int y;
	char viewDir;
	int action;

	ClientInfo(unsigned id)
		//:ID{id},hp{INIT_HP},x{INIT_POS_X},y{INIT_POS_Y},INIT_DIR{}
	{

	}
};
