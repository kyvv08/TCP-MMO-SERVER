#include "NetworkManager.h"
#include "Defines.h"
#include "Protocol.h"
#include "memoryPool.cpp"
#include <iostream>

extern MemoryPool<SerialBuffer> serial_32_pool;
extern void _LOG(int LogLevel, const wchar_t* format, ...);

NetworkManager::NetworkManager() : listen_sock(INVALID_SOCKET), dw_sessionId(0) {
    tVal = { 0, 0 };
}

NetworkManager::~NetworkManager() {
    NetClean();
}

bool NetworkManager::NetStart(int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if(bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) return false;
    if(listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) return false;

    u_long on = 1;
    ioctlsocket(listen_sock, FIONBIO, &on);

    return true;
}

void NetworkManager::NetClean() {
    if(listen_sock != INVALID_SOCKET) {
        closesocket(listen_sock);
        listen_sock = INVALID_SOCKET;
    }
    WSACleanup();
}

void NetworkManager::Disconnect(Session& s) {
    s.isDelete = true;
    closesocket(s.sock);
    OnCharacterDisconnect(s.sessionId);
}

void NetworkManager::SendUniCast(Session& s, SerialBuffer& serial) {
    int len = serial.GetDataSize();
    int ret = s.SendQ->Enqueue(serial.GetBufferPtr(), len);
    if (ret != len) {
        Disconnect(s);
    }
}

void NetworkManager::AcceptProc() {
    SOCKADDR_IN clientAddr;
    DWORD addrlen = sizeof(clientAddr);
    SOCKET clientSock = accept(listen_sock, (SOCKADDR*)&clientAddr, (int*)&addrlen);
    if (clientSock == INVALID_SOCKET) return;

    LINGER ling; ling.l_onoff = 1; ling.l_linger = 0;
    setsockopt(clientSock, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

    Session* session = new Session(clientSock, dw_sessionId);
    sessions[dw_sessionId] = session;

    SerialBuffer* serial = serial_32_pool.allocate();
    serial = new (serial) SerialBuffer();

    CreateNewCharacter(*session, *serial);

    serial->~SerialBuffer();
    serial_32_pool.deallocate(serial);
    dw_sessionId++;
}

void NetworkManager::RecvPacket(Session& s) {
    char* buf = s.RecvQ->GetBufferRear();
    int len = s.RecvQ->DirectEnqueue();
    int recvRet = recv(s.sock, buf, len, 0);

    if (recvRet == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) Disconnect(s);
        return;
    } else if (recvRet == 0) {
        Disconnect(s);
        return;
    }
    s.RecvQ->MoveRear(recvRet);

    SerialBuffer* serial = serial_32_pool.allocate();
    serial = new (serial) SerialBuffer();

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
            Disconnect(s); break;
        }

        *serial >> header.bySize;

        // 보안 로직: 버퍼 사이즈 해킹 필터링 보호
        if(header.bySize > dfMAX_PACKET_SIZE) {
            Disconnect(s); break; 
        }

        if (checkRecv < sizeof(st_PACKET_HEADER) + header.bySize) break;
        *serial >> header.byType;
        s.RecvQ->MoveFront(sizeof(st_PACKET_HEADER));

        serial->Clear();
        s.RecvQ->Dequeue(serial->GetBufferPtr(), header.bySize);
        serial->MoveWritePos(header.bySize);
        serial->SetDataSize(header.bySize);
        
        PacketProc(s, header.byType, *serial);
    }
    serial->~SerialBuffer();
    serial_32_pool.deallocate(serial);
}

void NetworkManager::SendPacket(Session& s) {
    char* buf = s.SendQ->GetBufferFront();
    int len = s.SendQ->DirectDequeue();
    int ret = send(s.sock, buf, len, 0);
    if (ret == SOCKET_ERROR) {
        DWORD err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) Disconnect(s);
        return;
    }
    s.SendQ->MoveFront(ret);
}

void NetworkManager::NetworkManage() {
    if(sessions.empty() && listen_sock == INVALID_SOCKET) return;

    auto it = sessions.begin();
    auto selectIt = sessions.begin();
    FD_SET rset, wset;

    do {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(listen_sock, &rset);
        int clientNum = 0;

        for (; it != sessions.end() && clientNum < 63;) {
            if (it->second->isDelete) {
                Session* temp = it->second;
                if (selectIt == it) ++selectIt;
                it = sessions.erase(it);
                delete temp;
            } else {
                FD_SET(it->second->sock, &rset);
                if (it->second->SendQ->GetUseSize() > 0) {
                    FD_SET(it->second->sock, &wset);
                }
                ++clientNum; ++it;
            }
        }

        int selectRet = select(0, &rset, &wset, NULL, &tVal);
        if (FD_ISSET(listen_sock, &rset)) {
            AcceptProc();
            --selectRet;
        }

        for (; selectIt != it; selectIt++) {
            if (selectRet == 0) { selectIt = it; break; }
            if (FD_ISSET(selectIt->second->sock, &rset)) {
                RecvPacket(*selectIt->second);
                --selectRet;
            }
            if (FD_ISSET(selectIt->second->sock, &wset)) {
                --selectRet;
                if (selectIt->second->isDelete) continue;
                SendPacket(*selectIt->second);
            }
        }
    } while (it != sessions.end());
}
