#ifndef MOTOR_ANGLE_H
#define MOTOR_ANGLE_H

#include "stm32f4xx_hal.h"

/* ================== Motor Direction ================== */
#define MOTOR_DIR_CW   1   // Clockwise
#define MOTOR_DIR_CCW  0   // Counter-Clockwise

/* ================== Public API ================== */

/**
 * @brief Initialize encoder and motor calibration
 *        - Moves motor to left and right limits
 *        - Finds encoder min, max, and center
 *        - Returns to center position
 */
void Motor_Init_Angle(void);

/**
 * @brief Stop motor immediately (0% PWM).
 */
void Motor_Angle_Stop(void);

/**
 * @brief Run motor continuously at given speed and direction.
 * 
 * @param speed_percent : PWM duty cycle [0..100]
 * @param direction     : MOTOR_DIR_CW or MOTOR_DIR_CCW
 */
void Motor_Run(uint8_t speed_percent, uint8_t direction);

/**
 * @brief Move motor until encoder = target value (low-level).
 * 
 * @param target : encoder count value to reach
 */
void Motor_GotoEncoder(int32_t target, uint8_t direction);

/**
 * @brief Move motor to specific angle relative to center.
 * 
 * @param angle_deg : angle in degrees [0..90]
 * @param direction : MOTOR_DIR_CW (right) or MOTOR_DIR_CCW (left)
 */
void Motor_GotoAngle(uint8_t angle_deg, uint8_t direction);

/* ================== Encoder API ================== */

/**
 * @brief Initialize encoder timer.
 */
void Encoder_Init(TIM_HandleTypeDef *htim);

/**
 * @brief Get current encoder angle in degrees.
 */
 int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim);

/**
 * @brief Debug print encoder data (CNT, angle, direction) over UART.
 * 
 * @param htim    : encoder timer handle
 * @param motorID : motor index (for multi-motor debug print)
 */
void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID);

void Debug_Encoder(void);

#endif /* MOTOR_ANGLE_H */


// #ifndef MOTOR_ANGLE_H
// #define MOTOR_ANGLE_H

// #include "stm32f4xx_hal.h"
// #include <stdint.h>

// /* ==================== Defines ==================== */
// #define ENCODER_COUNTS_PER_REV 1024  // Adjust to your encoder spec
// #define ANGLE_TOLERANCE        2.0f  // Degrees tolerance

// #define MOTOR_DIR_CW  1
// #define MOTOR_DIR_CCW 0


// /* ==================== Encoder APIs ==================== */
// void Encoder_Init(TIM_HandleTypeDef *htim);
// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID);
// int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim);

// /* ==================== Motor APIs ==================== */
// void Motor_Angle_Stop(void);
// void Motor_Init_Angle(void);
// void Motor_GotoEncoder(int16_t target);
// // void Motor_SetAngle(int16_t targetAngle, int16_t currentAngle);
// void Motor_GotoAngle(int16_t angle_deg);

// #endif // MOTOR_ANGLE_H


// #ifndef ENCODER_DRIVER_H
// #define ENCODER_DRIVER_H

// #include <stdint.h>
// #include "stm32f4xx_hal.h"
// #include "uart.h"
// #include "Packet.h"

// // ================== Encoder Configuration ==================
// #define ENCODER_PPR            1024   // Pulses per revolution
// #define ENCODER_MODE_X4        4      // Quadrature encoder mode multiplier
// #define ENCODER_COUNTS_PER_REV (ENCODER_PPR * ENCODER_MODE_X4)

// // ================== Motor Direction Defines =================
// #define MOTOR_DIR_CW           1
// #define MOTOR_DIR_CCW          0

// // ================== Cytron MD10C Pin Config =================
// // PB6 -> TIM4_CH1 (PWM)
// // PB7 -> Direction GPIO
// #define MOTOR_PWM_TIMER        htim4
// #define MOTOR_PWM_CHANNEL      TIM_CHANNEL_1
// #define MOTOR_DIR_GPIO_PORT    GPIOB
// #define MOTOR_DIR_GPIO_PIN     GPIO_PIN_7


// // ================== External Timer Handles ===================
// extern TIM_HandleTypeDef htim3; // Encoder timer
// extern TIM_HandleTypeDef htim4; // PWM output timer

// // ================== Function Prototypes ======================
// // Encoder functions
// void Encoder_Init(TIM_HandleTypeDef *htim);
// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID);
// void Encoder_ReadAndControl(TIM_HandleTypeDef *htim,struct MotorAngle motor , uint8_t motorID);
// // Motor control functions
// // void Motor_SetTarget(struct MotorAngle *motor, float angle, uint8_t direction);
// // void Motor_UpdateControl(struct MotorAngle *motor); // Adjust PWM to reach target
// void Motor_Init_Angle(void);                       // Init PWM + DIR pin
// void Motor_SetAngle(int16_t targetAngle, int16_t currentAngle) ;
// // void Encoder_ReadAndControl(TIM_HandleTypeDef *htim, float targetAngle, uint8_t motorID)
// void Motor_GotoAngle(int16_t angle_deg);
// #endif // ENCODER_DRIVER_H
