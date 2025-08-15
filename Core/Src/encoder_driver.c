#include "encoder_driver.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

extern TIM_HandleTypeDef htim3;   // Encoder timer (TIM3)
extern TIM_HandleTypeDef htim4;   // PWM timer (TIM4)
extern UART_HandleTypeDef huart1; // UART for debug messages

#define MOTOR_PWM_CHANNEL TIM_CHANNEL_1
#define MOTOR_DIR_PORT    GPIOB
#define MOTOR_DIR_PIN     GPIO_PIN_7

#define ENCODER_COUNTS_PER_REV 1024  // Adjust per encoder spec
#define ANGLE_TOLERANCE        2.0f  // Degrees tolerance

// Direction flags
#define MOTOR_DIR_CW  1
#define MOTOR_DIR_CCW 0

/* ==================== Encoder Functions ==================== */

static float Encoder_GetAngle(TIM_HandleTypeDef *htim) {
    int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    return ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;
}

void Encoder_Init(TIM_HandleTypeDef *htim) {
    HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID) {
    static uint32_t readCounter = 0;
    const uint32_t printInterval = 10;

    int dirFlag = __HAL_TIM_IS_TIM_COUNTING_DOWN(htim) ? MOTOR_DIR_CCW : MOTOR_DIR_CW;
    int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);
    float angle = ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

    readCounter++;
    if (readCounter >= printInterval) {
        char msg[80];
        snprintf(msg, sizeof(msg),
                 "Enc M%d: CNT=%d, Angle=%.2f, Dir=%s\r\n",
                 motorID,
                 signedCount,
                 angle,
                 (dirFlag == MOTOR_DIR_CW) ? "CW" : "CCW");
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
        readCounter = 0;
    }
}

/* ==================== Motor Functions ==================== */

void Motor_Init(void) {
    HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
}

void Motor_SetAngle(float targetAngle, float currentAngle) {
    float error = targetAngle - currentAngle;

    // Normalize error to [-180, 180]
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;

    // Set direction and take absolute error
    if (error >= 0) {
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
        error = -error;
    }

    // Convert to duty cycle (simple proportional control)
    float pwmDuty = error * 2.0f;
    if (pwmDuty > 100.0f) pwmDuty = 100.0f;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, (uint32_t)((pwmDuty / 100.0f) * arr));
}

/* ==================== Combined Encoder + Motor Control ==================== */

void Encoder_ReadAndControl(TIM_HandleTypeDef *htim, struct MotorAngle motor, uint8_t motorID) {
    
    int16_t currentAngle = Encoder_GetAngle(htim);
    int16_t targetAngle = motor.angle;
    Motor_SetAngle(targetAngle, currentAngle);

    // Send status
    char angle_msg[64];
    snprintf(angle_msg, sizeof(angle_msg),
                 "sended M%d: Target=%02X, Current=%02X\r\n",
             motorID, targetAngle, currentAngle);
    HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);

    // Check tolerance
    float diff = fabsf((float)(targetAngle - currentAngle));
    if (diff > 180.0f) diff = 360.0f - diff;

    const char *status = (diff > ANGLE_TOLERANCE) ? "ERR" : "OK";
    char msg[64];
    snprintf(msg, sizeof(msg),
          "M%d: Target=%04X, Current=%04X\r\n",
             status, motorID, targetAngle, currentAngle);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    HAL_Delay(1);
}


// #include "encoder_driver.h"
// // #include "motor_driver.h"
// #include <stdio.h>
// #include <math.h>

// extern TIM_HandleTypeDef htim3;   // Encoder timer (TIM3)
// extern TIM_HandleTypeDef htim4;   // PWM timer (TIM4)
// extern UART_HandleTypeDef huart1; // UART for debug messages

// #define MOTOR_PWM_CHANNEL TIM_CHANNEL_1
// #define MOTOR_DIR_PORT GPIOB
// #define MOTOR_DIR_PIN GPIO_PIN_7

// #define PULSES_PER_REV 1024  // Change according to your encoder spec
// #define ANGLE_TOLERANCE 2.0f // Degrees tolerance for "match"

// static float Encoder_GetAngle(TIM_HandleTypeDef *htim)
// {
//     int32_t count = __HAL_TIM_GET_COUNTER(htim);
//     return (360.0f * (float)count) / (float)PULSES_PER_REV;
// }

// void Encoder_Init(TIM_HandleTypeDef *htim)
// {
//     HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
// }

// #include "encoder_driver.h"
// #include <stdio.h>

// void Encoder_ReadData(TIM_HandleTypeDef *htim, uint8_t motorID) {
//     static uint32_t readCounter = 0; // how many reads happened
//     static uint32_t printInterval = 10; // print every 10th read
//     // struct MotorAngle *motorAngle;
//     int32_t sign_dir;
//     float sign_angle;
//     // Get the raw encoder count
//   //  int32_t rawCount = __HAL_TIM_GET_COUNTER(htim);

//     // Detect direction from hardware
//     if (__HAL_TIM_IS_TIM_COUNTING_DOWN(htim)) {
//         sign_dir = MOTOR_DIR_CCW;
//     } else {
//         sign_dir = MOTOR_DIR_CW;
//     }

//     int16_t signedCount = (int16_t)__HAL_TIM_GET_COUNTER(htim);

//     // Convert count to angle in degrees
// //    sign_angle= ((float)rawCount / ENCODER_COUNTS_PER_REV) * 360.0f;
//     float angle = ((float)signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;


//     // // Limit angle to 0–360
//     // if (sign_angle >= 360.0f) sign_angle -= 360.0f;
//     // if (sign_angle < 0.0f)    sign_angle += 360.0f;

//     // Increment read counter
//     readCounter++;

//     // Only print every `printInterval` reads
//     if (readCounter >= printInterval) {
//         char msg[80];
//         snprintf(msg, sizeof(msg),
//                  "Enc M%d: CNT=%d, Angle=%f, Dir=%s\r\n",
//                  motorID,
//                  signedCount,
//                  angle,
//                  (sign_dir == MOTOR_DIR_CW) ? "CW" : "CCW");
//         // UART_SendString(msg); // send via UART3
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//         readCounter = 0; // reset counter
//     }
// }


// void Motor_Init(void)
// {
//     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
// }

// void Motor_SetAngle(float targetAngle, float currentAngle)
// {
//     float error = targetAngle - currentAngle;

//     // Normalize error to [-180, 180]
//     if (error > 180.0f)
//         error -= 360.0f;
//     if (error < -180.0f)
//         error += 360.0f;

//     // Set direction
//     if (error >= 0)
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // Forward
//     }
//     else
//     {
//         HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // Reverse
//         error = -error;                                                   // Use magnitude for PWM duty
//     }

//     // Scale to duty cycle
//     float pwmDuty = error * 2.0f; // Simple proportional gain
//     if (pwmDuty > 100.0f)
//         pwmDuty = 100.0f;

//     uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim4);
//     __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, (uint32_t)((pwmDuty / 100.0f) * arr));

// }

// void Encoder_ReadAndControl(TIM_HandleTypeDef *htim,struct MotorAngle motor ,float targetAngle, uint8_t motorID)
// {
//     float currentAngle = Encoder_GetAngle(htim);

//     // Control motor towards target
//     Motor_SetAngle(targetAngle, currentAngle);

//     char angle_msg[64];
//     snprintf(angle_msg, sizeof(angle_msg),
//              "sended M%d: Target=%02X, Current=%02X\r\n",
//              motorID, targetAngle, currentAngle);
//     HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);
    
//     // Check if within tolerance
//     float diff = fabsf(targetAngle - currentAngle);
//     if (diff > 180.0f)
//         diff = 360.0f - diff; // Wrap-around

//     if (diff > ANGLE_TOLERANCE)
//     {
//         char msg[64];
//         snprintf(msg, sizeof(msg),
//                  "ERR M%d: Target=%02X, Current=%02X\r\n",
//                  motorID, targetAngle, currentAngle);
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//     }
//     else
//     {
//         char msg[64];
//         snprintf(msg, sizeof(msg),
//                  "OK  M%d: Target=%02X, Current=%02X\r\n",
//                  motorID, targetAngle, currentAngle);
//         HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
//     }

//     HAL_Delay(1);
// }
