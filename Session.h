#pragma once
#include <winsock2.h>
#include "RingBuffer.h"

struct Session {
    SOCKET sock;
    DWORD sessionId;
    RingBuffer* RecvQ;
    RingBuffer* SendQ;
    bool isDelete;
    
    Session(SOCKET sessionSock, DWORD id);
    ~Session();
};
