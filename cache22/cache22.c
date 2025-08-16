#include "cache22.h"

bool scontinuation; // server continuation
bool ccontinuation;//child continuation


// zero out buffer
void zero(int8 *buf, int16 size) {
    memset(buf, 0, size);
}


// worker function for client handling
DWORD WINAPI childloop(LPVOID param) {
    Client *client = (Client *)param;
    ccontinuation = true;

    const char *msg = "100 connected to cache22 server\n";
    send(client->s, msg, (int)strlen(msg), 0);

    while (ccontinuation) {
        Sleep(1000); // simulate work
    }

    closesocket(client->s);
    free(client);
    return 0;
}



void mainloop(SOCKET s) {
    struct sockaddr_in cli;
    int len = sizeof(cli);
    SOCKET s2 = accept(s, (struct sockaddr *)&cli, &len);





    if (s2 == INVALID_SOCKET) {
        printf("accept() failed: %u\n", WSAGetLastError());
        return;
    }

    int16 port = (int16)ntohs(cli.sin_port);
    char *ip = inet_ntoa(cli.sin_addr);
    printf("Connection from %s:%d\n", ip, port);

    Client *client = (Client *)malloc(sizeof(struct s_client));
    assert(client);

    zero((int8 *)client, sizeof(struct s_client));
    client->s = s2;
    client->port = port;
    strncpy(client->ip, ip, 15);

    // create thread for client,,,
    //fork the current process,,,for windows it is called -> CreateThread (Windows style concurrency).
    HANDLE hThread = CreateThread(NULL, 0, childloop, client, 0, NULL);
    if (hThread == NULL) {
        printf("Failed to create thread: %lu\n", (unsigned long)GetLastError());
        closesocket(s2);
        free(client);
    } else {
        CloseHandle(hThread); // let thread run independently
    }
}


void initServer(int16 port) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in sock;


    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("accept() failed: %d\n", (int)WSAGetLastError());
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

     

        //closing the socket
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



//Right now your server still behaves like a single-client logger.



//     CONNECTION FROM THE CLIENT
//using Windows function
/*TO connect tot the Given server: PS C:\Desktop\Database-in-C\cache22> .\cache22.exe 8080
                                   Server listening on 127.0.0.1:8080...
use command in Different Terminal -->  PS C:\Desktop\Database-in-C\cache22> $client = New-Object System.Net.Sockets.TcpClient("127.0.0.1",8080)


To get :
PS C:\Desktop\Database-in-C\cache22> .\cache22.exe 8080
Server listening on 127.0.0.1:8080...
Connection from 127.0.0.1:50118    this is a ---->Client port
*/



/**********   using Just tellnet  -- >    telnet 127.0.0.1 8080      *************************/
