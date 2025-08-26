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
