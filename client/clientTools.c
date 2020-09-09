//
// Created by nicol on 7/09/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#include "clientTools.h"
#include <unistd.h>
#include <time.h>
#include <limits.h>

// =========================== Timer tools ================================

static struct timespec START_TIME_SPEC, END_TIME_SPEC;

/**
 * Sets the starting time in the timespec variable for reportTime()
 */
void startTimer() { clock_gettime(CLOCK_MONOTONIC, &START_TIME_SPEC); }

/**
 * Reports the time taken since startTimer() in milliseconds
 */
void reportTime(char *task) {
    clock_gettime(CLOCK_MONOTONIC, &END_TIME_SPEC);
    // We can reduce this
    double responseTime =
            (double) (END_TIME_SPEC.tv_sec - START_TIME_SPEC.tv_sec) * 1e3 +
            (double) (END_TIME_SPEC.tv_nsec - START_TIME_SPEC.tv_nsec) / 1e6;
    printf("Server took %.3lf milliseconds to respond to %s\n", responseTime, task);
}


/**
 * Utilises select with read to have a timeout. Also reports how long
 * the system took to respond with the task it did.
 * @param socket
 * @param buffer
 * @param sizeOfBuffer
 * @param task
 */
void timeOutRead(int socket, char *buffer, int sizeOfBuffer, char *task) {
    struct timeval maxTime;
    maxTime.tv_sec = 3;
    maxTime.tv_usec = 0;

    fd_set fileDescriptors;
    FD_ZERO(&fileDescriptors);
    FD_SET(socket, &fileDescriptors);

    startTimer();
    int readValue = select(socket + 1, &fileDescriptors,
                           NULL, NULL, &maxTime);
    if (readValue == -1) perror("Select ran into an error : ");
    if (readValue == 0){
        printf("An error occurred on the server's side doing %s. "
               "Perhaps your syntax is wrong?\n",
               task);
        exit(EXIT_FAILURE);
    }
    // a message came through
    reportTime(task);

    read(socket, buffer, sizeOfBuffer);
}


// ======================= Splitting into words =============================
/**
 * Counts the number of words in a string by searching for spaces - note
 * that whitespace doesn't work
 * @param string string to count through
 * @return number of words separated with space[s]
 */
int countWords(char const *string) {
    int count = 0;
    int counted = FALSE;
    for (int i = 0; string[i] != '\0'; i++) {
        if (string[i] == ' ' && !counted) {
            count++;
            counted = TRUE;
        } else counted = FALSE;
    }
    return count;
}

/**
 * Splits the string into tokens in toFill. Note that toFill is going to
 * need to be freed
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
    char stringCopy[BUFFER_MAX];
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

// ======================== Loading and measuring files ======================

/**
 * Fills the toFill with the text in the filePointer
 * @param toFill
 * @param filePointer
 */
void loadFileIntoString(char *toFill, FILE *filePointer) {
    char loadIn;
    for (int i = 0; i < INT_MAX; i++) {
        loadIn = fgetc(filePointer);
        if (loadIn == EOF) break; // done
        toFill[i] = loadIn;
    }
}

/**
 * Finds how many bytes are in a file; i.e. how big a string has to be
 * @param filePointer
 * @return length in bytes
 */
int measureFileBytes(FILE *filePointer) {
    // Counts the number of letters in a file
    fseek(filePointer, 0, SEEK_END);
    int length = ftell(filePointer); // include the end of file
    fseek(filePointer, 0, SEEK_SET);
    rewind(filePointer);
    return length;
}

// ====================== Put Command =======================================
/**
 * Given a file name it sends that file
 * @param fileRequest
 * @param socketFD
 */
void transferRequestedFile(char *fileRequest, int socketFD) {
    // First, open a file
    printf("Opening %s\n", fileRequest);
    FILE *filePointer = fopen(fileRequest, "r+");
    if (filePointer == NULL) {
        fprintf(stderr, "Failed to open \'%s\'\n", fileRequest);
        exit(EXIT_FAILURE);
    }

    int fileSize = measureFileBytes(filePointer);
    char fileSizeString[10];
    sprintf(fileSizeString, "%d\n\0", fileSize);
    write(socketFD, fileSizeString, sizeof(fileSizeString));
    char fileText[fileSize];
    loadFileIntoString(fileText, filePointer);
    // now send that over to the server
    write(socketFD, fileText, fileSize);
    fclose(filePointer);
}

/**
 * Handles the put command
 * @param command
 * @param commandSize
 * @param socketFD
 */
void putCommand(char *command, int commandSize, int socketFD) {
    // Begin the request for the transfer
    write(socketFD, command, commandSize);
    // Sends the files that the server requests
    char fileRequest[MAX_PATH_LENGTH];
    while (1) {
        bzero(fileRequest, sizeof(fileRequest));
        timeOutRead(socketFD, fileRequest, sizeof(fileRequest), "Downloading file");
        if (strncmp(fileRequest, "done", 4) == 0) {
            break;
        }
        transferRequestedFile(fileRequest, socketFD);
    }
}

// ====================== Printing large strings ============================
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

/**
 * Prints a string in a neat output to the client
 * @param string
 * @param stringSize
 * @param numberOfLines
 */
void printXLinesAtATime(char *string, int stringSize, int numberOfLines) {
    printf("\n=================================================================\n");
    int index = 0;
    int currentLines = 0;
    int needsCheck = FALSE;
    printf(" %-4i| ", currentLines + 1);
    while (TRUE) {
        if (currentLines % numberOfLines == 0 && needsCheck) {
            printf("Press any key to continue...");
            fflush(stdout);
            waitForKeypress();
            printf(" %-4i| ", currentLines + 1); // needs to get printed again
            needsCheck = FALSE;
        } else if (index > stringSize) {
            break; // its done
        }
        printf("%c", string[index]);
        if (string[index] == '\n') {
            printf(" %-4i| ", currentLines + 2);
            currentLines++;
            needsCheck = TRUE;
        }
        index++;
        fflush(stdout);
    }
    printf("\n=================================================================\n");
    // clean
}

// ========================== Get command ==================================
/**
 * This command is largely client-side. We request for a file, it sends it,
 * then on this side we manage the printing. Its the main function that cannot
 * be split into its own thing
 * @return
 */
void getCommand(char *command, int commandSize, int socketFD) {
    // Send the command, and the server responds with the size of the file
    char fileSizeBuffer[10];
    write(socketFD, command, commandSize); // Straight away send the request
    timeOutRead(socketFD, fileSizeBuffer, sizeof(fileSizeBuffer), "asking for file size");
    int fileSize = atoi(fileSizeBuffer);
    char file[fileSize];
    // Now we read the file from them
    timeOutRead(socketFD, file, fileSize, "downloading file");
    printXLinesAtATime(file, fileSize, 40);
}

/**
 * Given a string it saves it in the directory
 * @param fileString
 * @param fileSize
 * @param fileName
 * @return
 */
int saveFile(char *fileString, int fileSize, char *fileName) {
    FILE *filePointer = fopen(fileName, "w");
    if (filePointer == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < fileSize; i++) {
        fputc(fileString[i], filePointer);
    }
    fclose(filePointer);
    return EXIT_SUCCESS;
}

// ========================= Smaller Commands ================================

void checkPipeCommand(int socketFD, char * task){
    char buffer[BUFFER_MAX];
    timeOutRead(socketFD, buffer, sizeof(buffer), task);
    if((strncmp(buffer, "0", 1)) != 0) {
        // something went wrong
        int error = atoi(buffer);
        if(error < 0){
            printf("%s failed : Failed to close pipe %s", task, buffer);
        } else if (error > 0){
            printf("%s failed : Failed to run command %s", task, buffer);
        }
        exit(EXIT_FAILURE);
    }
}

/**
 * Granted the program exists, this runs it
 * @param command
 * @param commandSize
 * @param socketFD
 */
void runCommand(char *command, int commandSize, int socketFD) {
    // Now the question is whether we requested a force
    char **splitCommand;
    int words = splitIntoWords(command, &splitCommand);
    int forced = FALSE;
    if (words - 2 >= 0 && (strncmp(splitCommand[words - 2], "-f", 2)) == 0) {
        forced = TRUE;
    }

    char fileSizeBuffer[10];
    write(socketFD, command, commandSize); // Straight away send the request
    checkPipeCommand(socketFD, "compiling command");
    checkPipeCommand(socketFD, "running command");
    timeOutRead(socketFD, fileSizeBuffer, sizeof(fileSizeBuffer), "asking for size of file");
    int fileSize = atoi(fileSizeBuffer);
    char fileText[fileSize];
    // Now we read the file from them
    timeOutRead(socketFD, fileText, sizeof(fileSizeBuffer), "asking for the file text");
    if (forced) {
        saveFile(fileText, fileSize, splitCommand[words - 1]);
    } else {
        // We can just use the printXLines function from before
        printXLinesAtATime(fileText, fileSize, 40);
    }
}

void listCommand(char *command, int commandSize, int socketFD) {
    // Request the list
    write(socketFD, command, commandSize);
    checkPipeCommand(socketFD, "running list command");
    char fileSizeString[10];
    timeOutRead(socketFD, fileSizeString, sizeof(fileSizeString), "asking for size of file");
    int fileSize = atoi(fileSizeString);
    char stringToPrint[fileSize];
    timeOutRead(socketFD, stringToPrint, fileSize, "downloading file");
    printXLinesAtATime(stringToPrint, fileSize, 40);
}

void sysCommand(char *command, int commandSize, int socketFD) {
    write(socketFD, command, commandSize);
    checkPipeCommand(socketFD, "system command");
    char lineSizeString[10];
    timeOutRead(socketFD, lineSizeString, sizeof(lineSizeString), "asking for the line size");
    int lineSize = atoi(lineSizeString);
    char line[lineSize];
    timeOutRead(socketFD, line, lineSize, "downloaded file");
    printf("[System] %s", line);
}

// ===========================================================================
