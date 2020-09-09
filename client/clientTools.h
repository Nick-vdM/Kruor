//
// Created by nicol on 7/09/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#ifndef KRUOR_CLIENTTOOLS_H
#define KRUOR_CLIENTTOOLS_H

#include <stdlib.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>

#define PORT 12056
#define SA struct sockaddr
#define HOST_NAME "localhost"
#define BUFFER_MAX 1024
#define HOSTNAME_MAX 256
#define MAX_PATH_LENGTH 260
#define TRUE 1
#define FALSE 0

void putCommand(char *command, int commandSize, int socketFD);

void getCommand(char *command, int commandSize, int socketFD);

void runCommand(char *command, int commandSize, int socketFD);

void listCommand(char *command, int commandSize, int socketFD);

void sysCommand(char *command, int commandSize, int socketFD);

#endif //KRUOR_CLIENTTOOLS_H
