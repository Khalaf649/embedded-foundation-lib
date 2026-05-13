#include "Ipc.h"

uint8 IPC_CalculateChecksum(Ipc_Packet_t* packet) {
    uint8* ptr = (uint8*)packet;
    uint8 sum = 0;
    // XOR the first 7 bytes
    for(int i = 0; i < 7; i++) {
        sum ^= ptr[i];
    }
    return sum;
}