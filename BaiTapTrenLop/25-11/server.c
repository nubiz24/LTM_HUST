#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

/* Mutex để đồng bộ hóa việc ghi file */
static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Cấu trúc để truyền tham số cho thread */
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
        
        /* Khóa mutex trước khi ghi file */
        pthread_mutex_lock(&file_mutex);
        
        /* Mở file user.txt để ghi (append mode) */
        fp = fopen("user.txt", "a");
        if (fp != NULL) {
            /* Ghi vào file với format "User: Nội dung" */
            fprintf(fp, "User: %s\n", buffer);
            fflush(fp);
            fclose(fp);
        }
        
        /* Mở khóa mutex sau khi ghi file */
        pthread_mutex_unlock(&file_mutex);
        
        printf("Received from client %s:%d: %s\n", 
               inet_ntoa(client_info->client_addr.sin_addr),
               ntohs(client_info->client_addr.sin_port),
               buffer);
    }
    
    close(client_sock);
    free(client_info); /* Giải phóng bộ nhớ */
    pthread_exit(NULL);
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
    pthread_t thread_id;
    client_info_t* client_info;
    
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
        
        /* Cấp phát bộ nhớ cho thông tin client */
        client_info = (client_info_t*)malloc(sizeof(client_info_t));
        if (client_info == NULL) {
            perror("malloc");
            close(client_sock);
            continue;
        }
        
        client_info->client_sock = client_sock;
        client_info->client_addr = client_addr;
        
        /* Tạo thread để xử lý client mới */
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_info) != 0) {
            perror("pthread_create");
            free(client_info);
            close(client_sock);
            continue;
        }
        
        /* Tách thread để nó tự giải phóng tài nguyên khi kết thúc */
        pthread_detach(thread_id);
    }
    
    close(server_sock);
    pthread_mutex_destroy(&file_mutex);
    return 0;
}

