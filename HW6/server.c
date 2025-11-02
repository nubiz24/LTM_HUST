#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
#define CLOSE_SOCKET close
#define SOCKET_ERROR_CODE errno
#endif

#define SERVER_PORT 9000
#define BUFFER_SIZE 512

// Structure to store client session
typedef struct
{
    struct sockaddr_in address;
    char login_name[31];
    int is_active;
} ClientSession;

// Simple array-based session storage (can be replaced with hash map)
#define MAX_CLIENTS 100
static ClientSession client_sessions[MAX_CLIENTS];
static int session_count = 0;

// Function to compare two sockaddr_in structures
int compare_address(struct sockaddr_in *addr1, struct sockaddr_in *addr2)
{
    return (addr1->sin_addr.s_addr == addr2->sin_addr.s_addr &&
            addr1->sin_port == addr2->sin_port);
}

// Function to find client session by address
int find_client_session(struct sockaddr_in *client_addr)
{
    for (int i = 0; i < session_count; i++)
    {
        if (client_sessions[i].is_active &&
            compare_address(&client_sessions[i].address, client_addr))
        {
            return i;
        }
    }
    return -1;
}

// Function to check if login name is already used
int is_login_name_taken(const char *login_name)
{
    for (int i = 0; i < session_count; i++)
    {
        if (client_sessions[i].is_active &&
            strcmp(client_sessions[i].login_name, login_name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Function to validate login name
int is_valid_login_name(const char *login_name)
{
    if (login_name == NULL || strlen(login_name) == 0 || strlen(login_name) > MAX_NAME_LEN)
    {
        return 0;
    }
    // Check for valid characters (alphanumeric, underscore, hyphen)
    for (int i = 0; i < strlen(login_name); i++)
    {
        char c = login_name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            return 0;
        }
    }
    return 1;
}

// Function to add or update client session
int add_client_session(struct sockaddr_in *client_addr, const char *login_name)
{
    int index = find_client_session(client_addr);

    if (index >= 0)
    {
        // Update existing session
        strncpy(client_sessions[index].login_name, login_name, MAX_NAME_LEN);
        client_sessions[index].login_name[MAX_NAME_LEN] = '\0';
        return index;
    }
    else
    {
        // Add new session
        if (session_count >= MAX_CLIENTS)
        {
            return -1; // Max clients reached
        }
        client_sessions[session_count].address = *client_addr;
        strncpy(client_sessions[session_count].login_name, login_name, MAX_NAME_LEN);
        client_sessions[session_count].login_name[MAX_NAME_LEN] = '\0';
        client_sessions[session_count].is_active = 1;
        return session_count++;
    }
}

// Function to write message to log file
void write_to_log(const char *login_name, const char *message)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "%s.log", login_name);

    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        perror("Error opening log file");
        return;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Write to file
    fprintf(file, "[%s] %s\n", time_str, message);
    fclose(file);

    printf("Logged message from %s: %s\n", login_name, message);
}

// Function to send LOGIN_ACK
int send_login_ack(int sockfd, struct sockaddr_in *client_addr, uint8_t status)
{
    LoginAck ack;
    ack.type = LOGIN_ACK;
    ack.status = status;

    socklen_t addr_len = sizeof(*client_addr);
    int sent = sendto(sockfd, (char *)&ack, sizeof(LoginAck), 0,
                      (struct sockaddr *)client_addr, addr_len);

    if (sent < 0)
    {
        perror("Error sending LOGIN_ACK");
        return -1;
    }
    return 0;
}

// Function to send ERROR_NOT_LOGGED_IN
int send_error_not_logged_in(int sockfd, struct sockaddr_in *client_addr)
{
    ErrorMessage error;
    error.type = ERROR_NOT_LOGGED_IN;
    error.error_code = 1; // Not logged in

    socklen_t addr_len = sizeof(*client_addr);
    int sent = sendto(sockfd, (char *)&error, sizeof(ErrorMessage), 0,
                      (struct sockaddr *)client_addr, addr_len);

    if (sent < 0)
    {
        perror("Error sending ERROR_NOT_LOGGED_IN");
        return -1;
    }
    return 0;
}

// Function to handle LOGIN_REQUEST
void handle_login_request(int sockfd, char *buffer, int len,
                          struct sockaddr_in *client_addr)
{
    if (len < sizeof(LoginRequest))
    {
        printf("Invalid LOGIN_REQUEST: packet too small\n");
        send_login_ack(sockfd, client_addr, LOGIN_NAME_INVALID);
        return;
    }

    LoginRequest *req = (LoginRequest *)buffer;
    req->login_name[MAX_NAME_LEN] = '\0'; // Ensure null termination

    // Validate login name
    if (!is_valid_login_name(req->login_name))
    {
        printf("Invalid login name from client: %s\n",
               inet_ntoa(client_addr->sin_addr));
        send_login_ack(sockfd, client_addr, LOGIN_NAME_INVALID);
        return;
    }

    // Check if this client already has a session
    int session_idx = find_client_session(client_addr);
    if (session_idx >= 0)
    {
        // Update existing session with new login name (or same name)
        strncpy(client_sessions[session_idx].login_name, req->login_name, MAX_NAME_LEN);
        client_sessions[session_idx].login_name[MAX_NAME_LEN] = '\0';
        printf("Client updated login name: %s (from %s:%d)\n",
               req->login_name,
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        send_login_ack(sockfd, client_addr, LOGIN_OK);
        return;
    }

    // Allow multiple clients to use the same login_name
    // All messages will be logged to the same file
    int idx = add_client_session(client_addr, req->login_name);
    if (idx < 0)
    {
        printf("Failed to add client session (max clients reached)\n");
        send_login_ack(sockfd, client_addr, LOGIN_NAME_INVALID);
        return;
    }

    printf("Client logged in successfully: %s (from %s:%d)\n",
           req->login_name,
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port));

    send_login_ack(sockfd, client_addr, LOGIN_OK);
}

// Function to handle MESSAGE_DATA
void handle_message_data(int sockfd, char *buffer, int len,
                         struct sockaddr_in *client_addr)
{
    if (len < sizeof(MessageData))
    {
        printf("Invalid MESSAGE_DATA: packet too small\n");
        return;
    }

    MessageData *msg = (MessageData *)buffer;
    msg->message[MAX_MSG_LEN] = '\0'; // Ensure null termination

    // Find client session
    int session_idx = find_client_session(client_addr);

    if (session_idx < 0)
    {
        // Client not logged in
        printf("Message received from unauthenticated client: %s:%d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        send_error_not_logged_in(sockfd, client_addr);
        return;
    }

    // Write message to log file
    write_to_log(client_sessions[session_idx].login_name, msg->message);
}

int main()
{
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int bytes_received;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif

    // Initialize session storage
    memset(client_sessions, 0, sizeof(client_sessions));
    session_count = 0;

    // Create UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        perror("Error creating socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        CLOSE_SOCKET(server_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("UDP Server started on port %d\n", SERVER_PORT);
    printf("Waiting for client connections...\n\n");

    // Main loop
    while (1)
    {
        // Receive packet from client
        client_len = sizeof(client_addr);
        bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0,
                                  (struct sockaddr *)&client_addr, &client_len);

        if (bytes_received < 0)
        {
            perror("Error receiving data");
            continue;
        }

        if (bytes_received == 0)
        {
            continue;
        }

        // Check message type
        uint8_t msg_type = buffer[0];

        switch (msg_type)
        {
        case LOGIN_REQUEST:
            handle_login_request(server_socket, buffer, bytes_received, &client_addr);
            break;

        case MESSAGE_DATA:
            handle_message_data(server_socket, buffer, bytes_received, &client_addr);
            break;

        default:
            printf("Unknown message type received: 0x%02X from %s:%d\n",
                   msg_type,
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            break;
        }
    }

    // Cleanup (unreachable in this infinite loop, but good practice)
    CLOSE_SOCKET(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
