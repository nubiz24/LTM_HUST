#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

static int sock = -1;
static volatile sig_atomic_t data_ready = 0;

/* Signal handler cho SIGIO - xử lý khi có dữ liệu từ server */
void sigio_handler(int sig) {
    if (sig == SIGIO && sock >= 0) {
        data_ready = 1;
    }
}

/* Signal handler cho SIGTERM và SIGINT */
void sigterm_handler(int sig) {
    printf("\nReceived signal %d, closing connection...\n", sig);
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    struct sigaction sa;
    
    /* Thiết lập signal handler cho SIGIO */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigio_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGIO, &sa, NULL) < 0) {
        perror("sigaction SIGIO");
        exit(1);
    }
    
    /* Thiết lập signal handler cho SIGTERM và SIGINT */
    sa.sa_handler = sigterm_handler;
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction SIGTERM");
        exit(1);
    }
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction SIGINT");
        exit(1);
    }
    
    /* Tạo socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Thiết lập socket cho signal-driven I/O (để nhận dữ liệu từ server nếu cần) */
    if (fcntl(sock, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN");
        close(sock);
        exit(1);
    }
    
    int flags = fcntl(sock, F_GETFL);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(sock);
        exit(1);
    }
    
    if (fcntl(sock, F_SETFL, flags | O_ASYNC) < 0) {
        perror("fcntl F_SETFL");
        close(sock);
        exit(1);
    }
    
    /* Cấu hình địa chỉ server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
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
    
    printf("Connected to server %s:%d (signal-driven I/O)\n", server_ip, port);
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

