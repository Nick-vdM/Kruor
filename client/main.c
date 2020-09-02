//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com


#include "clientDriver.h"

int main(int argc, char * argv[]){
    // Check whether an ip and port was passed - if they were then
    // use those instead of the globals
    char hostName[HOSTNAME_MAX] = HOST_NAME;
    int port = PORT;
    if(argc > 2){
        strcpy(hostName, argv[1]);
        port = atoi(argv[2]);
    } else{
        printf("Protip: You can pass the ip port when running the client! "
               "./client <IP> <PORT>\n");
    }

    int socketFD = defineSocketToServer(hostName, port);
    printf("Connected to server!\n");
    chatWithServer(socketFD);
    return EXIT_SUCCESS;
}
