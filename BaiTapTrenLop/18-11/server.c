#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

/* Hàm xử lý zombie process */
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Hàm xử lý client trong child process */
void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    FILE *fp;
    
    /* Mở file user.txt để ghi (append mode) */
    fp = fopen("user.txt", "a");
    if (fp == NULL) {
        perror("fopen");
        close(client_sock);
        exit(1);
    }
    
    /* Nhận dữ liệu từ client */
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        /* Loại bỏ ký tự xuống dòng nếu có */
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        
        /* Ghi vào file với format "User: Nội dung" */
        fprintf(fp, "User: %s\n", buffer);
        fflush(fp);
        
        printf("Received from client: %s\n", buffer);
    }
    
    fclose(fp);
    close(client_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pid_t pid;
    
    /* Tạo socket */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Cấu hình địa chỉ server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind socket */
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(1);
    }
    
    /* Listen */
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(1);
    }
    
    /* Xử lý zombie process */
    signal(SIGCHLD, sigchld_handler);
    
    printf("Server listening on port %d\n", port);
    
    /* Xóa nội dung file user.txt khi server khởi động */
    FILE *fp = fopen("user.txt", "w");
    if (fp) {
        fclose(fp);
    }
    
    /* Vòng lặp chính: chấp nhận kết nối từ nhiều client */
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Fork process để xử lý client mới */
        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_sock);
            continue;
        }
        
        if (pid == 0) {
            /* Child process: xử lý client */
            close(server_sock); /* Đóng server socket trong child */
            handle_client(client_sock);
        } else {
            /* Parent process: đóng client socket và tiếp tục chờ client mới */
            close(client_sock);
        }
    }
    
    close(server_sock);
    return 0;
}

