//
// Created by nicol on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#include "serverDriver.h"

#define BUFF_MAX 1024
#define PORT 12056
#define SA struct sockaddr

int main() {
    int socketFD = getServerSocket(PORT, 1);
    socketFD = accept(socketFD, 0, 0);
    host(socketFD);
    close(socketFD);
}
