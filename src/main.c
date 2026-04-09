#include <stdio.h>
#include "STD_TYPES.h"
#include "MemScanner.h"
#include "Message.h"
#include "BIT_MATH.h"



void SensorData_Handler(Message_t *Msg) {
    printf("\n--- SENSOR_READING ---\n");
    printf("32-bit: 0x%08X\n", Msg->payload.fullword);
    printf("16-bit: [0]:%u, [1]:%u\n", Msg->payload.halfword[0], Msg->payload.halfword[1]);
    printf("8-bit:  0x%02X 0x%02X 0x%02X 0x%02X\n",
            Msg->payload.quarter[0], Msg->payload.quarter[1],
            Msg->payload.quarter[2], Msg->payload.quarter[3]);
}

void Command_Handler(Message_t *Msg) {
    printf("\n--- DEVICE_COMMAND ---\n");
    printf("ID: 0x%02X, Param: 0x%02X\n", Msg->payload.quarter[0], Msg->payload.quarter[1]);
}


void Setup_MessageHandlers(void) {
    Msg_RegisterHandler(MSG_TYPE_SENSOR_READING, SensorData_Handler);
    Msg_RegisterHandler(MSG_TYPE_DEVICE_COMMAND, Command_Handler);
}

void Demo_MessageProcessing(uint8* SensorBuffer, uint8* CommandBuffer) {
    Message_t DummyResult;
    Payload_t TempPayload;
    Payload_t SensorPayload;

    /* Encode Sensor & Command */
    PreProcess_SensorReading(255, 600, &TempPayload);
    Msg_Encode(MSG_TYPE_SENSOR_READING, MSG_PRIORITY_NORMAL, &TempPayload, SensorBuffer);

    PreProcess_DeviceCommand(0x01, 0xFF, &SensorPayload);
    Msg_Encode(MSG_TYPE_DEVICE_COMMAND, MSG_PRIORITY_HIGH, &SensorPayload, CommandBuffer);

    /* Decode & Trigger Handlers */
    printf("Processing Sensor Data...");
    Msg_Decode(SensorBuffer, &DummyResult);

    printf("Processing Device Command...");
    Msg_Decode(CommandBuffer, &DummyResult);
    printf("---------------------------------------------------------\n");
}

void Demo_MemoryReadOperations(uint8* SensorBuffer, uint8* CommandBuffer) {
    /* Raw Buffer Hex Dumps */
    printf("Sensor Buffer  : ");
    MemScan_HexDump(SensorBuffer, 6);
    printf("Command Buffer : ");
    MemScan_HexDump(CommandBuffer, 6);
    printf("---------------------------------------------------------\n");

    /* Read Header and Length */
    uint8 sensorHeader, deviceHeader, sensorLength, deviceLength;
    MemScan_ReadByte(SensorBuffer, 0, &sensorHeader);
    MemScan_ReadByte(CommandBuffer, 0, &deviceHeader);
    MemScan_ReadByte(SensorBuffer, 1, &sensorLength);
    MemScan_ReadByte(CommandBuffer, 1, &deviceLength);

    printf("Sensor  -> Header: 0x%02X, Length: %u\n", sensorHeader, sensorLength);
    printf("Command -> Header: 0x%02X, Length: %u\n", deviceHeader, deviceLength);
    printf("---------------------------------------------------------\n");

    /* Read Word and HalfWord */
    uint16 temp;
    uint32 word;
    MemScan_ReadHalfWord(SensorBuffer, 2, &temp);
    MemScan_ReadWord(SensorBuffer, 2, &word);

    printf("HalfWord (Index 2) : %u\n", temp);
    printf("Word     (Index 2) : 0x%08X\n", word);
    printf("---------------------------------------------------------\n");
}

void Demo_MemoryWriteAndSearch(uint8* SensorBuffer) {
    /* Buffer Modification with BIT_MATH */
    uint8 copyBuffer[6];
    for (uint8 i = 0; i < 6; i++) {
        copyBuffer[i] = SensorBuffer[i];
    }

    uint8 updatedHeader = copyBuffer[0];
    WRITE_BIT_FIELD(updatedHeader, 0x01, 3, 1, MSG_PRIORITY_HIGH);
    MemScan_WriteByte(copyBuffer, 0, updatedHeader);

    printf("Original Buffer : ");
    MemScan_HexDump(SensorBuffer, 6);
    printf("Modified Buffer : ");
    MemScan_HexDump(copyBuffer, 6);
    printf("---------------------------------------------------------\n");

    /* Memory Filling */
    uint8 emptyBuffer[6];
    MemScan_Fill(emptyBuffer, 6, 0xAA);
    printf("Filled Buffer   : ");
    MemScan_HexDump(emptyBuffer, 6);
    printf("---------------------------------------------------------\n");

    /* Memory Comparison */
    uint8 buffer1[6], buffer2[6];
    for (uint8 i = 0; i < 6; i++) {
        buffer1[i] = buffer2[i] = SensorBuffer[i];
    }

    uint32 offset1, offset2;
    MemScan_Compare(buffer1, buffer2, 6, &offset1);

    MemScan_WriteByte(buffer1, 1, 0xAA); /* Inject change */
    MemScan_Compare(buffer1, buffer2, 6, &offset2);

    printf("Offset 1 (Identical) : %u\n", offset1);
    printf("Offset 2 (Modified)  : %u\n", offset2);
    printf("---------------------------------------------------------\n");

    /* Memory Find Byte */
    sint32 findOffset1, findOffset2;
    MemScan_FindByte(SensorBuffer, 6, 0x58, &findOffset1); /* Exists */
    MemScan_FindByte(SensorBuffer, 6, 0xEE, &findOffset2); /* Missing */

    printf("Search for 0x58 (Exists) : %d\n", findOffset1);
    printf("Search for 0xEE (Missing): %d\n", findOffset2);
    printf("=========================================================\n\n");
}



int main(void) {
    uint8 SensorBuffer[6];
    uint8 CommandBuffer[6];

    Setup_MessageHandlers();

    Demo_MessageProcessing(SensorBuffer, CommandBuffer);
    Demo_MemoryReadOperations(SensorBuffer, CommandBuffer);
    Demo_MemoryWriteAndSearch(SensorBuffer);

    return 0;
}