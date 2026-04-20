#include "Character.h"

Character::Character(DWORD id, BYTE dir, WORD x, WORD y) 
    : sessionId(id), dir(dir), moveDir(dir), curX(x), curY(y), hp(dfPLAYER_HP) 
{
    action = dfPACKET_NONE_MOVE;
    curSector.x = x / dfSECTOR_RANGE;
    curSector.y = y / dfSECTOR_RANGE;
}
