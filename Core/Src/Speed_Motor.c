#include "Speed_Motor.h"

#include "stm32f4xx_hal.h"
#include "uart.h"
#include <stdio.h>

// External handles (defined in main.c)
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim4;

// PWM mapping
#define MOTOR1_PWM_TIMER &htim4
#define MOTOR1_PWM_CHANNEL TIM_CHANNEL_3
#define MOTOR2_PWM_TIMER &htim4
#define MOTOR2_PWM_CHANNEL TIM_CHANNEL_4

// Direction pins
#define MOTOR1_DIR_PORT GPIOB
#define MOTOR1_DIR_PIN GPIO_PIN_4
#define MOTOR2_DIR_PORT GPIOB
#define MOTOR2_DIR_PIN GPIO_PIN_5

// -------------------- Encoder functions --------------------

int32_t Encoder_ReadPosition(uint8_t motorID)
{
    if (motorID == 1)
    {
        return (int16_t)__HAL_TIM_GET_COUNTER(&htim2); // Encoder 1
    }
    else if (motorID == 2)
    {
        return (int16_t)__HAL_TIM_GET_COUNTER(&htim5); // Encoder 2
    }
    return 0;
}

float Encoder_ReadSpeed(uint8_t motorID, uint16_t counts_per_rev, float dt_sec)
{
    static int16_t last_count_motor1 = 0;
    static int16_t last_count_motor2 = 0;

    int16_t count, diff;
    if (motorID == 1)
    {
        count = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
        diff = count - last_count_motor1;
        last_count_motor1 = count;
    }
    else
    {
        count = (int16_t)__HAL_TIM_GET_COUNTER(&htim5);
        diff = count - last_count_motor2;
        last_count_motor2 = count;
    }

    float revs = (float)diff / counts_per_rev;
    return (revs / dt_sec) * 60.0f; // RPM
}

// -------------------- Motor control --------------------

void Motor_init()
{
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

void Motor_SetSpeed(uint8_t motorID, uint8_t speed, uint8_t direction)
{
    uint32_t arr;
    int16_t encoder_value;
    char msg[100];

    // Motor 1 only
    if (motorID == 1 || motorID == 3)
    {
        HAL_GPIO_WritePin(MOTOR1_DIR_PORT, MOTOR1_DIR_PIN,
                          direction ? GPIO_PIN_SET : GPIO_PIN_RESET);

        arr = __HAL_TIM_GET_AUTORELOAD(MOTOR1_PWM_TIMER);
        __HAL_TIM_SET_COMPARE(MOTOR1_PWM_TIMER, MOTOR1_PWM_CHANNEL,
                              (arr * speed) / 100);

        encoder_value = Encoder_ReadPosition(1);
        snprintf(msg, sizeof(msg),
                 "Motor 1 Enc=%d, PWM=%d%%, Dir=%d\r\n",
                 encoder_value, speed, direction);
        HAL_UART_Transmit_IT(&huart1, (uint8_t *)msg, strlen(msg)); // non-blocking
    }

    // Motor 2 only
    if (motorID == 2 || motorID == 3)
    {
        HAL_GPIO_WritePin(MOTOR2_DIR_PORT, MOTOR2_DIR_PIN,
                          direction ? GPIO_PIN_SET : GPIO_PIN_RESET);

        arr = __HAL_TIM_GET_AUTORELOAD(MOTOR2_PWM_TIMER);
        __HAL_TIM_SET_COMPARE(MOTOR2_PWM_TIMER, MOTOR2_PWM_CHANNEL,
                              (arr * speed) / 100);

        encoder_value = Encoder_ReadPosition(2);
        snprintf(msg, sizeof(msg),
                 "Motor 2 Enc=%d, PWM=%d%%, Dir=%d\r\n",
                 encoder_value, speed, direction);
        HAL_UART_Transmit_IT(&huart1, (uint8_t *)msg, strlen(msg)); // non-blocking
    }

    Motor_Stop(motorID);
}

void Motor_Stop(uint8_t motorID)
{
    HAL_Delay(500);
    if (motorID == 1)
    {
        __HAL_TIM_SET_COMPARE(MOTOR1_PWM_TIMER, MOTOR1_PWM_CHANNEL, 0);
    }
    else if (motorID == 2)
    {
        __HAL_TIM_SET_COMPARE(MOTOR2_PWM_TIMER, MOTOR2_PWM_CHANNEL, 0);
    }
    else if (motorID == 3)
    {
        __HAL_TIM_SET_COMPARE(MOTOR1_PWM_TIMER, MOTOR1_PWM_CHANNEL, 0);
        __HAL_TIM_SET_COMPARE(MOTOR2_PWM_TIMER, MOTOR2_PWM_CHANNEL, 0);
    }
}