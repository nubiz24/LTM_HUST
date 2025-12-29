#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    char *ip = argv[1];
    int port = atoi(argv[2]);
    
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }
    
    // Tạo socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        close(sockfd);
        exit(1);
    }
    
    // Kết nối đến server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(1);
    }
    
    printf("Connected to server %s:%d\n", ip, port);
    
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    
    // Vòng lặp chính với select() để nhận dữ liệu bất đồng bộ
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);  // Kiểm tra input từ bàn phím
        FD_SET(sockfd, &read_fds);        // Kiểm tra dữ liệu từ server
        
        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;
        
        // Chờ dữ liệu từ stdin hoặc socket
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }
        
        // Kiểm tra dữ liệu từ server
        if (FD_ISSET(sockfd, &read_fds)) {
            int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    printf("Server closed connection\n");
                } else {
                    perror("recv");
                }
                break;
            }
            
            buffer[n] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        }
        
        // Kiểm tra input từ bàn phím
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(input, sizeof(input), stdin) == NULL) {
                break;
            }
            
            // Loại bỏ ký tự xuống dòng
            int len = strlen(input);
            while (len > 0 && (input[len-1] == '\n' || input[len-1] == '\r')) {
                input[len-1] = '\0';
                len--;
            }
            
            // Nếu nhập xâu rỗng thì thoát
            if (len == 0) {
                break;
            }
            
            // Gửi đến server
            strcat(input, "\n");
            if (send(sockfd, input, strlen(input), 0) < 0) {
                perror("send");
                break;
            }
        }
    }
    
    close(sockfd);
    return 0;
}

