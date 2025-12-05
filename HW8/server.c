#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_ATTEMPTS 3
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "auth.log"
#define LOGGED_IN_FILE "logged_in.txt"
#define FAILED_ATTEMPTS_FILE "failed_attempts.txt"

// Mutex để bảo vệ các thao tác file
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char userid[64];
    char password[64];
    int status;
    int failed_attempts;
} Account;

typedef struct {
    char userid[64];
    char ip[INET_ADDRSTRLEN];
    int port;
} LoggedInUser;

// Cấu trúc để truyền thông tin client vào thread
typedef struct {
    int client_sock;
    struct sockaddr_in client_addr;
} ClientInfo;

// Hàm ghi log
void write_log(const char *action, const char *userid, const char *ip, int port, const char *result) {
    pthread_mutex_lock(&log_mutex);
    
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        
        if (port > 0) {
            fprintf(log_file, "[%s] %s %s from %s:%d %s\n", timestamp, action, userid, ip, port, result);
        } else {
            fprintf(log_file, "[%s] %s %s %s\n", timestamp, action, userid, result);
        }
        fclose(log_file);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

// Đọc số lần sai password từ file (internal, không lock mutex)
static int read_failed_attempts_internal(const char *userid) {
    FILE *file = fopen(FAILED_ATTEMPTS_FILE, "r");
    if (!file) {
        return 0;
    }
    
    char line[256];
    char uid[64];
    int attempts;
    int result = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %d", uid, &attempts) == 2) {
            if (strcmp(uid, userid) == 0) {
                result = attempts;
                break;
            }
        }
    }
    fclose(file);
    
    return result;
}

// Đọc số lần sai password từ file
int read_failed_attempts(const char *userid) {
    pthread_mutex_lock(&file_mutex);
    int result = read_failed_attempts_internal(userid);
    pthread_mutex_unlock(&file_mutex);
    return result;
}

// Ghi số lần sai password vào file
void write_failed_attempts(const char *userid, int attempts) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(FAILED_ATTEMPTS_FILE, "r");
    char lines[100][256];
    int count = 0;
    int found = 0;
    
    if (file) {
        char line[256];
        char uid[64];
        while (fgets(line, sizeof(line), file) && count < 100) {
            if (sscanf(line, "%s", uid) == 1) {
                if (strcmp(uid, userid) == 0) {
                    sprintf(lines[count], "%s %d\n", userid, attempts);
                    found = 1;
                } else {
                    strcpy(lines[count], line);
                }
                count++;
            }
        }
        fclose(file);
    }
    
    if (!found) {
        sprintf(lines[count], "%s %d\n", userid, attempts);
        count++;
    }
    
    file = fopen(FAILED_ATTEMPTS_FILE, "w");
    if (file) {
        for (int i = 0; i < count; i++) {
            fprintf(file, "%s", lines[i]);
        }
        fclose(file);
    }
    
    pthread_mutex_unlock(&file_mutex);
}

// Xóa failed attempts khi đăng nhập thành công
void clear_failed_attempts(const char *userid) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(FAILED_ATTEMPTS_FILE, "r");
    if (!file) {
        pthread_mutex_unlock(&file_mutex);
        return;
    }
    
    char lines[100][256];
    int count = 0;
    char line[256];
    char uid[64];
    
    while (fgets(line, sizeof(line), file) && count < 100) {
        if (sscanf(line, "%s", uid) == 1) {
            if (strcmp(uid, userid) != 0) {
                strcpy(lines[count], line);
                count++;
            }
        }
    }
    fclose(file);
    
    file = fopen(FAILED_ATTEMPTS_FILE, "w");
    if (file) {
        for (int i = 0; i < count; i++) {
            fprintf(file, "%s", lines[i]);
        }
        fclose(file);
    }
    
    pthread_mutex_unlock(&file_mutex);
}

// Đọc tài khoản từ file
int read_accounts(Account accounts[], int max_accounts) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (!file) {
        pthread_mutex_unlock(&file_mutex);
        return 0;
    }
    
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) && count < max_accounts) {
        if (sscanf(line, "%s %s %d", accounts[count].userid, accounts[count].password, &accounts[count].status) == 3) {
            accounts[count].failed_attempts = read_failed_attempts_internal(accounts[count].userid);
            count++;
        }
    }
    fclose(file);
    
    pthread_mutex_unlock(&file_mutex);
    return count;
}

// Ghi tài khoản vào file
void write_accounts(Account accounts[], int count) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (!file) {
        pthread_mutex_unlock(&file_mutex);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s %d\n", accounts[i].userid, accounts[i].password, accounts[i].status);
    }
    fclose(file);
    
    pthread_mutex_unlock(&file_mutex);
}

// Tìm tài khoản theo userid
Account* find_account(Account accounts[], int count, const char *userid) {
    for (int i = 0; i < count; i++) {
        if (strcmp(accounts[i].userid, userid) == 0) {
            return &accounts[i];
        }
    }
    return NULL;
}

// Đọc danh sách user đang đăng nhập từ file
int read_logged_in_users(LoggedInUser users[], int max_users) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(LOGGED_IN_FILE, "r");
    if (!file) {
        pthread_mutex_unlock(&file_mutex);
        return 0;
    }
    
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) && count < max_users) {
        if (sscanf(line, "%s %s %d", users[count].userid, users[count].ip, &users[count].port) == 3) {
            count++;
        }
    }
    fclose(file);
    
    pthread_mutex_unlock(&file_mutex);
    return count;
}

// Ghi danh sách user đang đăng nhập vào file
void write_logged_in_users(LoggedInUser users[], int count) {
    pthread_mutex_lock(&file_mutex);
    
    FILE *file = fopen(LOGGED_IN_FILE, "w");
    if (!file) {
        pthread_mutex_unlock(&file_mutex);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s %d\n", users[i].userid, users[i].ip, users[i].port);
    }
    fclose(file);
    
    pthread_mutex_unlock(&file_mutex);
}

// Thêm user vào danh sách đăng nhập
void add_logged_in_user(const char *userid, const char *ip, int port) {
    LoggedInUser users[100];
    int count = read_logged_in_users(users, 100);
    
    // Kiểm tra xem user đã có trong danh sách chưa (cùng ip và port)
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].userid, userid) == 0 &&
            strcmp(users[i].ip, ip) == 0 &&
            users[i].port == port) {
            return; // Đã tồn tại
        }
    }
    
    // Thêm user mới
    if (count < 100) {
        strcpy(users[count].userid, userid);
        strcpy(users[count].ip, ip);
        users[count].port = port;
        count++;
        write_logged_in_users(users, count);
    }
}

// Xóa user khỏi danh sách đăng nhập
void remove_logged_in_user(const char *userid, const char *ip, int port) {
    LoggedInUser users[100];
    int count = read_logged_in_users(users, 100);
    
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].userid, userid) == 0 &&
            strcmp(users[i].ip, ip) == 0 &&
            users[i].port == port) {
            // Di chuyển các phần tử còn lại lên
            for (int j = i; j < count - 1; j++) {
                users[j] = users[j + 1];
            }
            count--;
            write_logged_in_users(users, count);
            break;
        }
    }
}

// Xử lý lệnh LOGIN
// Trả về 1 nếu thành công, 0 nếu thất bại. userid được lưu vào current_user nếu thành công
int handle_login(const char *buffer, int client_sock, const char *ip, int port, char *current_user) {
    char userid[64], password[64];
    
    // Parse lệnh LOGIN: LOGIN userid password
    if (sscanf(buffer, "LOGIN %s %s", userid, password) != 2) {
        send(client_sock, "ERR invalid_format", 18, 0);
        return 0;
    }
    
    // Đọc lại accounts để có thông tin mới nhất
    Account accounts[100];
    int account_count = read_accounts(accounts, 100);
    Account *account = find_account(accounts, account_count, userid);
    
    if (!account) {
        send(client_sock, "ERR account_not_found", 22, 0);
        write_log("LOGIN", userid, ip, port, "FAIL (account not found)");
        return 0;
    }
    
    if (account->status == 0) {
        send(client_sock, "ERR account_locked", 18, 0);
        write_log("LOGIN", userid, ip, port, "FAIL (account locked)");
        return 0;
    }
    
    if (strcmp(account->password, password) != 0) {
        int failed = read_failed_attempts(userid) + 1;
        write_failed_attempts(userid, failed);
        
        if (failed >= MAX_ATTEMPTS) {
            account->status = 0;
            write_accounts(accounts, account_count);
            send(client_sock, "ERR account_locked", 18, 0);
            write_log("ACCOUNT_LOCKED", userid, ip, 0, "");
            write_log("LOGIN", userid, ip, port, "FAIL (wrong password - account locked)");
        } else {
            send(client_sock, "ERR wrong_password", 19, 0);
            write_log("LOGIN", userid, ip, port, "FAIL (wrong password)");
        }
        return 0;
    }
    
    // Đăng nhập thành công
    clear_failed_attempts(userid);
    add_logged_in_user(userid, ip, port);
    strcpy(current_user, userid);
    send(client_sock, "OK login_success", 16, 0);
    write_log("LOGIN", userid, ip, port, "SUCCESS");
    return 1;
}

// Xử lý lệnh LOGOUT
void handle_logout(int client_sock, const char *ip, int port, const char *userid) {
    remove_logged_in_user(userid, ip, port);
    send(client_sock, "OK logout_success", 17, 0);
    write_log("LOGOUT", userid, ip, port, "");
}

// Xử lý lệnh WHO
void handle_who(int client_sock) {
    LoggedInUser users[100];
    int count = read_logged_in_users(users, 100);
    char response[BUFFER_SIZE] = "LIST ";
    
    if (count == 0) {
        strcat(response, "(empty)");
    } else {
        // Lấy danh sách unique userid
        char unique_users[100][64];
        int unique_count = 0;
        
        for (int i = 0; i < count; i++) {
            int found = 0;
            for (int j = 0; j < unique_count; j++) {
                if (strcmp(unique_users[j], users[i].userid) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(unique_users[unique_count], users[i].userid);
                unique_count++;
            }
        }
        
        for (int i = 0; i < unique_count; i++) {
            if (i > 0) strcat(response, " ");
            strcat(response, unique_users[i]);
        }
    }
    
    send(client_sock, response, strlen(response), 0);
}

// Xử lý lệnh HELP
void handle_help(int client_sock) {
    const char *help_msg = "OK Supported commands: LOGIN, LOGOUT, WHO, HELP";
    send(client_sock, help_msg, strlen(help_msg), 0);
}

// Xử lý client connection (hàm này sẽ chạy trong thread)
void* handle_client(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    int client_sock = client_info->client_sock;
    struct sockaddr_in client_addr = client_info->client_addr;
    
    // Giải phóng bộ nhớ của ClientInfo
    free(client_info);
    
    char ip[INET_ADDRSTRLEN];
    int port = ntohs(client_addr.sin_port);
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
    
    char current_user[64] = "";
    int logged_in = 0;
    
    char buffer[BUFFER_SIZE];
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recv(client_sock, buffer, BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            if (logged_in) {
                remove_logged_in_user(current_user, ip, port);
                write_log("LOGOUT", current_user, ip, port, "(disconnected)");
            }
            break;
        }
        
        // Loại bỏ ký tự xuống dòng
        buffer[strcspn(buffer, "\r\n")] = 0;
        
        if (strncmp(buffer, "LOGIN", 5) == 0) {
            if (logged_in) {
                send(client_sock, "ERR already_logged_in", 21, 0);
            } else {
                if (handle_login(buffer, client_sock, ip, port, current_user)) {
                    logged_in = 1;
                }
            }
        }
        else if (strncmp(buffer, "LOGOUT", 6) == 0) {
            if (!logged_in) {
                send(client_sock, "ERR not_logged_in", 17, 0);
            } else {
                handle_logout(client_sock, ip, port, current_user);
                logged_in = 0;
                memset(current_user, 0, sizeof(current_user));
            }
        }
        else if (strncmp(buffer, "WHO", 3) == 0) {
            handle_who(client_sock);
        }
        else if (strncmp(buffer, "HELP", 4) == 0) {
            handle_help(client_sock);
        }
        else {
            send(client_sock, "ERR unknown_command", 19, 0);
        }
    }
    
    close(client_sock);
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
    
    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(1);
    }
    
    // Listen
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(1);
    }
    
    printf("Server listening on port %d (using pthread)\n", port);
    
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        
        // Tạo thread để xử lý client mới
        pthread_t thread_id;
        ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
        if (!client_info) {
            perror("malloc");
            close(client_sock);
            continue;
        }
        
        client_info->client_sock = client_sock;
        client_info->client_addr = client_addr;
        
        // Tạo thread mới để xử lý client
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_info) != 0) {
            perror("pthread_create");
            free(client_info);
            close(client_sock);
            continue;
        }
        
        // Detach thread để tự động giải phóng tài nguyên khi kết thúc
        pthread_detach(thread_id);
    }
    
    close(server_sock);
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&log_mutex);
    return 0;
}

