//
// Created by nicol on 27/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#include <stdlib.h>
#include "serverDriver.h"

#define PORT 12056

int main(int argc, char *argv[]) {
    int port = PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        printf("Protip! You can specify the port to host the server on "
               "using ./server <PORT>\n");
    }
    int socketFileDescriptor = openTCPSocket(port, 5);

    host(socketFileDescriptor);
}
