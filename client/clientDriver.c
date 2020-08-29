//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

#include <time.h>
#include "clientDriver.h"

// Static makes only this file able to edit it; act
// as if this is a class. Since the client is only
// connecting to one server this is find
static int SOCKET;

// =========================== Timer tools ================================

static struct timespec START_TIME_SPEC, END_TIME_SPEC;

void startTimer() { clock_gettime(CLOCK_MONOTONIC, &START_TIME_SPEC); }

void reportTime() {
    clock_gettime(CLOCK_MONOTONIC, &END_TIME_SPEC);
    // We can reduce this
    double responseTime =
            (double) (END_TIME_SPEC.tv_sec - START_TIME_SPEC.tv_sec) * 1e3 +
            (double) (END_TIME_SPEC.tv_nsec - START_TIME_SPEC.tv_nsec) / 1e6;
    printf("Server took %.3lf milliseconds to respond\n", responseTime);
}

// ==========================================================================

void chatWithServer() {
    char buffer[BUFFER_MAX];
    int bufferIndex = 0;
    while((strncmp(buffer, "exit", 4)) != 0){
        bzero(buffer, sizeof(buffer));
        bufferIndex = 0;

        printf(">");
        while((buffer[bufferIndex++] = getchar()) != '\n');

        write(SOCKET, buffer, sizeof(buffer));
        // now get ready to receive
        bzero(buffer, sizeof(buffer));
        startTimer();
        read(SOCKET, buffer, sizeof(buffer));
        reportTime();

        printf("[Server]: %s\n", buffer);
    }
    printf("Closing connection...\n");
}

void defineSocketToServer(char *host, int portNumber) {
    struct sockaddr_in saddr;
    struct hostent *hp;

    SOCKET = socket(AF_INET, SOCK_STREAM, 0);
    if (SOCKET == -1) {
        printf("Failed to define socket\n");
        exit(EXIT_FAILURE);
    }

    bzero(&saddr, sizeof(saddr));
    hp = gethostbyname(host);
    if (hp == NULL) {
        printf("Failed to find host by name\n");
        exit(EXIT_FAILURE);
    }

    bcopy(hp->h_addr, &saddr.sin_addr, hp->h_length);
    saddr.sin_port = htons(portNumber);
    saddr.sin_family = AF_INET;

    if (0 != connect(SOCKET, (SA *) &saddr, sizeof(saddr))) {
        printf("Failed to find host by name\n");
        exit(EXIT_FAILURE);
    }
}
