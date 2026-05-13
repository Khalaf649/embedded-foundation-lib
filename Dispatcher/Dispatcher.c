// //
// // Created by zramd on 13/05/2026.
// //
//
// #include "Dispatcher.h"
// #include "../IPC/Ipc.h"
//
// #define SCORE_IMMEDIATE    0
// #define SCORE_PERFECT      1
// #define SCORE_PASSED       2
// #define SCORE_IDLE         3
// #define SCORE_OPPOSITE     4
//
// // Evaluates a single elevator against a hallway call
// uint8 Calculate_Dispatch_Score(Ipc_Packet_t* elev, uint8 call_floor, uint8 call_dir_is_up) {
//     if (elev->state == ELEV_IDLE && elev->current_floor == call_floor) {
//         return SCORE_IMMEDIATE;
//     }
//
//     if (elev->state == ELEV_MOVING_UP && call_dir_is_up) {
//         if (elev->current_floor < call_floor) return SCORE_PERFECT;
//         else return SCORE_PASSED;
//     }
//
//     if (elev->state == ELEV_MOVING_DOWN && !call_dir_is_up) {
//         if (elev->current_floor > call_floor) return SCORE_PERFECT;
//         else return SCORE_PASSED;
//     }
//
//     if (elev->state == ELEV_IDLE) {
//         return SCORE_IDLE;
//     }
//
//     return SCORE_OPPOSITE;
// }
//
// // Master uses this to decide who takes the call
// uint8 Dispatch_Call(Ipc_Packet_t* master_elev, Ipc_Packet_t* slave_elev, uint8 call_floor, uint8 call_dir_is_up) {
//     uint8 scoreA = Calculate_Dispatch_Score(master_elev, call_floor, call_dir_is_up);
//     uint8 scoreB = Calculate_Dispatch_Score(slave_elev, call_floor, call_dir_is_up);
//
//     // Lower score wins. Master takes ties.
//     if (scoreA <= scoreB && scoreA != SCORE_OPPOSITE) {
//         return 1; // Assign to Master
//     } else if (scoreB != SCORE_OPPOSITE) {
//         return 2; // Assign to Slave
//     }
//
//     return 0; // Wait, both moving opposite
// }