#include "Light.h"

extern UART_HandleTypeDef huart1;
// Light functions

void Light_Init(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "light init\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Front_PIN, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Back_PIN, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Right_PIN, GPIO_PIN_RESET); 
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Left_PIN, GPIO_PIN_RESET); 
}


void   Light_Front_On(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Front_On\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Front_PIN, GPIO_PIN_SET);
}

void Light_Front_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Front_Off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Front_PIN, GPIO_PIN_RESET);
}

void Light_Back_On(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Back_On\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Back_PIN, GPIO_PIN_SET);
}

void Light_Back_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Back_Off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Back_PIN, GPIO_PIN_RESET);
}

void Light_Right_On(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Right_On\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Right_PIN, GPIO_PIN_SET);
}
void Light_Right_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Right_Off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Right_PIN, GPIO_PIN_RESET);
}

void Light_Left_On(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Left_On\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Left_PIN, GPIO_PIN_SET);
}
void Light_Left_Off(void) {
    char msg[50];
    snprintf(msg, sizeof(msg), "Light_Left_Off\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LIGHT_GPIO_PORT, LIGHT_Left_PIN, GPIO_PIN_RESET);
}

