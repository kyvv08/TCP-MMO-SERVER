#include "PacketCreator.h"
#include "Defines.h"

namespace PacketCreator {
    void CreateMyCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)10 << (BYTE)dfPACKET_SC_CREATE_MY_CHARACTER << id << dir << x << y << hp;
    }
    void CreateOtherCharacterMsg(SerialBuffer& serial, DWORD id, BYTE dir, WORD x, WORD y, BYTE hp) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)10 << (BYTE)dfPACKET_SC_CREATE_OTHER_CHARACTER << id << dir << x << y << hp;
    }
    void DeleteCharacterMsg(SerialBuffer& serial, DWORD id) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)4 << (BYTE)dfPACKET_SC_DELETE_CHARACTER << id;
    }
    void MoveStartMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_MOVE_START << id << dir << x << y;
    }
    void MoveStopMsg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_MOVE_STOP << id << dir << x << y;
    }
    void Attack1Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK1 << id << dir << x << y;
    }
    void Attack2Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK2 << id << dir << x << y;
    }
    void Attack3Msg(SerialBuffer& serial, DWORD id, CHAR dir, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_ATTACK3 << id << dir << x << y;
    }
    void DamageMsg(SerialBuffer& serial, DWORD atkId, DWORD defId, CHAR hp) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)9 << (BYTE)dfPACKET_SC_DAMAGE << atkId << defId << hp;
    }
    void EchoMsg(SerialBuffer& serial, DWORD tick) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)4 << (BYTE)dfPACKET_SC_ECHO << tick;
    }
    void SyncMsg(SerialBuffer& serial, DWORD id, WORD x, WORD y) {
        serial << (BYTE)dfPACKET_CODE << (BYTE)8 << (BYTE)dfPACKET_SC_SYNC << id << x << y;
    }
}
