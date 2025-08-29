#include "Motor_Angle.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <stdarg.h>

#include <stdlib.h> // for labs()

extern TIM_HandleTypeDef htim3;   // Encoder timer (TIM3)
extern TIM_HandleTypeDef htim4;   // PWM timer (TIM4)
extern UART_HandleTypeDef huart1; // UART for debug messages

#define MOTOR_PWM_CHANNEL TIM_CHANNEL_1
#define MOTOR_DIR_PORT GPIOB
#define MOTOR_DIR_PIN GPIO_PIN_7

#define ENCODER_COUNTS_PER_REV 1024 // Adjust per encoder spec
#define ANGLE_TOLERANCE 2.0f        // Degrees tolerance

// Direction flags
#define MOTOR_DIR_CW 1
#define MOTOR_DIR_CCW 0

typedef struct
{
    int32_t encoder_min;
    int32_t encoder_max;
    int32_t encoder_center;
} MotorCalibration;

static MotorCalibration motor1_calib;

/* ==================== Encoder Functions ==================== */
void Encoder_Init(TIM_HandleTypeDef *htim)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Start encoder mode
    __HAL_TIM_SET_COUNTER(&htim3, 0);               // Reset to 0
}

void Motor_Print(const char *fmt, ...)
{
    char buffer[128]; // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, "\r\n", 2, HAL_MAX_DELAY);
}

/* ==================== Motor Helper Functions ==================== */

void Motor_Angle_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
}

/**
 * Drive motor until encoder = target position (basic P control)
 */

void Motor_GotoEncoder(int32_t target, uint8_t direction)
{
    int sm = 0;
    while (1)
    {
        int16_t raw = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
        int16_t current = (int16_t)raw;

        int32_t error = current - target;

        sm++;
        if (sm % 10 == 0)
        {
            char msg[60];
            snprintf(msg, sizeof(msg), "Target=%ld ,Current = %ld max : %d min :%d error : %d \r\n", target, current, motor1_calib.encoder_max, motor1_calib.encoder_min, error);
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        }
        // Stop condition
        if (labs(error) <= 2) // tolerance = 10 counts
        {
            char msg[60];
            snprintf(msg, sizeof(msg),
                     "Errroror tolerance Target=%ld , Current = %ld\r\n",
                     target, current);

            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

            __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0); // stop PWM

            break;
        }

        if (target > motor1_calib.encoder_min || target < motor1_calib.encoder_max)
        {
            __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);

            char msg[60];
            snprintf(msg, sizeof(msg), "Errroror Target=%d , max : %d min :%d Current = %ld\r\n", target, current, motor1_calib.encoder_max, motor1_calib.encoder_min);
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            break;
        }

        uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);

        // Set motor direction
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, (direction == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        // Apply PWM (for now fixed at 60%, you can switch to pwmValue)
        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.4f);
        HAL_Delay(5);
        Motor_Angle_Stop();
        HAL_Delay(15);
    }
}

/* ==================== Calibration ==================== */

void Motor_Init_Angle(void)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    int32_t old, current;
    int stableCounter;

    Motor_Print("arr %ld", arr);
    HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
    Encoder_Init(&htim3);

    // ==== Move to LEFT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
    old = -1;
    stableCounter = 0;

    while (1)
    {
        int16_t raw = (int16_t)__HAL_TIM_GET_COUNTER(&htim3); // signed read
        current = (int32_t)raw;

        Motor_Print("raw=%d   current=%ld", raw, current);

        if (abs(current - old) <= 10) // within 50 counts tolerance
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.9f); // 40% duty
        HAL_Delay(10);
        Motor_Angle_Stop();
        HAL_Delay(20);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_min = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    motor1_calib.encoder_min += 2;

    HAL_Delay(500);

    // ==== Move to RIGHT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
    old = -1;
    stableCounter = 0;

    while (1)
    {
        int16_t raw = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
        current = (int32_t)raw;

        if (abs(current - old) <= 10)
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.9f);
        HAL_Delay(10);
        Motor_Angle_Stop();
        HAL_Delay(20);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_max = (int32_t)(int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    motor1_calib.encoder_max -= 2;

    HAL_Delay(500);

    // ==== Compute center ====
    motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

    Motor_Print("min=%ld   mid=%ld   max=%ld",
                motor1_calib.encoder_min,
                motor1_calib.encoder_center,
                motor1_calib.encoder_max);

    Motor_GotoAngle(0, 0);
}

/* ================Motor_GotoAngle==== User API ==================== */

/**
 * Move motor to angle (0..90) in given direction
 * @param angle_deg   : desired angle [0..90]
 * @param direction   : MOTOR_DIR_CW or MOTOR_DIR_CCW
 */

void Motor_GotoAngle(uint8_t angle_deg, uint8_t direction)
{
    if (angle_deg > 90)
        angle_deg = 90;

    int32_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
    int32_t target;

    if (direction == MOTOR_DIR_CCW)
    {
        target = motor1_calib.encoder_center - ((int32_t)angle_deg * halfRange) / 90;
    }
    else
    {
        target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;
    }

    int16_t raw = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    int32_t cnt = (int32_t)raw;

    char msg[60];
    snprintf(msg, sizeof(msg), "Angle sent =%ld  Target=%ld  Current=%ld\r\n", angle_deg, target, cnt);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    Motor_GotoEncoder(target, direction);
}

// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
// {
//     static uint32_t lastTick = 0;

//     // Check if 100 ms passed
//     if (HAL_GetTick() - lastTick >= 100)
//     {
//         lastTick = HAL_GetTick();

//         int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
//         int16_t signedCount = __HAL_TIM_GET_COUNTER(htim);
//         int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

//         char msg[80];
//         snprintf(msg, sizeof(msg),
//                  "CNT=%" PRId32 ", Angle=%" PRId16 ", Dir=%s\r\n",
//                  signedCount,
//                  angle,
//                  (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");

//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//     }
// }