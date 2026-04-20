#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <winsock2.h>
#include <windows.h>
#include "SerialBuffer.h"

namespace PacketCreator {
    void CreateMyCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp);
    void CreateOtherCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp);
    void DeleteCharacterMsg(SerialBuffer& serial, DWORD id);
    void MoveStartMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y);
    void MoveStopMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y);
    void Attack1Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y);
    void Attack2Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y);
    void Attack3Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y);
    void DamageMsg(SerialBuffer& serial, DWORD atkId, DWORD defId, CHAR hp);
    void EchoMsg(SerialBuffer& serial, DWORD tick);
    void SyncMsg(SerialBuffer& serial, DWORD id, WORD x, WORD y);
}
