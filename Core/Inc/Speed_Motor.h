#ifndef __SPEED_MOTOR_H
#define __SPEED_MOTOR_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------- Public API --------------------

/**
 * @brief Read encoder position (counter value).
 * @param motorID  Motor index: 1 = Motor1 (TIM2), 2 = Motor2 (TIM5)
 * @return Signed 16-bit encoder count
 */
int32_t Encoder_ReadPosition(uint8_t motorID);

/**
 * @brief Read encoder speed in RPM.
 * @param motorID         Motor index: 1 or 2
 * @param counts_per_rev  Encoder resolution (pulses per revolution)
 * @param dt_sec          Sampling interval in seconds
 * @return Speed in RPM
 */
float Encoder_ReadSpeed(uint8_t motorID, uint16_t counts_per_rev, float dt_sec);


void Motor_init();

/**
 * @brief Set motor speed and direction.
 * @param motorID   Motor index: 1 or 2
 * @param speed     Duty cycle % (0–100)
 * @param direction 0 = one direction, 1 = opposite
 */
void Motor_SetSpeed(uint8_t motorID, uint8_t speed, uint8_t direction);

/**
 * @brief Stop a motor (set PWM = 0).
 * @param motorID Motor index: 1 or 2
 */
void Motor_Stop(uint8_t motorID);

#ifdef __cplusplus
}
#endif

#endif // __SPEED_MOTOR_H
