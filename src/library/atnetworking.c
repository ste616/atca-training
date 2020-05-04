/**
 * ATCA Training Library: atnetworking.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * Some networking routines and definitions for all the tools.
 */
#include "atnetworking.h"
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "memory.h"

#define DEBUG_NBYTES 16

ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length) {
  // Send the buffer with length buffer_length over the socket.
  ssize_t init_bytes_sent, bytes_sent;
  /* int i; */

  // Send the size of the data first.
  //fprintf(stderr, "Sending size of data, %ld bytes\n", buffer_length);
  init_bytes_sent = send(socket, &buffer_length, sizeof(size_t), 0);
  //fprintf(stderr, "  that required us to send %ld bytes\n", init_bytes_sent);
  if (init_bytes_sent < 0) {
    fprintf(stderr, "UNABLE TO SEND ANY DATA!\n");
    return(init_bytes_sent);
  }
  
  // Now send the buffer.
  bytes_sent = send(socket, buffer, buffer_length, 0);
  /* fprintf(stderr, "Sent %ld data bytes\n", bytes_sent); */
  /* fprintf(stderr, "First %d bytes follow:\n", DEBUG_NBYTES); */
  /* for (i = 0; i < ((DEBUG_NBYTES < bytes_sent) ? DEBUG_NBYTES : bytes_sent); i++) { */
  /*   fprintf(stderr, "Byte %d: %d\n", i, buffer[i]); */
  /* } */
  
  return(bytes_sent);
}

ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length) {
  // Read the buffer from the network socket, allocating the necessary space for it.
  ssize_t bytes_to_read;
  ssize_t bytes_read, br;
  /* int i; */
  
  // Read the size of the data first.
  bytes_read = recv(socket, &bytes_to_read, sizeof(ssize_t), 0);
  /* fprintf(stderr, "Read size of data to come, %ld bytes, %ld bytes received\n", */
  /*         bytes_to_read, bytes_read); */
  if (bytes_read < 0) {
    // The socket was closed.
    fprintf(stderr, "Connection closed by peer.\n");
    return(bytes_read);
  }
  // Allocate the necessary memory.
  MALLOC(*buffer, bytes_to_read + 1);
  bytes_read = 0;
  while ((bytes_read < bytes_to_read) &&
         (br = recv(socket, *buffer + bytes_read, bytes_to_read - bytes_read, 0))) {
    bytes_read += br;
    /* fprintf(stderr, " received %ld bytes (%ld total)\n", br, bytes_read); */
  }
  if (bytes_read <= 0) {
    fprintf(stderr, "Connection closed by peer before data reception.\n");
    return(bytes_read);
  }
  /* fprintf(stderr, "Received expected buffer\n"); */
  *buffer_length = (size_t)bytes_read;
  /* fprintf(stderr, "First %d bytes follow:\n", DEBUG_NBYTES); */
  /* for (i = 0; i < ((DEBUG_NBYTES < bytes_read) ? DEBUG_NBYTES : bytes_read); i++) { */
  /*   fprintf(stderr, "Byte %d: %d\n", i, (*buffer)[i]); */
  /* } */
  
  return(bytes_read);
}
