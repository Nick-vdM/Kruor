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
#include <sys/stat.h>
#include <errno.h>
#include "serverDriver.h"
#include "serverTools.h"

#define TRUE 1

/**
 * Branches off and handles the socket
 * @param socketFileDescriptor
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */

/**
 * Opens a TCP socket on the server to communicate over
 * @param socketFD
 * @return
 */
int defineTCPSocket(int *socketFD) {
    *socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (*socketFD == -1) {
        fprintf(stderr, "Failed to define socket\n");
        exit(EXIT_FAILURE);
    }
//    printf("Defined socket\n");
    return EXIT_SUCCESS;
}

/**
 * Fills in all the parameters of a server address for you
 * @param serverAddress
 * @param portNumber
 */
void specifySockAddress_in(struct sockaddr_in *serverAddress, int portNumber) {
    bzero(serverAddress, sizeof(*serverAddress));
    serverAddress->sin_port = htons(portNumber);
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_addr.s_addr = htonl(INADDR_ANY);
}

/**
 * Runs bind with a safety check
 * @param socketFD
 * @param serverAddress
 */
void bindSocketAddress_inToSocket(const int *socketFD,
                                  const struct sockaddr_in *serverAddress) {
    if (0 != (bind(*socketFD, (SA *) serverAddress, sizeof(*serverAddress)))) {
        fprintf(stderr, "Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * This is a small flag to allow local addresses to get reused. In C a
 * maximum of 2000 socket connections are allowed, so if 2000 users connect
 * then disconnect later it will stop allowing connections. This changes that.
 * @param socketFD
 * @param opt
 */
void setAddressReuse(const int socketFD) {
    int opt = TRUE;
    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                   sizeof(opt)) < 0) {
        perror("Failed to permit the reuse of sockets");
    }
}

/**
 * Runs listen with a safety check
 * @param socketFD
 * @param backlog
 */
void listenToSocket(int const *socketFD, int const backlog) {
    if (0 != listen(*socketFD, backlog)) {
        fprintf(stderr, "Failed to listen to socket\n");
        EXIT_FAILURE;
    }
}

/**
 * Generates the main server socket for the server through using defining it,
 * using bind and listen. Accept must be used later
 * @param portNumber The port to listen on
 * @param backlog The maximum number of clients waiting
 * @return Socket file descriptor
 */
int openTCPSocket(int const portNumber, int const backlog) {
    struct sockaddr_in serverAddress;
    int socketFD;

    defineTCPSocket(&socketFD);
    specifySockAddress_in(&serverAddress, portNumber);
    setAddressReuse(socketFD);
    bindSocketAddress_inToSocket(&socketFD, &serverAddress);
    listenToSocket(&socketFD, backlog);

    printf("TCP Socket is open on port %d\n", portNumber);
    return socketFD;
}

/**
 * Adds a new socket to the list of sockets
 * @param masterSocket
 * @param maximumClients
 * @param clientSockets
 * @return
 */
int addSocket(int masterSocket, int maximumClients,
              int *clientSockets) {
    // When a client requests to start talking
    struct sockaddr_in address;
    int addressLength = sizeof(address);
    int newSocket = accept(masterSocket, (SA *) &address, (socklen_t *) &addressLength);
    if (newSocket < 0) {
        perror("Accept ran into an error: ");
        return EXIT_FAILURE;
    }
    for (int i = 0; i < maximumClients; i++) {
        if (clientSockets[i] == 0) {
            // Just use this index as their identifier.
            printf("[System] User %i connected!\n", i);
            clientSockets[i] = newSocket;
            break;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * Empties and defines the file descriptor set. This is changed every loop.
 * @param masterSocket
 * @param clientSockets
 * @param fileDescriptors
 * @return
 */
int redefineFDSet(const int masterSocket,
                  const int *clientSockets,
                  _types_fd_set *fileDescriptors) {
    int maximumSocket = masterSocket;
    FD_ZERO(fileDescriptors);
    FD_SET(masterSocket, fileDescriptors);
    for (int i = 0; i < 2000; i++) {
        // if the index has a valid socket insert it
        if (clientSockets[i] > 0) {
            FD_SET(clientSockets[i], fileDescriptors);
        }
        // Make sure the maximum is correct
        maximumSocket = clientSockets[i] > maximumSocket ?
                        clientSockets[i] : maximumSocket;
    }
    return maximumSocket;
}


void searchForCommunication(const int maximumClients,
                            int *clientSockets,
                            _types_fd_set *fileDescriptors) {
    for (int i = 0; i < maximumClients; i++) {
        if (FD_ISSET(clientSockets[i], fileDescriptors)) {
            char buffer[BUFFER_MAX];
            int messageSize = read(clientSockets[i], buffer, sizeof(buffer));
            buffer[messageSize] = '\0';
            // Client disconnected
            if (messageSize < 1) {
                printf("[System] Client %i disconnected\n", i);
                close(clientSockets[i]);
                clientSockets[i] = 0;
            } else {
                processCommand(buffer, clientSockets[i], i);
            }
        }
    }
}

_types_fd_set lookForActivity(_types_fd_set *fileDescriptors, int maximumSocket) {
    int activity = select(maximumSocket + 1, fileDescriptors,
                          NULL, NULL, NULL);
    if ((activity < 0) && (errno != EINTR)) {
        perror("Select ran into a problem: ");
    }
    return (*fileDescriptors);
}

_Noreturn void manageConnections(const int masterSocket,
                                 const int maximumClients) {
    // Since we're using a big while loop its best to avoid multiple allocations
    int clientSockets[maximumClients];
    bzero(clientSockets, sizeof(clientSockets));
    fd_set fileDescriptors;
    while (TRUE) {
        int maximumSocket = redefineFDSet(masterSocket, clientSockets, &fileDescriptors);

        // Wait for activity
        fileDescriptors = lookForActivity(&fileDescriptors, maximumSocket);

        // When its the master socket its a connection request
        if (FD_ISSET(masterSocket, &fileDescriptors)) {
            addSocket(masterSocket, maximumClients, clientSockets);
        }

        // Otherwise a client is trying to communicate. Find out which it is,
        // then manage its request
        searchForCommunication(maximumClients, clientSockets, &fileDescriptors);
    }
}

