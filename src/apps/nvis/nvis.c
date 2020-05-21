/**
 * ATCA Training Library: nvis.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This is a new version of VIS, which can work with both a
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

const char *argp_program_version = "nvis 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "new/network VIS";

// Argument description.
static char args_doc[] = "[options]";

// Our options.
static struct argp_option options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "The PGPLOT device to use" },
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { "port", 'p', "PORTNUM", 0,
    "The port number on the server to connect to" },
  { "server", 's', "SERVER", 0,
    "The server name or address to connect to" },
  { 0 }
};

#define VISBUFSIZE 1024
#define MAXVISBANDS 2

// The arguments structure.
struct arguments {
  bool use_file;
  char input_file[VISBUFSIZE];
  char vis_device[VISBUFSIZE];
  int port_number;
  char server_name[VISBUFSIZE];
  bool network_operation;
};

// And some fun, totally necessary, global state variables.
int action_required;
int vis_device_number, data_selected_index;
int xaxis_type, yaxis_type, nxpanels, nypanels, nvisbands;
char **visband;
//float describe_time;
struct vis_plotcontrols vis_plotcontrols;
struct panelspec vis_panelspec;
struct vis_data vis_data;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch(key) {
  case 'd':
    strncpy(arguments->vis_device, arg, VISBUFSIZE);
    break;
  case 'f':
    arguments->use_file = true;
    strncpy(arguments->input_file, arg, VISBUFSIZE);
    break;
  case 'p':
    arguments->port_number = atoi(arg);
    break;
  case 's':
    arguments->network_operation = true;
    strncpy(arguments->server_name, arg, VISBUFSIZE);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void read_data_from_file(char *filename, struct vis_data *vis_data) {
  FILE *fh = NULL;
  cmp_ctx_t cmp;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    error_and_exit("Error opening input file");
  }
  cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
  unpack_vis_data(&cmp, vis_data);
  fclose(fh);
}

void prepare_vis_device(char *device_name, bool *device_opened) {
  if (!(*device_opened)) {
    // Open the device.
    vis_device_number = cpgopen(device_name);
    *device_opened = true;
  }

  // Set up the device with the current settings.
  cpgask(0);
  vis_panelspec.measured = NO;
}

void release_vis_device(bool *device_opened) {
  if (*device_opened) {
    // Close the device.
    cpgslct(vis_device_number);
    cpgclos();
    *device_opened = false;
    vis_device_number = -1;
  }

  free_panelspec(&vis_panelspec);
}

bool sigwinch_received, sigint_received;
const char *prompt = "NVIS> ";

// Handle SIGWINCH and window size changes when readline is
// not active and reading a character.
static void sighandler(int sig) {
  if (sig == SIGWINCH) {
    sigwinch_received = true;
  } else if (sig == SIGINT) {
    sigint_received = true;
  }
}

// The action magic numbers.
#define ACTION_REFRESH_PLOT        1<<0
#define ACTION_QUIT                1<<1
#define ACTION_CHANGE_PLOTSURFACE  1<<2
#define ACTION_NEW_DATA_RECEIVED   1<<3
#define ACTION_DESCRIBE_DATA       1<<4
#define ACTION_VISBANDS_CHANGED    1<<5

// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  char **line_els = NULL, *cycomma = NULL, delim[] = " ";
  char duration[VISBUFSIZE], historystart[VISBUFSIZE];
  int nels, i, j, k, array_change_spec, nproducts, pr, closeidx = -1;
  float histlength, data_seconds = 0, delta_time = 0, close_time = 0;
  struct vis_product **vis_products = NULL, *tproduct = NULL;
  bool products_selected, data_time_parsed = false;

  if ((line == NULL) || (strcasecmp(line, "exit") == 0) ||
      (strcasecmp(line, "quit") == 0)) {
    action_required = ACTION_QUIT;
    if (line == 0) {
      return;
    }
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

    if (minmatch("select", line_els[0], 3)) {
      // We've been given a selection command.
      products_selected = false;
      nproducts = 0;
      for (i = 1; i < nels; i++) {
        tproduct = NULL;
        pr = vis_interpret_product(line_els[i], &tproduct);
        if (pr == 0) {
          // Successfully matched a product.
          REALLOC(vis_products, ++nproducts);
          vis_products[nproducts - 1] = tproduct;
          products_selected = true;
        } else {
          FREE(tproduct);
        }
      }
      if (products_selected) {
        // Free the memory in the current options.
        for (i = 0; i < vis_plotcontrols.nproducts; i++) {
          FREE(vis_plotcontrols.vis_products[i]);
        }
        FREE(vis_plotcontrols.vis_products);
        vis_plotcontrols.nproducts = nproducts;
        vis_plotcontrols.vis_products = vis_products;
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
        } // We'll need to put more code here if we are to support
        // arrays with more than 9 antennas.
      }
      if (array_change_spec > 0) {
        vis_plotcontrols.array_spec = array_change_spec;
        action_required = ACTION_REFRESH_PLOT;
      }
    } else if (minmatch("history", line_els[0], 4)) {
      // Change the amount of data being shown.
      if (nels < 2) {
        // Just print the history length.
        minutes_representation(vis_plotcontrols.history_length, duration);
        minutes_representation(vis_plotcontrols.history_start, historystart);
        
        printf(" History currently set to show %s starting %s ago\n",
               duration, historystart);
      } else {
        // Get the history length.
        histlength = string_to_minutes(line_els[1]);
        if (histlength > 0) {
          action_required = ACTION_REFRESH_PLOT;
          vis_plotcontrols.history_length = histlength;
          vis_plotcontrols.history_start = histlength;
          // Check for a history start time.
          if (nels > 2) {
            histlength = string_to_minutes(line_els[2]);
            if (histlength > 0) {
              vis_plotcontrols.history_start = histlength;
            }
          }
        }
      }
    } else if (minmatch("data", line_els[0], 3)) {
      // Output a description of the data, at some time or for
      // the latest time.
      data_time_parsed = false;
      closeidx = -1;
      if (nels == 2) {
        // The second element should be the time.
        data_time_parsed = string_to_seconds(line_els[1], &data_seconds);
        if (data_time_parsed) {
          // Find the data with the right time.
          close_time = fabsf(vis_data.vis_quantities[0][0][0]->ut_seconds -
                             data_seconds);
          closeidx = 0;
          for (i = 1; i < vis_data.nviscycles; i++) {
            delta_time = fabsf(vis_data.vis_quantities[i][0][0]->ut_seconds -
                               data_seconds);
            if (delta_time < close_time) {
              close_time = delta_time;
              closeidx = i;
            }
          }
        }
      }
      if ((closeidx >= 0) && (closeidx < vis_data.nviscycles)) {
        // Change the data selection.
        data_selected_index = closeidx;
      }
      
      action_required = ACTION_DESCRIBE_DATA;
    } else if (minmatch("calband", line_els[0], 4)) {
      // We can change which bands get plotted as aa/bb/ab and cc/dd/cd.
      if (nels > 1) {
        nvisbands = ((nels - 1) > MAXVISBANDS) ? MAXVISBANDS : (nels - 1);
        for (i = 0; i < nvisbands; i++) {
          if (strcmp(line_els[(i + 1)], "f") == 0) {
            // We put the position argument on the end.
            snprintf(visband[i], VISBANDLEN, "f%d", (i + 1));
          } else if (strcmp(line_els[(i + 1)], "z") == 0) {
            // We select the first zoom in the current band.
            snprintf(visband[i], VISBANDLEN, "z%d-1", (i + 1));
          } else {
            // We assume the user knows what they're doing.
            strncpy(visband[i], line_els[(i + 1)], VISBANDLEN);
          }
        }
        action_required = ACTION_VISBANDS_CHANGED;
      } else {
        // Output the current settings.
        printf(" Bands being plotted are ");
        for (i = 0; i < nvisbands; i++) {
          printf("%s ", visband[i]);
        }
        printf("\n");
      }
    }
    
    FREE(line_els);
  } 

  // We need to free the memory.
  FREE(line);
}

// This is the maximum number of messages that can be passed to
// the readline message routine (simply because we have a statically allocated
// array to shove the messages in).
#define MAX_N_MESSAGES 100

int main(int argc, char *argv[]) {
  struct arguments arguments;
  struct vis_data vis_data;
  int i, j, r, bytes_received, nmesg = 0;
  cmp_ctx_t cmp;
  cmp_mem_access_t mem;
  struct requests server_request;
  struct responses server_response;
  SOCKET socket_peer, max_socket = -1;
  char *recv_buffer = NULL, send_buffer[VISBUFSIZE], htime[20];
  char **mesgout = NULL;
  fd_set watchset, reads;
  bool vis_device_opened = false;
  size_t recv_buffer_length;
  struct vis_quantities ***described_ptr = NULL;

  // Allocate some memory.
  MALLOC(mesgout, MAX_N_MESSAGES);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    CALLOC(mesgout[i], VISBUFSIZE);
  }
  nvisbands = (2 > MAXVISBANDS) ? MAXVISBANDS : 2;
  MALLOC(visband, MAXVISBANDS);
  for (i = 0; i < MAXVISBANDS; i++) {
    MALLOC(visband[i], VISBANDLEN);
    // Set a default.
    if (i < nvisbands) {
      snprintf(visband[i], VISBANDLEN, "f%d", (i + 1));
    }
  }
  
  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.input_file[0] = 0;
  arguments.vis_device[0] = 0;
  arguments.server_name[0] = 0;
  arguments.port_number = 8880;
  arguments.network_operation = false;

  // And defaults for some of the parameters.
  nxpanels = 1;
  nypanels = 3;
  
  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Handle window size changes.
  signal(SIGWINCH, sighandler);
  // And interrupts.
  signal(SIGINT, sighandler);
  
  // Open and unpack the file if we have one.
  if (arguments.use_file) {
    read_data_from_file(arguments.input_file, &vis_data);
  } else if (arguments.network_operation) {
    // Prepare our connection.
    if (!prepare_client_connection(arguments.server_name, arguments.port_number,
                                   &socket_peer, false)) {
      return(1);
    }
    // Send a request for the currently available VIS data.
    server_request.request_type = REQUEST_CURRENT_VISDATA;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
    pack_requests(&cmp, &server_request);
    send(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem), 0);
  }

  // Open the plotting device.
  prepare_vis_device(arguments.vis_device, &vis_device_opened);

  // Install the line handler.
  rl_callback_handler_install(prompt, interpret_command);

  // Set up our main loop.
  FD_ZERO(&watchset);
  // We'll be looking at the terminal.
  FD_SET(fileno(rl_instream), &watchset);
  MAXASSIGN(max_socket, fileno(rl_instream));
  if (arguments.network_operation) {
    // And the connection to the server.
    FD_SET(socket_peer, &watchset);
    MAXASSIGN(max_socket, socket_peer);
  }

  // We will need to have a default plot upon entry.
  xaxis_type = PLOT_TIME;
  yaxis_type = PLOT_AMPLITUDE | PLOT_PHASE | PLOT_DELAY;
  init_vis_plotcontrols(&vis_plotcontrols, xaxis_type, yaxis_type,
                        nvisbands, visband, vis_device_number, &vis_panelspec);
  vis_plotcontrols.array_spec = interpret_array_string("1,2,3,4,5,6");
  CALLOC(vis_plotcontrols.vis_products, 1);
  vis_plotcontrols.nproducts = 1;
  vis_interpret_product("aa", &(vis_plotcontrols.vis_products[0]));
  vis_plotcontrols.cycletime = 10;
  action_required = 0;
  while(true) {
    reads = watchset;
    if (action_required & ACTION_NEW_DATA_RECEIVED) {
      action_required -= ACTION_NEW_DATA_RECEIVED;
      action_required |= ACTION_VISBANDS_CHANGED;
      // Reset the selected index from the data.
      data_selected_index = vis_data.nviscycles - 1;
    }

    if (action_required & ACTION_VISBANDS_CHANGED) {
      // TODO: Check that the visbands are actually present in the data.
      change_vis_plotcontrols_visbands(&vis_plotcontrols, nvisbands, visband);
      action_required -= ACTION_VISBANDS_CHANGED;
      action_required |= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_REFRESH_PLOT) {
      // Let's make a plot.
      make_vis_plot(vis_data.vis_quantities, vis_data.nviscycles,
                    vis_data.num_ifs, 4,
                    &vis_panelspec, &vis_plotcontrols, vis_data.header_data);
      action_required -= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_DESCRIBE_DATA) {
      // Describe the data.
      described_ptr = vis_data.vis_quantities[data_selected_index];
      seconds_to_hourlabel(described_ptr[0][0]->ut_seconds, htime);
      nmesg = 0;
      snprintf(mesgout[nmesg++], VISBUFSIZE, "DATA AT %s %s:\n", described_ptr[0][0]->obsdate, htime);
      snprintf(mesgout[nmesg++], VISBUFSIZE, "  HAS %d IFS CYCLE TIME %d\n\r",
               vis_data.num_ifs[data_selected_index],
               vis_data.header_data[data_selected_index]->cycle_time);
      snprintf(mesgout[nmesg++], VISBUFSIZE, "  SOURCE %s OBSTYPE %s\n",
               vis_data.header_data[data_selected_index]->source_name,
               vis_data.header_data[data_selected_index]->obstype);
      for (i = 0; i < vis_data.num_ifs[data_selected_index]; i++) {
        snprintf(mesgout[nmesg++], VISBUFSIZE, " IF %d: WINDOW %d NAMES: ", (i + 1),
                 vis_data.header_data[data_selected_index]->if_label[i]);
        for (j = 0; j < 3; j++) {
          snprintf(mesgout[nmesg++], VISBUFSIZE, "%s ",
                   vis_data.header_data[data_selected_index]->if_name[i][j]);
        }
        snprintf(mesgout[nmesg++], VISBUFSIZE, "\n");
      }
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_DESCRIBE_DATA;
    }
    
    if (action_required & ACTION_QUIT) {
      break;
    }

    // Have a look at our inputs.
    r = select((max_socket + 1), &reads, NULL, NULL, NULL);
    if ((r < 0) && (errno != EINTR)) {
      perror(" NVIS FAILS!\n");
      break;
    }
    if (sigwinch_received) {
      rl_resize_terminal();
      sigwinch_received = false;
    }
    if (sigint_received) {
      break;
    }
    if (r < 0) {
      // Nothing got selected.
      continue;
    }
    if (FD_ISSET(fileno(rl_instream), &reads)) {
      rl_callback_read_char();
    }
    if (arguments.network_operation && FD_ISSET(socket_peer, &reads)) {
      bytes_received = socket_recv_buffer(socket_peer, &recv_buffer, &recv_buffer_length);
      if (bytes_received <= 0) {
        // The server has closed.
        action_required = ACTION_QUIT;
        continue;
      }
      init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
      unpack_responses(&cmp, &server_response);
      // Check we're getting what we expect.
      if (server_response.response_type == RESPONSE_CURRENT_VISDATA) {
        unpack_vis_data(&cmp, &vis_data);
        action_required = ACTION_VISBANDS_CHANGED;
      }
      FREE(recv_buffer);
    }
  }

  // Remove our callbacks.
  rl_callback_handler_remove();
  rl_clear_history();
  printf("\n\n NVIS EXITS\n");

  // Release the plotting device.
  release_vis_device(&vis_device_opened);

  // Release all the memory.
  free_vis_data(&vis_data);
  free_vis_plotcontrols(&vis_plotcontrols);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    FREE(mesgout[i]);
  }
  FREE(mesgout);
  for (i = 0; i < MAXVISBANDS; i++) {
    FREE(visband[i]);
  }
  FREE(visband);
  
  /* // Let's print something to check. */
  /* printf("VIS data has %d cycles:\n", vis_data.nviscycles); */
  /* for (i = 0; i < vis_data.nviscycles; i++) { */
  /*   printf(" Cycle %03d: NUM IFS = %d\n", i, vis_data.num_ifs[i]); */
  /*   for (j = 0; j < vis_data.num_ifs[i]; j++) { */
  /*     printf("  IF %02d: NUM POLS = %d\n", j, vis_data.num_pols[i][j]); */
  /*   } */
  /* } */

}
