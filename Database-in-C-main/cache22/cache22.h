#define _GNU_SOURCE
#ifndef CACHE22
#define CACHE22
 

#include <stdio.h>
// #include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
// #include <signal.h>    shutdown my pressing ctrl + c 
#include <conio.h> // for _kbhit() and _getch()  -- > shutdown by pressing a char



#include <winsock2.h>  /*for linus :#include <sys/socket.h>
                            #include <netinet/in.h>
                            #include <arpa/inet.h>*/
#include <ws2tcpip.h>
#include <time.h>   // added for time_t

#define HOST        "127.0.0.1"
#define PORT        "8080"


typedef unsigned int int32;
typedef unsigned short int int16;
typedef unsigned char int8;



//to store values
typedef struct s_hash_table HashTable; // forward declaration

typedef struct s_client {
    int s;
    int16 port;
    char ip[16];
    // ...other fields...
    HashTable *cache; // <-- per-client cache
} s_client;

typedef s_client Client;

typedef int32 (*Callback)(Client*, int8*, int8*);//for cmd , folder , args
typedef struct s_cmdhandler {
    int8 *cmd;
    Callback func;
} CmdHandler;

void zero(int8 *,int16);
DWORD WINAPI childloop(LPVOID param);
void mainloop(SOCKET s2, struct sockaddr_in cli);
void initServer(int16);
int main(int argc, char **argv);

// Hash table entry structure
typedef struct s_hash_entry {
    char *key;
    char *value;
    time_t expire;               // <-- added: expiry timestamp (0 = never)
    struct s_hash_entry *next;
} HashEntry;

// Hash table structure
typedef struct s_hash_table {
    HashEntry **table;
    int size;
} s_hash_table;

#endif