#pragma once
#include <winsock2.h>
#include <unordered_map>
#include "Session.h"
#include "SerialBuffer.h"

// 패킷 사이즈 공격 방어용 최대 버퍼 한도 지정
#define dfMAX_PACKET_SIZE 1024

class NetworkManager {
public:
    static NetworkManager& GetInstance() {
        static NetworkManager instance;
        return instance;
    }

    bool NetStart(int port);
    void NetClean();
    void NetworkManage();
    
    void Disconnect(Session& s);
    void SendUniCast(Session& s, SerialBuffer& serial);

    std::unordered_map<DWORD, Session*>& GetSessions() { return sessions; }

private:
    NetworkManager();
    ~NetworkManager();

    void AcceptProc();
    void RecvPacket(Session& s);
    void SendPacket(Session& s);

    SOCKET listen_sock;
    DWORD dw_sessionId;
    std::unordered_map<DWORD, Session*> sessions;
    timeval tVal;
};

// 외부 논리 바인딩용
extern void CreateNewCharacter(Session& s, SerialBuffer& serial);
extern void PacketProc(Session& s, BYTE type, SerialBuffer& serial);
extern void OnCharacterDisconnect(DWORD sessionId);
