#include "cache22.h"

bool scontinuation; // server continuation
bool ccontinuation;//child continuation


// zero out buffer
void zero(int8 *buf, int16 size) {
    memset(buf, 0, size);
}

struct command_map {
    void (*func)(void);   // function pointer
    const char *name;     // command string
};

// Example function
void func_select(void) {
    printf("func_select called!\n");
}

// Create mapping
struct command_map abr[] = {
    { func_select, "select" },
    { NULL, NULL }   // terminator
};


// worker function for client handling
DWORD WINAPI childloop(LPVOID param) {
    Client *client = (Client *)param;

    const char *msg = "100 connected to cache22 server\r\n";
    send(client->s, msg, (int)strlen(msg), 0);

    char buf[512], cmd[256], folder[256], args[512];
    int n;

    // Set client socket to blocking mode
    u_long mode = 0;
    ioctlsocket(client->s, FIONBIO, &mode);

    //printf("Connection from %s:%d\n", client->ip, client->port);

    char linebuf[512];
int linepos = 0;

while (true) {
    n = recv(client->s, buf, sizeof(buf), 0);
    if (n <= 0) break; // client disconnected or error

    for (int i = 0; i < n; i++) {
        char c = buf[i];

        if (c == '\r' || c == '\n') { // end of line
            linebuf[linepos] = '\0';
            linepos = 0;

            if (strlen(linebuf) == 0) continue; // skip empty lines

            // --- Now parse linebuf like before ---
            zero((int8*)cmd, sizeof(cmd));
            zero((int8*)folder, sizeof(folder));
            zero((int8*)args, sizeof(args));

            char *p = linebuf;
            char *space1 = strchr(p, ' ');
            if (space1) {
                *space1 = '\0';
                strcpy(cmd, p);
                p = space1 + 1;
                while (*p == ' ') p++;

                char *space2 = strchr(p, ' ');
                if (space2) {
                    *space2 = '\0';
                    strcpy(folder, p);
                    p = space2 + 1;
                    while (*p == ' ') p++;

                    strcpy(args, p);
                } else {
                    strcpy(folder, p);
                    args[0] = '\0';
                }
            } else {
                strcpy(cmd, p);
                folder[0] = '\0';
                args[0] = '\0';
            }

            // --- Print ---
            printf("cmd: %s\n", cmd);
            if (folder[0] != '\0') printf("folder: %s\n", folder);
            if (args[0] != '\0') printf("args: %s\n", args);

            // --- If command is quit, close connection ---
                if (strcmp(cmd, "quit") == 0) {
                    printf("Client requested quit, closing connection.\n");//Server Side jayeag ye message
                    closesocket(client->s);
                    free(client);
                    return 0;
                }

                // --- Print nicely in straight columns ---shyd not working
                printf("cmd: %-10s", cmd);
                if (folder[0] != '\0') printf(" folder: %-20s", folder);
                if (args[0] != '\0') printf(" args: %s", args);
                printf("\n");


            // --- Send back to client ---
            char reply[1024];
            int pos = 0;
            pos += snprintf(reply + pos, sizeof(reply) - pos, "cmd: %s\n", cmd);
            if (folder[0] != '\0') pos += snprintf(reply + pos, sizeof(reply) - pos, "folder: %s\n", folder);
            if (args[0] != '\0') pos += snprintf(reply + pos, sizeof(reply) - pos, "args: %s\n", args);
            send(client->s, reply, (int)strlen(reply), 0);

        } else if (linepos < sizeof(linebuf) - 1) {
            linebuf[linepos++] = c; // accumulate character
        }
    }
}


    closesocket(client->s);
    free(client);
    return 0;
}


void mainloop(SOCKET s2, struct sockaddr_in cli) {
    Client *client = (Client *)malloc(sizeof(struct s_client));
    assert(client);

    zero((int8 *)client, sizeof(struct s_client));
    client->s = s2;
    client->port = (int16)ntohs(cli.sin_port);
    strncpy(client->ip, inet_ntoa(cli.sin_addr), 15);
    printf("Connection from %s:%d\n", client->ip, client->port);
    HANDLE hThread = CreateThread(NULL, 0, childloop, client, 0, NULL);
    if (hThread) CloseHandle(hThread);
    else {
        printf("Failed to create thread: %lu\n", (unsigned long)GetLastError());
        closesocket(s2);
        free(client);
    }
}

void initServer(int16 port) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in sock;

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) exit(1);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) exit(1);

    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    sock.sin_addr.s_addr = inet_addr(HOST);

    if (bind(s, (struct sockaddr *)&sock, sizeof(sock)) == SOCKET_ERROR) exit(1);
    if (listen(s, 20) == SOCKET_ERROR) exit(1);

    printf("Server listening on %s:%d...\n", HOST, port);

    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);

    while (scontinuation) {
        if (_kbhit() && _getch() == 'q') {
            printf("\n'q' pressed, shutting down...\n");
            break;
        }

        struct sockaddr_in cli;
        int len = sizeof(cli);
        SOCKET s2 = accept(s, (struct sockaddr *)&cli, &len);
        if (s2 != INVALID_SOCKET) mainloop(s2, cli);
        else Sleep(50);
    }
}

int main(int argc, char *argv[]) {
    const char *sport = (argc < 2) ? PORT : argv[1];
    int16 port = (int16)atoi(sport);
    scontinuation = true;

    initServer(port);

    printf("Shutdown the Server!\n");
    return 0;
}











//to run the server : mingw32-make   , then :  .\cache22




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



























