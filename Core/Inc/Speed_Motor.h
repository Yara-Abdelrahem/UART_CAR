#ifndef __SPEED_MOTOR_H
#define __SPEED_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "uart.h"
#include <stdint.h>

/* === External handles === */
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;

/* === PWM Mapping === */
#define MOTOR1_PWM_TIMER    (&htim4)
#define MOTOR1_PWM_CHANNEL  TIM_CHANNEL_3

#define MOTOR2_PWM_TIMER    (&htim4)
#define MOTOR2_PWM_CHANNEL  TIM_CHANNEL_4

/* === Direction Pin Mapping === */
#define MOTOR1_DIR_PORT     GPIOB
#define MOTOR1_DIR_PIN      GPIO_PIN_4

#define MOTOR2_DIR_PORT     GPIOB
#define MOTOR2_DIR_PIN      GPIO_PIN_5

/* === Function Prototypes === */


int32_t Encoder_ReadPosition(uint8_t motorID);


float Encoder_ReadSpeed(uint8_t motorID, uint16_t counts_per_rev, float dt_sec);


void Motor_SetSpeed(uint8_t motorID, uint8_t speed, uint8_t direction);

#ifdef __cplusplus
}
#endif

#endif /* __SPEED_MOTOR_H */
