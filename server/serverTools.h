//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#ifndef KRUOR_SERVERTOOLS_H
#define KRUOR_SERVERTOOLS_H

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#define BUFF_MAX 1024
#define FALSE 0
#define TRUE 1

void putCommand(char *command, int socketFD);

void getCommand(char *command, int socketFD);

void runCommand(char *command, int socketFD);

void listCommand(char *command, int socketFD);

void sysCommand(int socketFD);

#endif //KRUOR_SERVERTOOLS_H
