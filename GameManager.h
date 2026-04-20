#pragma once
#include <unordered_map>
#include <list>
#include "Character.h"
#include "NetworkManager.h"
#include "SerialBuffer.h"
#include "Defines.h"

// 원래 매크로를 클래스에서 볼 수 있도록 해줍니다.
#ifndef dfSECTOR_MAX_X
#define dfSECTOR_MAX_X (dfRANGE_MOVE_RIGHT / dfSECTOR_RANGE) + 1
#define dfSECTOR_MAX_Y (dfRANGE_MOVE_BOTTOM / dfSECTOR_RANGE) + 1
#endif

class GameManager {
public:
    static GameManager& GetInstance() {
        static GameManager instance;
        return instance;
    }

    void Update();

    // 기존 std::map 에서 O(1) 속도로 대폭 최적화된 unordered_map
    std::unordered_map<DWORD, Character*>& GetPlayers() { return players; }

    void OnCharacterConnect(Character& c);
    void OnCharacterDisconnect(Character& c);

    void SendAround(Character& c, SerialBuffer& serial, bool sendMe = false);
    
    // 각종 게임 내 패킷 처리부
    void PacketProc_MoveStart(Session& s, SerialBuffer& packet);
    void PacketProc_MoveStop(Session& s, SerialBuffer& packet);
    void PacketProc_Attack1(Session& s, SerialBuffer& packet);
    void PacketProc_Attack2(Session& s, SerialBuffer& packet);
    void PacketProc_Attack3(Session& s, SerialBuffer& packet);
    void PacketProc_Attack(Session& s, SerialBuffer& packet, BYTE type);
    void PacketProc_Echo(Session& s, SerialBuffer& packet);

private:
    GameManager() = default;
    ~GameManager() = default;

    std::unordered_map<DWORD, Character*> players;
    std::list<Character*> sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

    void GetSectorPos(int x, int y, stSectorPos& sectorPos);
    void GetSectorAround(int x, int y, stSectorAround& around);
    
    bool CheckCharacterMove(short x, short y);
    bool CheckSectorUpdate(Character& c);
    void SectorUpdate(Character& c);
    
    void SendSectorCreateDeleteMsg(Character& c, const stSectorAround& oldAround, const stSectorAround& curAround);
    void SendSectorCreateMsg(Character& c, SerialBuffer& msg, const stSectorPos& pos);
    void SendSectorDeleteMsg(Character& c, SerialBuffer& msg, const stSectorPos& pos);
    void SendDamage(Character& c, SerialBuffer& msg, BYTE dir, BYTE type);
};
