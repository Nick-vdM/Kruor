//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#ifndef KRUOR_SERVERDRIVER_H
#define KRUOR_SERVERDRIVER_H

#define BUFF_MAX 1024
#define HOSTNAME_MAX 1024
#define SA struct sockaddr

#include <time.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Takes in a buffer with the command and returns the buffer
 * to send back
 * @param buffer
 */
void processLabCommand(char *buffer);

void hostWithSelect(int masterSocketFD, FILE *out);

/**
 * Hosts the main server
 * @param socketFD
 */
void host(int socketFD);

/**
 * Handles the socket, bind and listen parts of a socket
 * @param portNumber the port to set it to
 * @param backlog the maximum number of requests in the queue
 * @return socket file descriptor
 */
int openTCPSocket(int portNumber, int backlog);


#endif //KRUOR_SERVERDRIVER_H
