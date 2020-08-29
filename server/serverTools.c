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

