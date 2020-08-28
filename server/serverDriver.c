//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

/**
 * Consists of code that the server directly requires to function.
 */

#include "serverDriver.h"
#include "serverTools.h"

void host(int socketFD){
    char buffer[BUFF_MAX];
    while ((strncmp(buffer, "exit", 4)) != 0) {
        bzero(buffer, sizeof(buffer));
        int cc = recv(socketFD, buffer, sizeof(buffer), 0);
        buffer[cc] = 0;

        printf("[User]: %s", buffer);
        processCommand(buffer);

        write(socketFD, buffer, sizeof(buffer));
    }
}

int getServerSocket(int portNumber, int backlog){
    struct sockaddr_in saddr;

    // Define our socket
    int socketID = socket(AF_INET, SOCK_STREAM, 0);
    if (socketID == -1) {
        printf("Failed to define socket\n");
        return -1;
    }
    printf("Defined socket\n");

    // Specify our socket
    bzero(&saddr, sizeof(saddr));
    saddr.sin_port = htons(portNumber);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Now bind and listen, and check for failures
    if (0 != (bind(socketID, (SA *) &saddr, sizeof(saddr)))) {
        printf("Failed to bind socket\n");
        return -1;
    }
    printf("Bound socket\n");

    if (0 != listen(socketID, backlog)) {
        printf("Failed to listen to socket\n");
        return -1;
    }
    printf("Listening to socket\n");

    return socketID;
}

void processCommand(char *buffer) {
    /// Buffer is filled with command sent
    if ((strncmp(buffer, "time", 4)) == 0) {
        printf("Grabbing time...\n");
        getTime(buffer);
    } else if ((strncmp(buffer, "host", 4)) == 0) {
        printf("Grabbing hostname...\n");
        getHost(buffer);
    } else if ((strncmp(buffer, "type", 4)) == 0) {
        printf("Grabbing system type...\n");
        getType(buffer);
    } else {
        strcpy(buffer, "Unknown command");
    }
}

