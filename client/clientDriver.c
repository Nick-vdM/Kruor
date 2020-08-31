//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

#include <time.h>
#include "clientDriver.h"
#include <stdlib.h>
#include <limits.h>

#define MAX_PATH_LENGTH 260
#define CIPHER_KEY 'D'

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


void loadInFile(char *toFill, FILE *filePointer) {
    char loadIn;
    for (int i = 0; i < INT_MAX; i++) {
        loadIn = fgetc(filePointer);
        if (loadIn == EOF) break; // done
        toFill[i] = loadIn;
    }
}

int measureFile(FILE *filePointer) {
    // Counts the number of letters in a file
    fseek(filePointer, 0, SEEK_END);
    int length = ftell(filePointer); // include the end of file
    fseek(filePointer, 0, SEEK_SET);
    return length;
}

// TODO: Swap this over to a UDP port
int sendFiles() {
    // Sends the files that the server requests
    char fileRequest[MAX_PATH_LENGTH];
    while (1) {
        bzero(fileRequest, sizeof(fileRequest));
        read(SOCKET, fileRequest, sizeof(fileRequest));
        if (strncmp(fileRequest, "done", 4) == 0) {
            break;
        }

        // First, open a file
        printf("Opening %s\n", fileRequest);
        FILE *filePointer = fopen(fileRequest, "r+");
        if (filePointer == NULL) {
            fprintf(stderr, "Failed to open \'%s\'\n", fileRequest);
            return EXIT_FAILURE;
        }

        int fileSize = measureFile(filePointer);
        char fileSizeString[20];
        sprintf(fileSizeString, "%d\n\0", fileSize);
        write(SOCKET, fileSizeString, sizeof(fileSizeString));
        char fileText[fileSize];
        filePointer = fopen(fileRequest, "r+");
        loadInFile(fileText, filePointer);
        // now send that over to the server
        write(SOCKET, fileText, fileSize);
        fclose(filePointer);
    }
}

/**
 * Some commands require special interaction with the server
 * @return
 */
int handleCommand(char *command) {
    printf("Command: %s", command);
    if (strncmp(command, "put", 3) == 0) {
        sendFiles();
    } else {
        printf("[Client]: Unknown command\n");
    }
    return EXIT_SUCCESS;
}

void chatWithServer() {
    char buffer[BUFFER_MAX];
    int bufferIndex;
    while (1) {
        bzero(buffer, sizeof(buffer));
        bufferIndex = 0;

        printf(">");
        // Would prefer to use gets(), however, it doesn't append a new line
        // to the end of the input and the server requires it
        while((buffer[bufferIndex++] = getchar()) != '\n');
        if (strncmp(buffer, "quit", 4) == 0) {
            break;
        }

        write(SOCKET, buffer, sizeof(buffer));
        // now get ready to receive
        startTimer();
        handleCommand(buffer);
//        read(SOCKET, buffer, sizeof(buffer)); // for testing
        reportTime();
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
