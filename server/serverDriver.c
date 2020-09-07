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
#include <limits.h>
#include <errno.h>
#include <sys/dirent.h>
#include <dirent.h>
#include "serverDriver.h"
#include "serverTools.h"

#define MAX_PATH_LENGTH 260 // Windows only allows this much in a file name
#define BUFF_MAX 1024
#define FALSE 0
#define TRUE 1
#define PROGRAM_DIRECTORY "/programs"
#define EXE_NAME "executable.exe"

/**
 * Extracts the dirName from the command and returns where
 * start of the source files is
 * @param command
 * @param dirName
 * @return
 */

int countOccurrences(char const *string, char token) {
    int count = 0;
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == token) count++;
    }
    return count;
}

/**
 * Splits the string into tokens using strtok
 * @param string the command
 * @param toFill data to fill in with the split
 * @param split the token to split by
 * @return Number of words
 */
int split(char *string, char ***toFill, char split) {
    // First count the number of times the character appears and add
    // one for how many words it'll split into
    int words = countOccurrences(string, split) + 1;
    (*toFill) = (char **) malloc(sizeof(char *) * words);
    for (int i = 0; i < words; i++) {
        (*toFill)[i] = (char *) malloc(sizeof(char) * MAX_PATH_LENGTH);
    }
    char *token;

    // The first split needs the string while the
    // rest don't so its formatted like this
    token = strtok(string, &split);
    for (int i = 0; i < words; i++) {
        strcpy((*toFill)[i], token);
        token = strtok(NULL, &split);
        if (token == NULL) break;
    }
    // Make sure our last word has no \n character
    for (int i = 0; i < MAX_PATH_LENGTH; i++) {
        if ((*toFill)[words - 1][i] == '\n') {
            (*toFill)[words - 1][i] = '\0';
        } else if ((*toFill)[words - 1][i] == '\n') {
            break; // there was no newline
        }
    }

    return words;
}

void deleteDirectoryContents(char *path) {
    DIR *directory;
    struct dirent *directoryInfo;
    char tempPath[MAX_PATH_LENGTH];

    if ((directory = opendir(path)) == NULL) {
        fprintf(stderr, "ls: failed to open %s\n", path);
        exit(1);
    }

    while ((directoryInfo = readdir(directory)) != NULL) {
        strcpy(tempPath, path);
        strcat(tempPath, "/");
        strcat(tempPath, directoryInfo->d_name);
        unlink(tempPath); // delete the file
    }
    closedir(directory);
}

void getProgramsDirectory(char *fullDirectory) {
    if (getcwd(fullDirectory, MAX_PATH_LENGTH) == NULL) {
        fprintf(stderr, "Failed to get current working directory\n");
        exit(EXIT_FAILURE);
    }
    strcat(fullDirectory, PROGRAM_DIRECTORY);
}

/**
 * Finds the cwd and appends the folder to it
 * @param fullDirectory
 * @param folder
 */
void getPrognameDirectory(char *fullDirectory, char *folder) {
    getProgramsDirectory(fullDirectory);
    strcat(fullDirectory, "/");
    strcat(fullDirectory, folder);
}

/**
 * Fills the fullDirectory with the working directory, the program directory,
 * the directory its being downloaded into and the file name directory
 * @param fullDirectory to fill
 * @param directory the progname in the programs
 * @param fileName the name of the file being opened
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
int getProgramFileDirectory(char *fullDirectory, char *directory,
                            char *fileName) {
    getPrognameDirectory(fullDirectory, directory);
    strcat(fullDirectory, "/");
    strcat(fullDirectory, fileName);
    return EXIT_SUCCESS;
}

/**
 * Generates a directory in the programs directory if it doesn't exist
 * @param dirName Directory to add to the programs directory
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int makeDirectoryOrDeleteIfForced(char *dirName, int forced) {
    // Change it to be the full directory address on the server
    char fullDirectory[MAX_PATH_LENGTH];
    if (getcwd(fullDirectory, sizeof(fullDirectory)) == NULL) {
        fprintf(stderr, "Failed to get current working directory\n");
        return EXIT_FAILURE;
    }

    strcat(fullDirectory, PROGRAM_DIRECTORY);
    strcat(fullDirectory, dirName);

    // Check whether the directory already exists
    struct stat st = {0};
    if (stat(fullDirectory, &st) != -1) {
        if (forced) {
            deleteDirectoryContents(fullDirectory);
        }
        // it exists so leave
        return EXIT_SUCCESS;
    }
    // Make the directory
    if (mkdir(fullDirectory, 0777) != 0) { // Just allow all permissions
        fprintf(stderr, "Failed to make directory\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


/**
 *  Requests the client to send a file and downloads it over a UDP protocol
 * @param socketFileDescriptor the client's socket
 * @param directory the directory to download it to
 * @param fileName The name of the file to request
 * @param forced Whether overwriting was forced
 * @return
 */
int downloadFile(int socketFileDescriptor, char *directory,
                 char *fileName, int fileNameLen, int forced) {
    char fullDirectory[MAX_PATH_LENGTH];
    getProgramFileDirectory(fullDirectory, directory, fileName);
    if (access(fullDirectory, F_OK) != -1 && !forced) {
        printf("File %s already exists and forced is turned off", fileName);
        return EXIT_SUCCESS;
    }
    // Now open the file
    FILE *filePointer = fopen(fullDirectory, "w");
    if (filePointer == NULL) {
        fprintf(stderr, "Failed to open file %s", fullDirectory);
        return EXIT_FAILURE;
    }

    char miniBuffer[10]; // to hold the size of the file
    // Request that the client sends the file
    write(socketFileDescriptor, fileName, fileNameLen);
    // Receive the size of the file
    read(socketFileDescriptor, miniBuffer, sizeof(miniBuffer));
    int messageSize = atoi(miniBuffer);
    char fileText[messageSize];
    // Receive the file
    read(socketFileDescriptor, fileText, sizeof(fileText));

    // Now write the filetext into the filePointer
    char c = 'a'; // as long as it isn't eof its fine
    for (int i = 0; i < messageSize; i++) {
        fputc(fileText[i], filePointer);
    }
    fclose(filePointer);

    return EXIT_SUCCESS;
}

/**
 * Initialises a directory for the program and downloads all the necessary files
 * @param command
 * @param socketFileDescriptor
 * @return
 */
int downloadProgram(char *command, int socketFileDescriptor) {
    // Split the command into the important information
    char **splitCommand;
    int words = split(command, &splitCommand, ' ');
    char *dirName = splitCommand[1];
    int forced = FALSE;
    if ((strncmp(splitCommand[words - 1], "-f", 2)) == 0) {
        forced = TRUE;
    }

    if (makeDirectoryOrDeleteIfForced(splitCommand[1], forced) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to make directory\n");
        return EXIT_FAILURE;
    }

    // If someone asks for forced one argument is going to be -f so we need to
    // cut that off
    int endOfSourceFiles = forced ? words - 1 : words;
    for (int i = 2; i < endOfSourceFiles; i++) {
        downloadFile(socketFileDescriptor, dirName,
                     splitCommand[i], MAX_PATH_LENGTH, forced);
    }
    // Tell the client that the server is done downloading
    char done[] = "done";
    write(socketFileDescriptor, done, sizeof(done));
    return EXIT_SUCCESS;
}

int measureFile(FILE *filePointer) {
    // Counts the number of letters in a file
    fseek(filePointer, 0L, SEEK_END);
    int length = ftell(filePointer); // include the end of file
    fseek(filePointer, 0L, SEEK_SET);
    rewind(filePointer);
    return length;
}

void loadInFile(char *toFill, FILE *filePointer) {
    char loadIn;
    for (int i = 0; i < INT_MAX; i++) {
        loadIn = fgetc(filePointer);
        if (loadIn == EOF) break; // done
        toFill[i] = loadIn;
    }
}

void sendFile(FILE *fp, int socketFD) {
    int fileSize = measureFile(fp);
    char fileSizeString[10];
    sprintf(fileSizeString, "%d", fileSize);
    printf("Sending file size\n");
    write(socketFD, fileSizeString, sizeof(fileSizeString));
    // Now we send the file
    char file[fileSize];
    loadInFile(file, fp);
    write(socketFD, file, sizeof(file));
}

int getCommand(char *command, int socketFD) {
    // Open the file they requested
    char **splitCommand;
    split(command, &splitCommand, ' ');
    char *programName = splitCommand[1];
    char *sourcefile = splitCommand[2];
    char fullDirectory[MAX_PATH_LENGTH];
    getProgramFileDirectory(fullDirectory, programName, sourcefile);
    FILE *toSend = fopen(fullDirectory, "r");
    sendFile(toSend, socketFD);
    return EXIT_SUCCESS; // and our job is done
}


void extractRunCommand(char *extractInto, char **splitCommand,
                       int endOfSplitCommand) {
    bzero(extractInto, BUFF_MAX);
    for (int i = 1; i < endOfSplitCommand; i++) {
        strcat(extractInto, splitCommand[i]);
    }
}

/**
 * If we use make to handle this, it should fully handle all of the compilation
 * things for us. Since make manages our time based constraint of the executable
 * should only be compiled if there's no change in other files it works fine.
 * @param directory
 */
void compileFile(char *directory) {
    // In our /programs/ directory there's a generalised makefile
    char progNameDir[MAX_PATH_LENGTH];
    getPrognameDirectory(progNameDir, directory);

    char makeCommand[MAX_PATH_LENGTH] = "cd \"";
    strcat(makeCommand, progNameDir);
    strcat(makeCommand, "\" && make -f ../makefile");

    FILE *pipePointer = popen(makeCommand, "r");
    if (pipePointer == NULL) {
        perror("Failed to open process");
        exit(EXIT_FAILURE);
    }
    // We don't really need to know anything granted it runs
    pclose(pipePointer);
}

int loadInPipe(char **loadInto, FILE *pipePointer) {
    int dynamicSize = 1;
    int currentIndex = 0;
    (*loadInto) = (char *) malloc(dynamicSize);

    char loadIn;
    while ((loadIn = fgetc(pipePointer)) != EOF) {
        if (currentIndex >= dynamicSize) {
            // must expand before inserting. We'll double the size
            // every time because then it has a lower complexity
            dynamicSize *= 2;
            (*loadInto) = (char *) realloc((*loadInto), dynamicSize);
        }
        (*loadInto)[currentIndex] = loadIn;
        currentIndex++;
    }
    // Now lets reduce it back down to the actual currentIndex
    (*loadInto) = (char *) realloc((*loadInto), ((currentIndex) * sizeof(char)));
    return currentIndex;
}

void sendPipe(FILE *pipePointer, int socketFD) {
    // Due to how a pipe cannot use fseek and can only be read once
    // its quite difficult to get the size of it then send it as if
    // its a normal file. So we're going to make a vector<> structure
    // and fill that instead
    char *pipeText;
    int pipeSize = loadInPipe(&pipeText, pipePointer);
    char fileSizeString[10];
    sprintf(fileSizeString, "%d", pipeSize);
    write(socketFD, fileSizeString, sizeof(fileSizeString));
    write(socketFD, pipeText, pipeSize);
    free(pipeText);
}

void runFileAndSendOutput(int socketFD, char *progName) {
    char executableLocation[MAX_PATH_LENGTH] = "\""; // make sure its surrounded
    // with quotations
    char progNameLocation[MAX_PATH_LENGTH];
    getPrognameDirectory(progNameLocation, progName);
    strcat(executableLocation, progNameLocation);
    strcat(executableLocation, "/");
    strcat(executableLocation, EXE_NAME);
    strcat(executableLocation, "\"");
    // Now we just popen this and send the output to the client
    printf("Process opening %s\n", executableLocation);
    FILE *pipePointer = popen(executableLocation, "r");
    sendPipe(pipePointer, socketFD);
    pclose(pipePointer);
}

void runCommand(char *command, int socketFD) {
    char **splitCommand;
    int words = split(command, &splitCommand, ' ');
    // Printing to screen vs local file doesn't have to be handled
    // on this side - we only need to worry about the arguments. However,
    // we should look at whether the localfile is included in case
    int endOfCommand; // A minor issue is that <run progname> would be too short
    // so we check if words - 2 is valid
    if (words - 2 >= 0 && (strncmp(splitCommand[words - 2], "-f", 2)) == 0) {
        endOfCommand = words - 2;
    } else endOfCommand = words;

    // Now that we have that we need to extract the actual run command
    char extractInto[BUFF_MAX];
    extractRunCommand(extractInto, splitCommand, endOfCommand);
    compileFile(splitCommand[1]);
    runFileAndSendOutput(socketFD, extractInto);
}


void processCommand(char *command, int socketFileDescriptor, int clientID) {
    if (strncmp(command, "put", 3) == 0) {
        printf("[System] Downloading file(s) from %d\n", clientID);
        downloadProgram(command, socketFileDescriptor);
    } else if ((strncmp(command, "get", 3)) == 0) {
        printf("[System] Sending file to %d\n", clientID);
        getCommand(command, socketFileDescriptor);
    } else if ((strncmp(command, "run", 3)) == 0) {
        printf("[System] Running program...\n");
        runCommand(command, socketFileDescriptor);
    } else if ((strncmp(command, "list", 4)) == 0) {
        printf("Incomplete command. Will be completed soon\n");
        strcpy(command, "Unknown command");
    } else {
        printf("Unknown command\n");
        strcpy(command, "Incomplete command");
    }
    printf("Finished processing command\n");
}

/**
 * Branches off and handles the socket
 * @param socketFileDescriptor
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
void handleClient(int socketFileDescriptor, unsigned int clientID) {
    char buffer[BUFF_MAX];
    int messageSize;
    while (1) {
        bzero(buffer, sizeof(buffer));
        messageSize = read(socketFileDescriptor, buffer,
                           sizeof(buffer));
        if (messageSize < 1) {
            break; // they disconnected
        }
//        buffer[messageSize] = '\0'; // Ensure there's a \0
        printf("[User %d]: %s", clientID, buffer, messageSize);
        processCommand(buffer, socketFileDescriptor, clientID);

        // Lab processing for testing
//        processLabCommand(buffer);
//        write(socketFileDescriptor, buffer, sizeof(buffer));
    }
    printf("[Client %d disconnected]\n", clientID);
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
 * Calls the socket() function with a safety check
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
 * Fills in all the parameters of a saddr for you
 * @param saddr
 * @param portNumber
 */
void specifySockAddress_in(struct sockaddr_in *saddr, int portNumber) {
    bzero(saddr, sizeof(*saddr));
    saddr->sin_port = htons(portNumber);
    saddr->sin_family = AF_INET;
    saddr->sin_addr.s_addr = htonl(INADDR_ANY);
}

/**
 * Runs bind with a safety check
 * @param socketFD
 * @param saddr
 */
void bindSocketAddress_inToSocket(const int *socketFD,
                                  const struct sockaddr_in *saddr) {
    if (0 != (bind(*socketFD, (SA *) saddr, sizeof(*saddr)))) {
        fprintf(stderr, "Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Runs listen with a safety check
 * @param socketFD
 * @param backlog
 */
void listenToSocket(int *socketFD, int const backlog) {
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

/**
 * A small lab command runner. This is to be removed at the end of testing
 * @param buffer
 */
void processLabCommand(char *buffer) {
    /// Buffer is filled with command sent
    if ((strncmp(buffer, "time", 4)) == 0) {
        getTime(buffer);
        printf("Request complete\n");
    } else if ((strncmp(buffer, "host", 4)) == 0) {
        getHost(buffer);
        printf("Request complete\n");
    } else if ((strncmp(buffer, "type", 4)) == 0) {
        getType(buffer);
        printf("Request complete\n");
    } else {
        printf("Unknown command\n");
        strcpy(buffer, "Unknown command");
    }
}

