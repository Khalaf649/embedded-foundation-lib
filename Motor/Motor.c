#include "Motor.h"

static const uint8 motorSpeedMapper[3] = {0, 20, 100};

void Motor_Init(void) {

    GPIO_Pin_Location_t motor_pin = {MOTOR_PORT, MOTOR_PIN};
    GPIO_PinConfig_t motor_cfg;

    motor_cfg.mode  = GPIO_MODE_AF;
    motor_cfg.pull  = GPIO_PULL_NONE;
    motor_cfg.speed = GPIO_SPEED_HIGH;
    motor_cfg.otype = GPIO_OTYPE_PP;

    GPIO_InitPin(&motor_pin, &motor_cfg);

    GPIO_SetAlternateFunction(&motor_pin, GPIO_AF_2);

    Pwm_Init(MOTOR_TIMER, MOTOR_CHANNEL, 15, 100);
    Pwm_Start(MOTOR_TIMER, MOTOR_CHANNEL);

    Motor_SetSpeed(MOTOR_REST);
}

void Motor_SetSpeed(Motor_Speed_t speed_lvl) {
    if (speed_lvl <= MOTOR_HIGH_SPEED) {

        uint8 duty = motorSpeedMapper[speed_lvl];

        Pwm_SetDutyPercent(MOTOR_TIMER, MOTOR_CHANNEL, duty);

        switch(speed_lvl) {
            case MOTOR_REST:       Usart1_TransmitString("MOTOR: STOP (0%)\r\n");   break;
            case MOTOR_LOW_SPEED:  Usart1_TransmitString("MOTOR: SLOW (20%)\r\n");   break;
            case MOTOR_HIGH_SPEED: Usart1_TransmitString("MOTOR: FULL (100%)\r\n"); break;
        }
    }
}