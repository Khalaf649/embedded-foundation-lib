//
// Created by khalaf on 3/25/26.
//

#ifndef MEMSCANNER_H_
#define MEMSCANNER_H_

#include "STD_TYPES.h"

/* A3: Memory Read Operations */
Std_ReturnType MemScan_ReadByte(P_void BaseAddr, uint32 Offset, uint8 *OutData);
Std_ReturnType MemScan_ReadHalfWord(P_void BaseAddr, uint32 Offset, uint16 *OutData);
Std_ReturnType MemScan_ReadWord(P_void BaseAddr, uint32 Offset, uint32 *OutData);

/* A3: Memory Write Operations */
Std_ReturnType MemScan_WriteByte(P_void BaseAddr, uint32 Offset, uint8 Data);
Std_ReturnType MemScan_Fill(P_void BaseAddr, uint32 Size, uint8 Data);

/* A3: Memory Inspection Operations */
Std_ReturnType MemScan_HexDump(P_void BaseAddr, uint32 Size);
Std_ReturnType MemScan_Compare(P_void BaseAddr1, P_void BaseAddr2, uint32 Size, uint32 *OutOffset);
Std_ReturnType MemScan_FindByte(P_void BaseAddr, uint32 Size, uint8 Data, sint32 *OutOffset);

#endif /* MEMSCANNER_H_ */