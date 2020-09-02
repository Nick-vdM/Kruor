//
// Created by nicol on 28/08/2020.
// https://www.linkedin.com/in/nick-vd-merwe/
// nick.jvdmerwe@gmail.com

#ifndef KRUOR_SERVERTOOLS_H
#define KRUOR_SERVERTOOLS_H

/**
 * Fills the buffer with the server's current time
 * @param buffer
 */
void getTime(char *buffer);

/**
 * Fills the buffer with the host's computer name
 * @param buffer
 */
void getHost(char *buffer);

/**
 * Fills the buffer with the operating system type
 * @param buffer : to fill
 */
void getType(char *buffer);

void processLabCommand(char * buffer);

#endif //KRUOR_SERVERTOOLS_H
