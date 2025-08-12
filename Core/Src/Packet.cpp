#include <cstdint>
#include <array>
#include "../inc/Packet.hpp"
#include "../inc/CheckSum.hpp" // Assuming CheckSum.cpp contains the checksum function
#include<iostream>
#include <cstring>

using namespace std;

uint16_t FillData(const array<uint8_t, 4>& payload, PacketID packetID) {

    if (payload.size() != 4)
    {
        cout << "Payload size must be 4 bytes." << endl;
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

// uint8_t SerializePacket(const Packet packet){
//     // Check start and end packet values
//     if (packet.start_packet != 0xAA55 || packet.end_packet != 0x0D0A) {
//         cout << "Invalid packet start or end." << endl;
//         return -1; 
//     }

//     // Checksum validation
//     uint8_t checksumData[6];
//     memcpy(checksumData, packet.payload, 4);
//     checksumData[4]=packet.packetID;
//     checksumData[5]= packet.count;    
//     uint16_t checksumresult =checksum(checksumData, 6);

//     if(packet.checksum != checksumresult) {
//         cout << "Checksum mismatch." << endl;
//         return -1; 
//     }
//     //serialize the packet
//     switch (packet.packetID)
//     {
//         case (int)PacketID::Motor:
//             Motor motor;
//             motor.ID = packet.payload[0];
//             motor.speed = packet.payload[1];
//             motor.direction = packet.payload[2];
//             cout << "Motor ID: " << (int)motor.ID << ", Speed: " << (int)motor.speed << ", Direction: " << (int)motor.direction << endl;     
//             break;

//         case (int)PacketID::MotorAngle:
//             MotorAngel motorAngle;
//             motorAngle.ID = packet.payload[0];
//             motorAngle.angle = packet.payload[1];
//             motorAngle.direction = packet.payload[2];    
//             cout << "Motor Angle ID: " << (int)motorAngle.ID << ", Angle: " << (int)motorAngle.angle << ", Direction: " << (int)motorAngle.direction << endl; 
//             break;
            
//         case (int)PacketID::CarHorn:
//             CarHorn carHorn;
//             carHorn.ID = packet.payload[0];
//             carHorn.duartion = packet.payload[1]; // Assuming payload[1] is duration
//             cout << "Car Horn ID: " << (int)carHorn.ID << ", Duration: " << (int)carHorn.duartion << endl;
//             break;
            
//         case (int)PacketID::CarLight:
//             CarLight carLight;
//             carLight.ID = packet.payload[0];
//             carLight.lightStatus = packet.payload[1];
//             cout << "Car Light ID: " << (int)carLight.ID << ", Light Status: " << (int)carLight.lightStatus << endl;
//             break;
            
//         case (int)PacketID::CarConfirmation:
//             CorConfirmation carConfirmation;
//             carConfirmation.ID = packet.payload[0];
//             carConfirmation.confirmationStatus = packet.payload[1];
//             carConfirmation.value = packet.payload[2]; // Assuming payload[2] is the value
//             cout << "Car Confirmation ID: " << (int)carConfirmation.ID 
//                  << ", Confirmation Status: " << (int)carConfirmation.confirmationStatus 
//                  << ", Value: " << (int)carConfirmation.value << endl;
//             break;  

//         default:
//             break;
//         }
    
//     return 0;
// }


// In src/Packet.cpp

uint8_t SerializePacket(const Packet packet) {
    // Check start and end packet values
    if (packet.start_packet != 0xAA55 || packet.end_packet != 0x0D0A) {
        // On an embedded device, it's better to send an error code via UART
        // than to use std::cout, which doesn't go anywhere.
        // For now, we'll just return an error code.
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
            // std::cout is not available on the STM32.
            // You would typically use these values to control a motor here.
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
            // It's good practice to handle unknown packet IDs
            return 3; // Error code for unknown packet ID
    }

    return 0; // Success
}
