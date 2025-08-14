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
uint16_t FillData_MotorAngle(uint8_t id, int16_t angle, uint8_t direction) {
    struct Packet packet;
    packet.start_packet = 0xAA55;
    packet.end_packet = 0x0D0A;
    packet.count = 1;
    packet.packetID = MotorAngle_ID;

    packet.payload[0] = id;
    packet.payload[1] = (uint8_t)(angle & 0xFF);
    packet.payload[2] = (uint8_t)((angle >> 8) & 0xFF);
    packet.payload[3] = direction;

    uint8_t checksumData[6];
    memcpy(checksumData, packet.payload, PAYLOAD_SIZE);
    checksumData[4] = packet.packetID;
    checksumData[5] = packet.count;

    packet.checksum = checksum(checksumData, sizeof(checksumData));

    return packet.checksum;
}


uint8_t SerializePacket(const struct Packet *packet)
{
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
        if (packet->payload[1] > 25 || packet->payload[1] < (-160))
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid angle value.\r\n", sizeof("Invalid angle value.\r\n"), HAL_MAX_DELAY);
            return 4; // Invalid angle
        }
        // struct MotorAngle motorAngle = {
        //     .ID = packet->payload[0],
        //     .angle = packet->payload[1],
        //     .direction = packet->payload[2]};
        struct MotorAngle motorAngle;
        motorAngle.ID = packet->payload[0];
        motorAngle.angle = (int16_t)(packet->payload[1] | (packet->payload[2] << 8));
        motorAngle.direction = packet->payload[3];

        HAL_UART_Transmit(&huart1, (uint8_t *)"Encoder Data Read in Packet:\r\n", sizeof("Encoder Data Read in Packet:\r\n"), HAL_MAX_DELAY);
        char angle_msg[64];
        snprintf(angle_msg, sizeof(angle_msg),
         "sended M%02X: Target=%04X, Current=%04X\r\n",
                 motorAngle.ID, motorAngle.angle, motorAngle.direction);
        HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);

        // Encoder_ReadData(&htim3, &motorAngle, 0x1);
        Encoder_ReadAndControl(&htim3, motorAngle, 1); // target 90 degrees

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
