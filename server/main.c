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

    // We're going to use 2000 because C only allows 2000 connections at one
    // time
    // https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/maxsoc.htm
    manageConnections(socketFileDescriptor, 2000);
}
