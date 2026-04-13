//
// Created by khalaf on 4/6/26.
//
#include "Message.h"
#include "STD_TYPES.h"
#include <stddef.h>
#include <stdio.h>

#include "BIT_MATH.h"


static uint8 Msg_SeqNum = 0;
static MsgHandler_t HandlerTable[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
/* Preprocessor for Sensor Reading Data */
Std_ReturnType PreProcess_SensorReading(uint16 Temperature, uint16 Humidity, Payload_t *OutPayload) {
    if (OutPayload == NULL) {
        return E_NOT_OK;
    }

    /* Lower 16 bits = temperature, upper 16 bits = humidity  */
    OutPayload->fullword = ((uint32)Temperature) | (((uint32)Humidity) << 16);

    return E_OK;
}
/* Preprocessor for Device Command Data */
Std_ReturnType PreProcess_DeviceCommand(uint8 CommandID, uint8 Parameter, Payload_t *OutPayload) {
    if (OutPayload == NULL) {
        return E_NOT_OK;
    }

    /* 1-byte command ID + 1-byte parameter + 2 bytes unused (zero-filled)  */
    OutPayload->quarter[0] = CommandID;
    OutPayload->quarter[1] = Parameter;
    OutPayload->quarter[2] = 0x00; /* Zero-filled  */
    OutPayload->quarter[3] = 0x00; /* Zero-filled  */

    return E_OK;
}

Std_ReturnType Msg_Encode(uint8 Type, uint8 Priority, Payload_t *InPayload, P_void OutBuffer) {
    if (InPayload == NULL || OutBuffer == NULL) {
        return E_NOT_OK;
    }
    Message_t *OutMsg = (Message_t *)OutBuffer;
    OutMsg->header=0;
    WRITE_BIT_FIELD(OutMsg->header,0x07, 0,3,Type); /* Bits 2:0 */
    WRITE_BIT_FIELD(OutMsg->header,0x01,3,1,Priority); /* Bit 3    */
    WRITE_BIT_FIELD(OutMsg->header,0x0F,1,4,Msg_SeqNum); /* Bits 7:4 */

    /* Auto-increment Sequence Number */
    Msg_SeqNum = (Msg_SeqNum + 1) & 0x0F;

    /* Set the Fixed Length */
    OutMsg->length = MSG_PAYLOAD_LENGTH;

    OutMsg->payload=*InPayload;

    return E_OK;

}

Std_ReturnType Msg_RegisterHandler(uint8 MsgType, MsgHandler_t Handler) {
    /* Message type must be 0-7. */
    if (MsgType > 7) {
        return E_NOT_OK;
    }

    /* Store the function pointer in our encapsulated array */
    HandlerTable[MsgType] = Handler;

    return E_OK;
}

Std_ReturnType Msg_Decode(P_void InBuffer, Message_t *OutMsg) {

    if (InBuffer == NULL || OutMsg == NULL) {
        return E_NOT_OK;
    }

    uint8 *RawBytes = (uint8 *)InBuffer;

    OutMsg->header = RawBytes[0];
    OutMsg->length = RawBytes[1];

    OutMsg->payload.quarter[0] = RawBytes[2];
    OutMsg->payload.quarter[1] = RawBytes[3];
    OutMsg->payload.quarter[2] = RawBytes[4];
    OutMsg->payload.quarter[3] = RawBytes[5];

    uint8 MsgType = (OutMsg->header & 0x07);


    if (HandlerTable[MsgType] != NULL) {
        HandlerTable[MsgType](OutMsg);
    }

    return E_OK;
}
Std_ReturnType assert_size() {
    printf("the size of the message struct %zu\n", sizeof(Message_t));
}
