//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

/**
 * Code that the server uses to complete commands
 */


#include "serverTools.h"
#include <dirent.h>
#include <sys/stat.h>

#define PROGRAM_DIRECTORY "/programs"
#define EXE_NAME "executable.exe"
#define MAX_PATH_LENGTH 260 // Windows only allows this much in a file name

// =================== Split command ===========================================
/**
 * Extracts the dirName from the command and returns where
 * start of the source files is
 * @param command
 * @param dirName
 * @return
 */
int countWords(char const *string) {
    int count = 0;
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == ' ') count++;
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
int splitIntoWords(char *string, char ***toFill) {
    // First count the number of times the character appears and add
    // one for how many words it'll split into
    int words = countWords(string) + 1;
    (*toFill) = (char **) malloc(sizeof(char *) * words);
    for (int i = 0; i < words; i++) {
        (*toFill)[i] = (char *) malloc(sizeof(char) * MAX_PATH_LENGTH);
    }
    char *token;

    // The first split needs the string while the
    // rest don't so its formatted like this
    char stringCopy[BUFF_MAX];
    strcpy(stringCopy, string);
    char *temp = stringCopy; // needs to be a char*, not char[] one two
    int index = 0;
    while ((token = strtok_r(temp, " ", &temp))) {
        strcpy((*toFill)[index++], token);
    }
    // Make sure our last word has no \n character
    for (int i = 0; i < MAX_PATH_LENGTH; i++) {
        if ((*toFill)[words - 1][i] == '\n') {
            (*toFill)[words - 1][i] = '\0';
        } else if ((*toFill)[words - 1][i] == '\0') {
            break; // there was no newline
        }
    }

    return words;
}

// =================== Put command =============================================
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
    strcat(fullDirectory, "/");
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
void putCommand(char *command, int socketFileDescriptor) {
    // Split the command into the important information
    char **splitCommand;
    int words = splitIntoWords(command, &splitCommand);
    char *dirName = splitCommand[1];
    int forced = FALSE;
    if ((strncmp(splitCommand[words - 1], "-f", 2)) == 0) {
        forced = TRUE;
    }

    if (makeDirectoryOrDeleteIfForced(splitCommand[1], forced) != EXIT_SUCCESS) {
        fprintf(stderr, "Failed to make directory\n");
        exit(EXIT_FAILURE);
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

    for (int i = 0; i < words; i++) free(splitCommand[i]);
    free(splitCommand);
}

// =================== Get command =============================================
int measureFileBytes(FILE *filePointer) {
    // Counts the number of letters in a file
    fseek(filePointer, 0L, SEEK_END);
    int length = ftell(filePointer); // include the end of file
    fseek(filePointer, 0L, SEEK_SET);
    rewind(filePointer);
    return length;
}

void loadFileIntoString(char *toFill, FILE *filePointer) {
    char loadIn;
    for (int i = 0; i < INT_MAX; i++) {
        loadIn = fgetc(filePointer);
        if (loadIn == EOF) break; // done
        toFill[i] = loadIn;
    }
}

void sendFile(FILE *fp, int socketFD) {
    int fileSize = measureFileBytes(fp);
    char fileSizeString[10];
    sprintf(fileSizeString, "%d", fileSize);
    printf("Sending file size\n");
    write(socketFD, fileSizeString, sizeof(fileSizeString));
    // Now we send the file
    char file[fileSize];
    loadFileIntoString(file, fp);
    write(socketFD, file, sizeof(file));
}

void getCommand(char *command, int socketFD) {
    // Open the file they requested
    char **splitCommand;
    int words = splitIntoWords(command, &splitCommand);
    char *programName = splitCommand[1];
    char *sourcefile = splitCommand[2];
    char fullDirectory[MAX_PATH_LENGTH];
    getProgramFileDirectory(fullDirectory, programName, sourcefile);
    FILE *toSend = fopen(fullDirectory, "r");
    if (toSend == NULL) {
        perror("File doesn't exist");
        exit(EXIT_FAILURE);
    }
    sendFile(toSend, socketFD);
    for (int i = 0; i < words; i++) free(splitCommand[i]);
    free(splitCommand);
}


// =================== Run command =============================================
/**
 * Tells the client how the popen went
 * @param state
 */
void tellClientPipeState(int socketFD, int state) {
    char buffer[BUFF_MAX];
    sprintf(buffer, "%i", state);
    write(socketFD, buffer, sizeof(buffer));

    // if it was an actual error commit suicide
    if(state != 0) exit(EXIT_FAILURE);
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
 * @return the error code of pclose
 */
int compileFile(char *directory) {
    // In our /programs/ directory there's a generalised makefile
    char progNameDir[MAX_PATH_LENGTH];
    getPrognameDirectory(progNameDir, directory);

    char makeCommand[MAX_PATH_LENGTH] = "cd \"";
    strcat(makeCommand, progNameDir);
    strcat(makeCommand, "\" && make -f \"../.makefile\"");

    FILE *pipePointer = popen(makeCommand, "r");
    if (pipePointer == NULL) {
        perror("Failed to open process");
        exit(EXIT_FAILURE);
    }
    // popens only run if you read it? So empty it out
    while (fgetc(pipePointer) != EOF) {};
    return pclose(pipePointer);
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

int sendBigString(int socket, char *string, int size) {
    char buffer[10];
    sprintf(buffer, "%i", size);
    write(socket, buffer, 10);
    write(socket, string, size);
}

//void sendPipe(FILE *pipePointer, int socketFD) {
//    // Due to how a pipe cannot use fseek and can only be read once
//    // its quite difficult to get the size of it then send it as if
//    // its a normal file. So we're going to make a vector<> structure
//    // and fill that instead
//    char *pipeText;
//    int pipeSize = loadInPipe(&pipeText, pipePointer);
//    char fileSizeString[10];
//    sprintf(fileSizeString, "%d", pipeSize);
//    write(socketFD, fileSizeString, sizeof(fileSizeString));
//    write(socketFD, pipeText, pipeSize);
//    free(pipeText);
//}

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
    char *pipeMessage;
    int size = loadInPipe(&pipeMessage, pipePointer);
    int error = pclose(pipePointer);
    tellClientPipeState(socketFD, error);
    sendBigString(socketFD, pipeMessage, size);
    free(pipeMessage);
}

void runCommand(char *command, int socketFD) {
    char **splitCommand;
    int words = splitIntoWords(command, &splitCommand);
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
    int error = compileFile(splitCommand[1]);
    runFileAndSendOutput(socketFD, extractInto);

    for (int i = 0; i < words; i++) free(splitCommand[i]);
    free(splitCommand);
}

// =================== List command ============================================
void defineListVariables(char *command, int *wasProgNameRequested,
                         int *longList, char *progName) {
    char **splitCommand;
    int words = splitIntoWords(command, &splitCommand);
    if (words == 2) {
        // Either long or progname
        if (strncmp(splitCommand[1], "-l", 2) == 0) {
            *longList = TRUE;
        } else {
            *wasProgNameRequested = TRUE;
            strcpy(progName, splitCommand[1]);
        }
    } else if (words == 3) {
        *wasProgNameRequested = TRUE;
        *longList = TRUE;
        // It could be either the first or second index
        if (strncmp(splitCommand[1], "-l", 2) == 0) {
            strcpy(progName, splitCommand[2]);
        } else {
            strcpy(progName, splitCommand[1]);
        }
    }
}

FILE *openListCommand(const int wasProgNameRequested, const int longList,
                      char *progName) {
    char command[BUFF_MAX] = "cd .";
    strcat(command, PROGRAM_DIRECTORY);
    if (wasProgNameRequested) {
        strcat(command, "/");
        strcat(command, progName);
    }
    strcat(command, " && ls");
    if (longList) {
        strcat(command, " -l");
    }
    printf("List command: %s\n", command);
    FILE *pipePointer = popen(command, "r");
    if (pipePointer == NULL) {
        perror("Failed to use popen : ");
    }
    return pipePointer;
}

void listCommand(char *command, int socketFD) {
    int wasProgNameRequested = FALSE;
    int longList = FALSE;
    char progName[MAX_PATH_LENGTH];
    defineListVariables(command, &wasProgNameRequested, &longList, progName);
    FILE *pipePointer = openListCommand(wasProgNameRequested,
                                        longList, progName);
    char *text;
    int size = loadInPipe(&text, pipePointer);
    int error = pclose(pipePointer);
    tellClientPipeState(socketFD, error);
    sendBigString(socketFD, text, size);
    free(text);
}

// =========================== System command ==================================
/**
 * Finds the version of the system using compilation ifdefs
 * @param toFill
 */
void getSystemType(char *buffer) {
    // More information at https://stackoverflow.com/questions/15580179/how-do-i-find-the-name-of-an-operating-system
#ifdef _WIN32
    strcpy(buffer, "Windows 32-bit");
#elif _WIN64
    strcpy(buffer, "Windows 64-bit");
#elif __APPLE__ || __MACH__
    strcpy(buffer, "Mac OSC");
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
 * Runs uname -a because this can only compile on Unix anyways. Requires free'ing
 * @param toFill
 */
int getSystemVersion(char **toFill) {
    FILE *uname = popen("uname -r", "r");
    loadInPipe(toFill, uname);
    int error = pclose(uname);
}

/**
 *  Handles the sys command
 * @param socketFD
 */
void sysCommand(int socketFD) {
    char *line = NULL;
    size_t length;
    ssize_t read;
    FILE *cpuinfoFile = fopen("/proc/cpuinfo", "r");
    if (cpuinfoFile == NULL) {
        perror("Couldn't open cpuinfo: ");
    }

    while ((read = getline(&line, &length, cpuinfoFile)) != -1) {
        if ((strncmp(line, "model name", 10)) == 0) {
            char buffer[BUFF_MAX];
            getSystemType(buffer);
            strcat(line, "\t Type: ");
            strcat(line, buffer);
            char *kernelVersion;
            int error = getSystemVersion(&kernelVersion);
            tellClientPipeState(socketFD, error);
            strcat(line, " Kernel: ");
            strcat(line, kernelVersion);

            char lineSizeAsString[10];
            sprintf(lineSizeAsString, "%lu", strlen(line));
            int lineSize = strlen(line);
            write(socketFD, lineSizeAsString, sizeof(lineSizeAsString));
            write(socketFD, line, lineSize);
            return;
        }
    }
    printf("Failed to find information\n");

}
// =============================================================================

