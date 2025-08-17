#include "Light.h"

extern UART_HandleTypeDef huart1;
// Light functions

void Light_Init(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "light init\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_PIN, GPIO_PIN_RESET); 
}

void Light_On(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light on\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_PIN, GPIO_PIN_SET);
}
void Light_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "light off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_PIN, GPIO_PIN_RESET);
}

void Light_Toggle(void) {
    Light_On();
    HAL_Delay(100); // 100 ms light on
    Light_Off();
    char msg[50];
    snprintf(msg, sizeof(msg), "Light toggle\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}