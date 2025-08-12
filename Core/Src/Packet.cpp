#include "Packet.hpp"
#include "CheckSum.h" 
#include <array>
#include<iostream>
#include <cstring>
#include "uart.h"

using namespace std;

uint16_t FillData(const array<uint8_t, 4>& payload, PacketID packetID) {

    if (payload.size() != 4)
    {
        uart_log_printf("Payload size must be 4 bytes.\r\n");

        return -1; 
    }
    Packet packet;
    packet.start_packet = 0xAA55;
    packet.end_packet = 0x0D0A;
    packet.count = 1; 
    packet.packetID = static_cast<uint8_t>(packetID);

    for (int i = 0; i < 4; i++)
    {
       packet.payload[i] = payload[i];
    }
    uint8_t checksumData[6];
    memcpy(checksumData, packet.payload, 4);
    checksumData[4]=packet.packetID;
    checksumData[5]= packet.count;    
    packet.checksum =checksum(checksumData, 6);

    return packet.checksum;
}

uint8_t SerializePacket(const Packet packet) {
    // Check start and end packet values
    if (packet.start_packet != 0xAA55 || packet.end_packet != 0x0D0A) {
        uart_log_printf("Invalid start or end packet values.\r\n");
        return 1; // Error code for invalid start/end bytes
    }

    // Checksum validation
    uint8_t checksumData[6];
    memcpy(checksumData, packet.payload, 4);
    checksumData[4] = packet.packetID;
    checksumData[5] = packet.count;
    uint16_t checksumresult = checksum(checksumData, 6);

    if (packet.checksum != checksumresult) {
        return 2; // Error code for checksum mismatch
    }

    // Process the packet based on its ID
    switch (packet.packetID) {
        case Motor_ID:  // <--- Add opening brace
            Motor motor;
            motor.ID = packet.payload[0];
            motor.speed = packet.payload[1];
            motor.direction = packet.payload[2];

            break;

        case MotorAngle_ID: 
            MotorAngel motorAngle;
            motorAngle.ID = packet.payload[0];
            motorAngle.angle = packet.payload[1];
            motorAngle.direction = packet.payload[2];
            break;
        

        case CarHorn_ID: 
            CarHorn carHorn;
            carHorn.ID = packet.payload[0];
            carHorn.duartion = packet.payload[1];
            break;
        

        case CarLight_ID: 
            CarLight carLight;
            carLight.ID = packet.payload[0];
            carLight.lightStatus = packet.payload[1];
            break;
        

        case CarConfirmation_ID: 
            CorConfirmation carConfirmation;
            carConfirmation.ID = packet.payload[0];
            carConfirmation.confirmationStatus = packet.payload[1];
            carConfirmation.value = packet.payload[2];
            break;

        default:
            uart_log_printf("Unknown packet ID: %d\r\n", packet.packetID);
            return 3; // Error code for unknown packet ID
    }

    return 0; // Success
}
