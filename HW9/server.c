#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <poll.h>
#include <ctype.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_ATTEMPTS 3
#define ACCOUNT_FILE "nguoidung.txt"

// Chọn phương thức: SELECT hoặc POLL
#define USE_SELECT 1
// #define USE_POLL 1

typedef struct {
    char username[64];
    char password[64];
    int status;  // 1 = active, 0 = blocked/not ready
    int failed_attempts;
} Account;

typedef struct {
    int sockfd;
    char username[64];
    int logged_in;
    int failed_attempts;
    struct sockaddr_in addr;
} ClientInfo;

Account accounts[100];
int account_count = 0;
ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
int server_sock;

// Đọc file nguoidung.txt
void load_accounts() {
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (!file) {
        perror("Cannot open account file");
        exit(1);
    }
    
    account_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) && account_count < 100) {
        if (sscanf(line, "%s %s %d", 
                   accounts[account_count].username,
                   accounts[account_count].password,
                   &accounts[account_count].status) == 3) {
            accounts[account_count].failed_attempts = 0;
            account_count++;
        }
    }
    fclose(file);
}

// Lưu lại trạng thái accounts
void save_accounts() {
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (!file) {
        perror("Cannot write account file");
        return;
    }
    
    for (int i = 0; i < account_count; i++) {
        fprintf(file, "%s %s %d\n", 
                accounts[i].username,
                accounts[i].password,
                accounts[i].status);
    }
    fclose(file);
}

// Tìm account theo username
int find_account(const char *username) {
    for (int i = 0; i < account_count; i++) {
        if (strcmp(accounts[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

// Tìm client theo socket
int find_client(int sockfd) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

// Tìm tất cả clients đăng nhập cùng username
void find_clients_by_username(const char *username, int *client_indices, int *count) {
    *count = 0;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].logged_in && strcmp(clients[i].username, username) == 0) {
            client_indices[(*count)++] = i;
        }
    }
}

// Gửi message đến client
void send_to_client(int sockfd, const char *message) {
    send(sockfd, message, strlen(message), 0);
}

// Xử lý đăng nhập
void handle_login(int client_idx, const char *username, const char *password) {
    int acc_idx = find_account(username);
    
    if (acc_idx == -1) {
        send_to_client(clients[client_idx].sockfd, "Not OK\n");
        return;
    }
    
    // Kiểm tra nếu account đã bị block
    if (accounts[acc_idx].status == 0) {
        send_to_client(clients[client_idx].sockfd, "Account not ready\n");
        return;
    }
    
    // Kiểm tra password
    if (strcmp(accounts[acc_idx].password, password) == 0) {
        // Password đúng
        if (accounts[acc_idx].status == 1) {
            // Account active
            clients[client_idx].logged_in = 1;
            strcpy(clients[client_idx].username, username);
            clients[client_idx].failed_attempts = 0;
            accounts[acc_idx].failed_attempts = 0;
            send_to_client(clients[client_idx].sockfd, "OK\n");
        } else {
            // Account blocked hoặc not ready
            send_to_client(clients[client_idx].sockfd, "Account not ready\n");
        }
    } else {
        // Password sai
        clients[client_idx].failed_attempts++;
        accounts[acc_idx].failed_attempts++;
        
        if (accounts[acc_idx].failed_attempts >= MAX_ATTEMPTS) {
            accounts[acc_idx].status = 0;  // Block account
            save_accounts();
            send_to_client(clients[client_idx].sockfd, "Account is blocked\n");
        } else {
            send_to_client(clients[client_idx].sockfd, "Not OK\n");
            // Yêu cầu nhập lại password cho cùng username
            send_to_client(clients[client_idx].sockfd, "Insert password\n");
        }
    }
}

// Kiểm tra password mới có hợp lệ không (chỉ chữ cái và số)
int is_valid_password(const char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (!isalnum(password[i])) {
            return 0;
        }
    }
    return 1;
}

// Mã hóa password: tách thành 2 xâu (chữ cái và số)
void encode_password(const char *password, char *letters, char *digits) {
    int l_idx = 0, d_idx = 0;
    
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            letters[l_idx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[d_idx++] = password[i];
        }
    }
    letters[l_idx] = '\0';
    digits[d_idx] = '\0';
}

// Xử lý đổi password
void handle_change_password(int client_idx, const char *new_password) {
    if (!clients[client_idx].logged_in) {
        return;
    }
    
    // Kiểm tra password hợp lệ
    if (!is_valid_password(new_password)) {
        send_to_client(clients[client_idx].sockfd, "Error\n");
        return;
    }
    
    // Tìm account và cập nhật password
    int acc_idx = find_account(clients[client_idx].username);
    if (acc_idx == -1) {
        return;
    }
    
    strcpy(accounts[acc_idx].password, new_password);
    save_accounts();
    
    // Mã hóa password
    char letters[256] = {0};
    char digits[256] = {0};
    encode_password(new_password, letters, digits);
    
    // Gửi kết quả cho client hiện tại
    char response[512];
    if (strlen(letters) > 0 && strlen(digits) > 0) {
        snprintf(response, sizeof(response), "%s\n%s\n", digits, letters);
    } else if (strlen(digits) > 0) {
        snprintf(response, sizeof(response), "%s\n", digits);
    } else if (strlen(letters) > 0) {
        snprintf(response, sizeof(response), "%s\n", letters);
    } else {
        snprintf(response, sizeof(response), "\n");
    }
    send_to_client(clients[client_idx].sockfd, response);
    
    // Điểm cộng: Gửi cho tất cả clients đăng nhập cùng account
    int client_indices[MAX_CLIENTS];
    int count;
    find_clients_by_username(clients[client_idx].username, client_indices, &count);
    
    // Gửi password đã mã hóa cho các clients khác đăng nhập cùng account
    for (int i = 0; i < count; i++) {
        int idx = client_indices[i];
        if (idx != client_idx && clients[idx].logged_in) {
            // Gửi password đã mã hóa cho client thứ 2, 3, ...
            send_to_client(clients[idx].sockfd, response);
        }
    }
}

// Xử lý signout
void handle_signout(int client_idx) {
    if (clients[client_idx].logged_in) {
        char response[128];
        snprintf(response, sizeof(response), "Goodbye %s\n", clients[client_idx].username);
        send_to_client(clients[client_idx].sockfd, response);
        clients[client_idx].logged_in = 0;
        memset(clients[client_idx].username, 0, sizeof(clients[client_idx].username));
    }
}

// Xử lý message từ client
void handle_client_message(int client_idx, const char *message) {
    char msg[BUFFER_SIZE];
    strncpy(msg, message, sizeof(msg) - 1);
    msg[sizeof(msg) - 1] = '\0';
    
    // Loại bỏ ký tự xuống dòng
    int len = strlen(msg);
    while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r')) {
        msg[len-1] = '\0';
        len--;
    }
    
    if (!clients[client_idx].logged_in) {
        // Chưa đăng nhập: nhận username và password
        if (strlen(clients[client_idx].username) == 0) {
            // Nhận username
            strncpy(clients[client_idx].username, msg, sizeof(clients[client_idx].username) - 1);
            clients[client_idx].username[sizeof(clients[client_idx].username) - 1] = '\0';
            send_to_client(clients[client_idx].sockfd, "Insert password\n");
        } else {
            // Nhận password
            int acc_idx = find_account(clients[client_idx].username);
            handle_login(client_idx, clients[client_idx].username, msg);
            if (!clients[client_idx].logged_in) {
                // Kiểm tra xem account có bị block không
                if (acc_idx >= 0 && accounts[acc_idx].status == 0) {
                    // Account đã bị block, reset username để có thể thử account khác
                    memset(clients[client_idx].username, 0, sizeof(clients[client_idx].username));
                }
                // Nếu chưa block, giữ lại username để nhập lại password
            }
        }
    } else {
        // Đã đăng nhập
        if (strcmp(msg, "bye") == 0) {
            handle_signout(client_idx);
        } else {
            handle_change_password(client_idx, msg);
        }
    }
}

// Xóa client khỏi danh sách
void remove_client(int client_idx) {
    close(clients[client_idx].sockfd);
    if (client_idx < client_count - 1) {
        clients[client_idx] = clients[client_count - 1];
    }
    client_count--;
}

// Thêm client mới
void add_client(int sockfd, struct sockaddr_in addr) {
    if (client_count >= MAX_CLIENTS) {
        close(sockfd);
        return;
    }
    
    clients[client_count].sockfd = sockfd;
    clients[client_count].logged_in = 0;
    clients[client_count].failed_attempts = 0;
    clients[client_count].addr = addr;
    memset(clients[client_count].username, 0, sizeof(clients[client_count].username));
    client_count++;
    
    printf("Client connected from %s:%d\n", 
           inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

// Server sử dụng select()
void run_server_select(int port) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set read_fds, master_fds;
    int max_fd;
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // Listen
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Server listening on port %d\n", port);
    
    FD_ZERO(&master_fds);
    FD_SET(server_sock, &master_fds);
    max_fd = server_sock;
    
    while (1) {
        read_fds = master_fds;
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }
        
        // Kiểm tra kết nối mới
        if (FD_ISSET(server_sock, &read_fds)) {
            int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
            if (client_sock >= 0) {
                add_client(client_sock, client_addr);
                FD_SET(client_sock, &master_fds);
                if (client_sock > max_fd) {
                    max_fd = client_sock;
                }
            }
        }
        
        // Kiểm tra dữ liệu từ clients
        for (int i = client_count - 1; i >= 0; i--) {
            int sockfd = clients[i].sockfd;
            if (FD_ISSET(sockfd, &read_fds)) {
                int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0) {
                    // Client đóng kết nối
                    FD_CLR(sockfd, &master_fds);
                    remove_client(i);
                } else {
                    buffer[n] = '\0';
                    handle_client_message(i, buffer);
                }
            }
        }
    }
}

// Server sử dụng poll()
void run_server_poll(int port) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }
    
    // Cấu hình địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    // Listen
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Server listening on port %d (using poll)\n", port);
    
    // Khởi tạo pollfd cho server socket
    fds[0].fd = server_sock;
    fds[0].events = POLLIN;
    
    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            continue;
        }
        
        // Kiểm tra kết nối mới
        if (fds[0].revents & POLLIN) {
            int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
            if (client_sock >= 0 && nfds < MAX_CLIENTS + 1) {
                add_client(client_sock, client_addr);
                fds[nfds].fd = client_sock;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }
        
        // Kiểm tra dữ liệu từ clients
        for (int i = nfds - 1; i >= 1; i--) {
            if (fds[i].revents & POLLIN) {
                int n = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0) {
                    // Client đóng kết nối
                    int client_idx = find_client(fds[i].fd);
                    if (client_idx >= 0) {
                        remove_client(client_idx);
                    }
                    close(fds[i].fd);
                    // Xóa khỏi mảng pollfd
                    fds[i] = fds[nfds - 1];
                    nfds--;
                } else {
                    buffer[n] = '\0';
                    int client_idx = find_client(fds[i].fd);
                    if (client_idx >= 0) {
                        handle_client_message(client_idx, buffer);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        exit(1);
    }
    
    load_accounts();
    client_count = 0;
    
#ifdef USE_SELECT
    run_server_select(port);
#elif defined(USE_POLL)
    run_server_poll(port);
#else
    // Mặc định dùng select
    run_server_select(port);
#endif
    
    return 0;
}

