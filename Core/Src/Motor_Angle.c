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

int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim)
{
    int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    return ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;
}

void Encoder_Init(TIM_HandleTypeDef *htim)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL); // Start encoder mode
    __HAL_TIM_SET_COUNTER(&htim3, 0);               // Reset to 0
}
void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
{
    static uint32_t lastTick = 0;

    // Check if 100 ms passed
    if (HAL_GetTick() - lastTick >= 100)
    {
        lastTick = HAL_GetTick();

        int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
        int16_t signedCount = __HAL_TIM_GET_COUNTER(htim);
        int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

        char msg[80];
        snprintf(msg, sizeof(msg),
                 "CNT=%" PRId32 ", Angle=%" PRId16 ", Dir=%s\r\n",
                 signedCount,
                 angle,
                 (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");

        HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
    }
}


void Motor_Print(const char *fmt, ...)
{
    char buffer[128];   // adjust size as needed
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, "\r\n", 2, HAL_MAX_DELAY);

}

/* ==================== Motor Helper Functions ==================== */

void Motor_Angle_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
}

/**
 * Run motor with given speed (0–100%) and direction
 */
void Motor_Run(uint8_t speed_percent, uint8_t direction)
{
    if (speed_percent > 100)
        speed_percent = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    uint32_t pwmValue = (arr * speed_percent) / 100;

    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN,
                      (direction == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, pwmValue);
}

/**
 * Drive motor until encoder = target position (basic P control)
 */
void Motor_GotoEncoder(int32_t target, uint8_t direction)
{
    while (1)
    {
        int32_t current = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
        int32_t error = target - current;

        // Stop condition
        if (labs(error) <= 10) // tolerance = 10 counts
        {
            __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0); // stop PWM
            break;                                               // exit while loop
        }

        // Use the direction parameter
        uint8_t dir = direction;

        // Scale PWM based on error
        float pwmDuty = fabsf((float)error) * 0.5f; // proportional control
        if (pwmDuty > 100.0f)
            pwmDuty = 100.0f;

        uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
        uint32_t pwmValue = (arr * pwmDuty) / 100;

        // Set motor direction
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN,
                          (dir == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

        // Apply PWM
        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr*0.6f);
    }
}

void Debug_Encoder(void)
{
    int32_t cnt = __HAL_TIM_GET_COUNTER(&htim3);
    char msg[40];
    snprintf(msg, sizeof(msg), "ENC CNT = %ld\r\n", cnt);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

/* ==================== Calibration ==================== */

void Motor_Init_Angle(void)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    int32_t old, current;
    int stableCounter;

    Motor_Print("arr %d",arr);
    HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
    Encoder_Init(&htim3);

    // ==== Move to LEFT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
    old = -1;
    stableCounter = 0;

    while (1)
    {

        int16_t tmpe=__HAL_TIM_GET_COUNTER(&htim3);
        current = (int32_t)tmpe;

    Motor_Print("tmpe %d   current %d",tmpe,current);

        
        if (abs(current - old) <= 200) // within 2 counts tolerance
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f); // 40% duty
        HAL_Delay(10);
        Motor_Angle_Stop();
        HAL_Delay(20);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_min = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

    HAL_Delay(500);

    // ==== Move to RIGHT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
    old = -1;
    stableCounter = 0;

    while (1)
    {
        current = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

        if (abs(current - old) <= 10) // within 2 counts tolerance
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f);
        HAL_Delay(10);
        Motor_Angle_Stop();
        HAL_Delay(20);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_max = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
    HAL_Delay(500);

    // Compute center
    motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

    char angle_msg[64];
snprintf(angle_msg, sizeof(angle_msg),
         "Init: Max=%ld , Mid=%ld , Min=%ld\r\n",
         (long) motor1_calib.encoder_max,
         (long) motor1_calib.encoder_center,
         (long) motor1_calib.encoder_min);

    HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);
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

    int16_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
    int16_t target;

    if (direction == MOTOR_DIR_CCW)
    {
        target = motor1_calib.encoder_center - ((int32_t)angle_deg * halfRange) / 90;
    }
    else
    {
        target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;
    }

    int32_t cnt = __HAL_TIM_GET_COUNTER(&htim3);
    char msg[40];
    snprintf(msg, sizeof(msg), "Sent value = %ld\r\n", cnt);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
    // Motor_GotoEncoder (target, direction);
}
 
// #include "Motor_Angle.h"
// #include <stdio.h>
// #include <string.h>
// #include <math.h>
// #include <inttypes.h>
// #include <stdlib.h>

// extern TIM_HandleTypeDef htim3;   // Encoder timer (TIM3)
// extern TIM_HandleTypeDef htim4;   // PWM timer (TIM4)
// extern UART_HandleTypeDef huart1; // UART for debug messages

// #define MOTOR_PWM_CHANNEL TIM_CHANNEL_1
// #define MOTOR_DIR_PORT GPIOB
// #define MOTOR_DIR_PIN GPIO_PIN_7

// #define ENCODER_COUNTS_PER_REV 1024 // Adjust per encoder spec
// #define ANGLE_TOLERANCE 2.0f        // Degrees tolerance

// // Direction flags
// #define MOTOR_DIR_CW 1
// #define MOTOR_DIR_CCW 0

// typedef struct
// {
//     int32_t encoder_min;
//     int32_t encoder_max;
//     int32_t encoder_center;
// } MotorCalibration;

// static MotorCalibration motor1_calib;

// /* ==================== Encoder Functions ==================== */

// int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim)
// {
//     int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
//     return ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;
// }

// void Encoder_Init(TIM_HandleTypeDef *htim)
// {
//     HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);  // Start encoder mode
//     __HAL_TIM_SET_COUNTER(&htim3, 0);                // Reset to 0
// }

// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
// {
//     static int32_t readCounter = 0;
//     const int32_t printInterval = 10;

//     int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
//     int32_t signedCount = (int32_t)__HAL_TIM_GET_COUNTER(htim);
//     int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

//     readCounter++;
//     if (readCounter >= printInterval)
//     {
//         char msg[80];
//         snprintf(msg, sizeof(msg),
//                  "CNT=%" PRId32 ", Angle=%" PRId16 ", Dir=%s\r\n",
//                  signedCount,
//                  angle,
//                  (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//         readCounter = 0;
//     }
// }

// /* ==================== Motor Helper Functions ==================== */

// void Motor_Angle_Stop(void)
// {
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
// }

// /**
//  * Run motor with given speed (0–100%) and direction
//  */
// void Motor_Run(uint8_t speed_percent, uint8_t direction)
// {
//     if (speed_percent > 100)
//         speed_percent = 100;

//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     uint32_t pwmValue = (arr * speed_percent) / 100;

//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN,
//                       (direction == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, pwmValue);
// }

// /**
//  * Drive motor until encoder = target position (basic P control)
//  */
// void Motor_GotoEncoder(int32_t target, uint8_t direction)
// {
//     while (1)
//     {
//         int32_t current = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
//         int32_t error = target - current;

//         // Stop condition
//         if (abs(error) <= 2) // tolerance = 2 counts
//         {
//             __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0); // stop PWM
//             break; // exit while loop
//         }

//         // Use the direction parameter instead of auto-detect
//         uint8_t dir = direction;

//         // Scale PWM based on error
//         float pwmDuty = fabs(error) * 0.5f; // proportional control
//         if (pwmDuty > 100.0f)
//             pwmDuty = 100.0f;

//         uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//         uint32_t pwmValue = (arr * pwmDuty) / 100;

//         // Set motor direction
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN,
//                           (dir == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

//         // Apply PWM
//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr *0.6f);
//     }
// }

// void Debug_Encoder(void)
// {
//     int32_t cnt = __HAL_TIM_GET_COUNTER(&htim3);
//     char msg[40];
//     snprintf(msg, sizeof(msg), "ENC CNT = %ld\r\n", cnt);
//     HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
// }

// /* ==================== Calibration ==================== */

// void Motor_Init_Angle(void)
// {
//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     int32_t oarrld, current;
//     int stableCounter;

//     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
//     Encoder_Init(&htim3);

//     // ==== Move to LEFT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
//     old = -1;
//     stableCounter = 0;

//     while (1)
//     {
//         current = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

//         if (current == old)
//             stableCounter++;
//         else
//         {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 50)
//             break;

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f); // 40% duty
//         HAL_Delay(5);
//         Motor_Angle_Stop();
//         HAL_Delay(15);
//     }

//     Motor_Angle_Stop();
//     motor1_calib.encoder_min = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

//     HAL_Delay(400);

//     // ==== Move to RIGHT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
//     old = -1;
//     stableCounter = 0;

//     while (1)
//     {
//         current = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);

//         if (current == old)
//             stableCounter++;
//         else
//         {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 50)
//             break;

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f);
//     HAL_Delay(5);
//         Motor_Angle_Stop();
//         HAL_Delay(15);
//     }

//     Motor_Angle_Stop();
//     motor1_calib.encoder_max = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
//     HAL_Delay(500);

//     // Compute center
//     motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

//     char angle_msg[64];
//     snprintf(angle_msg, sizeof(angle_msg),
//              "Init: Max=%08" PRIX32 " , Mid=%08" PRIX32 " , Min=%08" PRIX32 "\r\n",
//              (uint32_t)motor1_calib.encoder_max,
//              (uint32_t)motor1_calib.encoder_center,
//              (uint32_t)motor1_calib.encoder_min);

//     HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);
// }

// /* ==================== User API ==================== */

// /**
//  * Move motor to angle (0..90) in given direction
//  * @param angle_deg   : desired angle [0..90]
//  * @param direction   : MOTOR_DIR_CW or MOTOR_DIR_CCW
//  */
// void Motor_GotoAngle(uint8_t angle_deg, uint8_t direction)
// {
//     if (angle_deg > 90)
//         angle_deg = 90;

//     int16_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
//     int16_t target;

//     if (direction == MOTOR_DIR_CCW)
//     {
//         target = motor1_calib.encoder_center - ((int32_t)angle_deg * halfRange) / 90;
//     }
//     else
//     {
//         target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;
//     }

//     Motor_GotoEncoder(target , direction);
// }
