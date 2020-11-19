/** \file atnetworking.c
 *  \brief Networking routines to ensure smooth communication between all the
 *         tools
 *
 * ATCA Training Library
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

/*!
 *  \brief Send a buffer over the network socket
 *  \param socket the already open network socket to use to send the data
 *  \param buffer the byte buffer to send
 *  \param buffer_length the number of bytes to send from \a buffer
 *  \return the number of bytes that were successfully sent; if an error
 *          occurs while sending, the return value should be negative
 *
 * This routine is here to normalise the sending process for variable-length
 * buffers. Before the buffer itself is sent, the size of the buffer is
 * sent over the socket so the receiver can anticipate how much data to expect.
 */
ssize_t socket_send_buffer(SOCKET socket, char *buffer, size_t buffer_length) {
  // Send the buffer with length buffer_length over the socket.
  ssize_t init_bytes_sent, bytes_sent;

  // Send the size of the data first.
  init_bytes_sent = send(socket, &buffer_length, sizeof(size_t), 0);
  if (init_bytes_sent < 0) {
    fprintf(stderr, "UNABLE TO SEND ANY DATA!\n");
    return(init_bytes_sent);
  }
  
  // Now send the buffer.
  bytes_sent = send(socket, buffer, buffer_length, 0);

  // Check if it got blocked.
  
  return(bytes_sent);
}

/*!
 *  \brief Receive a buffer from the network socket
 *  \param socket the already open network socket to get the data from
 *  \param buffer a pointer to a buffer variable in which to put the data
 *  \param buffer_length a pointer to a variable that upon exit will contain
 *                       the amount of data received and present in the \a buffer
 *                       pointer
 *  \return the number of bytes actually read from the socket; if an error occurs
 *          while receiving, the return value should be negative
 *
 * This routine is here to normalise the receiving process for variable-length
 * buffers. The size of the buffer to come is received first, and then this
 * routine allocates enough memory to accept the data.
 */
ssize_t socket_recv_buffer(SOCKET socket, char **buffer, size_t *buffer_length) {
  // Read the buffer from the network socket, allocating the necessary space for it.
  ssize_t bytes_to_read, bytes_read, br;
  
  // Read the size of the data first.
  bytes_read = recv(socket, &bytes_to_read, sizeof(ssize_t), 0);
  if (bytes_read <= 0) {
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
  }
  if (bytes_read <= 0) {
    fprintf(stderr, "Connection closed by peer before data reception.\n");
    return(bytes_read);
  }
  *buffer_length = (size_t)bytes_read;
  
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

/*!
 *  \brief Get a string representation of the type of request or response
 *  \param type the type of magic number for translation purposes, one of
 *              TYPE_REQUEST for request magic numbers, or TYPE_RESPONSE for
 *              response magic numbers
 *  \param id the magic number to stringify
 *  \return a string indication of the magic number
 */
const char *get_type_string(int type, int id) {
  // Get a string representation of the type of request or response,
  // specified by type=TYPE_REQUEST or TYPE_RESPONSE, and
  // id being one of the definitions in the header.
  int max_request = 13, max_response = 14;
  const char* const request_strings[] = { "",
                                          "REQUEST_CURRENT_SPECTRUM",
                                          "REQUEST_CURRENT_VISDATA",
                                          "REQUEST_COMPUTE_VISDATA",
                                          "REQUEST_COMPUTED_VISDATA",
                                          "CHILDREQUEST_VISDATA_COMPUTED",
                                          "REQUEST_SERVERTYPE",
					  "REQUEST_SPECTRUM_MJD",
					  "REQUEST_MJD_SPECTRUM",
					  "CHILDREQUEST_SPECTRUM_MJD",
					  "REQUEST_TIMERANGE",
					  "REQUEST_CYCLE_TIMES",
					  "REQUEST_SUPPLY_USERNAME"
  };
  const char* const response_strings[] = { "",
                                           "RESPONSE_CURRENT_SPECTRUM",
                                           "RESPONSE_CURRENT_VISDATA",
                                           "RESPONSE_VISDATA_COMPUTED",
                                           "RESPONSE_COMPUTED_VISDATA",
                                           "RESPONSE_VISDATA_COMPUTING",
                                           "RESPONSE_SERVERTYPE",
					   "RESPONSE_SPECTRUM_LOADING",
					   "RESPONSE_SPECTRUM_LOADED",
					   "RESPONSE_LOADED_SPECTRUM",
					   "RESPONSE_SPECTRUM_OUTSIDERANGE",
					   "RESPONSE_TIMERANGE",
					   "RESPONSE_CYCLE_TIMES",
					   "RESPONSE_REQUEST_USERNAME"
  };

  if ((type == TYPE_REQUEST) && (id >= 0) && (id < max_request)) {
    return request_strings[id];
  }
  if ((type == TYPE_RESPONSE) && (id >= 0) && (id < max_response)) {
    return response_strings[id];
  }
  return NULL;
}

/*!
 *  \brief Get a string representation of a server type magic number
 *  \param type server type magic number, one of the SERVERTYPE_* definitions
 *  \return a string indication of the magic number
 */
const char *get_servertype_string(int type) {
  // Get a string representation of the type of server.
  if (type == SERVERTYPE_SIMULATOR) {
    return "SIMULATOR";
  } else if (type == SERVERTYPE_CORRELATOR) {
    return "CORRELATOR";
  } else if (type == SERVERTYPE_TESTING) {
    return "EXAMINATION";
  } else {
    return "UNKNOWN!";
  }
}

/*!
 *  \brief Find the client identified either by its ID or username
 *  \param clients the client_sockets structure containing all the information
 *                 about the clients
 *  \param client_id the ID to search for, or zero-length string if not searching
 *                   for client ID
 *  \param client_username the username to search for, or zero-length string if
 *                         not searching for client username
 *  \param n_clients a pointer to a variable which upon exit will be filled with
 *                   the number of matched clients
 *  \param client_sockets a pointer to a variable which upon exit will be an
 *                        array of length `n_clients`, with each element being
 *                        the socket number of a matched client; if you don't
 *                        want this array to be formed, specify `client_sockets`
 *                        as NULL
 *  \param indices a pointer to a variable which upon exit will be an array of
 *                  length `n_clients`, with each element bneing the index of
 *                  the matches in the arrays in `clients`; if you don't want
 *                  this array to be formed, specify `indices` as NULL
 *
 * If searching for a client_id, there should only ever be a single match, since
 * the library randomly generates client IDs. However, any number of clients may
 * share a username.
 */
void find_client(struct client_sockets *clients,
		 char *client_id, char *client_username,
		 int *n_clients, SOCKET **client_sockets, int **indices) {
  int i, n = 0, *local_indices = NULL;
  SOCKET *sockets = NULL;
  
  for (i = 0; i < clients->num_sockets; i++) {
    if (((strlen(client_id) > 0) && (strlen(clients->client_id[i]) > 0) &&
	 (strncmp(clients->client_id[i], client_id, CLIENTIDLENGTH) == 0)) ||
	((strlen(client_username) > 0) && (strlen(clients->client_username[i]) > 0) &&
	 (strncmp(clients->client_username[i], client_username, CLIENTIDLENGTH) == 0))) {
      n += 1;
      REALLOC(sockets, n);
      REALLOC(local_indices, n);
      sockets[n - 1] = clients->socket[i];
      local_indices[n - 1] = i;
      //return(clients->socket[i]);
    }
  }
  //return(-1);

  *n_clients = n;
  if (client_sockets == NULL) {
    FREE(sockets);
  } else {
    *client_sockets = sockets;
  }
  if (indices == NULL) {
    FREE(local_indices);
  } else {
    *indices = local_indices;
  }
}

/*!
 *  \brief Add a client to a list
 *  \param clients the `client_sockets` structure holding information about
 *                 all the clients
 *  \param client_id the ID string supplied by the client
 *  \param client_username the username string supplied by the client
 *  \param socket the socket number that the client has connected on
 *
 * This routine checks if the client has already been connected, but only
 * while using the ID, since multiple clients can connect with the same
 * username. If the client has already connected, it is not added again. If
 * you need to change details about an already connected client, use
 * `modify_client`.
 */
void add_client(struct client_sockets *clients, char *client_id,
		char *client_username, SOCKET socket) {
  int n, n_check;
  SOCKET *check = NULL;
  // Check if we already know about it.
  find_client(clients, client_id, "", &n_check, &check, NULL);
  if (n_check == 0) {
    // Add this.
    n = clients->num_sockets + 1;
    REALLOC(clients->socket, n);
    clients->socket[n - 1] = socket;
    
    REALLOC(clients->client_id, n);
    MALLOC(clients->client_id[n - 1], CLIENTIDLENGTH);
    strncpy(clients->client_id[n - 1], client_id, CLIENTIDLENGTH);
    CALLOC(clients->client_username[n - 1], CLIENTIDLENGTH);
    strncpy(clients->client_username[n - 1], client_username, CLIENTIDLENGTH);

    clients->num_sockets = n;
  }
  FREE(check);
}

/*!
 *  \brief Modify an existing client
 *  \param clients the `client_sockets` structure holding information about
 *                 all the clients
 *  \param client_id the ID string to find; this must be used since this
 *                   routine can only alter a single client
 *  \param client_username the username string to set in the client matching
 *                         the ID `client_id`
 *  \param socket the socket to set in the client matching the ID `client_id`,
 *                but only if the socket is valid; if you don't want to modify
 *                the existing socket, set this to a negative number
 */
void modify_client(struct client_sockets *clients, char *client_id,
		   char *client_username, SOCKET socket) {
  int n_check, *check = NULL;

  find_client(clients, client_id, "", &n_check, NULL, &check);
  if (n_check == 1) {
    // We copy the new socket and username if they're valid.
    if (ISVALIDSOCKET(socket)) {
      clients->socket[check[0]] = socket;
    }
    strncpy(clients->client_username[check[0]], client_username, CLIENTIDLENGTH);
  }
}

void free_client_sockets(struct client_sockets *clients) {
  int i;
  for (i = 0; i < clients->num_sockets; i++) {
    FREE(clients->client_id[i]);
    FREE(clients->client_username[i]);
  }
  FREE(clients->socket);
  FREE(clients->client_id);
  FREE(clients->client_username);
}
