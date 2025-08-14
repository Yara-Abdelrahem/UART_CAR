#include "Packet.h"
#include "CheckSum.h"
#include "uart.h"
#include "encoder_driver.h"
#include <string.h>

// Assuming these are declared elsewhere
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;

uint16_t FillData(const uint8_t payload[PAYLOAD_SIZE], PacketID packetID)
{
    struct Packet packet;
    packet.start_packet = 0xAA55;
    packet.end_packet = 0x0D0A;
    packet.count = 1;
    packet.packetID = (uint8_t)packetID;

    memcpy(packet.payload, payload, PAYLOAD_SIZE);

    uint8_t checksumData[6];
    memcpy(checksumData, packet.payload, PAYLOAD_SIZE);
    checksumData[4] = (uint8_t)packetID;
    checksumData[5] = packet.count;

    packet.checksum = checksum(checksumData, sizeof(checksumData));

    return packet.checksum;
}

uint8_t SerializePacket(const struct Packet *packet){
    if (!packet)
        return 4; // Null pointer

    // Validate start/end markers
    if (packet->start_packet != 0xAA55 || packet->end_packet != 0x0D0A)
    {
        HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid start or end packet values.\r\n", 36, HAL_MAX_DELAY);
        return 1;
    }

    // Validate checksum
    uint8_t checksumData[6];
    memcpy(checksumData, packet->payload, PAYLOAD_SIZE);
    checksumData[4] = packet->packetID;
    checksumData[5] = packet->count;

    uint16_t checksumResult = checksum(checksumData, 6);
    if (packet->checksum != checksumResult)
    {
        return 2; // Checksum mismatch
    }

    // Process packet by ID
    switch (packet->packetID)
    {
    case Motor_ID:
    {
        struct Motor motor = {
            .ID = packet->payload[0],
            .speed = packet->payload[1],
            .direction = packet->payload[2]};
        // TODO: Add motor control logic
        break;
    }

    case MotorAngle_ID:
    {
        struct MotorAngle motorAngle = {
            .ID = packet->payload[0],
            .angle = packet->payload[1],
            .direction = packet->payload[2]
        };

        HAL_UART_Transmit(&huart1, (const uint8_t *)"Encoder Data Read:\r\n", 21, HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (const uint8_t *)"ID: ", 4, HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart1, &motorAngle.ID, 1, HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (const uint8_t *)"\r\nAngle: ", 9, HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart1, &motorAngle.angle, 1, HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (const uint8_t *)"\r\nDirection: ", 13, HAL_MAX_DELAY);
        HAL_UART_Transmit(&huart1, &motorAngle.direction, 1, HAL_MAX_DELAY);

        HAL_UART_Transmit(&huart1, (const uint8_t *)"\r\n", 2, HAL_MAX_DELAY);

        Encoder_ReadData(&htim3, &motorAngle, 0x1);
        break;
    }

    case CarHorn_ID:
    {
        struct CarHorn carHorn = {
            .ID = packet->payload[0],
            .duartion = packet->payload[1]};
        break;
    }

    case CarLight_ID:
    {
        struct CarLight carLight = {
            .ID = packet->payload[0],
            .lightStatus = packet->payload[1]};
        break;
    }

    case CarConfirmation_ID:
    {
        struct CarConfirmation carConfirmation = {
            .ID = packet->payload[0],
            .confirmationStatus = packet->payload[1],
            .value = packet->payload[2]};
        break;
    }

    default:
    {
        HAL_UART_Transmit(&huart1,
                          (const uint8_t *)"Unknown packet ID\r\n",
                          19, HAL_MAX_DELAY);
        return 3;
    }
    }

    return 0; // Success
}
