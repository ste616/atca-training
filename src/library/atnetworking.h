/**
 * ATCA Training Library: atnetworking.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * Some networking routines and definitions for all the tools.
 */

#pragma once
#include <inttypes.h>
#include <sys/types.h>


#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int

// The types of request that can be made.
#define REQUEST_CURRENT_SPECTRUM     1

// This structure handles request headers.
struct requests {
  // The request type.
  int request_type;
};

// The types of response that can be given.
#define RESPONSE_CURRENT_SPECTRUM    183

// This structure describes response headers.
struct responses {
  // The response type.
  int response_type;
};

ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length);
ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length);
