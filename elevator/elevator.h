//
// Created by Khalaf on 13/05/2026.
//

#ifndef STM32_TEMPLATE_ELEVATOR_H
#define STM32_TEMPLATE_ELEVATOR_H
/** * @brief Elevator States for the FSM [cite: 30]
 */
typedef enum {
    STATE_IDLE,
    STATE_MOVING_UP,
    STATE_MOVING_DOWN,
    STATE_DOORS_OPEN,
    STATE_EMERGENCY  // Triggered by Emergency Button [cite: 10]
} ElevatorState_t;

/**
 * @brief Directional tracking for the Task Allocation Algorithm
 */
typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_DOWN
} Direction_t;

/**
 * @brief Floor definitions (1-4) [cite: 5]
 */
typedef enum {
    FLOOR_1 = 0,
    FLOOR_2,
    FLOOR_3,
    FLOOR_4,
    FLOOR_UNKNOWN = 0xFF
} Floor_t;

/**
 * @brief Structure representing the real-time status of an elevator
 * Note: Use 'volatile' when this struct is updated in ISRs
 */
typedef struct {
    volatile ElevatorState_t current_state;
    volatile Floor_t current_floor;
    volatile Direction_t current_direction;
    volatile uint8_t internal_requests; // Bitmask: bit0=F1, bit1=F2...
    volatile uint8_t hallway_calls;     // Only used by Master [cite: 10]
} Elevator_t;

#endif //STM32_TEMPLATE_ELEVATOR_H
