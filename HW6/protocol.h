#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// Message Types
#define LOGIN_REQUEST         0x01
#define LOGIN_ACK            0x81
#define MESSAGE_DATA         0x02
#define ERROR_NOT_LOGGED_IN  0xF1

// Maximum lengths
#define MAX_NAME_LEN  30
#define MAX_MSG_LEN   510

// Login Request Structure (Client -> Server)
// Total size: 32 bytes
typedef struct {
    uint8_t type;              // Always 0x01
    char login_name[31];       // Login name (max 30 chars + null terminator)
} LoginRequest;

// Login Acknowledge Structure (Server -> Client)
// Total size: 2 bytes
typedef struct {
    uint8_t type;              // Always 0x81
    uint8_t status;            // 0 = OK, 1 = Name exists, 2 = Invalid name
} LoginAck;

// Message Data Structure (Client -> Server)
// Total size: 512 bytes
typedef struct {
    uint8_t type;              // Always 0x02
    char message[511];         // Message content (max 510 chars + null terminator)
} MessageData;

// Error Message Structure (Server -> Client)
// Total size: 2 bytes (optional)
typedef struct {
    uint8_t type;              // Always 0xF1
    uint8_t error_code;        // Error code
} ErrorMessage;

// Status codes for LoginAck
#define LOGIN_OK              0
#define LOGIN_NAME_EXISTS     1
#define LOGIN_NAME_INVALID    2

#endif // PROTOCOL_H

