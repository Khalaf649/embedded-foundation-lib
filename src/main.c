#include <stdio.h>
#include "STD_TYPES.h"
#include "MemScanner.h"
int main(void) {
    uint32 y = 0x11223344;

    MemScan_HexDump(&y,5);
}