/**
 * ATCA Training Library: atnetworking.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * Some networking routines and definitions for all the tools.
 */

#pragma once
#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int

#define DEBUG_NBYTES 16
#define SOCKBUFSIZE 1024


// The types of request that can be made.
#define REQUEST_CURRENT_SPECTRUM     1
#define REQUEST_CURRENT_VISDATA      2

// This structure handles request headers.
struct requests {
  // The request type.
  int request_type;
};

// The types of response that can be given.
#define RESPONSE_CURRENT_SPECTRUM    1
#define RESPONSE_CURRENT_VISDATA     2

// This structure describes response headers.
struct responses {
  // The response type.
  int response_type;
};

ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length);
ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length);
bool prepare_client_connection(char *server_name, int port_number,
                               SOCKET *socket_peer, bool debugging);
