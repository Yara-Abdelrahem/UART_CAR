// inc/Packet.hpp

#ifndef PACKET_H
#define PACKET_H

// This guard allows the header to be used in both C and C++ files
#ifdef __cplusplus
#include <cstdint> // Use C++ headers for C++ code
#include <array>   // std::array is C++ only
#include <cstring>
#include <iostream>

extern "C" {
#else
#include <stdint.h> // Use standard C headers for C code
#include <stdbool.h> // Include for the 'bool' type in C

#endif

// --- C-Compatible Part ---
// This section can be understood by both C and C++ compilers.
#define PAYLOAD_SIZE 4

// Use a C-style 'typedef enum' for compatibility.
typedef enum {
    Motor_ID = 0x01,
    MotorAngle_ID = 0x02,
    CarHorn_ID = 0x03,
    CarLight_ID = 0x04,
    CarConfirmation_ID = 0x05
} PacketID;

// These structs are C-compatible.
struct Motor {
    uint8_t ID;
    uint8_t speed;
    uint8_t direction;
};
struct MotorAngle {
    uint8_t ID;
    uint8_t angle;
    uint8_t direction;
};
struct CarHorn {
    uint8_t ID;
    uint8_t duartion;
};
struct CarLight {
    uint8_t ID;
    uint8_t lightStatus;
};
struct CarConfirmation {
    uint8_t ID;
    uint8_t packetID;
    uint8_t confirmationStatus;
    uint8_t value;
};

// The main Packet struct is also C-compatible.
struct Packet {
    uint16_t start_packet;
    uint8_t packetID;
    uint8_t payload[4];
    uint8_t count;
    uint16_t checksum;
    uint16_t end_packet;
};

// This function declaration is C-compatible and can be called from uart.c or main.cpp.
// Note the use of 'struct Packet' for strict C compatibility.
uint16_t FillData(const uint8_t payload[PAYLOAD_SIZE], PacketID packetID);
uint8_t SerializePacket(const struct Packet *packet);
// uint16_t FillData(const uint8_t payload[PAYLOAD_SIZE], PacketID packetID)


#ifdef __cplusplus
} // End of the extern "C" block
#endif


// --- C++-Only Part ---
// This section is only visible to the C++ compiler.

#ifdef __cplusplus

// The FillData function uses C++ features (std::array, PacketID enum class if you were using it)
// and should only be declared for C++ code.
uint16_t FillData(const std::array<uint8_t, 4>& payload, PacketID packetID);

#endif // __cplusplus

#endif // PACKET_HPP
