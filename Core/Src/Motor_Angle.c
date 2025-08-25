#include "Motor_Angle.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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

static int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim)
{
    int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    return ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;
}

void Encoder_Init(TIM_HandleTypeDef *htim)
{
    HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
{
    static uint32_t readCounter = 0;
    const uint32_t printInterval = 10;

    int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
    int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

    readCounter++;
    if (readCounter >= printInterval)
    {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Enc M%d: CNT=%d, Angle=%d, Dir=%s\r\n",
                 motorID,
                 signedCount,
                 angle,
                 (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        readCounter = 0;
    }
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
    if (speed_percent > 100) speed_percent = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    uint32_t pwmValue = (arr * speed_percent) / 100;

    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN,
                      (direction == MOTOR_DIR_CW) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, pwmValue);
}

/**
 * Drive motor until encoder = target position (basic P control)
 */
void Motor_GotoEncoder(int16_t target)
{
    int16_t current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    float error = (float)(target - current);

    uint8_t dir = (error >= 0) ? MOTOR_DIR_CW : MOTOR_DIR_CCW;
    if (error < 0) error = -error;

    float pwmDuty = error * 0.5f; // proportional gain
    if (pwmDuty > 100.0f) pwmDuty = 100.0f;

    Motor_Run((uint8_t)pwmDuty, dir);
}

/* ==================== Calibration ==================== */

void Motor_Init_Angle(void)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    int16_t old, current;
    int stableCounter;

    HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
    Encoder_Init(&htim3);

    // ==== Move to LEFT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
    old = -1;
    stableCounter = 0;

    while (1)
    {
        current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

        if (current == old)
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f); // 40% duty
        HAL_Delay(5);
        Motor_Angle_Stop();
        HAL_Delay(15);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_min = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    HAL_Delay(400);

    // ==== Move to RIGHT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
    old = -1;
    stableCounter = 0;

    while (1)
    {
        current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

        if (current == old)
            stableCounter++;
        else
        {
            stableCounter = 0;
            old = current;
        }

        if (stableCounter > 50)
            break;

        __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.6f);
        HAL_Delay(5);
        Motor_Angle_Stop();
        HAL_Delay(15);
    }

    Motor_Angle_Stop();
    motor1_calib.encoder_max = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    HAL_Delay(500);

    // Compute center
    motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

    // Move to center
    Motor_GotoEncoder(motor1_calib.encoder_min);

    char angle_msg[64];
    snprintf(angle_msg, sizeof(angle_msg),
             "Init: Max=%04X , Mid=%04X , Min=%04X\r\n",
             motor1_calib.encoder_max, motor1_calib.encoder_center, motor1_calib.encoder_min);
    HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);
}

/* ==================== User API ==================== */

/**
 * Move motor to angle (0..90) in given direction
 * @param angle_deg   : desired angle [0..90]
 * @param direction   : MOTOR_DIR_CW or MOTOR_DIR_CCW
 */
void Motor_GotoAngle(uint8_t angle_deg, uint8_t direction)
{
    if (angle_deg > 90) angle_deg = 90;

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

    Motor_GotoEncoder(target);
}


// #include"Motor_Angle.h"
// #include <stdio.h>
// #include <string.h>
// #include <math.h>

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
//     int16_t encoder_min;
//     int16_t encoder_max;
//     int16_t encoder_center;
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
//     HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
// }

// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
// {
//     static uint32_t readCounter = 0;
//     const uint32_t printInterval = 10;

//     int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
//     int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
//     int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

//     readCounter++;
//     if (readCounter >= printInterval)
//     {
//         char msg[80];
//         snprintf(msg, sizeof(msg),
//                  "Enc M%d: CNT=%d, Angle=%d, Dir=%s\r\n",
//                  motorID,
//                  signedCount,
//                  angle,
//                  (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//         readCounter = 0;
//     }
// }

// /* ==================== Motor Functions ==================== */

// void Motor_Angle_Stop(void)
// {
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
// }

// /**
//  * Drive motor until encoder = target position
//  */
// void Motor_GotoEncoder(int16_t target)
// {
//     int16_t current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
//     float error = (float)(target - current);

//     if (error > 0)
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
//     }
//     else
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
//         error = -error;
//     }

//     // proportional duty
//     float pwmDuty = error * 0.5f; // tune gain
//     if (pwmDuty > 100.0f)
//         pwmDuty = 100.0f;

//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, (uint32_t)((pwmDuty / 100.0f) * arr));
// }

// /**
//  * Perform left/right calibration to detect limits
//  */
// void 
// Motor_Init_Angle(void)
// {
//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     int16_t old, current;
//     int stableCounter;

//     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
//     Encoder_Init(&htim3);

//     // ==== Move to LEFT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
//     old = -1;
//     stableCounter = 0;

//     while (1)
//     {
//         current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//         if (current == old)
//             stableCounter++;
//         else
//         {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 10) // ~10ms stable → stop
//             break;

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.4f); // 50%
//         HAL_Delay(5);
//         Motor_Angle_Stop();
//         HAL_Delay(20);
//     }

//     Motor_Angle_Stop();
//     motor1_calib.encoder_min = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//     HAL_Delay(500);

//     // ==== Move to RIGHT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
//     old = -1;
//     stableCounter = 0;

//     while (1)
//     {
//         current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//         if (current == old)
//             stableCounter++;
//         else
//         {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 50)
//             break;

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.4f); // 50%
//         HAL_Delay(5);
//         Motor_Angle_Stop();
//         HAL_Delay(20);
//     }

//     Motor_Angle_Stop();
//     motor1_calib.encoder_max = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//     // compute center
//     motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

//     // move to center
//     Motor_GotoEncoder(motor1_calib.encoder_center);

//       // Send status
//     char angle_msg[64];
//     snprintf(angle_msg, sizeof(angle_msg),
//              "Init: Max = %04X , middle = %04X ,Min = %04X \r\n",
//              motor1_calib.encoder_max, motor1_calib.encoder_center, motor1_calib.encoder_min);
//     HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);


// }

// void Motor_GotoAngle(int16_t angle_deg)
// {
//     if (angle_deg < -90)
//         angle_deg = -90;
//     if (angle_deg > 90)
//         angle_deg = 90;

//     int16_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
//     int16_t target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;

//     Motor_GotoEncoder(target);
// }

// /**
//  * Closed-loop angle control
//  */
// void Motor_SetAngle(int16_t targetAngle, int16_t currentAngle)
// {
//     float error = targetAngle - currentAngle;

//     // Normalize error to [-180, 180]
//     if (error > 180.0f)
//         error -= 360.0f;
//     if (error < -180.0f)
//         error += 360.0f;

//     // Direction
//     if (error >= 0)
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
//     }
//     else
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
//         error = -error;
//     }

//     // PWM duty
//     float pwmDuty = error * 2.0f;
//     if (pwmDuty > 100.0f)
//         pwmDuty = 100.0f;

//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, (uint32_t)((pwmDuty / 100.0f) * arr));
// }

/**
 * Map user angle [-90..90] to calibrated encoder range
 */


// #include "Motor_Angle.h"
// #include <stdio.h>
// #include <string.h>
// #include <math.h>

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
//     int16_t encoder_min;
//     int16_t encoder_max;
//     int16_t encoder_center;
// } MotorCalibration;

// static MotorCalibration motor1_calib;

// /* ==================== Encoder Functions ==================== */

// static int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim)
// {
//     int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
//     return ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;
// }

// void Encoder_Init(TIM_HandleTypeDef *htim)
// {
//     HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
// }

// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID)
// {
//     static uint32_t readCounter = 0;
//     const uint32_t printInterval = 10;

//     int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
//     int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
//     int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

//     readCounter++;
//     if (readCounter >= printInterval)
//     {
//         char msg[80];
//         snprintf(msg, sizeof(msg),
//                  "Enc M%d: CNT=%d, Angle=%d, Dir=%s\r\n",
//                  motorID,
//                  signedCount,
//                  angle,
//                  (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//         readCounter = 0;
//     }
// }

// /* ==================== Motor Functions ==================== */

// // void Motor_Init_Angle(void) {
// //     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
// //     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
// // }

// void Motor_Init_Angle(void) {
//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     int16_t old, current;
//     int stableCounter;

//     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
//     Encoder_Init(&htim3);

//     // ==== Move to LEFT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
//     old = -1;
//     stableCounter = 0;

//     while (1) {
//         current = Encoder_GetAngle(&htim3);

//         if (current == old) {
//             stableCounter++;
//         } else {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 50) { // ~50 ms stable → hit stop
//             break;
//         }

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.3f); // 30% duty
//         HAL_Delay(5);
//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0); // stop
//         HAL_Delay(20);
//     }

//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
//     motor1_calib.encoder_min = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//     HAL_Delay(500); // pause a bit before going the other way


//     // ==== Move to RIGHT limit ====
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
//     old = -1;
//     stableCounter = 0;

//     while (1) {
//         current = Encoder_GetAngle(&htim3);

//         if (current == old) {
//             stableCounter++;
//         } else {
//             stableCounter = 0;
//             old = current;
//         }

//         if (stableCounter > 50) { // ~50 ms stable → hit stop
//             break;
//         }

//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, arr * 0.3f); // 30% duty
//         HAL_Delay(5);
//         __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
//         HAL_Delay(20);
//     }

//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
//     motor1_calib.encoder_max = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//     // Optionally center the motor after calibration:
//     int16_t mid = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;
//     Motor_GotoEncoder(mid);
// }


// void Motor_SetAngle(int16_t targetAngle, int16_t currentAngle)
// {
//     float error = targetAngle - currentAngle;

//     // Normalize error to [-180, 180]
//     if (error > 180.0f)
//         error -= 360.0f;
//     if (error < -180.0f)
//         error += 360.0f;

//     // Set direction and take absolute error
//     if (error >= 0)
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET);
//     }
//     else
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
//         error = -error;
//     }

//     // Convert to duty cycle (simple proportional control)
//     float pwmDuty = error * 2.0f;
//     if (pwmDuty > 100.0f)
//         pwmDuty = 100.0f;

//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, (uint32_t)((pwmDuty / 100.0f) * arr));
// }

// /* ==================== Combined Encoder + Motor Control ==================== */

// void Encoder_ReadAndControl(TIM_HandleTypeDef *htim, struct MotorAngle motor, uint8_t motorID)
// {
//     int16_t currentAngle = Encoder_GetAngle(htim);
//     Motor_SetAngle(motor.angle, currentAngle);

//     // Send status
//     char angle_msg[64];
//     snprintf(angle_msg, sizeof(angle_msg),
//              "sended M%d: Target=%02X, Current=%02X\r\n",
//              motorID, motor.angle, currentAngle);
//     HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);

//     // Check tolerance
//     float diff = fabsf((float)(motor.angle - currentAngle));
//     if (diff > 180.0f)
//         diff = 360.0f - diff;

//     const char *status = (diff > ANGLE_TOLERANCE) ? "ERR" : "OK";
//     char msg[64];
//     snprintf(msg, sizeof(msg),
//              "M%d: Target=%02X, Current=%02X\r\n",
//              motorID, motor.angle, currentAngle);
//     HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

//     HAL_Delay(1);
// }

// void Motor_GotoAngle(int16_t angle_deg)
// {
//     // Map angle [-90..90] to encoder range
//     int16_t target;
//     if (angle_deg < -90)
//         angle_deg = -90;
//     if (angle_deg > 90)
//         angle_deg = 90;

//     int16_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
//     target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;

//     int16_t current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

//     // Use your Motor_SetAngle logic
//     Motor_SetAngle(target, current);
// }
