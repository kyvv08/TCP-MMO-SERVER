#include "Session.h"

Session::Session(SOCKET sessionSock, DWORD id) : sock(sessionSock), sessionId(id), isDelete(false) {
    RecvQ = new RingBuffer(1024 * 4);
    SendQ = new RingBuffer(1024 * 4);
}

Session::~Session() {
    delete RecvQ;
    delete SendQ;
}
