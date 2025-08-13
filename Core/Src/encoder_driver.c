#include "encoder_driver.h"
#include "Packet.h"
#include "stm32f4xx_hal.h"
#include "uart.h"

// Define this according to your encoder's specifications
#define ENCODER_PPR             600     // Pulses Per Revolution for E38S6G5-600B-G24F
#define ENCODER_RESOLUTION_MODE 4       // X4 mode (counts on all edges of both channels)
#define COUNTS_PER_REVOLUTION   (ENCODER_PPR * ENCODER_RESOLUTION_MODE)

// Declare huart1 as extern, assuming it's defined in main.c or similar
extern UART_HandleTypeDef huart1;

void Encoder_Init(TIM_HandleTypeDef *htim)
{
    // Start the encoder interface for the specified timer
    HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

void Encoder_ReadData(TIM_HandleTypeDef *htim, struct MotorAngle *motorAngle, uint8_t motorID)
{
    // Read the current counter value from the timer
    int32_t current_count = __HAL_TIM_GET_COUNTER(htim);

    // Determine direction based on the timer's direction bit
    if ((htim->Instance->CR1 & TIM_CR1_DIR) == 0) {
        motorAngle->direction = 0; // Clockwise
    } else {
        motorAngle->direction = 1; // Counter-Clockwise
    }

    // Calculate the angle in degrees (0-359)
    int32_t normalized_count = current_count % COUNTS_PER_REVOLUTION;
    if (normalized_count < 0) {
        normalized_count += COUNTS_PER_REVOLUTION;
    }

    float angle_float = (float)normalized_count / COUNTS_PER_REVOLUTION * 360.0f;

    // Scale the angle from 0-359 to 0-255 for uint8_t
    motorAngle->angle = (uint8_t)((angle_float / 360.0f) * 255.0f);

    // Set the motor ID
    motorAngle->ID = motorID;

    // Corrected UART transmission calls
    HAL_UART_Transmit(&huart1 , (uint8_t*)"Encoder Data Read:\r\n", 21 , HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , (uint8_t*)"ID: ", 4, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , &motorAngle->ID, 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , (uint8_t*)"\r\nAngle: ", 9, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , &motorAngle->angle, 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , (uint8_t*)"\r\nDirection: ", 13, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , &motorAngle->direction, 1, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1 , (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);
}
