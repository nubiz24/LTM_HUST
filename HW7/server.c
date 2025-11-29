#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
#define MAX_ATTEMPTS 3
#define ACCOUNT_FILE "account.txt"
#define LOG_FILE "auth.log"
#define LOGGED_IN_FILE "logged_in.txt"
#define FAILED_ATTEMPTS_FILE "failed_attempts.txt"

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

// Hàm ghi log
void write_log(const char *action, const char *userid, const char *ip, int port, const char *result) {
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
}

// Đọc số lần sai password từ file
int read_failed_attempts(const char *userid) {
    FILE *file = fopen(FAILED_ATTEMPTS_FILE, "r");
    if (!file) {
        return 0;
    }
    
    char line[256];
    char uid[64];
    int attempts;
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%s %d", uid, &attempts) == 2) {
            if (strcmp(uid, userid) == 0) {
                fclose(file);
                return attempts;
            }
        }
    }
    fclose(file);
    return 0;
}

// Ghi số lần sai password vào file
void write_failed_attempts(const char *userid, int attempts) {
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
}

// Xóa failed attempts khi đăng nhập thành công
void clear_failed_attempts(const char *userid) {
    FILE *file = fopen(FAILED_ATTEMPTS_FILE, "r");
    if (!file) {
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
}

// Đọc tài khoản từ file
int read_accounts(Account accounts[], int max_accounts) {
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (!file) {
        return 0;
    }
    
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file) && count < max_accounts) {
        if (sscanf(line, "%s %s %d", accounts[count].userid, accounts[count].password, &accounts[count].status) == 3) {
            accounts[count].failed_attempts = read_failed_attempts(accounts[count].userid);
            count++;
        }
    }
    fclose(file);
    return count;
}

// Ghi tài khoản vào file
void write_accounts(Account accounts[], int count) {
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if (!file) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s %d\n", accounts[i].userid, accounts[i].password, accounts[i].status);
    }
    fclose(file);
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
    FILE *file = fopen(LOGGED_IN_FILE, "r");
    if (!file) {
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
    return count;
}

// Ghi danh sách user đang đăng nhập vào file
void write_logged_in_users(LoggedInUser users[], int count) {
    FILE *file = fopen(LOGGED_IN_FILE, "w");
    if (!file) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s %d\n", users[i].userid, users[i].ip, users[i].port);
    }
    fclose(file);
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
    char temp[128];
    
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

// Xử lý client connection
void handle_client(int client_sock, struct sockaddr_in client_addr) {
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
    exit(0);
}

// Xử lý zombie processes
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
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
    
    // Xử lý zombie processes
    signal(SIGCHLD, sigchld_handler);
    
    printf("Server listening on port %d\n", port);
    
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        
        // Fork để xử lý client mới
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_sock);
            handle_client(client_sock, client_addr);
        } else if (pid > 0) {
            // Parent process
            close(client_sock);
        } else {
            perror("fork");
            close(client_sock);
        }
    }
    
    close(server_sock);
    return 0;
}

