#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#endif

#define MAX_ACCOUNTS 1024
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 10

typedef struct {
    char username[128];
    char password[128];
    char email[256];
    char homepage[256];
    int status; /* 1 active, 0 blocked */
} Account;

typedef enum {
    STAGE_WAIT_USERNAME = 0,
    STAGE_WAIT_PASSWORD = 1,
    STAGE_LOGGED_IN = 2
} SessionStage;

typedef struct {
    SessionStage stage;
    int failCount;
    int currentUserIndex; /* index into accounts, -1 if none */
    int clientSocket;
    int isActive;
} Session;

static Account accounts[MAX_ACCOUNTS];
static int numAccounts = 0;
static char accountFilePath[512] = "account.txt"; /* path actually used for load/save */

static int stringsAreEqual(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static int tryOpenAccountFile(FILE **f, const char *mode) {
    /* Try account.txt in current dir, then parent dir. If opened, remember path. */
    const char *candidates[] = { "account.txt", "../account.txt", "..\\account.txt" };
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
        *f = fopen(candidates[i], mode);
        if (*f) {
            strncpy(accountFilePath, candidates[i], sizeof(accountFilePath) - 1);
            accountFilePath[sizeof(accountFilePath) - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

static void loadAccounts(void) {
    FILE *fp = NULL;
    if (!tryOpenAccountFile(&fp, "r")) {
        fprintf(stderr, "Cannot open account.txt\n");
        exit(1);
    }
    char line[1024];
    numAccounts = 0;
    while (fgets(line, sizeof(line), fp)) {
        char user[128] = {0}, pass[128] = {0}, email[256] = {0}, home[256] = {0};
        int status = 1;
        int count = sscanf(line, "%127s %127s %255s %255s %d", user, pass, email, home, &status);
        if (count >= 3) { /* support forms: user pass status | or user pass email homepage status */
            Account acc;
            memset(&acc, 0, sizeof(acc));
            strncpy(acc.username, user, sizeof(acc.username) - 1);
            strncpy(acc.password, pass, sizeof(acc.password) - 1);
            if (count == 3) {
                acc.status = atoi(email); /* third token interpreted as status */
            } else {
                strncpy(acc.email, email, sizeof(acc.email) - 1);
                strncpy(acc.homepage, home, sizeof(acc.homepage) - 1);
                acc.status = status;
            }
            accounts[numAccounts++] = acc;
            if (numAccounts >= MAX_ACCOUNTS) break;
        }
    }
    fclose(fp);
}

static void persistAccounts(void) {
    FILE *fp = fopen(accountFilePath, "w");
    if (!fp) {
        /* fallback attempts if direct path failed */
        if (!tryOpenAccountFile(&fp, "w")) {
            fprintf(stderr, "Cannot write account file at %s\n", accountFilePath);
            return;
        }
    }
    for (int i = 0; i < numAccounts; ++i) {
        Account *a = &accounts[i];
        if (a->email[0] == '\0' && a->homepage[0] == '\0') {
            fprintf(fp, "%s %s %d\n", a->username, a->password, a->status);
        } else {
            fprintf(fp, "%s %s %s %s %d\n", a->username, a->password, a->email[0]?a->email:"-", a->homepage[0]?a->homepage:"-", a->status);
        }
    }
    fflush(fp);
    fclose(fp);
}

static int findAccountByUsername(const char *username) {
    for (int i = 0; i < numAccounts; ++i) {
        if (stringsAreEqual(accounts[i].username, username)) return i;
    }
    return -1;
}

static void extractLettersAndDigits(const char *input, char *digitsOut, char *lettersOut) {
    size_t d = 0, l = 0;
    for (size_t i = 0; input[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (isdigit(c)) {
            digitsOut[d++] = (char)c;
        } else if (isalpha(c)) {
            lettersOut[l++] = (char)tolower(c);
        }
    }
    digitsOut[d] = '\0';
    lettersOut[l] = '\0';
}

static int containsOnlyAlnum(const char *s) {
    for (size_t i = 0; s[i] != '\0'; ++i) {
        if (!isalnum((unsigned char)s[i])) return 0;
    }
    return 1;
}

static void handleClient(int clientSocket) {
    Session session;
    memset(&session, 0, sizeof(session));
    session.stage = STAGE_WAIT_USERNAME;
    session.failCount = 0;
    session.currentUserIndex = -1;
    session.clientSocket = clientSocket;
    session.isActive = 1;

    char buffer[BUFFER_SIZE];
    
    while (session.isActive) {
        int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            /* Client disconnected */
            break;
        }
        buffer[bytes] = '\0';

        /* Trim CRLF */
        for (int i = bytes - 1; i >= 0; --i) {
            if (buffer[i] == '\r' || buffer[i] == '\n') buffer[i] = '\0'; else break;
        }

        const char *reply = NULL;
        char replyBuf[BUFFER_SIZE];
        memset(replyBuf, 0, sizeof(replyBuf));

        if (session.stage == STAGE_WAIT_USERNAME) {
            int idx = findAccountByUsername(buffer);
            if (idx < 0) {
                reply = "Account not found";
            } else if (accounts[idx].status == 0) {
                reply = "Account is blocked"; /* or 'account not ready' */
            } else {
                session.stage = STAGE_WAIT_PASSWORD;
                session.currentUserIndex = idx;
                session.failCount = 0;
                reply = "Insert password";
            }
        } else if (session.stage == STAGE_WAIT_PASSWORD) {
            Account *acc = &accounts[session.currentUserIndex];
            if (stringsAreEqual(buffer, acc->password)) {
                if (acc->status == 1) {
                    session.stage = STAGE_LOGGED_IN;
                    reply = "OK";
                } else {
                    reply = "account not ready";
                }
            } else {
                session.failCount++;
                if (session.failCount >= 3) {
                    acc->status = 0;
                    persistAccounts();
                    session.stage = STAGE_WAIT_USERNAME;
                    session.currentUserIndex = -1;
                    reply = "Account is blocked";
                } else {
                    reply = "Not OK";
                }
            }
        } else if (session.stage == STAGE_LOGGED_IN) {
            if (stringsAreEqual(buffer, "bye")) {
                snprintf(replyBuf, sizeof(replyBuf), "Goodbye %s", accounts[session.currentUserIndex].username);
                reply = replyBuf;
                session.stage = STAGE_WAIT_USERNAME;
                session.currentUserIndex = -1;
                session.failCount = 0;
            } else if (stringsAreEqual(buffer, "homepage")) {
                const char *hp = accounts[session.currentUserIndex].homepage;
                if (hp[0] == '\0') hp = "homepage not set";
                reply = hp;
            } else {
                if (!containsOnlyAlnum(buffer)) {
                    reply = "Error";
                } else {
                    /* Update password in memory and persist to account.txt */
                    strncpy(accounts[session.currentUserIndex].password, buffer,
                            sizeof(accounts[session.currentUserIndex].password) - 1);
                    accounts[session.currentUserIndex].password[
                        sizeof(accounts[session.currentUserIndex].password) - 1] = '\0';
                    persistAccounts();

                    /* Return masked representation: digits and letters separated */
                    char digits[BUFFER_SIZE];
                    char letters[BUFFER_SIZE];
                    extractLettersAndDigits(buffer, digits, letters);
                    if (digits[0] != '\0' && letters[0] != '\0') {
                        snprintf(replyBuf, sizeof(replyBuf), "%s\n%s", digits, letters);
                    } else if (digits[0] != '\0') {
                        snprintf(replyBuf, sizeof(replyBuf), "%s", digits);
                    } else {
                        snprintf(replyBuf, sizeof(replyBuf), "%s", letters);
                    }
                    reply = replyBuf;
                }
            }
        }

        if (!reply) reply = "";
        send(clientSocket, reply, (int)strlen(reply), 0);
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        return 1;
    }

    loadAccounts();

    unsigned short port = (unsigned short)atoi(argv[1]);
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN); /* Ignore broken pipe signals */
#endif

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        perror("socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    /* Allow socket reuse */
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
#ifdef _WIN32
        closesocket(serverSock);
        WSACleanup();
#else
        close(serverSock);
#endif
        return 1;
    }

    if (listen(serverSock, MAX_CLIENTS) < 0) {
        perror("listen");
#ifdef _WIN32
        closesocket(serverSock);
        WSACleanup();
#else
        close(serverSock);
#endif
        return 1;
    }

    printf("TCP Server listening on port %d\n", port);

    while (1) {
        clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected from %s:%d\n", 
               inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        /* Handle client in current thread (for simplicity) */
        /* In production, you'd want to fork/thread here */
        handleClient(clientSock);
        
        printf("Client disconnected\n");
    }

#ifdef _WIN32
    closesocket(serverSock);
    WSACleanup();
#else
    close(serverSock);
#endif
    return 0;
}
