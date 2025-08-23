#include "cache22.h"
#include "hash_table.h"

// forward declaration of handlers (defined later)
extern CmdHandler handlers[];
extern const size_t num_handlers;

int32 handle_hello(Client*, int8*, int8*);

bool scontinuation; // server continuation
bool ccontinuation; // child continuation


int32 handle_hello(Client *cli, int8 *folder, int8 *args) {
    char reply[512];
    snprintf(reply, sizeof(reply), "hello: %s\n", folder);
    send(cli->s, reply, (int)strlen(reply), 0);
    return 0;
}

Callback getcmd(int8 *cmd) {
    for (size_t i = 0; i < num_handlers; i++) {
        if (strcmp((char*)cmd, (char*)handlers[i].cmd) == 0) {
            return handlers[i].func;
        }
    }
    return NULL;
}

int32 handle_add(Client *cli, int8 *key, int8 *value) {
    if (key == NULL || value == NULL) {
        char reply[256];
        snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }

    // default TTL 120 seconds
    int ttl = 120;

    // value may contain optional " -ttl N" suffix, e.g. "myval -ttl 30" ye example approacj
    char *valstr = strdup((char*)value);
    if (!valstr) {
        char reply[128];
        snprintf(reply, sizeof(reply), "ERR memory\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }

    // look for " -ttl"
    char *p = strstr(valstr, " -ttl");
    if (p) {
        // terminate value part
        *p = '\0';
        char *num = p + 5; // points after "-ttl"
        while (*num == ' ') num++;
        if (*num) {
            int t = atoi(num);
            if (t > 0) ttl = t;
        }
    }

    // trim trailing spaces from valstr
    size_t L = strlen(valstr);
    while (L > 0 && valstr[L-1] == ' ') { valstr[L-1] = '\0'; L--; }

    int rc = insert_key_value(cli->cache, (char*)key, valstr, ttl);
    free(valstr);

    if (rc == 0) {
        char reply[256];
        snprintf(reply, sizeof(reply), "OK\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return 0;
    } else {
        char reply[256];
        snprintf(reply, sizeof(reply), "ERR insert failed\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }
}

int32 handle_serve(Client *cli, int8 *key, int8 *args) {
    if (key == NULL) {
        char reply[256];
        snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }

    // use per-client cache
    char *value = get_key_value(cli->cache, (char*)key);
    if (value == NULL) {
        char reply[256];
        snprintf(reply, sizeof(reply), "ERR key not found\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }

    char reply[512];
    snprintf(reply, sizeof(reply), "%s\r\n", value);
    send(cli->s, reply, (int)strlen(reply), 0);
    return 0;
}

int32 handle_del(Client *cli, int8 *key, int8 *args) {
    if (!cli || !key) {
        char reply[128];
        snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }

    int res = delete_key_value(cli->cache, (char*)key);
    if (res == 1) {
        char reply[64];
        snprintf(reply, sizeof(reply), "OK\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return 0;
    } else if (res == 0) {
        char reply[64];
        snprintf(reply, sizeof(reply), "ERR key not found\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    } else {
        char reply[64];
        snprintf(reply, sizeof(reply), "ERR delete failed\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }
}

int32 handle_flush(Client *cli, int8 *k, int8 *args) {
    if (!cli) return -1;
    // destroy current cache and create a fresh one
    if (cli->cache) destroy_hash_table(cli->cache);
    cli->cache = create_hash_table(1024);
    if (cli->cache == NULL) {
        char reply[128];
        snprintf(reply, sizeof(reply), "ERR could not allocate cache\r\n");
        send(cli->s, reply, (int)strlen(reply), 0);
        return -1;
    }
    char reply[64];
    snprintf(reply, sizeof(reply), "OK\r\n");
    send(cli->s, reply, (int)strlen(reply), 0);
    return 0;
}

// ---- Define handlers here ----
CmdHandler handlers[] = {
    { (int8*)"hello", handle_hello },
    { (int8*)"add",   handle_add },
    { (int8*)"serve", handle_serve },
    { (int8*)"del",   handle_del },
    { (int8*)"flush", handle_flush }
};
const size_t num_handlers = sizeof(handlers) / sizeof(handlers[0]);
// --------------------------------


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
                if (folder[0] != '\0') printf("key: %s\n", folder);
                if (args[0] != '\0') printf("value: %s\n", args);

                // --- If command is quit, close connection ---
                if (strcmp(cmd, "quit") == 0) {
                    printf("Client requested quit, closing connection.\n");
                    break;  // ✅ just break the recv loop
                }

                // --- Try command handler ---
                Callback cb = NULL;
                int8 *arg1 = (int8*)folder;   // first argument to handler (key)
                int8 *arg2 = (int8*)args;     // second argument to handler (value)

                if (strcmp(cmd, "add") == 0) {
                    cb = getcmd((int8*)"add");
                    // arg1 = folder, arg2 = args (value) — leave as-is
                } else if (strcmp(cmd, "serve") == 0) {
                    cb = getcmd((int8*)"serve");
                    arg2 = NULL; // serve takes key only
                } else {
                    cb = getcmd((int8*)cmd);
                }

                if (cb) {
                    cb(client, arg1, arg2);   // call exactly once
                    // pehle do baar OK print ho raha tha
                } else {
                    char reply[256];
                    snprintf(reply, sizeof(reply), "NO handler found for: %s\r\n", cmd);
                    send(client->s, reply, (int)strlen(reply), 0);
                    printf("NO handler found for command: %s\n", cmd);
                    continue;
                }

            } else if (linepos < sizeof(linebuf) - 1) {
                linebuf[linepos++] = c; // accumulate character
            }
        }
    }

    closesocket(client->s);

    // destroy per-client cache
    if (client->cache) destroy_hash_table(client->cache);

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

    // create per-client cache
    client->cache = create_hash_table(1024);
    if (client->cache == NULL) {
        printf("Failed to create client cache\n");
        closesocket(s2);
        free(client);
        return;
    }

    printf("Connection from %s:%d\n", client->ip, client->port);
    HANDLE hThread = CreateThread(NULL, 0, childloop, client, 0, NULL);
    if (hThread) CloseHandle(hThread);
    else {
        printf("Failed to create thread: %lu\n", (unsigned long)GetLastError());
        closesocket(s2);
        // destroy cache on failure
        if (client->cache) destroy_hash_table(client->cache);
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

// #include "cache22.h"
// #include "hash_table.h"

// extern CmdHandler handlers[];
// // at the top define

// int32 handle_hello(Client*,int8*,int8*);

// bool scontinuation; // server continuation
// bool ccontinuation;//child continuation


// int32 handle_hello(Client *cli, int8 *folder, int8 *args) {
//     char reply[512];
//     snprintf(reply, sizeof(reply), "hello: %s\n", folder);
//     send(cli->s, reply, (int)strlen(reply), 0);
//     return 0;
// }

// // Hash table instance
// HashTable *cache_table;

// Callback getcmd(int8 *cmd) {

//     for (int i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++) {
//         if (strcmp((char*)cmd, (char*)handlers[i].cmd) == 0) {
//             return handlers[i].func;
//         }
//     }
//     return NULL;
// }

// int32 handle_add(Client *cli, int8 *key, int8 *value) {
//     if (key == NULL || value == NULL) {
//         char reply[256];
//         snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }

//     insert_key_value(cache_table, (char*)key, (char*)value);

//     char reply[256];
//     snprintf(reply, sizeof(reply), "OK\r\n");
//     send(cli->s, reply, (int)strlen(reply), 0);
//     return 0;
// }

// int32 handle_serve(Client *cli, int8 *key, int8 *args) {
//     if (key == NULL) {
//         char reply[256];
//         snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }

//     char *value = get_key_value(cache_table, (char*)key);
//     if (value == NULL) {
//         char reply[256];
//         snprintf(reply, sizeof(reply), "ERR key not found\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }

//     char reply[512];
//     snprintf(reply, sizeof(reply), "%s\r\n", value);
//     send(cli->s, reply, (int)strlen(reply), 0);
//     return 0;
// }

// int32 handle_del(Client *cli, int8 *key, int8 *args) {
//     if (!cli || !key) {
//         char reply[128];
//         snprintf(reply, sizeof(reply), "ERR invalid command\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }

//     int res = delete_key_value(cli->cache, (char*)key);
//     if (res == 1) {
//         char reply[64];
//         snprintf(reply, sizeof(reply), "OK\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return 0;
//     } else if (res == 0) {
//         char reply[64];
//         snprintf(reply, sizeof(reply), "ERR key not found\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     } else {
//         char reply[64];
//         snprintf(reply, sizeof(reply), "ERR delete failed\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }
// }

// int32 handle_flush(Client *cli, int8 *k, int8 *args) {
//     if (!cli) return -1;
//     // destroy current cache and create a fresh one
//     if (cli->cache) destroy_hash_table(cli->cache);
//     cli->cache = create_hash_table(1024);
//     if (cli->cache == NULL) {
//         char reply[128];
//         snprintf(reply, sizeof(reply), "ERR could not allocate cache\r\n");
//         send(cli->s, reply, (int)strlen(reply), 0);
//         return -1;
//     }
//     char reply[64];
//     snprintf(reply, sizeof(reply), "OK\r\n");
//     send(cli->s, reply, (int)strlen(reply), 0);
//     return 0;
// }

// CmdHandler handlers[] = {
//     { (int8*)"hello", handle_hello },
//     { (int8*)"add", handle_add },
//     { (int8*)"serve", handle_serve },
//     { (int8*)"del", handle_del },
//     { (int8*)"flush", handle_flush }
// };


// // zero out buffer
// void zero(int8 *buf, int16 size) {
//     memset(buf, 0, size);
// }

// struct command_map {
//     void (*func)(void);   // function pointer
//     const char *name;     // command string
// };

// // Example function
// void func_select(void) {
//     printf("func_select called!\n");
// }

// // Create mapping
// struct command_map abr[] = {
//     { func_select, "select" },
//     { NULL, NULL }   // terminator
// };


// // worker function for client handling
// DWORD WINAPI childloop(LPVOID param) {
//     Client *client = (Client *)param;

//     const char *msg = "100 connected to cache22 server\r\n";
//     send(client->s, msg, (int)strlen(msg), 0);

//     char buf[512], cmd[256], folder[256], args[512];
//     int n;

//     // Set client socket to blocking mode
//     u_long mode = 0;
//     ioctlsocket(client->s, FIONBIO, &mode);

//     //printf("Connection from %s:%d\n", client->ip, client->port);

//     char linebuf[512];
//     int linepos = 0;

//     while (true) {
//         n = recv(client->s, buf, sizeof(buf), 0);
//         if (n <= 0) break; // client disconnected or error

//         for (int i = 0; i < n; i++) {
//             char c = buf[i];

//             if (c == '\r' || c == '\n') { // end of line
//                 linebuf[linepos] = '\0';
//                 linepos = 0;

//                 if (strlen(linebuf) == 0) continue; // skip empty lines

//                 // --- Now parse linebuf like before ---
//                 zero((int8*)cmd, sizeof(cmd));
//                 zero((int8*)folder, sizeof(folder));
//                 zero((int8*)args, sizeof(args));

//                 char *p = linebuf;
//                 char *space1 = strchr(p, ' ');
//                 if (space1) {
//                     *space1 = '\0';
//                     strcpy(cmd, p);
//                     p = space1 + 1;
//                     while (*p == ' ') p++;

//                     char *space2 = strchr(p, ' ');
//                     if (space2) {
//                         *space2 = '\0';
//                         strcpy(folder, p);
//                         p = space2 + 1;
//                         while (*p == ' ') p++;

//                         strcpy(args, p);
//                     } else {
//                         strcpy(folder, p);
//                         args[0] = '\0';
//                     }
//                 } else {
//                     strcpy(cmd, p);
//                     folder[0] = '\0';
//                     args[0] = '\0';
//                 }

//                 // --- Print ---
//                 printf("cmd: %s\n", cmd);
//                 if (folder[0] != '\0') printf("key: %s\n", folder);
//                 if (args[0] != '\0') printf("value: %s\n", args);

//                 // --- If command is quit, close connection ---
//                 if (strcmp(cmd, "quit") == 0) {
//                     printf("Client requested quit, closing connection.\n");
//                     break;  // ✅ just break the recv loop
//                 }

//                 // --- Try command handler ---
//                 Callback cb = NULL;
//                 if (strcmp(cmd, "add") == 0) {
//                     cb = getcmd((int8*)"add");
//                     //ADD KEY <key> VALUE <value>
//                     char *key = folder;
//                     char *value = args;
//                     if (cb) {
//                         cb(client, (int8*)key, (int8*)value);
//                     }
//                 } else if (strcmp(cmd, "serve") == 0) {
//                     cb = getcmd((int8*)"serve");
//                     char *key = folder;
//                     if (cb) {
//                         cb(client, (int8*)key, NULL);
//                     }
//                 }
//                 else{
//                     cb = getcmd((int8*)cmd);


//                 }




//                 if (cb) {
//                     cb(client, (int8*)folder, (int8*)args);
//                 }
//                 else {
//                     char reply[256];
//                     snprintf(reply, sizeof(reply), "NO handler found for: %s\r\n", cmd);
//                     send(client->s, reply, (int)strlen(reply), 0);

//                     // Optional: also log it on server side
//                     printf("NO handler found for command: %s\n", cmd);

//                       // ✅ Keep waiting for more commands
//                     continue;
//                 }





//             } else if (linepos < sizeof(linebuf) - 1) {
//                 linebuf[linepos++] = c; // accumulate character
//             }
//         }
//     }


//     closesocket(client->s);
//     free(client);
//     return 0;
// }


// void mainloop(SOCKET s2, struct sockaddr_in cli) {
//     Client *client = (Client *)malloc(sizeof(struct s_client));
//     assert(client);

//     zero((int8 *)client, sizeof(struct s_client));
//     client->s = s2;
//     client->port = (int16)ntohs(cli.sin_port);
//     strncpy(client->ip, inet_ntoa(cli.sin_addr), 15);
//     printf("Connection from %s:%d\n", client->ip, client->port);
//     HANDLE hThread = CreateThread(NULL, 0, childloop, client, 0, NULL);
//     if (hThread) CloseHandle(hThread);
//     else {
//         printf("Failed to create thread: %lu\n", (unsigned long)GetLastError());
//         closesocket(s2);
//         free(client);
//     }
// }

// void initServer(int16 port) {
//     // Initialize hash table
//     cache_table = create_hash_table(1024);
//     if (cache_table == NULL) {
//         printf("Failed to create hash table\n");
//         exit(1);
//     }
//     WSADATA wsa;
//     SOCKET s;
//     struct sockaddr_in sock;

//     if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) exit(1);

//     s = socket(AF_INET, SOCK_STREAM, 0);
//     if (s == INVALID_SOCKET) exit(1);

//     sock.sin_family = AF_INET;
//     sock.sin_port = htons(port);
//     sock.sin_addr.s_addr = inet_addr(HOST);

//     if (bind(s, (struct sockaddr *)&sock, sizeof(sock)) == SOCKET_ERROR) exit(1);
//     if (listen(s, 20) == SOCKET_ERROR) exit(1);

//     printf("Server listening on %s:%d...\n", HOST, port);

//     u_long mode = 1;
//     ioctlsocket(s, FIONBIO, &mode);

//     while (scontinuation) {
//         if (_kbhit() && _getch() == 'q') {
//             printf("\n'q' pressed, shutting down...\n");
//             break;
//         }

//         struct sockaddr_in cli;
//         int len = sizeof(cli);
//         SOCKET s2 = accept(s, (struct sockaddr *)&cli, &len);
//         if (s2 != INVALID_SOCKET) mainloop(s2, cli);
//         else Sleep(50);
//     }
// }

// int main(int argc, char *argv[]) {
//     const char *sport = (argc < 2) ? PORT : argv[1];
//     int16 port = (int16)atoi(sport);
//     scontinuation = true;

//     initServer(port);
    

//     printf("Shutdown the Server!\n");
//     return 0;
// }











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


/*
🔎 What Happened

Server side:

Connection from 127.0.0.1:54717
cmd: hello
folder: world
args: test
cmd: foo
folder: bar


Client side:

100 connected to cache22 server
hello world test
hello: world      ✅ handled by handle_hello()
foo bar
cmd:foo folder: bar args:   (fallback echo)
 */

























