#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

/* Mutex để đồng bộ hóa việc ghi file */
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Biến toàn cục để lưu server socket */
static int server_sock = -1;
static volatile sig_atomic_t server_running = 1;

/* Cấu trúc để truyền dữ liệu vào thread */
typedef struct {
    int client_sock;
    struct sockaddr_in client_addr;
} client_info_t;

/* Hàm xử lý client trong thread */
void* handle_client(void* arg) {
    client_info_t* client_info = (client_info_t*)arg;
    int client_sock = client_info->client_sock;
    char buffer[BUFFER_SIZE];
    FILE *fp;
    
    /* Nhận dữ liệu từ client */
    int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        /* Loại bỏ ký tự xuống dòng nếu có */
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        
        /* Sử dụng mutex để đồng bộ hóa việc ghi file */
        pthread_mutex_lock(&file_mutex);
        
        /* Mở file user.txt để ghi (append mode) */
        fp = fopen("user.txt", "a");
        if (fp == NULL) {
            perror("fopen");
            pthread_mutex_unlock(&file_mutex);
            close(client_sock);
            free(client_info);
            return NULL;
        }
        
        /* Ghi vào file với format "User : Nội dung" */
        fprintf(fp, "User : %s\n", buffer);
        fflush(fp);
        fclose(fp);
        
        pthread_mutex_unlock(&file_mutex);
        
        printf("Received from client %s:%d: %s\n", 
               inet_ntoa(client_info->client_addr.sin_addr),
               ntohs(client_info->client_addr.sin_port),
               buffer);
    }
    
    close(client_sock);
    free(client_info);
    return NULL;
}

/* Signal handler cho SIGIO - xử lý kết nối mới */
void sigio_handler(int sig) {
    if (sig == SIGIO && server_sock >= 0) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock;
        pthread_t thread_id;
        
        /* Chấp nhận kết nối mới */
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept");
            }
            return;
        }
        
        printf("Client connected from %s:%d (signal-driven)\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Tạo cấu trúc thông tin client */
        client_info_t* client_info = (client_info_t*)malloc(sizeof(client_info_t));
        if (client_info == NULL) {
            perror("malloc");
            close(client_sock);
            return;
        }
        client_info->client_sock = client_sock;
        client_info->client_addr = client_addr;
        
        /* Tạo thread để xử lý client mới */
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_info) != 0) {
            perror("pthread_create");
            free(client_info);
            close(client_sock);
            return;
        }
        
        /* Detach thread để tự động giải phóng tài nguyên khi kết thúc */
        pthread_detach(thread_id);
    }
}

/* Signal handler cho SIGTERM và SIGINT - dừng server */
void sigterm_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    server_running = 0;
    if (server_sock >= 0) {
        close(server_sock);
        server_sock = -1;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    struct sockaddr_in server_addr;
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
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    
    /* Thiết lập socket cho signal-driven I/O */
    if (fcntl(server_sock, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN");
        close(server_sock);
        exit(1);
    }
    
    int flags = fcntl(server_sock, F_GETFL);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(server_sock);
        exit(1);
    }
    
    if (fcntl(server_sock, F_SETFL, flags | O_ASYNC | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        close(server_sock);
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
    
    printf("Server listening on port %d (signal-driven I/O)\n", port);
    printf("Press Ctrl+C to stop the server\n");
    
    /* Xóa nội dung file user.txt khi server khởi động */
    FILE *fp = fopen("user.txt", "w");
    if (fp) {
        fclose(fp);
    }
    
    /* Vòng lặp chính: chờ signal */
    while (server_running) {
        pause(); /* Chờ signal */
    }
    
    pthread_mutex_destroy(&file_mutex);
    printf("Server stopped.\n");
    return 0;
}

