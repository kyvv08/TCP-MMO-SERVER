#pragma comment(lib, "winmm.lib") // timeGetTime 라이브러리 추가
#include "GameManager.h"
#include "PacketCreator.h"
#include "memoryPool.cpp" // MemoryPool 선언을 위해 추가
#include <cmath>
#include <windows.h>
#include <mmsystem.h> // timeGetTime 사용을 위해 명시적으로 추가

extern void _LOG(int level, const wchar_t* format, ...);
extern DWORD curTime;
extern MemoryPool<SerialBuffer> serial_32_pool;

// Utility functions that used to be global
void GameManager::GetSectorPos(int x, int y, stSectorPos& sectorPos) {
    sectorPos.x = x / dfSECTOR_RANGE;
    sectorPos.y = y / dfSECTOR_RANGE;
}

void GameManager::GetSectorAround(int x, int y, stSectorAround& around) {
    int startX = max(0, x - 1);
    int endX = min(dfSECTOR_MAX_X - 1, x + 1);
    int startY = max(0, y - 1);
    int endY = min(dfSECTOR_MAX_Y - 1, y + 1);

    around.count = 0;
    for (int i = startY; i <= endY; ++i) {
        for (int j = startX; j <= endX; ++j) {
            around.around[around.count].x = j;
            around.around[around.count].y = i;
            ++around.count;
        }
    }
}

void GameManager::SendAround(Character& c, SerialBuffer& serial, bool sendMe) {
    stSectorAround around;
    stSectorPos curSector = c.GetSector();
    GetSectorAround(curSector.x, curSector.y, around);

    for (int i = 0; i < around.count; i++) {
        int sectorX = around.around[i].x;
        int sectorY = around.around[i].y;
        for (auto it : sector[sectorY][sectorX]) {
            if (!sendMe && (it->GetSessionId() == c.GetSessionId())) continue;
            auto& sessions = NetworkManager::GetInstance().GetSessions();
            if (sessions.count(it->GetSessionId())) {
                NetworkManager::GetInstance().SendUniCast(*sessions[it->GetSessionId()], serial);
            }
        }
    }
}

bool GameManager::CheckCharacterMove(short x, short y) {
    if (x < dfRANGE_MOVE_LEFT || y < dfRANGE_MOVE_TOP || x > dfRANGE_MOVE_RIGHT || y > dfRANGE_MOVE_BOTTOM) return false;
    return true;
}

bool GameManager::CheckSectorUpdate(Character& c) {
    stSectorPos oldSector = c.GetSector();
    stSectorPos curSector;
    GetSectorPos(c.GetCurX(), c.GetCurY(), curSector);
    return (curSector.x != oldSector.x || curSector.y != oldSector.y);
}

void GameManager::SendSectorCreateDeleteMsg(Character& c, const stSectorAround& oldAround, const stSectorAround& curAround) {
    SerialBuffer* msg = serial_32_pool.allocate();
    msg = new (msg) SerialBuffer(64);

    PacketCreator::DeleteCharacterMsg(*msg, c.GetSessionId());
    for (int i = 0; i < oldAround.count; ++i) {
        bool isRemoved = true;
        for (int j = 0; j < curAround.count; ++j) {
            if (oldAround.around[i] == curAround.around[j]) { isRemoved = false; break; }
        }
        if (isRemoved) SendSectorDeleteMsg(c, *msg, oldAround.around[i]);
    }
    msg->Clear();
    
    PacketCreator::CreateOtherCharacterMsg(*msg, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    if (c.GetAction() != dfPACKET_NONE_MOVE) {
        PacketCreator::MoveStartMsg(*msg, c.GetSessionId(), c.GetMoveDir(), c.GetCurX(), c.GetCurY());
    }
    
    for (int i = 0; i < curAround.count; ++i) {
        bool isNew = true;
        for (int j = 0; j < oldAround.count; ++j) {
            if (curAround.around[i] == oldAround.around[j]) { isNew = false; break; }
        }
        if (isNew) SendSectorCreateMsg(c, *msg, curAround.around[i]);
    }
    
    msg->~SerialBuffer();
    serial_32_pool.deallocate(msg);
}

void GameManager::SectorUpdate(Character& c) {
    stSectorAround oldAround, curAround;
    stSectorPos oldSector = c.GetSector();
    GetSectorAround(oldSector.x, oldSector.y, oldAround);

    stSectorPos curSector;
    GetSectorPos(c.GetCurX(), c.GetCurY(), curSector);
    GetSectorAround(curSector.x, curSector.y, curAround);

    c.SetSector(curSector);

    for (auto it = sector[oldSector.y][oldSector.x].begin(); it != sector[oldSector.y][oldSector.x].end(); it++) {
        if ((*it)->GetSessionId() == c.GetSessionId()) {
            sector[oldSector.y][oldSector.x].erase(it); break;
        }
    }
    sector[curSector.y][curSector.x].push_back(&c);
    SendSectorCreateDeleteMsg(c, oldAround, curAround);
}

void GameManager::OnCharacterConnect(Character& c) {
    stSectorAround curAround;
    stSectorPos curSector;
    GetSectorPos(c.GetCurX(), c.GetCurY(), curSector);
    GetSectorAround(curSector.x, curSector.y, curAround);

    c.SetSector(curSector);
    sector[curSector.y][curSector.x].push_back(&c);

    SerialBuffer* serial = serial_32_pool.allocate();
    serial = new (serial) SerialBuffer();

    PacketCreator::CreateMyCharacterMsg(*serial, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    auto& sessions = NetworkManager::GetInstance().GetSessions();
    if (sessions.count(c.GetSessionId())) {
        NetworkManager::GetInstance().SendUniCast(*sessions[c.GetSessionId()], *serial);
    }

    serial->Clear();
    PacketCreator::CreateOtherCharacterMsg(*serial, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    for (int i = 0; i < curAround.count; i++) {
        SendSectorCreateMsg(c, *serial, curAround.around[i]);
    }
    
    serial->~SerialBuffer();
    serial_32_pool.deallocate(serial);
}

void GameManager::OnCharacterDisconnect(Character& c) {
    SerialBuffer* serial = serial_32_pool.allocate();
    serial = new (serial) SerialBuffer();
    
    PacketCreator::DeleteCharacterMsg(*serial, c.GetSessionId());
    SendAround(c, *serial, true);

    stSectorPos oldSector = c.GetSector();
    for (auto it = sector[oldSector.y][oldSector.x].begin(); it != sector[oldSector.y][oldSector.x].end(); ++it) {
        if ((*it)->GetSessionId() == c.GetSessionId()) {
            sector[oldSector.y][oldSector.x].erase(it);
            break;
        }
    }
    
    delete &c; // 메모리 해제
    
    serial->~SerialBuffer();
    serial_32_pool.deallocate(serial);
}

void GameManager::SendSectorCreateMsg(Character& c, SerialBuffer& msg, const stSectorPos& pos) {
    SerialBuffer* msg2 = serial_32_pool.allocate();
    msg2 = new (msg2) SerialBuffer();
    auto& sessions = NetworkManager::GetInstance().GetSessions();

    for (const auto it : sector[pos.y][pos.x]) {
        if (it->GetSessionId() == c.GetSessionId()) continue;
        
        if (sessions.count(it->GetSessionId())) {
            NetworkManager::GetInstance().SendUniCast(*sessions[it->GetSessionId()], msg);
        }
        
        PacketCreator::CreateOtherCharacterMsg(*msg2, it->GetSessionId(), it->GetDir(), it->GetCurX(), it->GetCurY(), it->GetHp());
        if (it->GetAction() != dfPACKET_NONE_MOVE) {
            PacketCreator::MoveStartMsg(*msg2, it->GetSessionId(), it->GetMoveDir(), it->GetCurX(), it->GetCurY());
        }

        if (sessions.count(c.GetSessionId())) {
            NetworkManager::GetInstance().SendUniCast(*sessions[c.GetSessionId()], *msg2);
        }
        msg2->Clear();
    }
    
    msg2->~SerialBuffer();
    serial_32_pool.deallocate(msg2);
}

void GameManager::SendSectorDeleteMsg(Character& c, SerialBuffer& msg, const stSectorPos& pos) {
    SerialBuffer* delMsg = serial_32_pool.allocate();
    delMsg = new (delMsg) SerialBuffer();
    auto& sessions = NetworkManager::GetInstance().GetSessions();

    for (const auto it : sector[pos.y][pos.x]) {
        if (it->GetSessionId() == c.GetSessionId()) continue;
        
        PacketCreator::DeleteCharacterMsg(*delMsg, it->GetSessionId());
        
        if (sessions.count(it->GetSessionId())) {
            NetworkManager::GetInstance().SendUniCast(*sessions[it->GetSessionId()], msg);
        }
        if (sessions.count(c.GetSessionId())) {
            NetworkManager::GetInstance().SendUniCast(*sessions[c.GetSessionId()], *delMsg);
        }
        delMsg->Clear();
    }
    
    delMsg->~SerialBuffer();
    serial_32_pool.deallocate(delMsg);
}

void GameManager::SendDamage(Character& c, SerialBuffer& msg, BYTE dir, BYTE type) {
    bool isHit;
    char dmg;
    short xRange, yRange;
    int startX, endX, startY, endY;
    int x = c.GetCurX();
    int y = c.GetCurY();

    if (type == dfPACKET_CS_ATTACK1) { dmg = dfATTACK1_DAMAGE; xRange = dfATTACK1_RANGE_X; yRange = dfATTACK1_RANGE_Y; }
    else if (type == dfPACKET_CS_ATTACK2) { dmg = dfATTACK2_DAMAGE; xRange = dfATTACK2_RANGE_X; yRange = dfATTACK2_RANGE_Y; }
    else { dmg = dfATTACK3_DAMAGE; xRange = dfATTACK3_RANGE_X; yRange = dfATTACK3_RANGE_Y; }

    if (dir) {
        startX = x / dfSECTOR_RANGE;
        endX = min(dfSECTOR_MAX_X - 1, (x + xRange) / dfSECTOR_RANGE);
    } else {
        startX = max(0, (x - xRange) / dfSECTOR_RANGE);
        endX = x / dfSECTOR_RANGE;
    }
    startY = max(0, (y - yRange) / dfSECTOR_RANGE);
    endY = min(dfSECTOR_MAX_Y - 1, (y + yRange) / dfSECTOR_RANGE);

    for (int i = startY; i <= endY; ++i) {
        for (int j = startX; j <= endX; ++j) {
            for (auto it : sector[i][j]) {
                if (it == &c) continue;
                isHit = false;
                if (dir) {
                    if ((it->GetCurX() > c.GetCurX()) && (it->GetCurX() < c.GetCurX() + xRange) &&
                        (it->GetCurY() > c.GetCurY() - yRange) && (it->GetCurY() < c.GetCurY() + yRange)) isHit = true;
                } else {
                    if ((it->GetCurX() > c.GetCurX() - xRange) && (it->GetCurX() < c.GetCurX()) &&
                        (it->GetCurY() > c.GetCurY() - yRange) && (it->GetCurY() < c.GetCurY() + yRange)) isHit = true;
                }
                if (isHit) {
                    msg.Clear();
                    it->HitByAttacker(dmg);
                    PacketCreator::DamageMsg(msg, c.GetSessionId(), it->GetSessionId(), it->GetHp());
                    SendAround(*it, msg, true);
                }
            }
        }
    }
}

// ----------------- PACKET PROCESSES -----------------
void GameManager::PacketProc_MoveStart(Session& s, SerialBuffer& packet) {
    BYTE byDir; WORD curX, curY;
    packet >> byDir >> curX >> curY;
    
    Character* c = players[s.sessionId];
    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        packet.Clear();
        PacketCreator::SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX(); curY = c->GetCurY();
    }

    c->SetAction(byDir);
    c->SetMoveDir(byDir);
    if (byDir == dfPACKET_MOVE_DIR_RU || byDir == dfPACKET_MOVE_DIR_RR || byDir == dfPACKET_MOVE_DIR_RD) c->SetDir(dfPACKET_MOVE_DIR_RR);
    else c->SetDir(dfPACKET_MOVE_DIR_LL);
    
    c->SetCurX(curX);
    c->SetCurY(curY);
    c->SetLastRecvTime(timeGetTime());
    if (CheckSectorUpdate(*c)) SectorUpdate(*c);

    packet.Clear();
    PacketCreator::MoveStartMsg(packet, c->GetSessionId(), byDir, curX, curY);
    SendAround(*c, packet);
}

void GameManager::PacketProc_MoveStop(Session& s, SerialBuffer& packet) {
    BYTE byDir; WORD curX, curY;
    packet >> byDir >> curX >> curY;

    Character* c = players[s.sessionId];
    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        packet.Clear();
        PacketCreator::SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX(); curY = c->GetCurY();
    }

    c->SetAction(dfPACKET_NONE_MOVE);
    if (byDir == dfPACKET_MOVE_DIR_RU || byDir == dfPACKET_MOVE_DIR_RR || byDir == dfPACKET_MOVE_DIR_RD) c->SetDir(dfPACKET_MOVE_DIR_RR);
    else c->SetDir(dfPACKET_MOVE_DIR_LL);

    c->SetCurX(curX); c->SetCurY(curY);
    c->SetLastRecvTime(timeGetTime());
    if (CheckSectorUpdate(*c)) SectorUpdate(*c);

    packet.Clear();
    PacketCreator::MoveStopMsg(packet, c->GetSessionId(), byDir, curX, curY);
    SendAround(*c, packet);
}

void GameManager::PacketProc_Attack(Session& s, SerialBuffer& packet, BYTE type) {
    BYTE byDir; WORD curX, curY;
    packet >> byDir >> curX >> curY;

    Character* c = players[s.sessionId];
    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        packet.Clear();
        PacketCreator::SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX(); curY = c->GetCurY();
    }

    c->SetDir(byDir & 0x4);
    c->SetLastRecvTime(timeGetTime());
    
    packet.Clear();
    if (type == dfPACKET_SC_ATTACK1) PacketCreator::Attack1Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    else if (type == dfPACKET_SC_ATTACK2) PacketCreator::Attack2Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    else PacketCreator::Attack3Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    
    SendAround(*c, packet);
    SendDamage(*c, packet, byDir & 0x4, type);
}

void GameManager::PacketProc_Attack1(Session& s, SerialBuffer& packet) { PacketProc_Attack(s, packet, dfPACKET_CS_ATTACK1); }
void GameManager::PacketProc_Attack2(Session& s, SerialBuffer& packet) { PacketProc_Attack(s, packet, dfPACKET_CS_ATTACK2); }
void GameManager::PacketProc_Attack3(Session& s, SerialBuffer& packet) { PacketProc_Attack(s, packet, dfPACKET_CS_ATTACK3); }

void GameManager::PacketProc_Echo(Session& s, SerialBuffer& packet) {
    DWORD tick; packet >> tick;
    Character* c = players[s.sessionId];
    c->SetLastRecvTime(timeGetTime());
    packet.Clear();
    PacketCreator::EchoMsg(packet, tick);
    NetworkManager::GetInstance().SendUniCast(s, packet);
}

// ----------------- 메인 로직 업데이트 -----------------
void GameManager::Update() {
    auto& sessions = NetworkManager::GetInstance().GetSessions();
    for (auto it = players.begin(); it != players.end();) {
        Character* c = it->second;
        ++it;
        if (c->GetHp() <= 0) {
            if (sessions.count(c->GetSessionId())) NetworkManager::GetInstance().Disconnect(*sessions[c->GetSessionId()]);
        } else {
            if (curTime - c->GetLastRecvTime() >= dfNETWORK_PACKET_RECV_TIMEOUT) {
                if (sessions.count(c->GetSessionId())) NetworkManager::GetInstance().Disconnect(*sessions[c->GetSessionId()]);
                continue;
            }
            if (c->GetAction() == dfPACKET_NONE_MOVE) continue;

            short moveX = 0, moveY = 0;
            switch (c->GetAction()) {
                case dfPACKET_MOVE_DIR_LL: moveX = -dfSPEED_PLAYER_X; break;
                case dfPACKET_MOVE_DIR_LU: moveX = -dfSPEED_PLAYER_X; moveY = -dfSPEED_PLAYER_Y; break;
                case dfPACKET_MOVE_DIR_UU: moveY = -dfSPEED_PLAYER_Y; break;
                case dfPACKET_MOVE_DIR_RU: moveX = dfSPEED_PLAYER_X; moveY = -dfSPEED_PLAYER_Y; break;
                case dfPACKET_MOVE_DIR_RR: moveX = dfSPEED_PLAYER_X; break;
                case dfPACKET_MOVE_DIR_RD: moveX = dfSPEED_PLAYER_X; moveY = dfSPEED_PLAYER_Y; break;
                case dfPACKET_MOVE_DIR_DD: moveY = dfSPEED_PLAYER_Y; break;
                case dfPACKET_MOVE_DIR_LD: moveX = -dfSPEED_PLAYER_X; moveY = dfSPEED_PLAYER_Y; break;
            }

            if (CheckCharacterMove(c->GetCurX() + moveX, c->GetCurY() + moveY)) {
                c->Move(moveX, moveY);
            }

            if (CheckSectorUpdate(*c)) SectorUpdate(*c);
        }
    }
}