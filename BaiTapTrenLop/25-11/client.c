#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    /* Tạo socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Cấu hình địa chỉ server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(sock);
        exit(1);
    }
    
    /* Kết nối tới server */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }
    
    printf("Connected to server %s:%d\n", server_ip, port);
    printf("Enter message to send: ");
    
    /* Đọc dữ liệu từ người dùng và gửi tới server */
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        /* Loại bỏ ký tự xuống dòng */
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        /* Gửi dữ liệu tới server */
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("send");
        } else {
            printf("Message sent: %s\n", buffer);
        }
    }
    
    close(sock);
    return 0;
}

