//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PORT 12056
#define SA struct sockaddr
#define HOST_NAME "localhost"
#define BUFFER_MAX 1024

void chat(int socket){
    char buffer[BUFFER_MAX];
    int bufferIndex = 0;
    while((strncmp(buffer, "exit", 4)) != 0){
        bzero(buffer, sizeof(buffer));
        bufferIndex = 0;

        printf("Send >");
        while((buffer[bufferIndex++] = getchar()) != '\n');

        write(socket, buffer, sizeof(buffer));
        // now get ready to recieve
        bzero(buffer, sizeof(buffer));
        read(socket, buffer, sizeof(buffer));
        printf("[Server]: %s\n", buffer);
    }
    printf("Closing connection...\n");
}

int main(int argc, char * argv[]){
    struct sockaddr_in server;
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent * hp = gethostbyname(HOST_NAME);

    bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    connect(sd, (SA*) & server, sizeof(server));

    chat(sd);
    close(sd);
}
