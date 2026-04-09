//
// Created by khalaf on 3/23/26.
//
#include <stddef.h>
#include <stdio.h>

#include "STD_TYPES.h"
#include "MemScanner.h"

/* A3: Memory Read Operations */

Std_ReturnType MemScan_ReadByte(P_void BaseAddr, uint32 Offset, uint8 *OutData) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL || OutData == NULL) return E_NOT_OK;
    *(OutData) = *(p + Offset);
    return E_OK;
}

Std_ReturnType MemScan_ReadHalfWord(P_void BaseAddr, uint32 Offset, uint16 *OutData) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL || OutData == NULL) return E_NOT_OK;

    *OutData = *((uint16*)(p + Offset));

    return E_OK;
}

Std_ReturnType MemScan_ReadWord(P_void BaseAddr, uint32 Offset, uint32 *OutData) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL || OutData == NULL) return E_NOT_OK;

    *OutData = *((uint32*)(p + Offset));

    return E_OK;
}

/* A3: Memory Write Operations */

Std_ReturnType MemScan_WriteByte(P_void BaseAddr, uint32 Offset, uint8 Data) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL) return E_NOT_OK;

    p[Offset] = Data; // sugary syntax instead of *(p+offset)=data

    return E_OK;
}

Std_ReturnType MemScan_Fill(P_void BaseAddr, uint32 Size, uint8 Data) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL) return E_NOT_OK;

    for (uint32 i = 0; i < Size; i++) {
        p[i] = Data;
    }

    return E_OK;
}

/* A3: Memory Inspection Operations */

Std_ReturnType MemScan_HexDump(P_void BaseAddr, uint32 Size) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL) return E_NOT_OK;

    for (uint32 i = 0; i < Size; i++) {
        printf("%02X ", p[i]);
    }
    printf("\n");

    return E_OK;
}

Std_ReturnType MemScan_Compare(P_void BaseAddr1, P_void BaseAddr2, uint32 Size, uint32 *OutOffset) {
    uint8* p1 = (uint8*)BaseAddr1;
    uint8* p2 = (uint8*)BaseAddr2;

    if (p1 == NULL || p2 == NULL || OutOffset == NULL) return E_NOT_OK;

    for (uint32 i = 0; i < Size; i++) {
        if (p1[i] != p2[i]) {
            *(OutOffset) = i + 1; // 1-based
            return E_OK;
        }
    }

    *(OutOffset) = 0;
    return E_OK;
}

Std_ReturnType MemScan_FindByte(P_void BaseAddr, uint32 Size, uint8 Data, sint32 *OutOffset) {
    uint8* p = (uint8*)BaseAddr;
    if (p == NULL || OutOffset == NULL) return E_NOT_OK;

    for(uint32 i = 0; i < Size; i++) {
        if (p[i] == Data) {
            *OutOffset = i; /* 0-indexed offset of the first occurrence */
            return E_OK;
        }
    }
    *OutOffset=-1;
    return E_OK;
}