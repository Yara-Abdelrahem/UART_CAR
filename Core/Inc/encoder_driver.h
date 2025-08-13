#ifndef ENCODER_DRIVER_H
#define ENCODER_DRIVER_H

#include <stdint.h>
#include "uart.h"

struct MotorAngle;
extern TIM_HandleTypeDef htim3;

// Define the encoder pulses per revolution and resolution mode

void Encoder_Init(TIM_HandleTypeDef *htim);

void Encoder_ReadData(TIM_HandleTypeDef *htim, struct MotorAngle *motorAngle, uint8_t motorID);

#endif // ENCODER_DRIVER_H


