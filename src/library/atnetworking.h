/** \file atnetworking.h
 *  \brief Networking definitions to ensure smooth communication between all the
 *         tools
 *
 * ATCA Training Library
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
#define SERVERTYPE_TESTING    3

#define TYPE_REQUEST    1
#define TYPE_RESPONSE   2

// The types of request that can be made.
#define REQUEST_CURRENT_SPECTRUM        1
#define REQUEST_CURRENT_VISDATA         2
#define REQUEST_COMPUTE_VISDATA         3
#define REQUEST_COMPUTED_VISDATA        4
#define CHILDREQUEST_VISDATA_COMPUTED   5
#define REQUEST_SERVERTYPE              6
#define REQUEST_SPECTRUM_MJD            7
#define REQUEST_MJD_SPECTRUM            8
#define CHILDREQUEST_SPECTRUM_MJD       9
#define REQUEST_TIMERANGE              10
#define REQUEST_CYCLE_TIMES            11
#define REQUEST_RESPONSE_USER_ID       12

/*! \struct requests
 *  \brief Structure to use when requesting data from a central server
 */
struct requests {
  /*! \var request_type
   *  \brief The request type magic number
   *
   * This is one of the REQUEST_* or CHILDREQUEST_* magic numbers defined
   * in this header file.
   */
  int request_type;
  /*! \var client_id
   *  \brief The ID of the client
   */
  char client_id[CLIENTIDLENGTH];
  /*! \var client_username
   *  \brief The username supplied to the client
   *
   * It should not be expected that this field be filled, since the tasks
   * are able to be run without needing a username.
   */
  char client_username[CLIENTIDLENGTH];
};

// The types of response that can be given.
#define RESPONSE_CURRENT_SPECTRUM        1
#define RESPONSE_CURRENT_VISDATA         2
#define RESPONSE_VISDATA_COMPUTED        3
#define RESPONSE_COMPUTED_VISDATA        4
#define RESPONSE_VISDATA_COMPUTING       5
#define RESPONSE_SERVERTYPE              6
#define RESPONSE_SPECTRUM_LOADING        7
#define RESPONSE_SPECTRUM_LOADED         8
#define RESPONSE_LOADED_SPECTRUM         9
#define RESPONSE_SPECTRUM_OUTSIDERANGE  10
#define RESPONSE_TIMERANGE              11
#define RESPONSE_CYCLE_TIMES            12
#define RESPONSE_REQUEST_USER_ID        13

/*! \struct responses
 *  \brief Structure to use when responding to a request
 */
struct responses {
  /*! \var response_type
   *  \brief The response type magic number
   *
   * This is one of the RESPONSE_* magic numbers defined in this header file.
   */
  int response_type;
  /*! \var client_id
   *  \brief The ID of the client that made the request
   *
   * If the response is to supply data previously requested, this ID will be
   * that of the client that initiated the request. It may also be completely
   * filled with 0s if the data being supplied is for all clients.
   */
  char client_id[CLIENTIDLENGTH];
};

/*! \struct client_sockets
 *  \brief This structure is for when a server needs to keep track
 *         of which client is asking for what
 */
struct client_sockets {
  /*! \var num_sockets
   *  \brief The number of sockets for which we store information
   */
  int num_sockets;
  /*! \var socket
   *  \brief The socket number for each socket
   *
   * This array has size `num_sockets`, and is indexed starting at 0.
   */
  SOCKET *socket;
  /*! \var client_id
   *  \brief The ID of the client connected to each socket
   *
   * This array of strings has size `num_sockets`, and is indexed starting at
   * 0. Each string has length CLIENTIDLENGTH.
   */
  char **client_id;
  /*! \var client_username
   *  \brief The username of the client connected to each socket
   *
   * This array of strings has size `num_sockets`, and is indexed starting at
   * 0. Each string has length CLIENTIDLENGTH. If a client did not supply a
   * username, the string will be entirely filled with 0s.
   */
  char **client_username;
};

#define JUSTRESPONSESIZE (10 * sizeof(struct responses))

ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length);
ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length);
bool prepare_client_connection(char *server_name, int port_number,
                               SOCKET *socket_peer, bool debugging);
const char *get_type_string(int type, int id);
const char *get_servertype_string(int type);
void find_client(struct client_sockets *clients,
		 char *client_id, char *client_username,
		 int *n_clients, SOCKET **client_sockets);
void add_client(struct client_sockets *clients, char *client_id,
		char *client_username, SOCKET socket);
void free_client_sockets(struct client_sockets *clients);
