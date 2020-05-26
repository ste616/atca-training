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

bool prepare_client_connection(char *server_name, int port_number,
                               SOCKET *socket_peer, bool debugging) {
  // Prepare a connection from the client to the server.
  // Returns true if socket is correctly connected, false otherwise.
  struct addrinfo hints, *peer_address;
  char port_string[SOCKBUFSIZE];
  char address_buffer[SOCKBUFSIZE], service_buffer[SOCKBUFSIZE];
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  snprintf(port_string, SOCKBUFSIZE, "%d", port_number);
  if (getaddrinfo(server_name, port_string, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return(false);
  }
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
              address_buffer, sizeof(address_buffer),
              service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
  if (debugging) {
    fprintf(stderr, "Remote address is: %s %s\n", address_buffer, service_buffer);
  }
  *socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                        peer_address->ai_protocol);
  if (!ISVALIDSOCKET(*socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return(false);
  }
  if (debugging) {
    fprintf(stderr, "Connecting...\n");
  }
  if (connect(*socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return(false);
  }
  freeaddrinfo(peer_address);
  if (debugging) {
    fprintf(stderr, "Connected.\n");
  }
  return(true);
}

const char *get_type_string(int type, int id) {
  // Get a string representation of the type of request or response,
  // specified by type=TYPE_REQUEST or TYPE_RESPONSE, and
  // id being one of the definitions in the header.
  int max_request = 3, max_response = 3;
  const char* const request_strings[] = { "",
                                          "REQUEST_CURRENT_SPECTRUM",
                                          "REQUEST_CURRENT_VISDATA"
  };
  const char* const response_strings[] = { "",
                                           "RESPONSE_CURRENT_SPECTRUM",
                                           "RESPONSE_CURRENT_VISDATA"
  };

  if ((type == TYPE_REQUEST) && (id >= 0) && (id < max_request)) {
    return request_strings[id];
  }
  if ((type == TYPE_RESPONSE) && (id >= 0) && (id < max_response)) {
    return response_strings[id];
  }
  return NULL;
  
}
