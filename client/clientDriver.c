//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

#include "clientTools.h"
#include <netdb.h>
#include <unistd.h>
#include <wait.h>


/**
 * Manages sending commands to the server
 * @param command the command sent
 * @param commandSize
 * @param socketFD The server's socket to interact through
 */
void handleCommand(char *command, int commandSize, int socketFD) {
    if (strncmp(command, "put", 3) == 0) {
        putCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "get", 3) == 0) {
        getCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "run", 3) == 0) {
        runCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "list", 4) == 0) {
        listCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "sys", 3) == 0) {
        sysCommand(command, commandSize, socketFD);
    } else {
        printf("[Client]: Unknown command\n");
    }
    exit(EXIT_SUCCESS);
}

/**
 * Handles all interaction with the server
 * @param socketFD
 */
void chatWithServer(int socketFD) {
    printf(">");
    fflush(stdout);
    char buffer[BUFFER_MAX];
    while (TRUE) {
        bzero(buffer, sizeof(buffer));
        int bufferIndex = 0;

        while ((buffer[bufferIndex++] = getchar()) != '\n');
        if ((strncmp(buffer, "quit", 4)) == 0) break;

        int pid = fork();
        int status;
        if(pid == 0){
            handleCommand(buffer, sizeof(buffer), socketFD);
        }
        waitpid(pid, &status, 0);
        printf(">");
        fflush(stdout);
    }
    printf("Closing connection...\n");
}

/**
 * Defines a TCP socket connection to the server with error checking
 * @param host the IP address to connect to
 * @param portNumber the port to connect to
 * @return the socket created
 */
int defineSocketToServer(char *host, int portNumber) {
    struct sockaddr_in saddr;
    struct hostent *hp;

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD == -1) {
        perror("Failed to define socket: ");
        exit(EXIT_FAILURE);
    }

    bzero(&saddr, sizeof(saddr));
    hp = gethostbyname(host);
    if (hp == NULL) {
        perror("Failed to find host by name: ");
        exit(EXIT_FAILURE);
    }

    bcopy(hp->h_addr, &saddr.sin_addr, hp->h_length);
    saddr.sin_port = htons(portNumber);
    saddr.sin_family = AF_INET;

    if (0 != connect(socketFD, (SA *) &saddr, sizeof(saddr))) {
        perror("Failed to connect to server: ");
        exit(EXIT_FAILURE);
    }
    return socketFD;
}

int main(int argc, char *argv[]) {
    // Check whether an ip and port was passed - if they were then
    // use those instead of the globals
    char hostName[HOSTNAME_MAX] = HOST_NAME;
    int port = PORT;
    if (argc > 2) {
        strcpy(hostName, argv[1]);
        port = atoi(argv[2]);
    } else {
        printf("Protip: You can pass the ip port when running the client! "
               "./client <IP> <PORT>\n");
    }

    int socketFD = defineSocketToServer(hostName, port);
    printf("Connected to server at %s:%i!\n", hostName, port);
    chatWithServer(socketFD);
    return EXIT_SUCCESS;
}
