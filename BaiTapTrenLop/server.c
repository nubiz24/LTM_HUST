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

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int bytes_received, bytes_sent;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif
    
    // Tạo UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Không thể tạo socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind socket với địa chỉ
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Không thể bind socket");
        exit(1);
    }
    
    printf("Server đang lắng nghe trên port %d...\n", PORT);
    
    while (1) {
        // Nhận dữ liệu từ client
        bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0,
                                (struct sockaddr*)&client_addr, &client_len);
        if (bytes_received < 0) {
            perror("Lỗi khi nhận dữ liệu");
            continue;
        }
        
        buffer[bytes_received] = '\0';
        printf("Nhận được từ client: %s\n", buffer);
        printf("Client IP: %s, Port: %d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Tạo phản hồi với thông tin IP và port
        snprintf(response, BUFFER_SIZE, "Hello %s:%d from %s:%d",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                inet_ntoa(server_addr.sin_addr), PORT);
        
        // Gửi phản hồi về client
        bytes_sent = sendto(server_socket, response, strlen(response), 0,
                           (struct sockaddr*)&client_addr, client_len);
        if (bytes_sent < 0) {
            perror("Lỗi khi gửi phản hồi");
        } else {
            printf("Đã gửi phản hồi: %s\n", response);
        }
    }
    
#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif
    return 0;
}
