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


#include <winsock2.h>  /*for linus :#include <sys/socket.h>
                            #include <netinet/in.h>
                            #include <arpa/inet.h>*/
#include <ws2tcpip.h>

#define HOST        "127.0.0.1"
#define PORT        "8080"

typedef unsigned int int32;
typedef unsigned short int int16;
typedef unsigned char int8;


void mainloop(int16);
int main(int , char**);
#endif 