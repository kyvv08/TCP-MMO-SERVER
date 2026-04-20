#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#include <windows.h>
#include "Defines.h"
#include "Protocol.h"

struct stSectorPos {
    int x;
    int y;
    stSectorPos& operator=(stSectorPos& pos) {
        x = pos.x;
        y = pos.y;
        return *this;
    }
    bool operator==(stSectorPos& pos) {
        return (x == pos.x && y == pos.y);
    }
    bool operator==(const stSectorPos& pos) const {
        return (x == pos.x && y == pos.y);
    }
};

struct stSectorAround {
    int count;
    stSectorPos around[9];
};

class Character {
private:
    DWORD sessionId;
    WORD curX;
    WORD curY;
    BYTE dir;
    BYTE moveDir;
    char hp;
    DWORD lastRecvTime;
    stSectorPos curSector;
    DWORD action;
public:
    Character(DWORD id, BYTE dir, WORD x, WORD y);
    
    inline stSectorPos& GetSector() { return curSector; }
    inline DWORD GetAction() { return action; }
    inline DWORD GetSessionId() { return sessionId; }
    inline BYTE GetDir() { return dir; }
    inline BYTE GetMoveDir() { return moveDir; }
    inline short GetCurX() { return curX; }
    inline short GetCurY() { return curY; }
    inline CHAR GetHp() { return hp; }
    inline DWORD GetLastRecvTime() { return lastRecvTime; }

    inline void Move(short x, short y) {
        curX += x;
        curY += y;
    }

    inline void SetSector(stSectorPos& pos) { curSector = pos; }
    inline void SetAction(DWORD act) { action = act; }
    inline void SetMoveDir(BYTE byDir) { moveDir = byDir; }
    inline void SetDir(BYTE byDir) { dir = byDir; }
    inline void SetCurX(WORD x) { curX = x; }
    inline void SetCurY(WORD y) { curY = y; }
    inline void SetLastRecvTime(DWORD tick) { lastRecvTime = tick; }
    inline void HitByAttacker(char dmg) { hp -= dmg; }
};
