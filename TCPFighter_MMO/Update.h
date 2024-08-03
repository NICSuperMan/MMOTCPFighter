#pragma once
#include "Client.h"
#include "Sector.h"
void SectorUpdateAndNotify(st_Client* pClient, BYTE byMoveDir, SHORT shOldSectorY, SHORT shOldSectorX, SHORT shNewSectorY, SHORT shNewSectorX, st_SECTOR_AROUND* pOutNewAround);

