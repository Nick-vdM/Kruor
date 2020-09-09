//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

#include "serverTools.h"
#include <signal.h>
#include <wait.h>
#include <cygwin/in.h>

#define SA struct sockaddr
#define PORT 12056
#define BACKLOG_SIZE 5

void processCommand(char *command, int socketFileDescriptor, int clientID) {
    pid_t processID = fork(); // requirement: fork on each command. However,
    // since we aren't forking on the client side it can actually only run one
    // at a time
    if (processID < 0) {
        fprintf(stderr, "Failed to fork from process\n");
    }
    if (processID != 0) {
        // Parent process waits for it to finish then goes back
        int status;
        waitpid(processID, &status, 0);
        printf("Finished processing command\n");
        return;
    }
    // Child process just goes on

    if (strncmp(command, "put", 3) == 0) {
        printf("[System] Downloading file(s) from %d\n", clientID);
        putCommand(command, socketFileDescriptor);
    } else if ((strncmp(command, "get", 3)) == 0) {
        printf("[System] Sending file to %d\n", clientID);
        getCommand(command, socketFileDescriptor);
    } else if ((strncmp(command, "run", 3)) == 0) {
        printf("[System] Running program...\n");
        runCommand(command, socketFileDescriptor);
    } else if ((strncmp(command, "list", 4)) == 0) {
        listCommand(command, socketFileDescriptor);
        strcpy(command, "[System] Running list");
    } else if ((strncmp(command, "sys", 3)) == 0) {
        sysCommand(socketFileDescriptor);
        strcpy(command, "[System] Getting CPU information");
    } else {
        printf("Unknown command\n");
        strcpy(command, "Unknown command");
    }
    exit(EXIT_SUCCESS); // kill child process
}

/**
 * Branches off and handles the socket
 * @param socketFileDescriptor
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
void handleClient(int socketFileDescriptor, unsigned int clientID) {
    char buffer[BUFF_MAX];
    int messageSize;
    while (TRUE) {
        bzero(buffer, sizeof(buffer));
        messageSize = read(socketFileDescriptor, buffer,
                           sizeof(buffer));
        if (messageSize < 1) {
            break; // they disconnected
        }
        printf("[User %d]: %s", clientID, buffer);
        processCommand(buffer, socketFileDescriptor, clientID);
    }
    printf("[Client %d disconnected]\n", clientID);
}

/**
 * Main client handler. Accepts clients and splits them into handleClient() by
 * using fork()
 * @param socketFD
 */
void host(int socketFD) {
    // Forks start here so watch zombies
    int newSocketFD;
    struct sockaddr_in clientAddress;
    pid_t processID;
    int clientAddressLen = sizeof(clientAddress);
    unsigned int lastUser = 0;
    while (TRUE) {
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

// ============================== Socket creation ==============================
int defineTCPSocket(int *socketFD) {
    *socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (*socketFD == -1) {
        fprintf(stderr, "Failed to define socket\n");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

void specifySockAddress_in(struct sockaddr_in *const socketAddress,
                           int portNumber) {
    bzero(socketAddress, sizeof(*socketAddress));
    socketAddress->sin_port = htons(portNumber);
    socketAddress->sin_family = AF_INET;
    socketAddress->sin_addr.s_addr = htonl(INADDR_ANY);
}

/**
 * Runs bind with a safety check
 * @param socketFD
 * @param socketAddress
 */
void bindSocketAddress_inToSocket(const int *socketFD,
                                  const struct sockaddr_in *socketAddress) {
    if (0 != (bind(*socketFD, (SA *) socketAddress, sizeof(*socketAddress)))) {
        fprintf(stderr, "Failed to bind socket\n");
        exit(EXIT_FAILURE);
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
    struct sockaddr_in saddr;
    int socketFD;

    defineTCPSocket(&socketFD);
    specifySockAddress_in(&saddr, portNumber);
    bindSocketAddress_inToSocket(&socketFD, &saddr);
    listenToSocket(&socketFD, backlog);

    return socketFD;
}
// =============================================================================


void killZombieProcess(int signal) {
    int processStatus;
    if (waitpid(-1, &processStatus, WNOHANG) < 0) {
        printf("[[[Executed a zombie]]]\n");
    }
}

void welcomeMessage() {
    printf("///////////////////////////////////////////////////////////////\n"
           "///////////██╗░░██╗██████╗░██╗░░░██╗░█████╗░██████╗░///////////\n"
           "///////////██║░██╔╝██╔══██╗██║░░░██║██╔══██╗██╔══██╗///////////\n"
           "///////////█████═╝░██████╔╝██║░░░██║██║░░██║██████╔╝///////////\n"
           "///////////██╔═██╗░██╔══██╗██║░░░██║██║░░██║██╔══██╗///////////\n"
           "///////////██║░╚██╗██║░░██║╚██████╔╝╚█████╔╝██║░░██║///////////\n"
           "///////////╚═╝░░╚═╝╚═╝░░╚═╝░╚═════╝░░╚════╝░╚═╝░░╚═╝///////////\n"
           "///////////////////////////////////////////////////////////////\n"
           "========== Made by Nick van der Merwe for 2803ICT   ===========\n"
           "========== Server side ------- Connect with client  ===========\n"
           "///////////////////////////////////////////////////////////////\n\n");
}

int main(int argc, char *argv[]) {
    welcomeMessage();
    signal(SIGCHLD, killZombieProcess); // make sure zombies are killed

    int port = PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        printf("Protip! You can specify the port to host the server on "
               "using ./server <PORT>\n");
    }
    int socketFileDescriptor = openTCPSocket(port, BACKLOG_SIZE);
    printf("Hosting server on port %i\n", PORT);

    host(socketFileDescriptor);
}
