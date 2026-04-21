#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"ws2_32.lib")

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h> // timeGetTime 및 timeBeginPeriod 식별자를 위해 명시적 추가
#include <iostream>
#include <conio.h>
#include <chrono>

#include "Defines.h"
#include "Protocol.h"
#include "NetworkManager.h"
#include "GameManager.h"

// 메모리 풀 헤더 포함
#include "MemoryPool.h"
#include "SerialBuffer.h"

#define dfSERVER_PORT 11850

// -------------- 글로벌 전역 변수 --------------
int g_logLevel = 1;
FILE* logfp = nullptr;
DWORD curTime;
DWORD ownTime;
DWORD startTime;
bool ShutDown = false;
DWORD frame = 0;
int UpdateCnt = 0;

MemoryPool<SerialBuffer> serial_32_pool;

// -------------- 로깅 함수 --------------
std::wstring GetCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_s(&local_tm, &now_time_t); 
    wchar_t buffer[64];
    std::wcsftime(buffer, sizeof(buffer) / sizeof(wchar_t), L"[%Y-%m-%d] [%H:%M:%S]", &local_tm);
    return std::wstring(buffer);
}

std::wstring FormatString(const wchar_t* format, ...) {
    wchar_t buffer[1024]; 
    va_list args;
    va_start(args, format);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
    va_end(args);
    return std::wstring(buffer);
}

void _LOG(int level, const wchar_t* format, ...) {
    if (g_logLevel > level) return;
    wchar_t buffer[1024]; 
    va_list args;
    va_start(args, format);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
    va_end(args);

    fopen_s(&logfp, "log.txt", "a, ccs=UTF-8");
    if (logfp) {
        std::wstring timestampedStr = GetCurrentDateTime() + L" " + std::wstring(buffer) + L"\n";
        std::fputws(timestampedStr.c_str(), logfp);
        std::fflush(logfp);
        fclose(logfp);
    }
}

// -------------- NetworkManager 기반 브릿지 --------------
void CreateNewCharacter(Session& s, SerialBuffer& serial) {
    BYTE dir = dfPACKET_MOVE_DIR_LL;
    WORD x = rand() % dfRANGE_MOVE_RIGHT;
    WORD y = rand() % dfRANGE_MOVE_BOTTOM;

    Character* player = new Character(s.sessionId, dir, x, y);
    GameManager::GetInstance().GetPlayers()[s.sessionId] = player;
    GameManager::GetInstance().OnCharacterConnect(*player);
    player->SetLastRecvTime(timeGetTime());
}

void PacketProc(Session& s, BYTE type, SerialBuffer& serial) {
    switch (type) {
    case dfPACKET_CS_MOVE_START: GameManager::GetInstance().PacketProc_MoveStart(s, serial); break;
    case dfPACKET_CS_MOVE_STOP:  GameManager::GetInstance().PacketProc_MoveStop(s, serial); break;
    case dfPACKET_CS_ATTACK1:    GameManager::GetInstance().PacketProc_Attack1(s, serial); break;
    case dfPACKET_CS_ATTACK2:    GameManager::GetInstance().PacketProc_Attack2(s, serial); break;
    case dfPACKET_CS_ATTACK3:    GameManager::GetInstance().PacketProc_Attack3(s, serial); break;
    case dfPACKET_CS_ECHO:       GameManager::GetInstance().PacketProc_Echo(s, serial); break;
    default: NetworkManager::GetInstance().Disconnect(s); break;
    }
}

void OnCharacterDisconnect(DWORD sessionId) {
    auto& players = GameManager::GetInstance().GetPlayers();
    if (players.count(sessionId)) {
        Character* c = players[sessionId];
        GameManager::GetInstance().OnCharacterDisconnect(*c);
        players.erase(sessionId);
    }
}

// -------------- 프로세스 제어 및 메인 --------------
void ServerControl() {
    static bool ControlMode = false;
    if (_kbhit()) {
        WCHAR key = _getwch();
        if (L'u' == key || L'U' == key) {
            ControlMode = true;
            wprintf(L"Control Mode : Press Q - Quit\n");
            wprintf(L"Control Mode : Press L - Key Lock\n");
        }
        if (L'l' == key || L'L' == key) {
            ControlMode = false;
            wprintf(L"Control Lock : Press U - Control Unlock\n");
        }
        if ((L'q' == key || L'Q' == key) && ControlMode) {
            ShutDown = true;
        }
    }
}

void Monitoring() {
    if (curTime >= startTime + 1000) {
        std::cout << "fps: " << frame << " sessions: " << NetworkManager::GetInstance().GetSessions().size() 
                  << " players: " << GameManager::GetInstance().GetPlayers().size() << std::endl;
        frame = UpdateCnt = 0;
        startTime += 1000;
    }
}

int wmain() {
    timeBeginPeriod(1);
    srand((unsigned int)time(NULL));

    if (!NetworkManager::GetInstance().NetStart(dfSERVER_PORT)) {
        std::cout << "Network Start Failed." << std::endl;
        return 1;
    }
    
    startTime = ownTime = timeGetTime();

    while (!ShutDown) {
        NetworkManager::GetInstance().NetworkManage();
        
        curTime = timeGetTime();
        if (curTime >= ownTime + 40) {
            GameManager::GetInstance().Update();
            ownTime += 40;
            ++frame;
        }

        ServerControl();
        Monitoring();
    }

    NetworkManager::GetInstance().NetClean();
    return 0;
}