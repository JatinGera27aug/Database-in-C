#include "cache22.h"

bool scontinuation; // server continuation

void mainloop(SOCKET s) {
    struct sockaddr_in cli;
    int len = sizeof(cli);
    SOCKET s2 = accept(s, (struct sockaddr *)&cli, &len);
    if (s2 == INVALID_SOCKET) {
        printf("accept() failed: %d\n", WSAGetLastError());
        return;
    }

    int16 port = (int16)ntohs(cli.sin_port);
    char *ip = inet_ntoa(cli.sin_addr);
    printf("Connection from %s:%d\n", ip, port);

    closesocket(s2);
}


void initServer(int16 port) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in sock;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        exit(1);
    }

    // Create socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(1);
    }

    // Prepare socket structure
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = inet_addr(HOST);

    // Bind
    if (bind(s, (struct sockaddr *)&sock, sizeof(sock)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }

    // Listen
    if (listen(s, 20) == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        exit(1);
    }

    printf("Server listening on %s:%d...\n", HOST, port);

    // Simulated accept loop until shutdown
    u_long mode = 1;
ioctlsocket(s, FIONBIO, &mode);

while (scontinuation) {
    if (_kbhit()) {
        char ch = _getch();
        if (ch == 'q') {
            printf("\n'q' pressed, shutting down...\n");
            scontinuation = false;
            break;
        }
    }

    struct sockaddr_in cli;
    int len = sizeof(cli);
    SOCKET s2 = accept(s, (struct sockaddr *)&cli, &len);
    if (s2 != INVALID_SOCKET) {
        int16 port = (int16)ntohs(cli.sin_port);
        char *ip = inet_ntoa(cli.sin_addr);
        printf("Connection from %s:%d\n", ip, port);
        closesocket(s2);
    } else {
        // No connection right now — just sleep a bit to avoid high CPU usage
        Sleep(50);
    }
}

}

int main(int argc, char *argv[]) {
    const char *sport;
    int16 port;

    if (argc < 2) {
        sport = PORT;
    } else {
        sport = argv[1];
    }

    port = (int16)atoi(sport);
    scontinuation = true;

    initServer(port);

    printf("Shutdown the Server!\n");
    return 0;
}









//to run the server : mingw32-make   , then :  .\cache22

//---->  runing by ctrl+shift+B