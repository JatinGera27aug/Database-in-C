#include "cache22.h"

bool scontinuation; // server continuation

void mainloop(int16 port) {
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

    // Right now, we just close the server socket (no accept loop yet)
    closesocket(s);
    WSACleanup();
}

int main(int argc, char *argv[]) {
    const char *sport;
    int16 port;

    if (argc < 2) {
        sport = PORT; // Default port from cache22.h
    } else {
        sport = argv[1];
    }

    port = (int16)atoi(sport);
    scontinuation = true;

    while (scontinuation) {
        mainloop(port);
    }

    return 0;
}
//---->  runing by cntrl+shift+B