#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "uart.h"
#include "Packet.h"

// ================== Encoder Configuration ==================
#define ENCODER_PPR            1024   // Pulses per revolution
#define ENCODER_MODE_X4        4      // Quadrature encoder mode multiplier
#define ENCODER_COUNTS_PER_REV (ENCODER_PPR * ENCODER_MODE_X4)

// ================== Motor Direction Defines =================
#define MOTOR_DIR_CW           1
#define MOTOR_DIR_CCW          0

// ================== Cytron MD10C Pin Config =================
// PB6 -> TIM4_CH1 (PWM)
// PB7 -> Direction GPIO
#define MOTOR_PWM_TIMER        htim4
#define MOTOR_PWM_CHANNEL      TIM_CHANNEL_1
#define MOTOR_DIR_GPIO_PORT    GPIOB
#define MOTOR_DIR_GPIO_PIN     GPIO_PIN_7


// ================== External Timer Handles ===================
extern TIM_HandleTypeDef htim3; // Encoder timer
extern TIM_HandleTypeDef htim4; // PWM output timer

// ================== Function Prototypes ======================
// Encoder functions
void Encoder_Init(TIM_HandleTypeDef *htim);
void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID);
void Encoder_ReadAndControl(TIM_HandleTypeDef *htim,struct MotorAngle motor , uint8_t motorID);
// Motor control functions
void Motor_SetTarget(struct MotorAngle *motor, float angle, uint8_t direction);
void Motor_UpdateControl(struct MotorAngle *motor); // Adjust PWM to reach target
void Motor_Init(void);                       // Init PWM + DIR pin
void Motor_SetAngle(float targetAngle, float currentAngle) ;
// void Encoder_ReadAndControl(TIM_HandleTypeDef *htim, float targetAngle, uint8_t motorID)

#endif // ENCODER_DRIVER_H
