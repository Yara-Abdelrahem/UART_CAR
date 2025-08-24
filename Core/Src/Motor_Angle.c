#include "Motor_Angle.h"
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

typedef struct {
    int16_t encoder_min;
    int16_t encoder_max;
    int16_t encoder_center;
} MotorCalibration;

static MotorCalibration motor1_calib;


/* ==================== Encoder Functions ==================== */

static int16_t Encoder_GetAngle(TIM_HandleTypeDef *htim) {
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
    int16_t angle = (signedCount / ENCODER_COUNTS_PER_REV) * 360.0f;

    readCounter++;
    if (readCounter >= printInterval) {
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

/* ==================== Motor Functions ==================== */

// void Motor_Init_Angle(void) {
//     HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
//     HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
// }

void Motor_Init_Angle(void) {
    HAL_TIM_PWM_Start(&htim4, MOTOR_PWM_CHANNEL);
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET);
    Encoder_Init(&htim3);

    // ==== Move to LEFT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_RESET); // CCW
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, __HAL_TIM_GET_AUTORELOAD(&htim4) * 0.6); // 60% duty
    HAL_Delay(1500); // wait till it hits left stop (adjust time)
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0); // stop
    motor1_calib.encoder_min = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    // ==== Move to RIGHT limit ====
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, GPIO_PIN_SET); // CW
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, __HAL_TIM_GET_AUTORELOAD(&htim4) * 0.6);
    HAL_Delay(1500); // wait till right stop
    __HAL_TIM_SET_COMPARE(&htim4, MOTOR_PWM_CHANNEL, 0);
    motor1_calib.encoder_max = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    // ==== Center ====
    motor1_calib.encoder_center = (motor1_calib.encoder_min + motor1_calib.encoder_max) / 2;

    char msg[100];
    snprintf(msg, sizeof(msg),
             "Calib Done: Min=%d, Max=%d, Center=%d\r\n",
             motor1_calib.encoder_min,
             motor1_calib.encoder_max,
             motor1_calib.encoder_center);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

    Motor_SetAngle(motor1_calib.encoder_center, (int16_t)__HAL_TIM_GET_COUNTER(&htim3));
}


void Motor_SetAngle(int16_t targetAngle, int16_t currentAngle) {
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
    Motor_SetAngle(motor.angle, currentAngle);

    // Send status
    char angle_msg[64];
    snprintf(angle_msg, sizeof(angle_msg),
                 "sended M%d: Target=%02X, Current=%02X\r\n",
             motorID, motor.angle, currentAngle);
    HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);

    // Check tolerance
    float diff = fabsf((float)(motor.angle - currentAngle));
    if (diff > 180.0f) diff = 360.0f - diff;

    const char *status = (diff > ANGLE_TOLERANCE) ? "ERR" : "OK";
    char msg[64];
    snprintf(msg, sizeof(msg),
          "M%d: Target=%02X, Current=%02X\r\n",
              motorID, motor.angle, currentAngle);
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);

    HAL_Delay(1);
}

void Motor_GotoAngle(int16_t angle_deg) {
    // Map angle [-90..90] to encoder range
    int16_t target;
    if (angle_deg < -90) angle_deg = -90;
    if (angle_deg > 90)  angle_deg = 90;

    int16_t halfRange = (motor1_calib.encoder_max - motor1_calib.encoder_min) / 2;
    target = motor1_calib.encoder_center + ((int32_t)angle_deg * halfRange) / 90;

    int16_t current = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);

    // Use your Motor_SetAngle logic
    Motor_SetAngle(target, current);
}
