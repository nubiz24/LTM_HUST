#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define BUFFER_SIZE 2048

static void trim_newline(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
    {
        s[n - 1] = '\0';
        n--;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IPAddress> <PortNumber>\n", argv[0]);
        return 1;
    }

    const char *serverIp = argv[1];
    unsigned short port = (unsigned short)atoi(argv[2]);

    int sock;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    socklen_t addrLen = sizeof(serverAddr);

    while (1)
    {
        printf("Username (empty to quit): ");
        if (!fgets(buffer, sizeof(buffer), stdin))
            break;
        trim_newline(buffer);
        if (buffer[0] == '\0')
            break;

        sendto(sock, buffer, (int)strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrLen);
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
        if (bytes < 0)
        {
            perror("recvfrom");
            break;
        }
        buffer[bytes] = '\0';
        printf("%s\n", buffer);

        if (strcmp(buffer, "Insert password") == 0 || strcmp(buffer, "Not OK") == 0)
        {
            /* Loop until OK (logged in) or any terminal state */
            while (1)
            {
                printf("Password: ");
                if (!fgets(buffer, sizeof(buffer), stdin))
                {
                    buffer[0] = '\0';
                }
                trim_newline(buffer);
                if (buffer[0] == '\0')
                {
                    break;
                }

                sendto(sock, buffer, (int)strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrLen);

                bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
                if (bytes < 0)
                {
                    perror("recvfrom");
                    break;
                }
                buffer[bytes] = '\0';
                printf("%s\n", buffer);

                if (strcmp(buffer, "Not OK") == 0)
                {
                    /* Ask password again */
                    continue;
                }
                /* Either OK, account not ready, or blocked -> exit password loop */
                break;
            }
        }

        if (strcmp(buffer, "OK") != 0)
        {
            /* login failed or blocked or cancelled */
            continue;
        }

        /* Logged in: accept commands until username loop restarts */
        while (1)
        {
            printf(
                "Enter new password (alnum only), or 'homepage', or 'bye' (empty to stop): ");
            if (!fgets(buffer, sizeof(buffer), stdin))
            {
                buffer[0] = '\0';
            }
            trim_newline(buffer);
            if (buffer[0] == '\0')
            {
                break;
            }
            sendto(sock, buffer, (int)strlen(buffer), 0, (struct sockaddr *)&serverAddr, addrLen);
            bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
            if (bytes < 0)
            {
                perror("recvfrom");
                break;
            }
            buffer[bytes] = '\0';
            printf("%s\n", buffer);
            if (strncmp(buffer, "Goodbye", 7) == 0)
            {
                /* Signed out, go back to username prompt */
                break;
            }
        }
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}
