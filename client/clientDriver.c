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
#include <termios.h>

#define MAX_PATH_LENGTH 260
#define TRUE 1
#define FALSE 0
#define STDIN 0

// Static makes only this file able to edit it; act
// as if this is a class. Since the client is only
// connecting to one server this is find

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
    rewind(filePointer);
    return length;
}

int transferRequestedFile(char *fileRequest, int socketFD) {
    // First, open a file
    printf("Opening %s\n", fileRequest);
    FILE *filePointer = fopen(fileRequest, "r+");
    if (filePointer == NULL) {
        fprintf(stderr, "Failed to open \'%s\'\n", fileRequest);
        return EXIT_FAILURE;
    }

    int fileSize = measureFile(filePointer);
    char fileSizeString[10];
    sprintf(fileSizeString, "%d\n\0", fileSize);
    write(socketFD, fileSizeString, sizeof(fileSizeString));
    char fileText[fileSize];
    filePointer = fopen(fileRequest, "r+");
    loadInFile(fileText, filePointer);
    // now send that over to the server
    write(socketFD, fileText, fileSize);
    fclose(filePointer);
}

int putCommand(char *command, int commandSize, int socketFD) {
    // Since this requires minimal chatter with the client, we can fork straight
    // away. Then we'll make a new socket and transfer with that.
    int pid = fork();
    if (pid < 0) {
        perror("Error on fork : ");
    } else if (pid > 0) {
        return EXIT_SUCCESS; // make the parent leave
    }
    // Now we're in the child domain
    close(socketFD);
    // Make a new socket to talk through
    socketFD = defineSocketToServer(HOST_NAME, PORT);
    // Begin the request for the transfer
    write(socketFD, command, commandSize);
    // Sends the files that the server requests
    char fileRequest[MAX_PATH_LENGTH];
    while (1) {
        bzero(fileRequest, sizeof(fileRequest));
        read(socketFD, fileRequest, sizeof(fileRequest));
        if (strncmp(fileRequest, "done", 4) == 0) {
            break;
        }
        transferRequestedFile(fileRequest, socketFD);
    }
    close(socketFD);
    exit(EXIT_SUCCESS); // Close this fork
}

/**
 * This is surprisingly complicated in a POSIX system. Since its basically the
 * same couple of lines every time I assume this section is going to get
 * marked for plagiarism by TurnItIn
 */
void waitForKeypress() {
    struct termios information; // Information about the terminal
    tcgetattr(0, &information); // Get a link to the terminal
    struct termios copy = information;
    information.c_lflag &= ~ICANON; // Turn off canonical mode; that is, don't
    // don't wait for the newline key to read input
    information.c_cc[VMIN] = 1; // wait for a keystroke
    information.c_cc[VTIME] = 0; // have no timeout
    tcsetattr(0, TCSANOW, &information); // Immediately set the terminal
    // information back to what it was
    getchar();
    printf("\33[2K\r"); // Reset the current line to continue printing
    information = copy; // Reset our terminal to canonical mode
}

void printXLinesAtATime(char *file, int fileSize, int numberOfLines) {
    printf("\n=================================================================\n");
    int index = 0;
    int currentLines = 0;
    int needsCheck = FALSE;
    while (TRUE) {
        if (currentLines % numberOfLines == 0 && needsCheck) {
            printf("Press any key to continue...");
            fflush(stdout);
            waitForKeypress();
            needsCheck = FALSE;
        } else if (index > fileSize) {
            break; // its done
        }
        if (file[index] == '\n') {
            printf("%-4i|", currentLines + 1);
            currentLines++;
            needsCheck = TRUE;
        }
        printf("%c", file[index]);
        index++;
        fflush(stdout);
    }
    printf("\n=================================================================\n");
    // clean
}

/**
 * This command is largely client-side. We request for a file, it sends it,
 * then on this side we manage the printing. Its the main function that cannot
 * be split into its own thing
 * @return
 */
int getCommand(char *command, int commandSize, int socketFD) {
    // Send the command, and the server responds with the size of the file
    char fileSizeBuffer[10];
    write(socketFD, command, commandSize); // Straight away send the request
    read(socketFD, fileSizeBuffer, sizeof(fileSizeBuffer));
    int fileSize = atoi(fileSizeBuffer);
    char file[fileSize];
    // Now we read the file from them
    read(socketFD, file, fileSize);
    printXLinesAtATime(file, fileSize, 40);
    return EXIT_SUCCESS;
}

/**
 * Some commands require special interaction with the server, and some don't
 * require any printing - as such they can fork away from this process and run
 * on their own
 * @return
 */
int handleCommand(char *command, int commandSize, int socketFD) {
    startTimer();
    if (strncmp(command, "put", 3) == 0) {
        putCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "get", 3) == 0) {
        getCommand(command, commandSize, socketFD);
    } else if (strncmp(command, "run", 3) == 0) {
    } else if (strncmp(command, "list", 4) == 0) {

    } else if (strncmp(command, "sys", 3) == 0) {

    } else {
        printf("[Client]: Unknown command\n");
    }
    reportTime();
    return EXIT_SUCCESS;
}

_types_fd_set initialiseFileDescriptors(int socketFD,
                                        _types_fd_set *fileDescriptors,
                                        int maximumFileDescriptor) {
    FD_ZERO(fileDescriptors);
    FD_SET(socketFD, fileDescriptors);
    FD_SET(STDIN, fileDescriptors);
    select(maximumFileDescriptor + 1, fileDescriptors,
           NULL, NULL, NULL);
    return (*fileDescriptors);
}

/**
 * This allows the server to pick between stdin and reading the server's
 * messages from the main socket at free will.
 * @param socketFD
 */
void chatWithServer(int socketFD) {
    printf(">");
    fflush(stdout);
    fd_set fileDescriptors;
    int maximumFileDescriptor = (socketFD > STDIN) ? socketFD : STDIN;
    char buffer[BUFFER_MAX];
    while (TRUE) {
        fileDescriptors = initialiseFileDescriptors(socketFD, &fileDescriptors,
                                                    maximumFileDescriptor);
        if (FD_ISSET(STDIN, &fileDescriptors)) {
            bzero(buffer, sizeof(buffer));
            int bufferIndex = 0;

            while ((buffer[bufferIndex++] = getchar()) != '\n');
            if ((strncmp(buffer, "quit", 4)) == 0) break;

            handleCommand(buffer, sizeof(buffer), socketFD);
            printf(">");
            fflush(stdout);
        }
        if (FD_ISSET(socketFD, &fileDescriptors)) {
            bzero(buffer, sizeof(buffer));
            read(socketFD, buffer, sizeof(buffer));
            // Erase the last line, print the server's message and print a new
            // >. Downside is that this clears what the client is typing
            // TODO: Find a way to not clear their typing
            printf("\33[2K\r[Server]: %s>", buffer);
            fflush(stdout);
        }
    }
    printf("Closing connection...\n");
}

int defineSocketToServer(char *host, int portNumber) {
    struct sockaddr_in saddr;
    struct hostent *hp;

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD == -1) {
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

    if (0 != connect(socketFD, (SA *) &saddr, sizeof(saddr))) {
        printf("Failed to find host by name\n");
        exit(EXIT_FAILURE);
    }
    return socketFD;
}
