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


typedef union {
    uint32 fullword;
    uint16 halfword[2];
    uint8  quarter[4];
} Payload_t;

/* The Main Message Struct (Exactly 6 Bytes) */
typedef struct {
    Payload_t payload; /* Bytes 2-5: Fixed 4-byte data field */
    uint8     header;  /* Byte 0: Bits 2:0 = Type, Bit 3 = Prio, Bits 7:4 = Seq */
    uint8     length;  /* Byte 1: Always set to 4 */
} __attribute__((packed)) Message_t;



#endif //EMBEDDED_PROJECT1_MESSAGE_H
