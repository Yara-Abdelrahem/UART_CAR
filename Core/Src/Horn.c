#include "Horn.h"

extern UART_HandleTypeDef huart1;


void Horn_Init(void) {
    HAL_GPIO_WritePin(HORN_GPIO_PORT, HORN_PIN, GPIO_PIN_RESET); // horn off
    char msg[50];
    snprintf(msg, sizeof(msg), "horn init\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    
}

void Horn_On(void) {
        char msg[50];
    snprintf(msg, sizeof(msg), "horn on\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(HORN_GPIO_PORT, HORN_PIN, GPIO_PIN_SET); // energize relay
}

void Horn_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "horn off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(HORN_GPIO_PORT, HORN_PIN, GPIO_PIN_RESET); // de-energize relay
}


void Horn_Toggle(uint32_t delay) {
    HAL_GPIO_WritePin(HORN_GPIO_PORT, HORN_PIN, GPIO_PIN_SET); // energize relay
    HAL_Delay(delay); // 100 ms honk
    HAL_GPIO_WritePin(HORN_GPIO_PORT, HORN_PIN, GPIO_PIN_RESET); // de-energize relay
    char msg[50];
    snprintf(msg, sizeof(msg), "Horn toggle\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}
