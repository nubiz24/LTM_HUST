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
    char response[BUFFER_SIZE];
    int logged_in = 0;
    
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
    printf("Type 'HELP' for available commands\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        /* Đọc lệnh từ người dùng */
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        /* Loại bỏ ký tự xuống dòng */
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        /* Gửi lệnh tới server */
        strcat(buffer, "\n");
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            break;
        }
        
        /* Nhận response từ server */
        memset(response, 0, BUFFER_SIZE);
        int recv_len = recv(sock, response, BUFFER_SIZE, 0);
        if (recv_len <= 0) {
            if (recv_len == 0) {
                printf("Server closed connection\n");
            } else {
                perror("recv");
            }
            break;
        }
        
        response[recv_len] = '\0';
        response[strcspn(response, "\r\n")] = 0;
        
        /* Xử lý response */
        if (strncmp(response, "OK", 2) == 0) {
            printf("%s\n", response);
            if (strncmp(buffer, "LOGIN", 5) == 0) {
                logged_in = 1;
            } else if (strncmp(buffer, "LOGOUT", 6) == 0) {
                logged_in = 0;
            }
        } else if (strncmp(response, "ERR", 3) == 0) {
            printf("Error: %s\n", response + 4);
        } else if (strncmp(response, "LIST", 4) == 0) {
            printf("%s\n", response);
        } else {
            printf("%s\n", response);
        }
    }
    
    close(sock);
    return 0;
}

