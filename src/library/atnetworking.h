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
#include <compute.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int

#define DEBUG_NBYTES 16
#define SOCKBUFSIZE 1024
#define CLIENTIDLENGTH 20

#define SERVERTYPE_SIMULATOR  1
#define SERVERTYPE_CORRELATOR 2

#define TYPE_REQUEST    1
#define TYPE_RESPONSE   2

// The types of request that can be made.
#define REQUEST_CURRENT_SPECTRUM       1
#define REQUEST_CURRENT_VISDATA        2
#define REQUEST_COMPUTE_VISDATA        3
#define REQUEST_COMPUTED_VISDATA       4
#define CHILDREQUEST_VISDATA_COMPUTED  5
#define REQUEST_SERVERTYPE             6

// This structure handles request headers.
struct requests {
  // The request type.
  int request_type;
  // The ID of the client.
  char client_id[CLIENTIDLENGTH];
};

// The types of response that can be given.
#define RESPONSE_CURRENT_SPECTRUM    1
#define RESPONSE_CURRENT_VISDATA     2
#define RESPONSE_VISDATA_COMPUTED    3
#define RESPONSE_COMPUTED_VISDATA    4
#define RESPONSE_VISDATA_COMPUTING   5
#define RESPONSE_SERVERTYPE          6

// This structure describes response headers.
struct responses {
  // The response type.
  int response_type;
  // The ID of the client that requested the new data, or
  // all 0s if it's just new data.
  char client_id[CLIENTIDLENGTH];
};

#define JUSTRESPONSESIZE (2 * sizeof(struct responses))

ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length);
ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length);
bool prepare_client_connection(char *server_name, int port_number,
                               SOCKET *socket_peer, bool debugging);
const char *get_type_string(int type, int id);
const char *get_servertype_string(int type);
