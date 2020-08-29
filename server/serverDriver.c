//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

/**
 * Consists of code that the server directly requires to function.
 */

#include <stdlib.h>
#include "serverDriver.h"
#include "serverTools.h"

/**
 * Branches off and handles the socket
 * @param socketFileDescriptor
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
void handleClient(int socketFileDescriptor, unsigned int clientID) {
    char buffer[BUFF_MAX];
    int messageSize;
    while(1){
        bzero(buffer, sizeof(buffer));
        printf("Waiting for message...\n");
        messageSize = recv(socketFileDescriptor, buffer,
                               sizeof(buffer), 0);
        printf("Received message from client %d!\n", clientID);
        buffer[messageSize] = '\0'; // Make sure to end the string

        if((strncmp(buffer, "quit", 4)) == 0){
            break;
        }

        printf("[User %d]: %s", clientID, buffer);
        processLabCommand(buffer);
        write(socketFileDescriptor, buffer, sizeof(buffer));
    }
    // Parent closes and exits for us
}

/**
 * Main client handler. Accepts clients and splits them into handleClient() by
 * using fork()
 * @param socketFD
 */
void host(int socketFD) {
    printf("Hosting server...\n");
    int newSocketFD;
    struct sockaddr_in clientAddress;
    int processID;
    int clientAddressLen = sizeof(clientAddress);
    unsigned int lastUser = 0;
    while (1) {
        newSocketFD = accept(socketFD, (SA *) &clientAddress, &clientAddressLen);
        printf("New client connected! Their ID is %d\n", ++lastUser);
        if (newSocketFD < 0) {
            fprintf(stderr, "Failed to accept client\n");
        }
        // This forks the process away from this function. Notice this is not
        // multiprocessing
        processID = fork();
        if (processID < 0) {
            fprintf(stderr, "Failed to fork from process\n");
        }
        if (processID == 0) {
            // Did it right - Child process comes here and runs the client
            close(socketFD);
            handleClient(newSocketFD, lastUser);
            exit(EXIT_SUCCESS); // close the fork once its done
        } else close(newSocketFD); // Parent process ends here and restarts
    }
}

/**
 * Generates the main server socket for the server through using defining it,
 * using bind and listen. Accept must be used later
 * @param portNumber The port to listen on
 * @param backlog The maximum number of clients waiting
 * @return Socket file descriptor
 */
int getServerSocket(int portNumber, int backlog) {
    struct sockaddr_in saddr;
    int socketFD;

    // Define our socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD == -1) {
        fprintf(stderr, "Failed to define socket\n");
        return -1;
    }
//    printf("Defined socket\n");

    // Specify our socket
    bzero(&saddr, sizeof(saddr));
    saddr.sin_port = htons(portNumber);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Now bind and listen, and check for failures
    if (0 != (bind(socketFD, (SA *) &saddr, sizeof(saddr)))) {
        fprintf(stderr, "Failed to bind socket\n");
        return -1;
    }
//    printf("Bound socket\n");

    if (0 != listen(socketFD, backlog)) {
        fprintf(stderr, "Failed to listen to socket\n");
        return -1;
    }
//    printf("Listening to socket\n");

    return socketFD;
}

/**
 * A small lab command runner. This is to be removed at the end of testing
 * @param buffer
 */
void processLabCommand(char *buffer) {
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

