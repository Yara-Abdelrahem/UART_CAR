#include "Packet.h"
#include "CheckSum.h"
#include "uart.h"
#include "Motor_Angle.h"
#include "Speed_Motor.h"
#include "Horn.h"
#include "Light.h"
#include <stdio.h>
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
uint16_t FillData_MotorAngle(uint8_t id, int16_t angle, uint8_t direction)
{
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

        if (motor.ID < 1 || motor.ID > 2)
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid motor ID.\r\n", 20, HAL_MAX_DELAY);
            return 5; // Invalid motor ID
        }
        if (motor.speed > 100 || motor.speed < 0)
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid speed value.\r\n", 23, HAL_MAX_DELAY);
            return 6; // Invalid speed
        }
        if (motor.direction > 1 || motor.direction < 0)
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid direction value.\r\n", 26, HAL_MAX_DELAY);
            return 7; // Invalid direction
        }
        // Run motor 1 forward at 60%

        Motor_SetSpeed(3, 60, 1);

        break;
    }

    case MotorAngle_ID:
    {
        struct MotorAngle motorAngle;
        motorAngle.ID = packet->payload[0];
        motorAngle.angle = (int16_t)(packet->payload[1] | (packet->payload[2] << 8));
        motorAngle.direction = packet->payload[3];
        if (motorAngle.angle > 25 || motorAngle.angle < (-160))
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid angle value.\r\n", sizeof("Invalid angle value.\r\n"), HAL_MAX_DELAY);
            return 4; // Invalid angle
        }

        HAL_UART_Transmit(&huart1, (uint8_t *)"Encoder Data Read in Packet:\r\n", sizeof("Encoder Data Read in Packet:\r\n"), HAL_MAX_DELAY);
        char angle_msg[64];
        snprintf(angle_msg, sizeof(angle_msg),
                 "sended M%02X: Target=%04X, Current=%04X\r\n",
                 motorAngle.ID, motorAngle.angle, motorAngle.direction);
        HAL_UART_Transmit(&huart1, (uint8_t *)angle_msg, strlen(angle_msg), HAL_MAX_DELAY);

        // Encoder_ReadData(&htim3, &motorAngle, 0x1);
        // Encoder_ReadAndControl(&htim3, motorAngle, 1); // target 90 degrees

        break;
    }

    case CarHorn_ID:
    {
        struct CarHorn carHorn = {
            .ID = packet->payload[0],
            .duartion = packet->payload[1]};

        if (carHorn.duartion < 0)
        {
            HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid duration.\r\n", sizeof("Invalid duration.\r\n"), HAL_MAX_DELAY);
            return 5; // Invalid angle
        }
        else
        {
            Horn_Toggle((carHorn.duartion) * 1000);
        }
        break;
    }

    case CarLight_ID:
    {
        struct CarLight carLight = {
            .ID = packet->payload[0],
            .lightStatus = packet->payload[1]};

        if (carLight.lightStatus > 8)
        {

            char msg[50];
            snprintf(msg, sizeof(msg), "Invaild ID %02X\r\n", carLight.lightStatus);
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
            // HAL_UART_Transmit(&huart1, (const uint8_t *)"Invalid light status value.\r\n", 30, HAL_MAX_DELAY);
            return 8; // Invalid light status
        }

        if (carLight.lightStatus == 0)
        {
            // front off
            Light_Front_Off();
        }
        else if (carLight.lightStatus == 1)
        {
            // front on
            Light_Front_On();
        }

        else if (carLight.lightStatus == 2)
        {
            // back on
            Light_Back_On();
        }
        else if (carLight.lightStatus == 3)
        {
            // back off
            Light_Back_Off();
        }
        else if (carLight.lightStatus == 4)
        {
            // rigth on
            Light_Right_On();
        }
        else if (carLight.lightStatus == 5)
        {
            // right off
            Light_Right_Off();
        }
        else if (carLight.lightStatus == 6)
        {
            // left on
            Light_Left_On();
        }
        else if (carLight.lightStatus == 7)
        {
            // left off
            Light_Left_Off();
        }

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
