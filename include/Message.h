//
// Created by khalaf on 4/6/26.
//

#ifndef EMBEDDED_PROJECT1_MESSAGE_H
#define EMBEDDED_PROJECT1_MESSAGE_H
#include "STD_TYPES.h"

/* Message Types */
#define MSG_TYPE_SENSOR_READING 0
#define MSG_TYPE_DEVICE_COMMAND 1

/* Priority Levels */
#define MSG_PRIORITY_NORMAL 0
#define MSG_PRIORITY_HIGH   1

/* Message Payload Length */
#define MSG_PAYLOAD_LENGTH 4


typedef union {
    uint32 fullword;
    uint16 halfword[2];
    uint8  quarter[4];
} Payload_t;

/* The Main Message Struct*/
typedef struct {
    uint8     header;  /* Byte 0: Bits 2:0 = Type, Bit 3 = Prio, Bits 7:4 = Seq */
    uint8     length;  /* Byte 1: Always set to 4 */
    Payload_t payload; /* Bytes 2-5: Fixed 4-byte data field */
} __attribute__((packed)) Message_t;

typedef void (*MsgHandler_t)(Message_t *DecodedMsg);

Std_ReturnType Msg_Encode(uint8 Type, uint8 Priority, Payload_t *InPayload, P_void OutBuffer);
Std_ReturnType PreProcess_SensorReading(uint16 Temperature, uint16 Humidity, Payload_t *OutPayload);
Std_ReturnType PreProcess_DeviceCommand(uint8 CommandID, uint8 Parameter, Payload_t *OutPayload);
Std_ReturnType Msg_RegisterHandler(uint8 MsgType, MsgHandler_t Handler);
Std_ReturnType Msg_Decode(P_void InBuffer, Message_t *OutMsg);

#endif //EMBEDDED_PROJECT1_MESSAGE_H
