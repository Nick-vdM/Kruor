//
// Created by nicolaas van der Merwe on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com
// s5151332 - nick.vandermerwe@griffithuni.edu.au
//

#ifndef KRUOR_CLIENTDRIVER_H
#define KRUOR_CLIENTDRIVER_H

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT 12056
#define SA struct sockaddr
#define HOST_NAME "localhost"
#define BUFFER_MAX 1024
#define HOSTNAME_MAX 256


void chatWithServer(int socketFD);

int defineSocketToServer(char * host, int portNumber);

#endif //KRUOR_CLIENTDRIVER_H
