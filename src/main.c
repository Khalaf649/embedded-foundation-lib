#include <stdio.h>
#include "STD_TYPES.h"
#include "MemScanner.h"
#include "Message.h"

/* Sensor Data Handler */
void SensorData_Handler(Message_t *Msg) {
    printf("\n--- SENSOR_READING ---\n");
    printf("32-bit: 0x%08X\n", Msg->payload.fullword);
    printf("16-bit: [0]:%u, [1]:%u\n", Msg->payload.halfword[0], Msg->payload.halfword[1]);
    printf("8-bit:  0x%02X 0x%02X 0x%02X 0x%02X\n",
            Msg->payload.quarter[0], Msg->payload.quarter[1],
            Msg->payload.quarter[2], Msg->payload.quarter[3]);
}

/* Device Command Handler */
void Command_Handler(Message_t *Msg) {
    printf("\n--- DEVICE_COMMAND ---\n");
    printf("ID: 0x%02X, Param: 0x%02X\n", Msg->payload.quarter[0], Msg->payload.quarter[1]);
}
int main(void) {
    uint8 SensorBuffer[6], CommandBuffer[6];
    Message_t DummyResult;
    Payload_t TempPayload;

    /* Register Handlers */
    Msg_RegisterHandler(MSG_TYPE_SENSOR_READING, SensorData_Handler);
    Msg_RegisterHandler(MSG_TYPE_DEVICE_COMMAND, Command_Handler);

    /*  Encode Sensor (25.5C -> 255, 60.0% -> 600) */
    PreProcess_SensorReading(255, 600, &TempPayload);
    Msg_Encode(MSG_TYPE_SENSOR_READING, MSG_PRIORITY_NORMAL, &TempPayload, SensorBuffer);

    /*  Encode Command (ID 0x01, Param 0xFF) */
    PreProcess_DeviceCommand(0x01, 0xFF, &TempPayload);
    Msg_Encode(MSG_TYPE_DEVICE_COMMAND, MSG_PRIORITY_HIGH, &TempPayload, CommandBuffer);

    /*  Decode & Trigger Handlers  */
    printf("Processing Sensor Data...");
    Msg_Decode(SensorBuffer, &DummyResult);

    printf("Processing Device Command...");
    Msg_Decode(CommandBuffer, &DummyResult);

    return 0;
}