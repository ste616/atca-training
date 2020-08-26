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
#include "plotting.h"
#include "atnetworking.h"
#include "atreadline.h"

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
int action_required, server_type;
int vis_device_number, data_selected_index;
int xaxis_type, yaxis_type, nxpanels, nypanels, nvisbands;
int *visband_idx;
char **visband;
// Whether to order the baselines in length order (true/false).
bool sort_baselines;
struct vis_plotcontrols vis_plotcontrols;
struct panelspec vis_panelspec;
struct vis_data vis_data;
struct ampphase_options ampphase_options;

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
const char *uprompt = "USERNAME> ";

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
#define ACTION_REFRESH_PLOT              1<<0
#define ACTION_QUIT                      1<<1
#define ACTION_CHANGE_PLOTSURFACE        1<<2
#define ACTION_NEW_DATA_RECEIVED         1<<3
#define ACTION_DESCRIBE_DATA             1<<4
#define ACTION_VISBANDS_CHANGED          1<<5
#define ACTION_AMPPHASE_OPTIONS_CHANGED  1<<6
#define ACTION_AMPPHASE_OPTIONS_PRINT    1<<7
#define ACTION_USERNAME_OBTAINED         1<<8

// Make a shortcut to stop action for those actions which can only be
// done by a simulator.
#define CHECKSIMULATOR                          \
  do                                            \
    {                                           \
      if (server_type != SERVERTYPE_SIMULATOR)	\
        {                                       \
          FREE(line_els);                       \
          FREE(line);                           \
          return;                               \
        }                                       \
    }                                           \
  while(0)

// Callback function called when the username prompt is displayed.
#define USERNAME_SIZE 10
char username[USERNAME_SIZE];
int username_tries;
static void interpret_username(char *line) {
  bool username_accepted = false, checkline = true;

  if (line == NULL) {
    printf("\n");
    checkline = false;
  }
  
  if (checkline && (*line)) {
    if (strlen(line) > 5) {
      // This is an acceptable username.
      strncpy(username, line, USERNAME_SIZE);
      action_required = ACTION_USERNAME_OBTAINED;
      username_accepted = true;
    }
  }
  
  if (!username_accepted) {
    fprintf(stderr, " USERNAME NOT ACCEPTABLE\n");
    username_tries++;
  }

  if (username_tries > 5) {
    // This isn't working.
    fprintf(stderr, " TOO MANY INVALID USERNAME ATTEMPTS\n");
    action_required = ACTION_QUIT;
  }

  FREE(line);
}

// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  char **line_els = NULL, *cycomma = NULL, delim[] = " ";
  char duration[VISBUFSIZE], historystart[VISBUFSIZE];
  int nels, i, j, k, array_change_spec, nproducts, pr, closeidx = -1;
  int change_panel = PLOT_ALL_PANELS, iarg;
  float histlength, data_seconds = 0, delta_time = 0, close_time = 0;
  float limit_min, limit_max;
  struct vis_product **vis_products = NULL, *tproduct = NULL;
  bool products_selected, data_time_parsed = false, product_usable, succ = false;

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
	  // Check if we can use this product.
	  product_usable = false;
	  for (j = 1; j <= nvisbands; j++) {
	    if (tproduct->if_spec & (1<<(j - 1))) {
	      product_usable = true;
	      break;
	    }
	  }
	  if (product_usable) {
	    REALLOC(vis_products, ++nproducts);
	    vis_products[nproducts - 1] = tproduct;
	    products_selected = true;
	  } else {
	    FREE(tproduct);
	  }
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
    } else if (minmatch("sort", line_els[0], 3)) {
      // Change the baseline sorting method.
      if (nels == 2) {
        // User has specified on/off, hopefully.
        if (minmatch("on", line_els[1], 2)) {
          sort_baselines = true;
        } else if (minmatch("off", line_els[1], 2)) {
          sort_baselines = false;
        }
      } else if (nels == 1) {
        // Toggle the state.
        sort_baselines = !sort_baselines;
      }
      // Print the status.
      printf(" Baseline sorting is in %s order\n",
             (sort_baselines ? "length" : "numerical"));
      action_required = ACTION_REFRESH_PLOT;
    } else if (minmatch("scale", line_els[0], 3)) {
      // Change the scaling of the plot panels.
      if (nels == 1) {
        // Reset the all the panels back to defaults.
        change_vis_plotcontrols_limits(&vis_plotcontrols, PLOT_ALL_PANELS,
                                       false, 0, 0);
        action_required = ACTION_REFRESH_PLOT;
      } else if (nels > 1) {
        // Get the panel type from the second argument.
        change_panel = PLOT_ALL_PANELS;
        if (minmatch("amplitude", line_els[1], 1)) {
          change_panel = PLOT_AMPLITUDE;
        } else if (minmatch("phase", line_els[1], 1)) {
          change_panel = PLOT_PHASE;
        } else if (minmatch("delay", line_els[1], 1)) {
          change_panel = PLOT_DELAY;
        }
        if (change_panel != PLOT_ALL_PANELS) {
          if (nels == 2) {
            // Reset just the named panel.
            change_vis_plotcontrols_limits(&vis_plotcontrols, change_panel,
                                           false, 0, 0);
            action_required = ACTION_REFRESH_PLOT;
          } else if (nels == 4) {
            // We've been given limits.
            if ((string_to_float(line_els[2], &limit_min)) &&
                (string_to_float(line_els[3], &limit_max))) {
              // The limits were successfully read.
              change_vis_plotcontrols_limits(&vis_plotcontrols, change_panel,
                                             true, limit_min, limit_max);
              action_required = ACTION_REFRESH_PLOT;
            }
          }
        }
      }
    } else if (minmatch("print", line_els[0], 2)) {
      // Output some bit of information.
      if (nels > 1) {
        if (minmatch("computation", line_els[1], 4)) {
          // Print the options used during computation.
          action_required = ACTION_AMPPHASE_OPTIONS_PRINT;
        }
      }
    } else if (minmatch("delavg", line_els[0], 5)) {
      CHECKSIMULATOR;
      // Change the delay averaging.
      if (nels == 2) {
        iarg = atoi(line_els[1]);
        if (iarg >= 1) {
          // Change the delay averaging for all the IFs.
          for (i = 0; i < ampphase_options.num_ifs; i++) {
            ampphase_options.delay_averaging[i] = iarg;
          }
          action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
        }
      } else if (nels == 3) {
        // Change the delay averaging for the two IFs in the calband.
        succ = false;
        for (i = 1; i < 3; i++) {
          iarg = atoi(line_els[i]);
          if (iarg >= 1) {
            ampphase_options.delay_averaging[visband_idx[i - 1]] = iarg;
            succ = true;
          }
        }
        if (succ) {
          action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
        }
      }
    } else if (minmatch("tvmedian", line_els[0], 5)) {
      // Change the averaging method.
      if ((nels == 2) || (nels == 3)) {
        CHECKSIMULATOR;
        for (i = 0; i < nvisbands; i++) {
          if (nels == 2) {
            k = 1;
          } else {
            k = i + 1;
          }
          if (strncmp(line_els[k], "on", 2) == 0) {
            // Turn median averaging on.
            if (ampphase_options.averaging_method[visband_idx[i]] & AVERAGETYPE_MEAN) {
              ampphase_options.averaging_method[visband_idx[i]] -= AVERAGETYPE_MEAN;
            }
            ampphase_options.averaging_method[visband_idx[i]] |= AVERAGETYPE_MEDIAN;
          } else if (strncmp(line_els[k], "off", 3) == 0) {
            // Turn mean averaging on.
            if (ampphase_options.averaging_method[visband_idx[i]] & AVERAGETYPE_MEDIAN) {
              ampphase_options.averaging_method[visband_idx[i]] -= AVERAGETYPE_MEDIAN;
            }
            ampphase_options.averaging_method[visband_idx[i]] |= AVERAGETYPE_MEAN;
          }
        }
        action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
      } else {
        // Output the averaging type.
        printf(" Currently using averaging type:");
        for (i = 0; i < nvisbands; i++) {
          if (ampphase_options.averaging_method[visband_idx[i]] & AVERAGETYPE_MEAN) {
            printf(" MEAN");
          } else if (ampphase_options.averaging_method[visband_idx[i]] & AVERAGETYPE_MEDIAN) {
            printf(" MEDIAN");
          } else {
            printf(" UNKNOWN!");
          }
        }
        printf("\n");
      }
    } else if (minmatch("onsource", line_els[0], 3)) {
      CHECKSIMULATOR;
      // Change whether we display data while off-source.
      if (ampphase_options.include_flagged_data == YES) {
        ampphase_options.include_flagged_data = NO;
      } else {
        ampphase_options.include_flagged_data = YES;
      }
      action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
    }
    
    FREE(line_els);
  } 

  // We need to free the memory.
  FREE(line);
}


int main(int argc, char *argv[]) {
  struct arguments arguments;
  int i, j, r, bytes_received, nmesg = 0, fidx = 0;
  cmp_ctx_t cmp;
  cmp_mem_access_t mem;
  struct requests server_request;
  struct responses server_response;
  SOCKET socket_peer, max_socket = -1;
  char *recv_buffer = NULL, send_buffer[VISBUFSIZE], htime[20];
  char **mesgout = NULL, client_id[CLIENTIDLENGTH];
  fd_set watchset, reads;
  bool vis_device_opened = false;
  size_t recv_buffer_length;
  struct vis_quantities ***described_ptr = NULL;
  struct scan_header_data *described_hdr = NULL;

  // Allocate some memory.
  MALLOC(mesgout, MAX_N_MESSAGES);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    CALLOC(mesgout[i], VISBUFSIZE);
  }
  nvisbands = (2 > MAXVISBANDS) ? MAXVISBANDS : 2;
  MALLOC(visband, MAXVISBANDS);
  MALLOC(visband_idx, MAXVISBANDS);
  for (i = 0; i < MAXVISBANDS; i++) {
    MALLOC(visband[i], VISBANDLEN);
    // Set a default.
    if (i < nvisbands) {
      snprintf(visband[i], VISBANDLEN, "f%d", (i + 1));
      visband_idx[i] = 0;
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

  // Generate our ID.
  generate_client_id(client_id, CLIENTIDLENGTH);
  printf("Client ID = %s\n", client_id);
  
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
    // Ask what type of server we're connecting to.
    server_request.request_type = REQUEST_SERVERTYPE;
    strncpy(server_request.client_id, client_id, CLIENTIDLENGTH);
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
    
    // Send a request for the currently available VIS data.
    server_request.request_type = REQUEST_CURRENT_VISDATA;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
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
      described_hdr = vis_data.header_data[data_selected_index];
      // And get the ampphase_options from this selected index.
      fidx = vis_data.num_ifs[data_selected_index];
      copy_ampphase_options(&ampphase_options,
                            vis_data.vis_quantities[data_selected_index][fidx - 1][0]->options);
      // We will need to fill in all the tvchannels though...
      for (i = 1; i < fidx; i++) {
        ampphase_options.min_tvchannel[i] =
          vis_data.vis_quantities[data_selected_index][i - 1][0]->options->min_tvchannel[i];
        ampphase_options.max_tvchannel[i] =
          vis_data.vis_quantities[data_selected_index][i - 1][0]->options->max_tvchannel[i];
      }
    }

    if (action_required & ACTION_VISBANDS_CHANGED) {
      // TODO: Check that the visbands are actually present in the data.
      change_vis_plotcontrols_visbands(&vis_plotcontrols, nvisbands, visband);
      // Update the visband indices.
      for (i = 0; i < nvisbands; i++) {
        visband_idx[i] = find_if_name(described_hdr, visband[i]);
      }
      action_required -= ACTION_VISBANDS_CHANGED;
      action_required |= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_REFRESH_PLOT) {
      // Let's make a plot.
      make_vis_plot(vis_data.vis_quantities, vis_data.nviscycles,
                    vis_data.num_ifs, 4, sort_baselines,
                    &vis_panelspec, &vis_plotcontrols, vis_data.header_data);
      action_required -= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_DESCRIBE_DATA) {
      // Describe the data.
      described_ptr = vis_data.vis_quantities[data_selected_index];
      described_hdr = vis_data.header_data[data_selected_index];
      seconds_to_hourlabel(described_ptr[0][0]->ut_seconds, htime);
      nmesg = 0;
      snprintf(mesgout[nmesg++], VISBUFSIZE, "DATA AT %s %s:\n",
	       described_ptr[0][0]->obsdate, htime);
      snprintf(mesgout[nmesg++], VISBUFSIZE, "  HAS %d IFS CYCLE TIME %d\n\r",
               vis_data.num_ifs[data_selected_index],
               described_hdr->cycle_time);
      snprintf(mesgout[nmesg++], VISBUFSIZE, "  SOURCE %s OBSTYPE %s\n",
               described_hdr->source_name,
               described_hdr->obstype);
      for (i = 0; i < vis_data.num_ifs[data_selected_index]; i++) {
        snprintf(mesgout[nmesg++], VISBUFSIZE, " IF %d: CF %.2f MHz NCHAN %d BW %.0f MHz",
                 (i + 1), described_hdr->if_centre_freq[i], described_hdr->if_num_channels[i],
                 described_hdr->if_bandwidth[i]);
        // Check if this is one of the calbands.
        for (j = 0; j < nvisbands; j++) {
          if (find_if_name(described_hdr, visband[j]) == (i + 1)) {
            snprintf(mesgout[nmesg++], VISBUFSIZE, " (%c%c %c%c %c%c)",
                     ('A' + (2 * j)), ('A' + (2 * j)),
                     ('B' + (2 * j)), ('B' + (2 * j)),
                     ('A' + (2 * j)), ('B' + (2 * j)));
          }
        }
        snprintf(mesgout[nmesg++], VISBUFSIZE, "\n");
      }
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_DESCRIBE_DATA;
    }

    if (action_required & ACTION_AMPPHASE_OPTIONS_PRINT) {
      // Output the ampphase_options that were used for the
      // computation of the data.
      nmesg = 0;
      snprintf(mesgout[nmesg++], VISBUFSIZE, "VIS DATA COMPUTED WITH OPTIONS:\n");
      snprintf(mesgout[nmesg++], VISBUFSIZE, " PHASE UNITS: %s\n",
               (ampphase_options.phase_in_degrees ? "degrees" : "radians"));
      for (i = 1; i < ampphase_options.num_ifs; i++) {
        snprintf(mesgout[nmesg++], VISBUFSIZE, " BAND F%d:\n", i);
        snprintf(mesgout[nmesg++], VISBUFSIZE, "   DELAY AVERAGING: %d\n",
                 ampphase_options.delay_averaging[i]);
        snprintf(mesgout[nmesg++], VISBUFSIZE, "   AVERAGING METHOD: ");
        if (ampphase_options.averaging_method[i] & AVERAGETYPE_VECTOR) {
          snprintf(mesgout[nmesg++], VISBUFSIZE, "VECTOR ");
        } else if (ampphase_options.averaging_method[i] & AVERAGETYPE_SCALAR) {
          snprintf(mesgout[nmesg++], VISBUFSIZE, "SCALAR ");
        }
        if (ampphase_options.averaging_method[i] & AVERAGETYPE_MEAN) {
          snprintf(mesgout[nmesg++], VISBUFSIZE, "MEAN");
        } else if (ampphase_options.averaging_method[i] & AVERAGETYPE_MEDIAN) {
          snprintf(mesgout[nmesg++], VISBUFSIZE, "MEDIAN");
        }
        snprintf(mesgout[nmesg++], VISBUFSIZE, "\n   TVCHANNELS: %d - %d\n",
                 ampphase_options.min_tvchannel[i],
                 ampphase_options.max_tvchannel[i]);
      }
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_AMPPHASE_OPTIONS_PRINT;
    }
    
    if (action_required & ACTION_AMPPHASE_OPTIONS_CHANGED) {
      // Send the new options to the server and ask for the
      // vis data to be recomputed.
      server_request.request_type = REQUEST_COMPUTE_VISDATA;
      init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
      pack_requests(&cmp, &server_request);
      pack_ampphase_options(&cmp, &ampphase_options);
      socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      action_required -= ACTION_AMPPHASE_OPTIONS_CHANGED;
    }

    if (action_required & ACTION_USERNAME_OBTAINED) {
      // Change our prompt back.
      rl_callback_handler_remove();
      rl_callback_handler_install(prompt, interpret_command);
      // Print a message.
      nmesg = 1;
      snprintf(mesgout[0], VISBUFSIZE, " Thankyou and hello %s\n", username);
      readline_print_messages(nmesg, mesgout);

      // Send the username back to the server.
      server_request.request_type = REQUEST_RESPONSE_USER_ID;
      init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
      pack_requests(&cmp, &server_request);
      pack_write_string(&cmp, username, USERNAME_SIZE);
      socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      action_required -= ACTION_USERNAME_OBTAINED;
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
      nmesg = 1;
      snprintf(mesgout[0], VISBUFSIZE, "Received %d bytes\n", bytes_received);
      readline_print_messages(nmesg, mesgout);
      init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
      unpack_responses(&cmp, &server_response);
      // Ignore this if we somehow get a message not addressed to us.
      if (strncmp(server_response.client_id, client_id, CLIENTIDLENGTH) != 0) {
        continue;
      }
      // Check we're getting what we expect.
      if ((server_response.response_type == RESPONSE_CURRENT_VISDATA) ||
          (server_response.response_type == RESPONSE_COMPUTED_VISDATA)) {
        unpack_vis_data(&cmp, &vis_data);
        action_required = ACTION_NEW_DATA_RECEIVED;
      } else if (server_response.response_type == RESPONSE_VISDATA_COMPUTED) {
        // We're being told new data is available after we asked for a new
        // computation. We request this new data.
        server_request.request_type = REQUEST_COMPUTED_VISDATA;
        init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)VISBUFSIZE);
        pack_requests(&cmp, &server_request);
        socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      } else if (server_response.response_type == RESPONSE_SERVERTYPE) {
        // We're being told what type of server we've connected to.
        pack_read_sint(&cmp, &server_type);
        nmesg = 1;
        snprintf(mesgout[0], VISBUFSIZE, "Connected to %s server.\n",
                 get_servertype_string(server_type));
        readline_print_messages(nmesg, mesgout);
      } else if (server_response.response_type == RESPONSE_REQUEST_USER_ID) {
        // The server wants us to find out who the user is.
        // We want to change the prompt first.
        rl_callback_handler_remove();
        rl_callback_handler_install(uprompt, interpret_username);
        username_tries = 0;
        // Print a message.
        nmesg = 1;
        snprintf(mesgout[0], VISBUFSIZE, "PLEASE INPUT ATNF USER NAME\n");
        // Change the prompt.
        readline_print_messages(nmesg, mesgout);
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

}
