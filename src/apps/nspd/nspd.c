/**
 * ATCA Training Library: nspd.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This is a new version of SPD, which can work with both a
 * correlator or the simulator.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <argp.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include "memory.h"
#include "packing.h"
#include "cpgplot.h"
#include "common.h"
#include "atnetworking.h"

const char *argp_program_version = "nspd 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "new/network SPD";

// Argument description.
static char args_doc[] = "[options]";

// Our options.
static struct argp_option options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "The PGPLOT device to use" },
  { "debug", 'D', 0, 0, "Output debugging information" },
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { "server", 's', "SERVER", 0, "The server name or address to connect to" },
  { "port", 'p', "PORTNUM", 0, "The port number on the server to connect to" },
  { 0 }
};

#define SPDBUFSIZE 1024

// The arguments structure.
struct arguments {
  bool use_file;
  bool debugging_output;
  char input_file[SPDBUFSIZE];
  char spd_device[SPDBUFSIZE];
  int port_number;
  char server_name[SPDBUFSIZE];
  bool network_operation;
};

// And some fun, totally necessary, global state variables.
int action_required;
int spd_device_number;
int xaxis_type, yaxis_type, plot_pols, yaxis_scaling, nxpanels, nypanels;
struct spd_plotcontrols spd_plotcontrols;
struct panelspec spd_panelspec;
struct spectrum_data spectrum_data;

// The maximum number of divisions supported on the plot.
#define MAX_XPANELS 6
#define MAX_YPANELS 6

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'd':
    strncpy(arguments->spd_device, arg, SPDBUFSIZE);
    break;
  case 'D':
    arguments->debugging_output = true;
    break;
  case 'f':
    arguments->use_file = true;
    strncpy(arguments->input_file, arg, SPDBUFSIZE);
    break;
  case 's':
    arguments->network_operation = true;
    strncpy(arguments->server_name, arg, SPDBUFSIZE);
    break;
  case 'p':
    arguments->port_number = atoi(arg);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void read_data_from_file(char *filename, struct spectrum_data *spectrum_data) {
  FILE *fh = NULL;
  cmp_ctx_t cmp;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    error_and_exit("Error opening input file");
  }
  cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
  unpack_spectrum_data(&cmp, spectrum_data);
  fclose(fh);
}

void prepare_spd_device(char *device_name, bool *device_opened) {

  if (!(*device_opened)) {
    // Open the device.
    spd_device_number = cpgopen(device_name);
    *device_opened = true;
  }

  // Set up the device with the current settings.
  cpgask(0);
  spd_panelspec.measured = NO;
  splitpanels(nxpanels, nypanels, spd_device_number, 0, 5, &spd_panelspec);
  /* fprintf(stderr, "PGPLOT X %.4f -> %.4f\n", spd_panelspec.orig_x1, */
  /*         spd_panelspec.orig_x2); */
  /* fprintf(stderr, "PGPLOT Y %.4f -> %.4f\n", spd_panelspec.orig_y1, */
  /*         spd_panelspec.orig_y2); */
  
}

void release_spd_device(bool *device_opened) {

  if (*device_opened) {
    // Close the device.
    cpgslct(spd_device_number);
    cpgclos();
    *device_opened = false;
    spd_device_number = -1;
  }

  free_panelspec(&spd_panelspec);
  
}

bool sigwinch_received;
const char *prompt = "NSPD> ";

// Handle SIGWINCH and window size changes when readline is
// not active and reading a character.
static void sighandler(int sig) {
  if (sig == SIGWINCH) {
    sigwinch_received = true;
  }
}

#define ACTION_REFRESH_PLOT        1<<0
#define ACTION_QUIT                1<<1
#define ACTION_CHANGE_PLOTSURFACE  1<<2
#define ACTION_NEW_DATA_RECEIVED   1<<3

bool minmatch(char *ref, char *chk, int minlength) {
  // This routine compares the string chk to the string
  // ref. If the entire string chk represents ref up the
  // the length of chk or ref, it returns true.
  // eg. if ref = "select", and chk = "sel", true
  //        ref = "select", and chk = "s", minlength = 3, false
  //        ref = "select", and chk = "selects", false
  int chklen, reflen;

  reflen = strlen(ref);
  chklen = strlen(chk);
  if ((minlength > chklen) || (minlength > reflen)) {
    return false;
  }

  if (chklen > reflen) {
    return false;
  }

  if (strncasecmp(chk, ref, chklen) == 0) {
    return true;
  }

  return false;
}

int split_string(char *s, char *delim, char ***elements) {
  int i = 0;
  char *token = NULL;

  // Skip any leading delimiters.
  while (*s == *delim) {
    s++;
  }
  while ((token = strtok_r(s, delim, &s))) {
    REALLOC(*elements, (i + 1));
    (*elements)[i] = token;
    i++;
  }

  return i;
}

// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  char **line_els = NULL, *cvt = NULL, *cycomma = NULL;
  char delim[] = " ";
  int nels = -1, i, j, k, pols_specified, if_no;
  int if_num_spec[MAXIFS], yaxis_change_type, array_change_spec;
  int flag_change, flag_change_mode, change_nxpanels, change_nypanels;
  bool pols_selected = false, if_selected = false, range_valid = false;
  float range_limit_low, range_limit_high, range_swap;
  
  if ((line == NULL) || (strcasecmp(line, "exit") == 0) ||
      (strcasecmp(line, "quit") == 0)) {
    action_required = ACTION_QUIT;
    if (line == 0) {
      return;
    }
  }

  for (i = 0; i < MAXIFS; i++) {
    if_num_spec[i] = 0;
  }
  
  if (*line) {
    // Keep this history.
    add_history(line);

    // Figure out what we're supposed to do.
    // We will split by spaces, but people might separate arguments with commas.
    // So we first replace all commas with spaces.
    cycomma = line;
    while ((cycomma = strchr(cycomma, ','))) {
      cycomma[0] = ' ';
    }
    nels = split_string(line, delim, &line_els);
    /* for (i = 0; i < nels; i++) { */
    /*   fprintf(stderr, "%d: %s\n", i, line_els[i]); */
    /* } */
    if (minmatch("select", line_els[0], 3)) {
      // We've been given a selection command.
      pols_specified = 0;
      for (i = 1; i < nels; i++) {
        if (strcasecmp(line_els[i], "aa") == 0) {
          pols_selected = true;
          // Select aa.
          pols_specified |= PLOT_POL_XX;
        } else if (strcasecmp(line_els[i], "bb") == 0) {
          pols_selected = true;
          pols_specified |= PLOT_POL_YY;
        } else if (strcasecmp(line_els[i], "ab") == 0) {
          pols_selected = true;
          pols_specified |= PLOT_POL_XY;
        } else if (strcasecmp(line_els[i], "ba") == 0) {
          pols_selected = true;
          pols_specified |= PLOT_POL_YX;
        } else if (strcasecmp(line_els[i], "*") == 0) {
          // Select all the pols.
          pols_selected = true;
          pols_specified |= PLOT_POL_XX | PLOT_POL_YY | PLOT_POL_XY | PLOT_POL_YX;
        } else if ((strncasecmp(line_els[i], "f", 1) == 0) ||
                   (strncasecmp(line_els[i], "z", 1) == 0)) {
          // Probably selecting a band, search for it.
          if_no = find_if_name(spectrum_data.header_data, line_els[i]);
          if ((if_no >= 0) && (if_no < MAXIFS)) {
            if_num_spec[if_no - 1] = 1;
            if_selected = true;
            /* fprintf(stderr, " IF band %s is band number %d\n", line_els[i], if_no); */
          } else {
            fprintf(stderr, " IF band %s not found\n", line_els[i]);
          }
        }
      }
      if (pols_selected) {
        // Reset the polarisations.
        change_spd_plotcontrols(&spd_plotcontrols, NULL, NULL, &pols_specified);
        action_required = ACTION_REFRESH_PLOT;
      }
      if (if_selected) {
        // Reset the IF selection.
        for (i = 0; i < MAXIFS; i++) {
          spd_plotcontrols.if_num_spec[i] = if_num_spec[i];
        }
        action_required = ACTION_REFRESH_PLOT;
      }
    } else if ((strcasecmp(line_els[0], "x") == 0) &&
               (nels == 1)) {
      // Change between frequency and channel X axes.
      if (xaxis_type == PLOT_FREQUENCY) {
        xaxis_type = PLOT_CHANNEL;
      } else if (xaxis_type == PLOT_CHANNEL) {
        xaxis_type = PLOT_FREQUENCY;
      }
      change_spd_plotcontrols(&spd_plotcontrols, &xaxis_type, NULL, NULL);
      action_required = ACTION_REFRESH_PLOT;
    } else if ((minmatch("phase", line_els[0], 1)) ||
               (minmatch("amplitude", line_els[0], 1))) {
      // We've been asked to show phase or amplitude.
      // Have we been given y-range limits?
      if (nels == 3) {
        // Try to interpret the limits.
        cvt = NULL;
        range_valid = true;
        range_limit_low = strtof(line_els[1], &cvt);
        if ((cvt == line_els[1]) || (errno == ERANGE)) {
          range_valid = false;
        }
        range_limit_high = strtof(line_els[2], &cvt);
        if ((cvt == line_els[2]) || (errno == ERANGE)) {
          range_valid = false;
        }
        if (range_valid == true) {
          if (range_limit_low > range_limit_high) {
            // Silly user, swap the numbers for them.
            range_swap = range_limit_low;
            range_limit_low = range_limit_high;
            range_limit_high = range_swap;
          }
          spd_plotcontrols.yaxis_range_min = range_limit_low;
          spd_plotcontrols.yaxis_range_max = range_limit_high;
          spd_plotcontrols.yaxis_range_limit = YES;
        }
      } else {
        spd_plotcontrols.yaxis_range_limit = NO;
      }
      if (minmatch("phase", line_els[0], 1)) {
        yaxis_type = PLOT_PHASE;
        yaxis_change_type = yaxis_type;
      } else if (minmatch("amplitude", line_els[0], 1)) {
        yaxis_type = PLOT_AMPLITUDE;
        yaxis_change_type = yaxis_type | yaxis_scaling;
      }
      change_spd_plotcontrols(&spd_plotcontrols, NULL, &yaxis_change_type, NULL);
      action_required = ACTION_REFRESH_PLOT;
    } else if ((minmatch("scale", line_els[0], 3)) &&
               (nels == 2)) {
      // We've been asked to change how to scale the y axis.
      yaxis_change_type = -1;
      if (minmatch("logarithmic", line_els[1], 3)) {
        yaxis_scaling = PLOT_AMPLITUDE_LOG;
        yaxis_change_type = yaxis_type | yaxis_scaling;
      } else if (minmatch("linear", line_els[1], 3)) {
        yaxis_scaling = PLOT_AMPLITUDE_LINEAR;
        yaxis_change_type = yaxis_type | yaxis_scaling;
      }
      if (yaxis_change_type >= 0) {
        change_spd_plotcontrols(&spd_plotcontrols, NULL, &yaxis_change_type, NULL);
        action_required = ACTION_REFRESH_PLOT;
      }
    } else if (minmatch("array", line_els[0], 3)) {
      // Change which antennas are being shown. We look at all the
      // elements on this line.
      array_change_spec = 0;
      for (j = 1; j < nels; j++) {
        if (MAXANTS < 10) {
          // We just look for the numeral in all the arguments.
          for (k = 1; k <= MAXANTS; k++) {
            if (strchr(line_els[j], (k + '0'))) {
              array_change_spec |= 1<<k;
            }
          }
        }// We'll need to put more code here if we are to support
        // arrays with more than 9 antennas.
      }

      if (array_change_spec > 0) {
        spd_plotcontrols.array_spec = array_change_spec;
        action_required = ACTION_REFRESH_PLOT;
      }
    } else if ((strcasecmp("on", line_els[0]) == 0) ||
               (strcasecmp("off", line_els[0]) == 0)) {
      // Enable or disable plotting flags.
      flag_change = 0;
      if (strcasecmp("on", line_els[0]) == 0) {
        flag_change_mode = 1;
      } else if (strcasecmp("off", line_els[0]) == 0) {
        flag_change_mode = -1;
      }
      for (j = 1; j < nels; j++) {
        if (strcasecmp(line_els[j], "acs") == 0) {
          // Autocorrelation switch.
          flag_change |= PLOT_FLAG_AUTOCORRELATIONS;
        } else if (strcasecmp(line_els[j], "ccs") == 0) {
          // Cross-correlation switch.
          flag_change |= PLOT_FLAG_CROSSCORRELATIONS;
        } else if (strcasecmp(line_els[j], "aa") == 0) {
          flag_change |= PLOT_FLAG_POL_XX;
        } else if (strcasecmp(line_els[j], "bb") == 0) {
          flag_change |= PLOT_FLAG_POL_YY;
        } else if (strcasecmp(line_els[j], "ab") == 0) {
          flag_change |= PLOT_FLAG_POL_XY;
        } else if (strcasecmp(line_els[j], "ba") == 0) {
          flag_change |= PLOT_FLAG_POL_YX;
        }
      }

      if (flag_change > 0) {
        if (change_spd_plotflags(&spd_plotcontrols, flag_change,
                                 flag_change_mode) == YES) {
          action_required = ACTION_REFRESH_PLOT;
        }
      }
    } else if (strcasecmp("nxy", line_els[0]) == 0) {
      // Change the panel layout.
      if (nels == 3) {
        // The arguments need to be x and y number of panels.
        change_nxpanels = atoi(line_els[1]);
        change_nypanels = atoi(line_els[2]);
        if ((change_nxpanels > 0) && (change_nxpanels <= MAX_XPANELS) &&
            (change_nypanels > 0) && (change_nypanels <= MAX_YPANELS)) {
          // We can change to this.
          nxpanels = change_nxpanels;
          nypanels = change_nypanels;
          action_required = ACTION_CHANGE_PLOTSURFACE;
        }
      }
    }
    FREE(line_els);
  }

  // We need to free the memory.
  FREE(line);
}

#define STRUCTCOPY(s, d, p) d->p = s->p

void reconcile_spd_plotcontrols(struct spectrum_data *spectrum_data,
				struct spd_plotcontrols *user_plotcontrols,
				struct spd_plotcontrols *data_plotcontrols) {
  int i;
  // This routine is here to ensure that the user plot controls doesn't
  // allow the plotter to try things which aren't supported by the data.

  // Check that we don't try to plot IFs that don't exist.
  for (i = 0; i < MAXIFS; i++) {
    // By default, we set not to plot.
    data_plotcontrols->if_num_spec[i] = 0;
    if (user_plotcontrols->if_num_spec[i]) {
      // The user will allow this to be plotted if possible.
      if (i < spectrum_data->num_ifs) {
        // This IF is in the data.
        data_plotcontrols->if_num_spec[i] = 1;
      }
    }
  }

  // Copy over the other parameters.
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, plot_options);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, plot_flags);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_limit);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_min);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_max);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_limit);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_min);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_max);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, array_spec);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, npols);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, interactive);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, pgplot_device);
  
}

void free_spectrum_data(struct spectrum_data *spectrum_data) {
  int i, j;
  for (i = 0; i < spectrum_data->num_ifs; i++) {
    for (j = 0; j < spectrum_data->num_pols; j++) {
      free_ampphase(&(spectrum_data->spectrum[i][j]));
    }
    FREE(spectrum_data->spectrum[i]);
  }
  FREE(spectrum_data->spectrum);
}

int main(int argc, char *argv[]) {
  struct arguments arguments;
  bool spd_device_opened = false;
  struct spd_plotcontrols spd_alteredcontrols;
  fd_set watchset, reads;
  int r, bytes_received, max_socket = -1;
  size_t recv_buffer_length;
  struct requests server_request;
  struct responses server_response;
  struct addrinfo hints, *peer_address;
  char address_buffer[SPDBUFSIZE], service_buffer[SPDBUFSIZE];
  char port_string[SPDBUFSIZE], send_buffer[SPDBUFSIZE];
  char *recv_buffer = NULL;
  SOCKET socket_peer;
  cmp_ctx_t cmp;
  cmp_mem_access_t mem;
  
  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.debugging_output = false;
  arguments.input_file[0] = 0;
  arguments.spd_device[0] = 0;
  arguments.server_name[0] = 0;
  arguments.port_number = 8880;
  arguments.network_operation = false;

  // And defaults for some of the parameters.
  nxpanels = 5;
  nypanels = 5;
  
  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Handle window size changes.
  signal(SIGWINCH, sighandler);

  // Open and unpack the file if we have one.
  if (arguments.use_file) {
    read_data_from_file(arguments.input_file, &spectrum_data);
  } else if (arguments.network_operation) {
    // Prepare our connection.
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_string, SPDBUFSIZE, "%d", arguments.port_number);
    if (getaddrinfo(arguments.server_name, port_string, &hints,
                    &peer_address)) {
      fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
      return(1);
    }
    getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                address_buffer, sizeof(address_buffer),
                service_buffer, sizeof(service_buffer), NI_NUMERICHOST);
    if (arguments.debugging_output) {
      fprintf(stderr, "Remote address is: %s %s\n", address_buffer, service_buffer);
    }
    socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                         peer_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_peer)) {
      fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
      return(1);
    }
    if (arguments.debugging_output) {
      fprintf(stderr, "Connecting...\n");
    }
    if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
      fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
      return(1);
    }
    freeaddrinfo(peer_address);
    if (arguments.debugging_output) {
      fprintf(stderr, "Connected.\n");
    }
    // Send a request for the currently available spectrum.
    server_request.request_type = REQUEST_CURRENT_SPECTRUM;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SPDBUFSIZE);
    pack_requests(&cmp, &server_request);
    send(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem), 0);
  }

  // Open the plotting device.
  prepare_spd_device(arguments.spd_device, &spd_device_opened);

  // Install the line handler.
  rl_callback_handler_install(prompt, interpret_command);
  
  // Set up our main loop.
  FD_ZERO(&watchset);
  // We'll be looking at the terminal.
  FD_SET(fileno(rl_instream), &watchset);
  if (fileno(rl_instream) > max_socket) {
    max_socket = fileno(rl_instream);
  }
  if (arguments.network_operation) {
    // And the connection to the server.
    FD_SET(socket_peer, &watchset);
    if (socket_peer > max_socket) {
      max_socket = socket_peer;
    }
  }

  // We will need to have a default plot upon entry.
  xaxis_type = PLOT_FREQUENCY;
  yaxis_type = PLOT_AMPLITUDE;
  yaxis_scaling = PLOT_AMPLITUDE_LINEAR;
  plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  init_spd_plotcontrols(&spd_plotcontrols, xaxis_type, yaxis_type | yaxis_scaling,
                        plot_pols, spd_device_number);
  // The number of pols is set by the data though, not the selection.
  /* spd_plotcontrols.npols = spectrum_data.num_pols; */

  action_required = ACTION_NEW_DATA_RECEIVED;
  while(true) {
    reads = watchset;
    if (action_required & ACTION_NEW_DATA_RECEIVED) {
      // Adjust the number of pols.
      spd_plotcontrols.npols = spectrum_data.num_pols;
      action_required -= ACTION_NEW_DATA_RECEIVED;
      action_required |= ACTION_CHANGE_PLOTSURFACE;
    }
    
    if (action_required & ACTION_CHANGE_PLOTSURFACE) {
      // The plotting window needs to be split into a different set of windows.
      if (arguments.debugging_output) {
        fprintf(stderr, "Changing plot surface.\n");
      }
      free_panelspec(&spd_panelspec);
      splitpanels(nxpanels, nypanels, spd_device_number, 0, 5, &spd_panelspec);
      action_required -= ACTION_CHANGE_PLOTSURFACE;
      action_required |= ACTION_REFRESH_PLOT;
    }
    
    if (action_required & ACTION_REFRESH_PLOT) {
      // Let's make a plot.
      if (arguments.debugging_output) {
        fprintf(stderr, "Refreshing plot.\n");
      }
      // First, ensure the plotter doesn't try to do something it can't.
      reconcile_spd_plotcontrols(&spectrum_data, &spd_plotcontrols, &spd_alteredcontrols);
      make_spd_plot(spectrum_data.spectrum, &spd_panelspec, &spd_alteredcontrols, true);
      action_required -= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_QUIT) {
      break;
    }

    // Have a look at our inputs.
    r = select((max_socket + 1), &reads, NULL, NULL, NULL);
    if ((r < 0) && (errno != EINTR)) {
      perror("  NSPD FAILS!\n");
      break;
    }
    if (sigwinch_received) {
      rl_resize_terminal();
      sigwinch_received = false;
    }
    if (r < 0) {
      // Nothing got selected.
      continue;
    }
    if (FD_ISSET(fileno(rl_instream), &reads)) {
      rl_callback_read_char();
    }
    if (arguments.network_operation && FD_ISSET(socket_peer, &reads)) {
      if (arguments.debugging_output) {
        fprintf(stderr, "Data coming in...\n");
      }
      // The first bit of data can be used to determine how much data to expect.
      bytes_received = socket_recv_buffer(socket_peer, &recv_buffer, &recv_buffer_length);
      if (arguments.debugging_output) {
        fprintf(stderr, "  received %d bytes total\n", bytes_received);
      }
      if (bytes_received <= 0) {
        if (arguments.debugging_output) {
          fprintf(stderr, "Connection closed by peer.\n");
        }
        action_required = ACTION_QUIT;
        continue;
      }
      init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
      unpack_responses(&cmp, &server_response);
      if (arguments.debugging_output) {
        fprintf(stderr, "Response is type %d\n", server_response.response_type);
      }
      // Check we're getting what we expect.
      if (server_response.response_type == RESPONSE_CURRENT_SPECTRUM) {
        unpack_spectrum_data(&cmp, &spectrum_data);
        // Refresh the plot next time through.
        action_required = ACTION_NEW_DATA_RECEIVED;
      }
    }
    
  }

  // Remove our callbacks.
  rl_callback_handler_remove();
  printf("\n\n  NSPD EXITS\n");
  
  // Release the plotting device.
  release_spd_device(&spd_device_opened);

  // Release all the memory.
  free_spectrum_data(&spectrum_data);
  
}
