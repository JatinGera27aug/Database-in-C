#define _GNU_SOURCE
#ifndef CACHE22
#define CACHE22
 

#include <stdio.h>
#include <unistd.h>
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

#define HOST        "127.0.0.1"
#define PORT        "8080"


typedef unsigned int int32;
typedef unsigned short int int16;
typedef unsigned char int8;


//to store values
struct s_client{
    int s;

    //212.212.212.212.\0
    char ip[16];
    int16 port;
};
typedef struct s_client Client;

void zero(int8 *,int16);
DWORD WINAPI childloop(LPVOID param);
void mainloop(SOCKET s2, struct sockaddr_in cli);
int initserver(int16);
int main(int argc, char **argv);

#endif 