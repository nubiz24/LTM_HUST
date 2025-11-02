#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
    #define SOCKET_ERROR_CODE WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <sys/select.h>
    #include <errno.h>
    #define CLOSE_SOCKET close
    #define SOCKET_ERROR_CODE errno
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000
#define BUFFER_SIZE 512
#define TIMEOUT_SECONDS 5

// Function to setup socket timeout
int setup_socket_timeout(int sockfd, int seconds) {
#ifdef _WIN32
    DWORD timeout = seconds * 1000; // milliseconds
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        return -1;
    }
#else
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return -1;
    }
#endif
    return 0;
}

// Function to send LOGIN_REQUEST and wait for LOGIN_ACK
int login_to_server(int sockfd, struct sockaddr_in *server_addr, const char *login_name) {
    LoginRequest req;
    req.type = LOGIN_REQUEST;
    strncpy(req.login_name, login_name, MAX_NAME_LEN);
    req.login_name[MAX_NAME_LEN] = '\0';
    
    // Send LOGIN_REQUEST
    socklen_t addr_len = sizeof(*server_addr);
    int sent = sendto(sockfd, (char *)&req, sizeof(LoginRequest), 0,
                      (struct sockaddr *)server_addr, addr_len);
    
    if (sent < 0) {
        perror("Error sending LOGIN_REQUEST");
        return -1;
    }
    
    printf("Login request sent. Waiting for server response...\n");
    
    // Wait for LOGIN_ACK
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    int received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&from_addr, &from_len);
    
    if (received < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            printf("Timeout: No response from server.\n");
        } else {
            perror("Error receiving LOGIN_ACK");
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Timeout: No response from server.\n");
        } else {
            perror("Error receiving LOGIN_ACK");
        }
#endif
        return -1;
    }
    
    // Verify response is LOGIN_ACK
    if (received < sizeof(LoginAck)) {
        printf("Invalid response from server.\n");
        return -1;
    }
    
    LoginAck *ack = (LoginAck *)buffer;
    
    if (ack->type != LOGIN_ACK) {
        printf("Unexpected message type received: 0x%02X\n", ack->type);
        return -1;
    }
    
    // Check status
    switch (ack->status) {
        case LOGIN_OK:
            printf("Login successful!\n");
            return 0;
            
        case LOGIN_NAME_EXISTS:
            printf("Error: Login name already exists. Please choose another name.\n");
            return -1;
            
        case LOGIN_NAME_INVALID:
            printf("Error: Invalid login name. Please use alphanumeric characters, underscore, or hyphen.\n");
            return -1;
            
        default:
            printf("Error: Unknown status code from server: %d\n", ack->status);
            return -1;
    }
}

// Function to send MESSAGE_DATA
int send_message(int sockfd, struct sockaddr_in *server_addr, const char *message) {
    MessageData msg;
    msg.type = MESSAGE_DATA;
    strncpy(msg.message, message, MAX_MSG_LEN);
    msg.message[MAX_MSG_LEN] = '\0';
    
    socklen_t addr_len = sizeof(*server_addr);
    int sent = sendto(sockfd, (char *)&msg, sizeof(MessageData), 0,
                      (struct sockaddr *)server_addr, addr_len);
    
    if (sent < 0) {
        perror("Error sending MESSAGE_DATA");
        return -1;
    }
    
    printf("Message sent successfully.\n");
    return 0;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char login_name[256];
    char message[512];
    int logged_in = 0;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif
    
    // Create UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Setup socket timeout for login
    if (setup_socket_timeout(client_socket, TIMEOUT_SECONDS) < 0) {
        perror("Error setting socket timeout");
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    
    printf("=== UDP Authentication Client ===\n");
    printf("Server: %s:%d\n\n", SERVER_IP, SERVER_PORT);
    
    // Login phase
    while (!logged_in) {
        printf("Enter login name (max %d characters): ", MAX_NAME_LEN);
        if (fgets(login_name, sizeof(login_name), stdin) == NULL) {
            printf("Error reading input.\n");
            CLOSE_SOCKET(client_socket);
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
        
        // Remove newline character
        size_t len = strlen(login_name);
        if (len > 0 && login_name[len - 1] == '\n') {
            login_name[len - 1] = '\0';
        }
        
        if (strlen(login_name) == 0) {
            printf("Login name cannot be empty.\n");
            continue;
        }
        
        // Attempt login
        if (login_to_server(client_socket, &server_addr, login_name) == 0) {
            logged_in = 1;
        } else {
            printf("\nWould you like to try again? (y/n): ");
            char choice[10];
            if (fgets(choice, sizeof(choice), stdin) == NULL) {
                break;
            }
            if (choice[0] != 'y' && choice[0] != 'Y') {
                break;
            }
            printf("\n");
        }
    }
    
    if (!logged_in) {
        printf("Login failed. Exiting...\n");
        CLOSE_SOCKET(client_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    
    // Remove timeout for message sending (optional)
    // For now, we'll keep it, but client doesn't need to wait for response
    
    // Message sending phase
    printf("\n=== Message Sending Mode ===\n");
    printf("Enter messages to send (type 'quit' or 'exit' to quit):\n\n");
    
    while (1) {
        printf("Message: ");
        if (fgets(message, sizeof(message), stdin) == NULL) {
            break;
        }
        
        // Remove newline character
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }
        
        // Check for quit command
        if (strcmp(message, "quit") == 0 || strcmp(message, "exit") == 0 ||
            strcmp(message, "q") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        if (strlen(message) == 0) {
            continue;
        }
        
        // Send message
        if (send_message(client_socket, &server_addr, message) < 0) {
            printf("Failed to send message.\n");
        }
    }
    
    // Cleanup
    CLOSE_SOCKET(client_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}

