#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[] = "Hello world";
    int bytes_sent, bytes_received;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif
    
    // Tạo UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Không thể tạo socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Gửi dữ liệu đến server
    printf("Gửi dữ liệu đến server: %s\n", message);
    bytes_sent = sendto(client_socket, message, strlen(message), 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        perror("Lỗi khi gửi dữ liệu");
        exit(1);
    }
    
    // Nhận phản hồi từ server
    bytes_received = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0, NULL, NULL);
    if (bytes_received < 0) {
        perror("Lỗi khi nhận phản hồi");
        exit(1);
    }
    
    buffer[bytes_received] = '\0';
    printf("Nhận được phản hồi từ server: %s\n", buffer);
    
#ifdef _WIN32
    closesocket(client_socket);
    WSACleanup();
#else
    close(client_socket);
#endif
    return 0;
}
