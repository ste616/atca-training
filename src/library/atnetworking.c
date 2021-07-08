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

/*! \def HEADER_LENGTH
 *  \brief This is the length of the header string indicating the request
 *         comes from our routines
 */
#define HEADER_LENGTH 5

/*! \var header_string
 *  \brief The header string indicating the request comes from our routines
 */
const char *header_string = "ATNET";

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
  ssize_t init_bytes_sent, bytes_sent, header_bytes_sent;
  
  // Send a header to indicate who we are. This is here so that random
  // connections to our port do not force us to attempt to read invalid
  // data.
  header_bytes_sent = send(socket, header_string, (size_t)HEADER_LENGTH, 0);
  if (header_bytes_sent < HEADER_LENGTH) {
    fprintf(stderr, "UNABLE TO SEND HEADER!\n");
    return(header_bytes_sent);
  }
  
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
  ssize_t bytes_to_read, bytes_read, br, header_bytes_read;
  char header_received[HEADER_LENGTH];
  
  // Try to get the header first.
  header_bytes_read = recv(socket, header_received, (size_t)HEADER_LENGTH, 0);
  if (header_bytes_read <= 0) {
    // The socket was closed.
    fprintf(stderr, "Connection closed by peer.\n");
    return (header_bytes_read);
  }
    
  if ((header_bytes_read < HEADER_LENGTH) ||
      (strncmp(header_received, header_string, HEADER_LENGTH) != 0)) {
    // We didn't receive the header we were expecting.
    fprintf(stderr, "Connected client did not send correct header, disconnecting.\n");
    return (0);
  }
  
  // Read the size of the data to come now.
  bytes_read = recv(socket, &bytes_to_read, sizeof(ssize_t), 0);
  if (bytes_read <= 0) {
    // The socket was closed.
    fprintf(stderr, "Connection closed by peer before size indication.\n");
    return(bytes_read);
  }
  // Allocate the necessary memory.
  if (bytes_to_read > 0) {
    MALLOC(*buffer, bytes_to_read + 1);
  } else {
    bytes_to_read = 0;
  }
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

/*!
 *  \brief Prepare a connection from the client to the server
 *  \param server_name the host name or address of the server
 *  \param port_number the port number to connect to
 *  \param socket_peer a pointer to the variable that will store the
 *                     socket number we create
 *  \param debugging a flag which specifies whether to output status
 *                   messages (true) or not (false)
 *  \return true if socket is correctly connected, false otherwise
 */
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
  int max_request = 15, max_response = 22;
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
					  "REQUEST_SUPPLY_USERNAME",
					  "REQUEST_ACAL",
					  "CHILDREQUEST_MJDS_SPECTRA"
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
					   "RESPONSE_REQUEST_USERNAME",
					   "RESPONSE_USERREQUEST_VISDATA",
					   "RESPONSE_USERREQUEST_SPECTRUM",
					   "RESPONSE_USERNAME_EXISTS",
					   "RESPONSE_SHUTDOWN",
					   "RESPONSE_COMPUTED_ACAL",
					   "RESPONSE_ACAL_COMPUTING",
					   "RESPONSE_ACAL_REQUEST_INVALID",
					   "RESPONSE_ACAL_COMPUTED"
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
 *  \param client_type the type of client, one of the CLIENTTYPE_* magic numbers
 *  \param socket the socket number that the client has connected on
 *  \return true if the client wasn't on our list and has been added, false
 *          otherwise
 *
 * This routine checks if the client has already been connected, but only
 * while using the ID, since multiple clients can connect with the same
 * username. If the client has already connected, it is not added again. If
 * you need to change details about an already connected client, use
 * `modify_client`.
 */
bool add_client(struct client_sockets *clients, char *client_id,
		char *client_username, int client_type, SOCKET socket) {
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
    REALLOC(clients->client_username, n);
    CALLOC(clients->client_username[n - 1], CLIENTIDLENGTH);
    strncpy(clients->client_username[n - 1], client_username, CLIENTIDLENGTH);
    REALLOC(clients->client_type, n);
    clients->client_type[n - 1] = client_type;

    clients->num_sockets = n;
    return (true);
  }
  FREE(check);
  return (false);
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

/*!
 *  \brief Remove a client from the list, identified by a socket number
 *  \param clients the client_sockets structure holding information about
 *                 all the clients
 *  \param socket the socket number the client had connected on
 *  \param client_id upon exit, if this pointer is not set to NULL, this
 *                   variable will be filled with the ID of the removed client
 *  \param client_username upon exit, if this pointer is not set to NULL, this
 *                         variable will be filled with the username of
 *                         the removed client
 *  \param client_type upon exit, if this pointer is not set to NULL, this
 *                     variable will be filled with the client type of the
 *                     removed client
 */
void remove_client(struct client_sockets *clients, SOCKET socket,
		   char *client_id, char *client_username, int *client_type) {
  int i, cidx;
  // Find the index of the client first.
  for (i = 0, cidx = -1; i < clients->num_sockets; i++) {
    if (clients->socket[i] == socket) {
      // Found the client.
      cidx = i;
      // Copy the ID and username if we can.
      if (client_id != NULL) {
	strncpy(client_id, clients->client_id[i], CLIENTIDLENGTH);
      }
      if (client_username != NULL) {
	strncpy(client_username, clients->client_username[i], CLIENTIDLENGTH);
      }
      if (client_type != NULL) {
	*client_type = clients->client_type[i];
      }
      break;
    }
  }

  if (cidx >= 0) {
    if (cidx < (clients->num_sockets - 1)) {
      // We have to do data shifting.
      for (i = (cidx + 1); i < clients->num_sockets; i++) {
	clients->socket[i - 1] = clients->socket[i];
	strncpy(clients->client_id[i - 1], clients->client_id[i], CLIENTIDLENGTH);
	strncpy(clients->client_username[i - 1], clients->client_username[i],
		CLIENTIDLENGTH);
	clients->client_type[i - 1] = clients->client_type[i];
      }
    }
    // Reallocate the arrays.
    clients->num_sockets -= 1;
    FREE(clients->client_id[clients->num_sockets]);
    FREE(clients->client_username[clients->num_sockets]);
    if (clients->num_sockets > 0) {
      REALLOC(clients->socket, clients->num_sockets);
      REALLOC(clients->client_id, clients->num_sockets);
      REALLOC(clients->client_username, clients->num_sockets);
      REALLOC(clients->client_type, clients->num_sockets);
    } else {
      // No more clients, free everything.
      FREE(clients->socket);
      FREE(clients->client_id);
      FREE(clients->client_username);
      FREE(clients->client_type);
    }
  } else {
    if (client_id != NULL) {
      client_id[0] = 0;
    }
    if (client_username != NULL) {
      client_username[0] = 0;
    }
    if (client_type != NULL) {
      *client_type = -1;
    }
  }
}

/*!
 *  \brief Free memory used in a client_sockets structure, but does not
 *         free the structure memory itself
 *  \param clients the structure to free memory within
 */
void free_client_sockets(struct client_sockets *clients) {
  int i;
  for (i = 0; i < clients->num_sockets; i++) {
    FREE(clients->client_id[i]);
    FREE(clients->client_username[i]);
  }
  FREE(clients->socket);
  FREE(clients->client_id);
  FREE(clients->client_username);
  FREE(clients->client_type);
}

/*!
 *  \brief Return a string representation of the client type
 *  \param client_type one of the CLIENTTYPE_* magic numbers
 *  \param type_string upon exit this variable will be filled with the
 *                     string representation of the client type
 */
void client_type_string(int client_type, char *type_string) {

  switch (client_type) {
  case CLIENTTYPE_NSPD:
    snprintf(type_string, 5, "NSPD");
    break;
  case CLIENTTYPE_NVIS:
    snprintf(type_string, 5, "NVIS");
    break;
  case CLIENTTYPE_CHILD:
    snprintf(type_string, 6, "CHILD");
    break;
  default:
    snprintf(type_string, 8, "UNKNOWN");
  }
}
