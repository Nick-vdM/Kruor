//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

/**
 * Code that the server uses to complete commands
 */

#include "serverTools.h"
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/dirent.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#define PROGRAM_DIRECTORY "/programs/"
#define TRUE 1
#define FALSE 0
#define MAX_PATH_LENGTH 260 // Windows only allows this much in a file name

void getTime(char *buffer) {
    time_t currentTime = time(NULL);
    strcpy(buffer, ctime(&currentTime));
}

void getHost(char *buffer) {
    // Additional information at https://www.geeksforgeeks.org/c-program-display-hostname-ip-address/
    gethostname(buffer, sizeof(buffer));
}

void getType(char *buffer) {
    // More information at https://stackoverflow.com/questions/15580179/how-do-i-find-the-name-of-an-operating-system
#ifdef _WIN32
    strcpy(buffer, "Windows 32-bit");
#elif _WIN64
    strcpy(buffer, "Windows 64-bit");
#elif __APPLE__ || __MACH__
    strcpy(buffer, "Mac OSX");
#elif __linux__
    strcpy(buffer, "Linux");
#elif __FreeBSD__
    strcpy(buffer, "FreeBSD");
#elif __unix || __unix__
    strcpy(buffer, "Unix");
#else
    strcpy(buffer, "Other");
#endif
}


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
 * Fills the fullDirectory with the working directory, the program directory,
 * the directory its being downloaded into and the file name directory
 * @param fullDirectory to fill
 * @param directory the progname in the programs
 * @param fileName the name of the file being opened
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
int generateFullDirectoryOfFile(char *fullDirectory, char *directory,
                                char *fileName) {
    if (getcwd(fullDirectory, sizeof(char) * MAX_PATH_LENGTH) == NULL) {
        fprintf(stderr, "Failed to get current working directory %i\n");
        return EXIT_FAILURE;
    }
    strcat(fullDirectory, PROGRAM_DIRECTORY);
    strcat(fullDirectory, "/");
    strcat(fullDirectory, directory);
    strcat(fullDirectory, "/");
    strcat(fullDirectory, fileName);
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
    generateFullDirectoryOfFile(fullDirectory, directory, fileName);
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

    char miniBuffer[20]; // to hold the size of the file
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
        c = fileText[i];
        fputc(c, filePointer);
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

void processCommand(char *command, int socketFileDescriptor, int clientID) {
    if (strncmp(command, "put", 3) == 0) {
        printf("Downloading...\n");
        downloadProgram(command, socketFileDescriptor);
    } else if ((strncmp(command, "get", 3)) == 0) {
        printf("Incomplete command. Will be completed soon\n");
        strcpy(command, "Incomplete command");
    } else if ((strncmp(command, "type", 3)) == 0) {
        printf("Incomplete command. Will be completed soon\n");
        strcpy(command, "Unknown command");
    } else if ((strncmp(command, "run", 3)) == 0) {
        printf("Incomplete command\n");
        strcpy(command, "Unknown command. Will be completed soon");
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

