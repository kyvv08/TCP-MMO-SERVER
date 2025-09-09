#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"ws2_32.lib")

#include <iostream>
#include <conio.h>
#include <list>
#include <unordered_map>
#include <map>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>

//#include "Log.h"
#include "Protocol.h"
#include "Defines.h"                                           
#include "RingBuffer.h"
#include "SerialBuffer.h"
#include "memoryPool.cpp"

#include "Timer.h"

#define dfSERVER_PORT		11850

typedef RingBuffer Ring;
typedef SerialBuffer Serial;

//--------------로깅-----------
#define LOG_LEVEL_DEBUG     0
#define LOG_LEVEL_ERROR     1
#define LOG_LEVEL_SYSTEM    2

extern int g_logLevel;

void Log(const std::wstring& str, int level);
std::wstring FormatString(const wchar_t* format, ...);

#define _LOG(LogLevel, format, ...) do                \
{                                               \
    if (g_logLevel <= LogLevel)                 \
    {                                           \
        Log(FormatString((format), ##__VA_ARGS__), LogLevel);                  \
    }                                           \
} \
while(0)

int g_logLevel;
std::wstring logBuf;
FILE* logfp;

std::wstring GetCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
#ifdef _WIN32
    localtime_s(&local_tm, &now_time_t); // Windows
#else
    localtime_r(&now_time_t, &local_tm); // POSIX
#endif

    wchar_t buffer[64];
    std::wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"[%Y-%m-%d] [%H:%M:%S]", &local_tm);
    return std::wstring(buffer);
}

std::wstring FormatString(const wchar_t* format, ...) {
    wchar_t buffer[1024]; // Adjust size as needed
    va_list args;
    va_start(args, format);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
    va_end(args);
    return std::wstring(buffer);
}

void Log(const std::wstring& str, int level) {
    //if (logfp != NULL) {
        /* wprintf(L"%s \n", str.c_str());
         fputws(str.c_str(), logfp);*/
         //std::wcout << str << std::endl;
    fopen_s(&logfp, "log.txt", "a, ccs=UTF-8");
    if (logfp == NULL) {
        return;
    }
    std::wstring timestampedStr = GetCurrentDateTime() + L" " + str + L"\n"; // 시간 추가
    std::fputws(timestampedStr.c_str(), logfp);
    //std::fputws(L"\n", logfp); // Add newline
    std::fflush(logfp);
    fclose(logfp);
    //}
}
//-------------------------------
struct Session {
    SOCKET sock;
    DWORD sessionId;
    RingBuffer* RecvQ;
    RingBuffer* SendQ;
    bool isDelete;
    Session(SOCKET sessionSock);
    ~Session();
};
//MemoryPool<Session> session_pool;

//-------네트워크 함수--------
void NetStart();
void NetClean();

void NetworkManage();

void Disconnect(Session& s);
void SendUniCast(Session& s, SerialBuffer& serial);
void AcceptProc();

void RecvPacket(Session&);
void SendPacket(Session&);

void PacketProc(Session&, BYTE, SerialBuffer&);

void CreateNewCharacter(Session&, SerialBuffer&);
void DeleteCharacter(DWORD);

void PacketProc_MoveStart(Session&, SerialBuffer&);
void PacketProc_MoveStop(Session&, SerialBuffer&);
void PacketProc_Attack1(Session&, SerialBuffer&);
void PacketProc_Attack2(Session&, SerialBuffer&);
void PacketProc_Attack3(Session&, SerialBuffer&);
void PacketProc_Echo(Session&, SerialBuffer&);
//----------------------------

//----------------메시지 생성부-----------
void CreateMyCharacterMsg(SerialBuffer&, DWORD, BYTE, WORD, WORD, BYTE);
void CreateOtherCharacterMsg(SerialBuffer&, DWORD, BYTE, WORD, WORD, BYTE);
void DeleteCharacterMsg(SerialBuffer&, DWORD);
void MoveStartMsg(SerialBuffer&, DWORD id, CHAR dir, WORD x, WORD y);
void MoveStopMsg(SerialBuffer&, DWORD id, CHAR dir, WORD x, WORD y);
void Attack1Msg(SerialBuffer&, DWORD id, CHAR dir, WORD x, WORD y);
void Attack2Msg(SerialBuffer&, DWORD id, CHAR dir, WORD x, WORD y);
void Attack3Msg(SerialBuffer&, DWORD id, CHAR dir, WORD x, WORD y);
void DamageMsg(SerialBuffer&, DWORD atkId, DWORD defId, CHAR hp);
void EchoMsg(SerialBuffer& serial, DWORD tick);
void SyncMsg(SerialBuffer&, DWORD id, WORD x, WORD y);

//----------------------------------------
//-------------컨텐츠-----------
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
    inline stSectorPos& GetSector();
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
        //wprintf(L"# MOVE # SessionId:%d / Direction:%d / X:%d / Y:%d\n", sessionId, dir, curX, curY);
    }

    inline void SetSector(stSectorPos& pos) {
        curSector = pos;
    }
    inline void SetAction(DWORD act) {
        action = act;
    }
    inline void SetMoveDir(BYTE byDir) {
        moveDir = byDir;
    }
    inline void SetDir(BYTE byDir) {
        dir = byDir;
    }
    inline void SetCurX(WORD x) {
        curX = x;
    }
    inline void SetCurY(WORD y) {
        curY = y;
    }
    inline void SetLastRecvTime(DWORD tick) {
        lastRecvTime = tick;
    }
    inline void HitByAttacker(char dmg) {
        hp -= dmg;
    }
    //friend void UpdateCharacterPos(Character&);
};
void OnCharacterConnect(Character&);
void OnCharacterDisconnect(Character&);
void GetSectorPos(int x, int y, stSectorPos&);
void GetSectorAround(int, int, stSectorAround&);

inline bool CheckCharacterMove(short x, short y);
inline bool CheckSectorUpdate(Character& c);

inline void SendAround(Character&, SerialBuffer&, bool sendMe = false);

void SectorUpdate(Character&);
void SendSectorCreateDeleteMsg(Character&, const stSectorAround&, const stSectorAround&);
inline void SendSyncMsg(Character&, Serial&);
inline void SendSectorCreateMsg(Character&, Serial&, const stSectorPos&);
inline void SendSectorDeleteMsg(Character&, Serial&, const stSectorPos&);
void SendDamage(Character&, Serial&, BYTE, BYTE);

void LoadData();
void Update();
//void UpdateCharacterPos(Character& );

//------------------------------

bool ShutDown = false;
int UpdateCnt;

void ServerControl(void) {
    static bool ControlMode = false;

    if (_kbhit()) {
        WCHAR key = _getwch();
        if (L'u' == key || L'U' == key) {
            ControlMode = true;
            wprintf(L"Control Mode : Press Q - Quit\n");
            wprintf(L"Control Mode : Press L - Key Lock\n");
            wprintf(L"Control Mode : Press D - LogLevel_Debug\n");
            wprintf(L"Control Mode : Press S - LogLevel_System\n");
        }
        //키보드 제어 잠금
        if (L'l' == key || L'L' == key) {
            ControlMode = false;
            wprintf(L"Control Lock : Press U - Control Unlock\n");
        }
        //키보드 제어 풀림 상태에서 특정 가능
        if ((L'q' == key || L'Q' == key) && ControlMode) {
            ShutDown = true;
        }
        if ((L'd' == key || L'D' == key) && ControlMode) {
            g_logLevel = LOG_LEVEL_DEBUG;
            wprintf(L"LogLevel Set by LogLevel_Debug\n");
        }
        if ((L's' == key || L'S' == key) && ControlMode) {
            g_logLevel = LOG_LEVEL_ERROR;
            wprintf(L"LogLevel Set by LogLevel_System\n");
        }
    }
}


//-------컨텐츠-------------
#define dfSECTOR_MAX_X (dfRANGE_MOVE_RIGHT / dfSECTOR_RANGE) +1
#define dfSECTOR_MAX_Y (dfRANGE_MOVE_BOTTOM / dfSECTOR_RANGE)+1

//std::unordered_map<DWORD, Character*> players;
std::map<DWORD, Character*> players;
std::list<Character*> sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

MemoryPool<Serial> serial_32_pool;
MemoryPool<Character> player_pool;

DWORD curTime;
DWORD ownTime;
DWORD startTime;

DWORD frame;
//-------------------------- 

//-------------------------네트워크 측------------------------
#define MAX_SELECT_COUNT 63

//FD_SET rset;
//FD_SET wset;
timeval tVal;

WSADATA wsa;
IN_ADDR addr;

DWORD dw_sessionId;

SOCKET listen_sock;
//std::list<Session*> sessions;
//std::unordered_map<DWORD, Session*> sessions;
std::map<DWORD, Session*> sessions;

//-------------------------------메인문---------------------------

#define FRAME_DURATION 40

void Mointoring() {
    if (curTime >= startTime + 1000) {
        std::cout << "fps: " << frame << " sessions: " << sessions.size() << " players: " << players.size() << std::endl;
        std::cout << "Update() ps: " << UpdateCnt << std::endl;
        frame = UpdateCnt = 0;
        startTime += 1000;
    }
}

int wmain()
{
    timeBeginPeriod(1);
    NetStart();
    LoadData();
    while (!ShutDown) {
        auto start = timeGetTime();
        NetworkManage();
        auto end = timeGetTime();

        curTime = timeGetTime();

        if (curTime >= ownTime + FRAME_DURATION) {
            auto start1 = timeGetTime();
            //auto start = std::chrono::high_resolution_clock::now();
            //std::cout << " curTime - ownTime: " << curTime - ownTime <<"\n";
            Update();
            auto end1 = timeGetTime();
            //std::cout << " unordered_map 순회 시간: " << end - start << "ms\n";
           // auto end = std::chrono::high_resolution_clock::now();
           //std::cout << " unordered_map 순회 시간: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "micros\n";
            ownTime += FRAME_DURATION;
            ++frame;
            if (curTime >= ownTime + FRAME_DURATION) {
                auto start2 = timeGetTime();
                Update();
                ++frame;
                auto end2 = timeGetTime();
                ownTime += FRAME_DURATION;
                _LOG(LOG_LEVEL_SYSTEM, L"Network Time: %d / Update Time: %d / FixedUpdate Time: %d\n", end - start, end1 - start1, end2 - start2);
                //std::cout << " unordered_map 순회 시간: " << end - start << "ms\n";
                //std::cout << " networkmanage 시간: " << end - start << "ms\n";
                //std::cout << "[Warning] Frame dropped due to drift. curTime - ownTime == "<< curTime - ownTime << "\n";
            }
        }
        ServerControl();
        Mointoring();
        //auto end = timeGetTime();
        //std::cout << " 로직 순회 시간: " << end - start << "ms\n";
    }
    NetClean();
    if (logfp) {
        std::fclose(logfp);
        logfp = nullptr;
    }
    PrintProfileData();
    return 0;
}
//-----------------------------------------------------------------
Session::Session(SOCKET sessionSock) :sock(sessionSock), sessionId(dw_sessionId), isDelete(false) {
    RecvQ = new Ring(1024 * 4);
    SendQ = new Ring(1024 * 4);
}

Session::~Session() {
    delete RecvQ;
    delete SendQ;
}

void NetStart() {
    tVal = { 0,0 };

    int bindRet;
    int listenRet;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "wsastartup err\n";
        DebugBreak();
    }

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    //bind
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(dfSERVER_PORT);
    bindRet = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));

    //listen
    listenRet = listen(listen_sock, SOMAXCONN_HINT(SOMAXCONN));
    if (listenRet == SOCKET_ERROR) {
        int listenErr = WSAGetLastError();
        _LOG(LOG_LEVEL_ERROR, L"# LISTEN ERROR # ErrorCode:%d\n", listenErr);
    }

    //nonblock
    u_long on = 1;
    int nonBlockRet = ioctlsocket(listen_sock, FIONBIO, &on);
    if (nonBlockRet == SOCKET_ERROR) {
        int NonBlockErr = WSAGetLastError();
        _LOG(LOG_LEVEL_ERROR, L"# NON-BLOCK ERROR ERROR # ErrorCode:%d\n", NonBlockErr);
    }
}

void NetClean() {
    closesocket(listen_sock);
    WSACleanup();
}

void Disconnect(Session& s) {
    //Timer t("disconnect");
    s.isDelete = true;
    closesocket(s.sock);
    Character* c = players[s.sessionId];
    OnCharacterDisconnect(*c);
}

void SendUniCast(Session& s, Serial& serial) {
    //Timer t("sendunicast");
    int len = serial.GetDataSize();
    int ret = s.SendQ->Enqueue(serial.GetBufferPtr(), len);
    if (ret != len) {
        Disconnect(s);
        return;
    }
}

void AcceptProc() {
    //   Timer t("accept");
    SOCKADDR_IN clientAddr;
    DWORD addrlen = sizeof(clientAddr);

    Session* session;
    Serial* serial = serial_32_pool.allocate();
    serial = new (serial) Serial();

    SOCKET clientSock = accept(listen_sock, (SOCKADDR*)&clientAddr, (int*)&addrlen);
    if (clientSock == INVALID_SOCKET) {
        int acceptErr = WSAGetLastError();
        _LOG(LOG_LEVEL_ERROR, L"# ACCEPT ERROR # ErrorCode:%d\n", acceptErr);
    }

    LINGER ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;

    setsockopt(clientSock, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));
    int optVal = 1;
    //setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optVal, sizeof(optVal));

    session = new Session(clientSock);
    sessions[dw_sessionId] = session;

    CreateNewCharacter(*session, *serial);

    serial->~Serial();
    serial_32_pool.deallocate(serial);
}

//-------------캐릭터 생성 후 메시지 송신
void CreateNewCharacter(Session& s, Serial& serial) {
    //   Timer t("CreateNewCharacter");
    WORD x = rand() % dfRANGE_MOVE_RIGHT; //temp;//
    WORD y = rand() % dfRANGE_MOVE_BOTTOM;//temp;//
    //temp += 30;
    BYTE dir = dfPACKET_MOVE_DIR_LL;
    //--캐릭터를 콘텐츠에서 관리하는 umap에 등록하는 함수
    Character* player = new Character(dw_sessionId, dir, x, y);
    players[dw_sessionId] = player;

    stSectorPos sectorPos = player->GetSector();
    sector[sectorPos.y][sectorPos.x].push_back(player);

    CreateMyCharacterMsg(serial, s.sessionId, dir, x, y, dfPLAYER_HP);
    SendUniCast(s, serial);

    OnCharacterConnect(*player);
    player->SetLastRecvTime(timeGetTime());
    ++dw_sessionId;
}

void RecvPacket(Session& s) {
    //    Timer t("recv");
    char* buf = s.RecvQ->GetBufferRear();
    int len = s.RecvQ->DirectEnqueue();
    int recvRet = recv(s.sock, buf, len, 0);

    if (recvRet == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return;
        }
        else {
            _LOG(LOG_LEVEL_SYSTEM, L"# RECV ERROR # SessionId:%d / ErrorCode:%d\n", s.sessionId, err);
            Disconnect(s);
            return;
        }
    }
    else if (recvRet == 0) {
        Disconnect(s);
        return;
    }
    else {
        s.RecvQ->MoveRear(recvRet);
    }
    Serial* serial = serial_32_pool.allocate();
    serial = new (serial) Serial();
    while (1) {
        serial->Clear();
        st_PACKET_HEADER header;

        int checkRecv = s.RecvQ->GetUseSize();
        if (checkRecv < sizeof(st_PACKET_HEADER)) break;

        s.RecvQ->Peek(serial->GetBufferPtr(), sizeof(st_PACKET_HEADER));
        serial->MoveWritePos(sizeof(st_PACKET_HEADER));
        serial->SetDataSize(sizeof(st_PACKET_HEADER));

        *serial >> header.byCode;
        if (header.byCode != dfPACKET_CODE) {
            Disconnect(s);
            break;
        }

        *serial >> header.bySize;
        if (checkRecv < sizeof(st_PACKET_HEADER) + header.bySize) break;
        *serial >> header.byType;
        s.RecvQ->MoveFront(sizeof(st_PACKET_HEADER));

        serial->Clear();
        s.RecvQ->Dequeue(serial->GetBufferPtr(), header.bySize);
        serial->MoveWritePos(header.bySize);
        serial->SetDataSize(header.bySize);
        PacketProc(s, header.byType, *serial);

    }
    serial->~Serial();
    serial_32_pool.deallocate(serial);
}
// ------------Send
//std::chrono::microseconds totalSend{ 0 };
void SendPacket(Session& s) {
    //auto start = std::chrono::high_resolution_clock::now();
    char* buf = s.SendQ->GetBufferFront();
    int len = s.SendQ->DirectDequeue();
    int ret = send(s.sock, buf, len, 0);
    if (ret == SOCKET_ERROR) {
        DWORD err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            return;
        }
        else {
            _LOG(LOG_LEVEL_SYSTEM, L"# SEND ERROR # SessionId:%d / ErrorCode:%d\n", s.sessionId, err);
            Disconnect(s);
            return;
        }
    }
    s.SendQ->MoveFront(ret);
    //auto end = std::chrono::high_resolution_clock::now();
    //auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    ////std::cout << " unordered_map 순회 시간: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "micros\n";
    //if (sessions.size() >= 5000) {
    //    totalSend += std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    //    //_LOG(LOG_LEVEL_SYSTEM, L"sendpacket 시간: %ld / 길이: %d / 리턴 : %d / 소켓 : %d\n\n\n", time, len , ret, s.sock);
    //}
}

void NetworkManage() {
    //    Timer t("network");
    auto it = sessions.begin();
    auto selectIt = sessions.begin();

    FD_SET rset, wset;
    DWORD total1 = 0, total2 = 0, total3 = 0;
    //auto start = timeGetTime();
    int cnt = 0;
    //
    // totalSend = std::chrono::microseconds(0);
    do {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(listen_sock, &rset);
        int clientNum = 0;
        //auto start1 = timeGetTime();
        for (; it != sessions.end() && clientNum < MAX_SELECT_COUNT;) {
            if (it->second->isDelete) {
                Session* temp = it->second;
                if (selectIt == it) {
                    ++selectIt;
                }
                it = sessions.erase(it);
                delete temp;
            }
            else {
                FD_SET(it->second->sock, &rset);
                if (it->second->SendQ->GetUseSize() > 0) {
                    FD_SET(it->second->sock, &wset);
                }
                ++clientNum;
                ++it;
            }
        }
        //auto end1 = timeGetTime();

        //auto start2 = timeGetTime();
        int selectRet = select(0, &rset, &wset, NULL, &tVal);
        if (FD_ISSET(listen_sock, &rset)) {
            AcceptProc();
            --selectRet;
        }
        //auto end2 = timeGetTime();
        //if (selectRet == SOCKET_ERROR) {
        //    int selectErr = WSAGetLastError();
        //    std::cout << "Select Error : " << selectErr;
        //    DebugBreak();
        //}

        //DWORD start2, start3, end2, end3;
        for (; selectIt != it; selectIt++) {
            if (selectRet == 0) {
                selectIt = it;
                break;
            }
            if (FD_ISSET(selectIt->second->sock, &rset)) {
                RecvPacket(*selectIt->second);
                --selectRet;
            }
            //start3 = timeGetTime();
            if (FD_ISSET(selectIt->second->sock, &wset)) {
                --selectRet;
                if (selectIt->second->isDelete) continue;
                SendPacket(*selectIt->second);
            }
            //end3 = timeGetTime();
            //if (sessions.size() >= 6000) {
            //    total3 += (end3 - start3);
            //}
        }
    } while (it != sessions.end());
    //auto end = timeGetTime();
    //if (sessions.size() >= 6000) {
    //    _LOG(LOG_LEVEL_SYSTEM, L"sendpacket 총 시간: %d / 총 횟수: %d / 개별 총 시간: %d \n\n\n", total3,cnt,totalSend);
    //    //_LOG(LOG_LEVEL_SYSTEM, L"전체 순회 시간: %d\n\n", end - start);
    //}
}

// ------------패킷 프로시져

void PacketProc(Session& s, BYTE type, Serial& serial) {
    //    Timer t("PacketProc");
    switch (type) {
    case dfPACKET_CS_MOVE_START: {
        PacketProc_MoveStart(s, serial);
        break;
    }
    case dfPACKET_CS_MOVE_STOP: {
        PacketProc_MoveStop(s, serial);
        break;
    }
    case dfPACKET_CS_ATTACK1: {
        PacketProc_Attack1(s, serial);
        break;
    }
    case dfPACKET_CS_ATTACK2: {
        PacketProc_Attack2(s, serial);
        break;
    }
    case dfPACKET_CS_ATTACK3: {
        PacketProc_Attack3(s, serial);
        break;
    }
    case dfPACKET_CS_ECHO: {
        PacketProc_Echo(s, serial);
        break;
    }
    default:
        Disconnect(s);
    }
}

void PacketProc_MoveStart(Session& s, Serial& packet) {
    //    Timer t("MoveStart");
    BYTE byDir;
    WORD  curX, curY;

    packet >> byDir;
    packet >> curX;
    packet >> curY;

    _LOG(LOG_LEVEL_DEBUG, L"# MOVESTART # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", s.sessionId, byDir, curX, curY, curTime);
    //wprintf(L"# MOVESTART # SessionId:%d / Direction:%d / X:%d / Y:%d\n", s.sessionId, byDir, curX, curY);

    Character* c = players[s.sessionId];

    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        _LOG(LOG_LEVEL_DEBUG, L"# MOVESTART SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d / curTime:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY, curTime);
        wprintf(L"# MOVESTART SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY);
        packet.Clear();
        auto sync = timeGetTime();
        SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX();
        curY = c->GetCurY();
    }

    c->SetAction(byDir);
    c->SetMoveDir(byDir);

    switch (byDir) {
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RD:
        c->SetDir(dfPACKET_MOVE_DIR_RR);
        break;
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LD:
        c->SetDir(dfPACKET_MOVE_DIR_LL);
        break;
    }
    c->SetCurX(curX);
    c->SetCurY(curY);
    c->SetLastRecvTime(timeGetTime());
    if (CheckSectorUpdate(*c)) {
        SectorUpdate(*c);
    }

    packet.Clear();
    MoveStartMsg(packet, c->GetSessionId(), byDir, curX, curY);

    SendAround(*c, packet);
}

void PacketProc_MoveStop(Session& s, SerialBuffer& packet) {
    //    Timer t("MoveStop");
    BYTE byDir;
    WORD curX, curY;

    packet >> byDir;
    packet >> curX;
    packet >> curY;

    _LOG(LOG_LEVEL_DEBUG, L"# MOVESTOP # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", s.sessionId, byDir, curX, curY, curTime);
    //wprintf(L"# MOVESTOP # SessionId:%d / Direction:%d / X:%d / Y:%d\n", s.sessionId, byDir, curX, curY);

    Character* c = players[s.sessionId];

    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        _LOG(LOG_LEVEL_DEBUG, L"# MOVESTOP SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d / curTime:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY, curTime);
        wprintf(L"# MOVESTOP SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY);
        packet.Clear();
        SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX();
        curY = c->GetCurY();
    }

    c->SetAction(dfPACKET_NONE_MOVE);

    switch (byDir) {
    case dfPACKET_MOVE_DIR_RU:
    case dfPACKET_MOVE_DIR_RR:
    case dfPACKET_MOVE_DIR_RD:
        c->SetDir(dfPACKET_MOVE_DIR_RR);
        break;
    case dfPACKET_MOVE_DIR_LL:
    case dfPACKET_MOVE_DIR_LU:
    case dfPACKET_MOVE_DIR_LD:
        c->SetDir(dfPACKET_MOVE_DIR_LL);
        break;
    }
    c->SetCurX(curX);
    c->SetCurY(curY);
    DWORD curtick = timeGetTime();
    c->SetLastRecvTime(curtick);

    if (CheckSectorUpdate(*c)) {
        SectorUpdate(*c);
    }

    packet.Clear();
    MoveStopMsg(packet, c->GetSessionId(), byDir, curX, curY);

    SendAround(*c, packet);
}

void PacketProc_Attack1(Session& s, SerialBuffer& packet) {
    //    Timer t("Attack1");
    BYTE byDir;
    WORD  curX, curY;

    packet >> byDir;
    packet >> curX;
    packet >> curY;

    _LOG(LOG_LEVEL_DEBUG, L"# ATTACK1 # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", s.sessionId, byDir, curX, curY, curTime);
    //wprintf(L"# ATTACK1 # SessionId:%d / Direction:%d / X:%d / Y:%d\n", s.sessionId, byDir, curX, curY);

    Character* c = players[s.sessionId];

    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        _LOG(LOG_LEVEL_DEBUG, L"# ATTACK1 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d / curTime:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY, curTime);
        wprintf(L"# ATTACK1 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY);
        packet.Clear();
        SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX();
        curY = c->GetCurY();
    }

    c->SetDir(byDir & 0x4);
    c->SetLastRecvTime(timeGetTime());
    packet.Clear();
    Attack1Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    SendAround(*c, packet);

    SendDamage(*c, packet, byDir & 0x4, dfPACKET_CS_ATTACK1);

}

void PacketProc_Attack2(Session& s, SerialBuffer& packet) {
    //    Timer t("Attack2");
    BYTE byDir;
    WORD  curX, curY;

    packet >> byDir;
    packet >> curX;
    packet >> curY;

    _LOG(LOG_LEVEL_DEBUG, L"# ATTACK2 # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", s.sessionId, byDir, curX, curY, curTime);
    //wprintf(L"# ATTACK2 # SessionId:%d / Direction:%d / X:%d / Y:%d\n", s.sessionId, byDir, curX, curY);

    Character* c = players[s.sessionId];

    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        _LOG(LOG_LEVEL_DEBUG, L"# ATTACK2 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d / curTime:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY, curTime);
        wprintf(L"# ATTACK2 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY);
        packet.Clear();
        SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX();
        curY = c->GetCurY();
    }

    c->SetDir(byDir & 0x4);
    c->SetLastRecvTime(timeGetTime());
    packet.Clear();
    Attack2Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    SendAround(*c, packet);

    SendDamage(*c, packet, byDir & 0x4, dfPACKET_CS_ATTACK2);

}

void PacketProc_Attack3(Session& s, SerialBuffer& packet) {
    //Timer t("Attack3");
    BYTE byDir;
    WORD  curX, curY;

    packet >> byDir;
    packet >> curX;
    packet >> curY;

    _LOG(LOG_LEVEL_DEBUG, L"# ATTACK3 # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", s.sessionId, byDir, curX, curY, curTime);
    //wprintf(L"# ATTACK3 # SessionId:%d / Direction:%d / X:%d / Y:%d\n", s.sessionId, byDir, curX, curY);

    Character* c = players[s.sessionId];

    if (abs(c->GetCurX() - curX) > dfERROR_RANGE || abs(c->GetCurY() - curY) > dfERROR_RANGE) {
        _LOG(LOG_LEVEL_DEBUG, L"# ATTACK3 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d / curTime:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY, curTime);
        wprintf(L"# ATTACK3 SYNC # SessionId:%d / X:%d / Y:%d / syncX:%d / syncY:%d\n", s.sessionId, c->GetCurX(), c->GetCurY(), curX, curY);
        packet.Clear();
        SyncMsg(packet, c->GetSessionId(), c->GetCurX(), c->GetCurY());
        SendAround(*c, packet, true);
        curX = c->GetCurX();
        curY = c->GetCurY();
    }

    c->SetDir(byDir & 0x4);
    c->SetLastRecvTime(timeGetTime());
    packet.Clear();
    Attack3Msg(packet, c->GetSessionId(), byDir & 0x4, curX, curY);
    SendAround(*c, packet);

    SendDamage(*c, packet, byDir & 0x4, dfPACKET_CS_ATTACK3);

}

void PacketProc_Echo(Session& s, SerialBuffer& packet) {
    // Timer t("Echo");
    DWORD tick;
    packet >> tick;
    Character* c = players[s.sessionId];
    c->SetLastRecvTime(timeGetTime());
    //wprintf(L"# ECHO # SessionId:%d /Msg:%d\n", s.sessionId,tick);
    packet.Clear();
    EchoMsg(packet, tick);
    SendUniCast(s, packet);
}

//--------------------메시지 생성부

void CreateMyCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp) {
    //BYTE size = 10;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
    serial << (BYTE)dfPACKET_CODE << (BYTE)10 << (BYTE)dfPACKET_SC_CREATE_MY_CHARACTER << id << dir << x << y << hp;
}

void CreateOtherCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp) {
    //BYTE size = 10;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y) + sizeof(hp);
    serial << (BYTE)dfPACKET_CODE << (BYTE)10 << (BYTE)dfPACKET_SC_CREATE_OTHER_CHARACTER << id << dir << x << y << hp;
}

void DeleteCharacterMsg(SerialBuffer& serial, DWORD id) {
    //BYTE size = 4;     //sizeof(id);
    serial << (BYTE)dfPACKET_CODE << (BYTE)4 << (BYTE)dfPACKET_SC_DELETE_CHARACTER << id;
}

void MoveStartMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
    //BYTE size = 9;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_MOVE_START << id << dir << x << y;
}

void MoveStopMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
    //BYTE size = 9;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_MOVE_STOP << id << dir << x << y;
}

void Attack1Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
    //BYTE size = 9;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK1 << id << dir << x << y;
}

void Attack2Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
    //BYTE size = 9;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK2 << id << dir << x << y;
}

void Attack3Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
    //BYTE size = 9;     //sizeof(id) + sizeof(dir) + sizeof(x) + sizeof(y);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK3 << id << dir << x << y;
}

void DamageMsg(SerialBuffer& serial, DWORD atkId, DWORD defId, CHAR hp) {
    //BYTE size = 9;     //sizeof(atkId) + sizeof(defId) + sizeof(hp);
    serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_DAMAGE << atkId << defId << hp;
}

void EchoMsg(SerialBuffer& serial, DWORD tick) {
    //BYTE size = 4;     //sizeof(tick);
    serial << (BYTE)dfPACKET_CODE << (BYTE)4 << (BYTE)dfPACKET_SC_ECHO << tick;
}


//int syncCnt = 0;
void SyncMsg(SerialBuffer& serial, DWORD id, WORD x, WORD y) {
    //BYTE size = 8;     //sizeof(id) + sizeof(x) + sizeof(y);
    //Timer t("sync msg");
    //ShutDown = true;
    serial << (BYTE)dfPACKET_CODE << (BYTE)8 << (BYTE)dfPACKET_SC_SYNC << id << x << y;
    //std::cout <<"sync: " << ++syncCnt << std::endl;
}
//---------------------컨텐츠

Character::Character(DWORD id, BYTE dir, WORD x, WORD y) :sessionId(id), dir(dir), moveDir(dir), curX(x), curY(y), hp(dfPLAYER_HP) {
    action = dfPACKET_NONE_MOVE;
    GetSectorPos(x, y, curSector);
}

stSectorPos& Character::GetSector() {
    return curSector;
}

inline void GetSectorPos(int x, int y, stSectorPos& sectorPos) {
    // Timer t("GetSectorPos");
     //if (isSectorDivisionEqual0) {
     //    sectorPos.x = (x == dfRANGE_MOVE_RIGHT) ? (dfRANGE_MOVE_RIGHT / dfSECTOR_RANGE) - 1 : x / dfSECTOR_RANGE;
     //    sectorPos.y = (y == dfRANGE_MOVE_BOTTOM) ? (dfRANGE_MOVE_BOTTOM / dfSECTOR_RANGE) - 1 : y / dfSECTOR_RANGE;
     //}
     //else {
     //    sectorPos.x = x / dfSECTOR_RANGE;
     //    sectorPos.y = y / dfSECTOR_RANGE;
     //}
    sectorPos.x = x / dfSECTOR_RANGE;
    sectorPos.y = y / dfSECTOR_RANGE;
}

inline void GetSectorAround(int x, int y, stSectorAround& around) {
    //Timer t("GetSectorAround");
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

inline void SendAround(Character& c, SerialBuffer& serial, bool sendMe) {
    //Timer t("SendAround");
    stSectorAround around;
    stSectorPos curSector = c.GetSector();
    GetSectorAround(curSector.x, curSector.y, around);

    for (int i = 0; i < around.count; i++) {
        int sectorX = around.around[i].x;
        int sectorY = around.around[i].y;
        for (auto it : sector[sectorY][sectorX]) {
            if (!sendMe && (it->GetSessionId() == c.GetSessionId())) continue;
            SendUniCast(*sessions[it->GetSessionId()], serial);
        }
    }

}

inline bool CheckCharacterMove(short x, short y) {
    //Timer t("CheckCharacterMove");
    if (x < dfRANGE_MOVE_LEFT || y < dfRANGE_MOVE_TOP || x > dfRANGE_MOVE_RIGHT || y > dfRANGE_MOVE_BOTTOM) {
        return false;
    }
    return true;
}

inline bool CheckSectorUpdate(Character& c) {
    //Timer t("CheckSectorUpdate");
    stSectorPos oldSector = c.GetSector();
    stSectorPos curSector;
    int x = c.GetCurX();
    int y = c.GetCurY();
    GetSectorPos(x, y, curSector);
    if ((curSector.x != oldSector.x) || (curSector.y != oldSector.y)) { return true; }
    else { return false; }
}

void SendSectorCreateDeleteMsg(Character& c, const stSectorAround& oldAround, const stSectorAround& curAround) {
    //Timer t("SendSectorCreateDeleteMsg");
    Serial* msg = serial_32_pool.allocate();
    msg = new (msg) Serial(64);

    //DeleteCharacterMsg(*msg, c.GetSessionId());
    //std::unordered_set<stSectorPos> curSet(curAround.around, curAround.around + curAround.count);
    //for (int i = 0; i < oldAround.count; ++i) {
    //    if (curSet.find(oldAround.around[i]) == curSet.end()) {
    //        SendSectorDeleteMsg(c, *msg, oldAround.around[i]);
    //    }
    //}
    //msg->Clear();

    //// 캐릭터 생성
    //CreateOtherCharacterMsg(*msg, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    //if (c.GetAction() != dfPACKET_NONE_MOVE) {
    //    MoveStartMsg(*msg, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY());
    //}
    //for (int i = 0; i < curAround.count; ++i) {
    //    if (std::find(oldAround.around, oldAround.around + oldAround.count, curAround.around[i]) == (oldAround.around + oldAround.count)) {
    //        SendSectorCreateMsg(c, *msg, curAround.around[i]);
    //    }
    //}

    // 캐릭터 삭제
    DeleteCharacterMsg(*msg, c.GetSessionId());
    for (int i = 0; i < oldAround.count; ++i) {
        bool isRemoved = true;
        for (int j = 0; j < curAround.count; ++j) {
            if (oldAround.around[i] == curAround.around[j]) {
                isRemoved = false;
                break;
            }
        }
        if (isRemoved) {
            SendSectorDeleteMsg(c, *msg, oldAround.around[i]);
        }
    }
    msg->Clear();
    // 캐릭터 생성
    CreateOtherCharacterMsg(*msg, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    if (c.GetAction() != dfPACKET_NONE_MOVE) {
        MoveStartMsg(*msg, c.GetSessionId(), c.GetMoveDir(), c.GetCurX(), c.GetCurY());
    }
    for (int i = 0; i < curAround.count; ++i) {
        bool isNew = true;
        for (int j = 0; j < oldAround.count; ++j) {
            if (curAround.around[i].x == oldAround.around[j].x &&
                curAround.around[i].y == oldAround.around[j].y) {
                isNew = false;
                break;
            }
        }
        if (isNew) {
            SendSectorCreateMsg(c, *msg, curAround.around[i]);
        }
    }
    msg->~Serial();
    serial_32_pool.deallocate(msg);
}

// 섹터 업데이트 처리 함수
void SectorUpdate(Character& c) {
    //Timer t("SectorUpdate");
    stSectorAround oldAround;
    stSectorAround curAround;

    // 기존 섹터 위치 가져오기
    stSectorPos oldSector = c.GetSector();
    GetSectorAround(oldSector.x, oldSector.y, oldAround);

    // 현재 섹터 위치 계산
    int x = c.GetCurX();
    int y = c.GetCurY();
    stSectorPos curSector;
    GetSectorPos(x, y, curSector);
    GetSectorAround(curSector.x, curSector.y, curAround);

    // 캐릭터의 섹터 정보 업데이트
    c.SetSector(curSector);

    for (auto it = sector[oldSector.y][oldSector.x].begin(); it != sector[oldSector.y][oldSector.x].end(); it++) {
        if ((*it)->GetSessionId() == c.GetSessionId()) {
            sector[oldSector.y][oldSector.x].erase(it);
            break;
        }
    }
    sector[curSector.y][curSector.x].push_back(&c);
    // 공통 메시지 처리 함수 호출
    SendSectorCreateDeleteMsg(c, oldAround, curAround);
}

void OnCharacterConnect(Character& c) {
    //Timer t("OnCharacterConnect");
    stSectorAround curAround;
    stSectorPos curSector;
    Serial* serial = serial_32_pool.allocate();
    serial = new (serial) Serial();
    //serial->Init();

    int x = c.GetCurX();
    int y = c.GetCurY();

    GetSectorPos(x, y, curSector);
    GetSectorAround(curSector.x, curSector.y, curAround);

    c.SetSector(curSector);

    CreateOtherCharacterMsg(*serial, c.GetSessionId(), c.GetDir(), c.GetCurX(), c.GetCurY(), c.GetHp());
    for (int i = 0; i < curAround.count; i++) {
        SendSectorCreateMsg(c, *serial, curAround.around[i]);
    }
    serial->~Serial();
    serial_32_pool.deallocate(serial);
}

void OnCharacterDisconnect(Character& c/*나중에 dword로 아이디 받아서 검색 후 삭제 메시지*/) {
    //Timer t("OnCharacterDisconnect");
    Serial* serial = serial_32_pool.allocate();
    serial = new (serial) Serial();
    //serial->Init();
    DeleteCharacterMsg(*serial, c.GetSessionId());
    SendAround(c, *serial, true);

    stSectorPos oldSector = c.GetSector();
    for (auto it = sector[oldSector.y][oldSector.x].begin(); it != sector[oldSector.y][oldSector.x].end(); ++it) {
        if ((*it)->GetSessionId() == c.GetSessionId()) {
            sector[oldSector.y][oldSector.x].erase(it);
            break;
        }
    }
    players.erase(c.GetSessionId());
    delete& c;
    serial->~Serial();
    serial_32_pool.deallocate(serial);
}

void SendSectorCreateMsg(Character& c, Serial& msg, const stSectorPos& pos) {
    //Timer t("SendSectorCreateMsg");
    Serial* msg2 = serial_32_pool.allocate();
    msg2 = new (msg2) Serial();

    for (const auto it : sector[pos.y][pos.x]) {
        if (it->GetSessionId() == c.GetSessionId())continue;
        SendUniCast(*sessions[it->GetSessionId()], msg);
        CreateOtherCharacterMsg(*msg2, it->GetSessionId(), it->GetDir(), it->GetCurX(), it->GetCurY(), it->GetHp());

        if (it->GetAction() != dfPACKET_NONE_MOVE) {
            MoveStartMsg(*msg2, it->GetSessionId(), it->GetMoveDir(), it->GetCurX(), it->GetCurY());
        }

        SendUniCast(*sessions[c.GetSessionId()], *msg2);

        msg2->Clear();
    }

    msg2->~Serial();
    serial_32_pool.deallocate(msg2);
}

void SendSectorDeleteMsg(Character& c, Serial& msg, const stSectorPos& pos) {
    //Timer t("SendSectorDeleteMsg");
    Serial* delMsg = serial_32_pool.allocate();
    delMsg = new (delMsg) Serial();
    for (const auto it : sector[pos.y][pos.x]) {
        if (it->GetSessionId() == c.GetSessionId())continue;
        DeleteCharacterMsg(*delMsg, it->GetSessionId());
        SendUniCast(*sessions[it->GetSessionId()], msg);
        SendUniCast(*sessions[c.GetSessionId()], *delMsg);
        delMsg->Clear();
    }
    delMsg->~Serial();
    serial_32_pool.deallocate(delMsg);
}

void SendDamage(Character& c, Serial& msg, BYTE dir, BYTE type) {
    //Timer t("SendDamage");
    bool isHit;
    char dmg;
    short xRange;
    short yRange;
    int startX, endX, startY, endY;
    int x = c.GetCurX();
    int y = c.GetCurY();

    switch (type) {
    case dfPACKET_CS_ATTACK1:
        dmg = dfATTACK1_DAMAGE;
        xRange = dfATTACK1_RANGE_X;
        yRange = dfATTACK1_RANGE_Y;
        break;
    case dfPACKET_CS_ATTACK2:
        dmg = dfATTACK2_DAMAGE;
        xRange = dfATTACK2_RANGE_X;
        yRange = dfATTACK2_RANGE_Y;
        break;
    case dfPACKET_CS_ATTACK3:
        dmg = dfATTACK3_DAMAGE;
        xRange = dfATTACK3_RANGE_X;
        yRange = dfATTACK3_RANGE_Y;
        break;
    }
    //============범위 내 섹터 탐색
    if (dir) {
        startX = x / dfSECTOR_RANGE;
        endX = min(dfSECTOR_MAX_X - 1, (x + xRange) / dfSECTOR_RANGE);
    }
    else {
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
                    if ((it->GetCurX() > c.GetCurX()) &&
                        (it->GetCurX() < c.GetCurX() + xRange) &&
                        (it->GetCurY() > c.GetCurY() - yRange) &&
                        (it->GetCurY() < c.GetCurY() + yRange))
                    {
                        isHit = true;
                    }
                }
                else {
                    if ((it->GetCurX() > c.GetCurX() - xRange) &&
                        (it->GetCurX() < c.GetCurX()) &&
                        (it->GetCurY() > c.GetCurY() - yRange) &&
                        (it->GetCurY() < c.GetCurY() + yRange))
                    {
                        isHit = true;
                    }
                }
                if (isHit) {
                    msg.Clear();
                    it->HitByAttacker(dmg);
                    DamageMsg(msg, c.GetSessionId(), it->GetSessionId(), it->GetHp());
                    SendAround(*it, msg, true);
                }
            }
        }
    }
}

void LoadData() {
    startTime = timeGetTime();
    ownTime = startTime;
    srand(time(NULL));
    g_logLevel = LOG_LEVEL_ERROR;
    //if (dfRANGE_MOVE_RIGHT % dfSECTOR_RANGE == 0) {
    //    isSectorDivisionEqual0 = true;
    //}
    //else {
    //    isSectorDivisionEqual0 = false;
    //}
}

void Update() {
    //Timer t("Update");
    ++UpdateCnt;
    Character* c = NULL;
    std::map<DWORD, Character*>::iterator it;
    // std::unordered_map<DWORD, Character*>::iterator it;
    for (it = players.begin(); it != players.end();) {
        //for(it = players.begin();it!=players.end();){
        c = it->second;
        ++it;
        if (0 >= c->GetHp()) {
            //캐릭터 연결 끊을 시에 대한 작업은 나중에
            Disconnect(*sessions[c->GetSessionId()]);
        }
        else {
            if (curTime - c->GetLastRecvTime() >= dfNETWORK_PACKET_RECV_TIMEOUT) {
                _LOG(LOG_LEVEL_DEBUG, L"# TIMEOUT DISCONNECT # SessionId:%d\n", c->GetSessionId());
                Disconnect(*sessions[c->GetSessionId()]);
                continue;
            }
            if (c->GetAction() == dfPACKET_NONE_MOVE) {
                continue;
            }
            switch (c->GetAction()) {
            case dfPACKET_MOVE_DIR_LL:
                if (CheckCharacterMove(c->GetCurX() - dfSPEED_PLAYER_X, c->GetCurY()))
                {
                    c->Move(-dfSPEED_PLAYER_X, 0);
                }
                break;
            case dfPACKET_MOVE_DIR_LU:
                if (CheckCharacterMove(c->GetCurX() - dfSPEED_PLAYER_X, c->GetCurY() - dfSPEED_PLAYER_Y))
                {
                    c->Move(-dfSPEED_PLAYER_X, -dfSPEED_PLAYER_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_UU:
                if (CheckCharacterMove(c->GetCurX(), c->GetCurY() - dfSPEED_PLAYER_Y)) {
                    c->Move(0, -dfSPEED_PLAYER_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_RU:
                if (CheckCharacterMove(c->GetCurX() + dfSPEED_PLAYER_X, c->GetCurY() - dfSPEED_PLAYER_Y))
                {
                    c->Move(dfSPEED_PLAYER_X, -dfSPEED_PLAYER_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_RR:
                if (CheckCharacterMove(c->GetCurX() + dfSPEED_PLAYER_X, c->GetCurY()))
                {
                    c->Move(dfSPEED_PLAYER_X, 0);
                }
                break;
            case dfPACKET_MOVE_DIR_RD:
                if (CheckCharacterMove(c->GetCurX() + dfSPEED_PLAYER_X, c->GetCurY() + dfSPEED_PLAYER_Y))
                {
                    c->Move(dfSPEED_PLAYER_X, dfSPEED_PLAYER_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_DD:
                if (CheckCharacterMove(c->GetCurX(), c->GetCurY() + dfSPEED_PLAYER_Y))
                {
                    c->Move(0, dfSPEED_PLAYER_Y);
                }
                break;
            case dfPACKET_MOVE_DIR_LD:
                if (CheckCharacterMove(c->GetCurX() - dfSPEED_PLAYER_X, c->GetCurY() + dfSPEED_PLAYER_Y))
                {
                    c->Move(-dfSPEED_PLAYER_X, dfSPEED_PLAYER_Y);
                }
                break;
            }
            _LOG(LOG_LEVEL_DEBUG, L"# MOVES # SessionId:%d / Direction:%d / X:%d / Y:%d / curTime:%d\n", c->GetSessionId(), c->GetMoveDir(), c->GetCurX(), c->GetCurY(), curTime);

            if (CheckSectorUpdate(*c)) {
                SectorUpdate(*c);
            }
        }
    }
}