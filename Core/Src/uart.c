#include "uart.h"
#include "Packet.hpp"

volatile uint8_t rxBuffer[sizeof(Packet)];
volatile uint8_t rxIndex = 0;
volatile uint8_t packetReceivedFlag = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        rxIndex++;
        if (rxIndex >= sizeof(Packet)) {
            rxIndex = 0;
            packetReceivedFlag = 1;
        }
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&rxBuffer[rxIndex], 1);
    }
}

void uart2_send_bytes(uint8_t *data, uint16_t size) {
    HAL_UART_Transmit(&huart2, data, size, HAL_MAX_DELAY);
}
