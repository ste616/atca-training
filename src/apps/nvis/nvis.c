/** \file nvis.c
 *  \brief New, completely recoded version of VIS, which works via network
 *         transfer of data rather than via shared memory
 *
 * ATCA Training Library
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
static char nvis_doc[] = "new/network VIS";

// Argument description.
static char nvis_args_doc[] = "[options]";

// Our options.
static struct argp_option nvis_options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "The PGPLOT device to use" },
  { "default-dump", 'D', "DUMP_TYPE", 0,
    "The plot type to use as default for output files (default: PNG)" },
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { "port", 'p', "PORTNUM", 0,
    "The port number on the server to connect to" },
  { "server", 's', "SERVER", 0,
    "The server name or address to connect to" },
  { "username", 'u', "USERNAME", 0,
    "The username to communicate to the server" },
  { 0 }
};

#define VISBUFSIZE 1024
#define VISBUFSHORT 512
#define VISBUFMEDIUM 768
#define VISBUFLONG 1280
#define MAXVISBANDS 2

// The arguments structure.
struct nvis_arguments {
  bool use_file;
  char input_file[VISBUFSIZE];
  char vis_device[VISBUFSIZE];
  int port_number;
  char server_name[VISBUFSIZE];
  bool network_operation;
  char username[CLIENTIDLENGTH];
  int default_dump;
};

// And some fun, totally necessary, global state variables.
int action_required, action_modifier, server_type, n_ampphase_options;
int data_selected_index, tsys_apply;
int xaxis_type, *yaxis_type, nxpanels, nypanels, nvisbands;
int *visband_idx, tvchan_change_min, tvchan_change_max;
int reference_antenna_index, nncal;
int *nncal_indices, nncal_cycletime;
char **visband, tvchan_visband[10], hardcopy_filename[VISBUFSHORT];
// Whether to order the baselines in length order (true/false).
bool sort_baselines;
float data_seconds, *nncal_seconds;
struct vis_plotcontrols vis_plotcontrols;
struct panelspec vis_panelspec;
struct vis_data vis_data;
struct ampphase_options **ampphase_options, *found_options;

static error_t nvis_parse_opt(int key, char *arg, struct argp_state *state) {
  struct nvis_arguments *arguments = state->input;

  switch(key) {
  case 'd':
    strncpy(arguments->vis_device, arg, VISBUFSIZE);
    break;
  case 'D':
    if (strcasecmp(arg, "ps") == 0) {
      arguments->default_dump = FILETYPE_POSTSCRIPT;
    } else if (strcasecmp(arg, "png") == 0) {
      arguments->default_dump = FILETYPE_PNG;
    }
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
  case 'u':
    strncpy(arguments->username, arg, CLIENTIDLENGTH);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { nvis_options, nvis_parse_opt, nvis_args_doc, nvis_doc };

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

void prepare_vis_device(char *device_name, int *vis_device_number,
			bool *device_opened, struct panelspec *panelspec) {
  if (!(*device_opened)) {
    // Open the device.
    *vis_device_number = cpgopen(device_name);
    *device_opened = true;
  }

  // Set up the device with the current settings.
  cpgask(0);
  panelspec->measured = NO;
}

void release_vis_device(int *vis_device_number, bool *device_opened,
			struct panelspec *panelspec) {
  if (*device_opened) {
    // Close the device.
    cpgslct(*vis_device_number);
    cpgclos();
    *device_opened = false;
    *vis_device_number = -1;
  }

  free_panelspec(panelspec);
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
#define ACTION_TVCHANNELS_CHANGED        1<<9
#define ACTION_SEND_USERNAME             1<<10
#define ACTION_TSYSCORR_CHANGED          1<<11
#define ACTION_OMIT_OPTIONS              1<<12
#define ACTION_UNKNOWN_COMMAND           1<<13
#define ACTION_COMPUTE_CLOSUREPHASE      1<<14
#define ACTION_HARDCOPY_PLOT             1<<15
#define ACTION_COMPUTE_DELAYS            1<<16
#define ACTION_RESET_DELAYS              1<<17

// The action modifier magic numbers.
#define ACTIONMOD_NOMOD                  0
#define ACTIONMOD_DELAYCORRECT_ALL       1
#define ACTIONMOD_DELAYCORRECT_AFTER     2
#define ACTIONMOD_DELAYCORRECT_BEFORE    3

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
char username[CLIENTIDLENGTH];
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
      strncpy(username, line, CLIENTIDLENGTH);
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

int char_to_product(char pstring) {
  switch (pstring) {
  case 'a':
    return VIS_PLOTPANEL_AMPLITUDE;
  case 'c':
    return VIS_PLOTPANEL_CLOSUREPHASE;
  case 'C':
    return VIS_PLOTPANEL_SYSTEMP_COMPUTED;
  case 'd':
    return VIS_PLOTPANEL_DELAY;
  case 'D':
    return VIS_PLOTPANEL_WINDDIR;
  case 'G':
    return VIS_PLOTPANEL_GTP;
  case 'H':
    return VIS_PLOTPANEL_HUMIDITY;
  case 'n':
    return VIS_PLOTPANEL_CALJY;
  case 'N':
    return VIS_PLOTPANEL_SDO;
  case 'p':
    return VIS_PLOTPANEL_PHASE;
  case 'P':
    return VIS_PLOTPANEL_PRESSURE;
  case 'R':
    return VIS_PLOTPANEL_RAINGAUGE;
  case 'S':
    return VIS_PLOTPANEL_SYSTEMP;
  case 't':
    // This is time, which is a valid x-axis.
    return PLOT_TIME;
  case 'T':
    return VIS_PLOTPANEL_TEMPERATURE;
  case 'V':
    return VIS_PLOTPANEL_WINDSPEED;
  case 'X':
    return VIS_PLOTPANEL_SEEMONPHASE;
  case 'Y':
    return VIS_PLOTPANEL_SEEMONRMS;
  }
  return -1;
}

/*!
 *  \brief Find the index containing data closest to the currently set
 *         nominated time
 *  \return a positive (or 0) integer, representing the index of data near
 *          the nominated time, or -1 if no sufficiently close data was found
 *
 * Upon entry to this routine, the global variable `data_seconds` should be
 * set to be the UTC number of seconds past midnight of the base date that should
 * be searched for in the global `vis_data`. If no sufficiently close data is found,
 * this routine returns -1, and `data_seconds` is also set back to -1.
 */
int time_index() {
  float close_time, delta_time, actual_time;
  int closeidx = 0, i;
  double basemjd, cycletime;
  
  if (data_seconds < 0) {
    // The caller hasn't set any time to search for.
    return -1;
  }
  basemjd = floor(date2mjd(vis_data.vis_quantities[0][0][0]->obsdate,
			   vis_data.vis_quantities[0][0][0]->ut_seconds));
  actual_time = fabs((float)(date2mjd(vis_data.vis_quantities[0][0][0]->obsdate,
				      vis_data.vis_quantities[0][0][0]->ut_seconds) -
			     basemjd) * 86400.0);
  close_time = fabsf((float)((date2mjd(vis_data.vis_quantities[0][0][0]->obsdate,
				       vis_data.vis_quantities[0][0][0]->ut_seconds) -
			      basemjd) * 86400.0) - data_seconds);
  closeidx = 0;
  for (i = 1; i < vis_data.nviscycles; i++) {
    delta_time = fabsf((float)((date2mjd(vis_data.vis_quantities[i][0][0]->obsdate,
					 vis_data.vis_quantities[i][0][0]->ut_seconds) -
				basemjd) * 86400.0) - data_seconds);
    if (delta_time < close_time) {
      close_time = delta_time;
      actual_time = fabs((float)(date2mjd(vis_data.vis_quantities[i][0][0]->obsdate,
					  vis_data.vis_quantities[i][0][0]->ut_seconds) -
				 basemjd) * 86400.0);
      closeidx = i;
    }
  }

  // Don't consider it a match if more than 10 minutes away.
  if ((close_time < 600) && (closeidx >= 0) &&
      (closeidx < vis_data.nviscycles)) {
    // This is close enough.
    cycletime = (double)vis_data.header_data[closeidx]->cycle_time / 86400.0;
    nncal_cycletime = vis_data.header_data[closeidx]->cycle_time;
    // We also try to fill in the indices for the nncal cycles prior to this data.
    nncal_indices[0] = closeidx;
    nncal_seconds[0] = actual_time;
    for (i = 1; i < nncal; i++) {
      if ((closeidx - i) >= 0) {
	delta_time = date2mjd(vis_data.vis_quantities[nncal_indices[i - 1]][0][0]->obsdate,
			      vis_data.vis_quantities[nncal_indices[i - 1]][0][0]->ut_seconds) -
	  date2mjd(vis_data.vis_quantities[closeidx - i][0][0]->obsdate,
		   vis_data.vis_quantities[closeidx - i][0][0]->ut_seconds);
	if (fabs(delta_time - cycletime) < 1e-6) { // 1e-6 days is 0.09 sec
	  // These are consecutive cycles.
	  nncal_indices[i] = closeidx - i;
	  nncal_seconds[i] = nncal_seconds[i - 1] - (float)nncal_cycletime;
	} else {
	  // Non-consecutive.
	  nncal_indices[i] = -1;
	  nncal_seconds[i] = -1;
	  break;
	}
      }
    }
    return closeidx;
  }
  // Reset the time.
  data_seconds = -1;
  return -1;
}

#define TIMEFILE_LENGTH 18
// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  char **line_els = NULL, *cycomma = NULL, delim[] = " ";
  char duration[VISBUFSIZE], historystart[VISBUFSIZE], ttime[TIMEFILE_LENGTH];
  int nels, i, j, k, array_change_spec, nproducts, pr;
  int change_panel = PLOT_ALL_PANELS, iarg, plen = 0, temp_xaxis_type;
  int *temp_yaxis_type = NULL, temp_nypanels, idx_low, idx_high, ty;
  int temp_refant, temp_nncal;
  float histlength;
  float limit_min, limit_max;
  struct vis_product **vis_products = NULL, *tproduct = NULL;
  bool products_selected, data_time_parsed = false, product_usable, succ = false;
  bool product_backwards = false;

  // Reset the modifiers.
  action_modifier = ACTIONMOD_NOMOD;
  
  if (line == NULL) {
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

    if (minmatch("exit", line_els[0], 4) ||
	minmatch("quit", line_els[0], 4)) {
      action_required = ACTION_QUIT;
    } else if (minmatch("select", line_els[0], 3)) {
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
      action_required = ACTION_DESCRIBE_DATA;
      if (nels == 2) {
        // The second element should be the time.
        data_time_parsed = string_to_seconds(line_els[1], &data_seconds);
        if (data_time_parsed) {
	  data_selected_index = time_index();
	  // We'll also want to plot where this time is.
	  action_required |= ACTION_REFRESH_PLOT;
        }
      }
      
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
        change_panel = VIS_PLOTPANEL_ALL;
        if (minmatch("amplitude", line_els[1], 1) ||
	    (strcmp(line_els[1], "a") == 0)) {
          change_panel = VIS_PLOTPANEL_AMPLITUDE;
        } else if (minmatch("phase", line_els[1], 1) ||
		   (strcmp(line_els[1], "p") == 0)) {
          change_panel = VIS_PLOTPANEL_PHASE;
        } else if (minmatch("delay", line_els[1], 1) ||
		   (strcmp(line_els[1], "d") == 0)) {
          change_panel = VIS_PLOTPANEL_DELAY;
        } else if (minmatch("temperature", line_els[1], 4) ||
		   (strcmp(line_els[1], "T") == 0)) {
          change_panel = VIS_PLOTPANEL_TEMPERATURE;
        } else if (minmatch("pressure", line_els[1], 4) ||
		   (strcmp(line_els[1], "P") == 0)) {
          change_panel = VIS_PLOTPANEL_PRESSURE;
        } else if (minmatch("humidity", line_els[1], 4) ||
		   (strcmp(line_els[1], "H") == 0)) {
          change_panel = VIS_PLOTPANEL_HUMIDITY;
        } else if (minmatch("systemp", line_els[1], 4) ||
		   (strcmp(line_els[1], "S") == 0)) {
          change_panel = VIS_PLOTPANEL_SYSTEMP;
        } else if (minmatch("windspeed", line_els[1], 5) ||
		   (strcmp(line_els[1], "V") == 0)) {
          change_panel = VIS_PLOTPANEL_WINDSPEED;
        } else if (minmatch("winddirection", line_els[1], 5) ||
		   (strcmp(line_els[1], "D") == 0)) {
          change_panel = VIS_PLOTPANEL_WINDDIR;
        } else if (minmatch("rain", line_els[1], 3) ||
		   (strcmp(line_els[1], "R") == 0)) {
          change_panel = VIS_PLOTPANEL_RAINGAUGE;
        } else if (minmatch("seemonphase", line_els[1], 7) ||
		   (strcmp(line_els[1], "X") == 0)) {
          change_panel = VIS_PLOTPANEL_SEEMONPHASE;
        } else if (minmatch("seemonrms", line_els[1], 7) ||
		   (strcmp(line_els[1], "Y") == 0)) {
          change_panel = VIS_PLOTPANEL_SEEMONRMS;
        } else if (minmatch("computed_systemp", line_els[1], 4) ||
		   (strcmp(line_els[1], "C") == 0)) {
          change_panel = VIS_PLOTPANEL_SYSTEMP_COMPUTED;
        } else if (minmatch("gtp", line_els[1], 3) ||
		   (strcmp(line_els[1], "G") == 0)) {
          change_panel = VIS_PLOTPANEL_GTP;
        } else if (minmatch("sdo", line_els[1], 3) ||
		   (strcmp(line_els[1], "N") == 0)) {
          change_panel = VIS_PLOTPANEL_SDO;
        } else if (minmatch("caljy", line_els[1], 3) ||
		   (strcmp(line_els[1], "n") == 0)) {
          change_panel = VIS_PLOTPANEL_CALJY;
        } else if (minmatch("closurephase", line_els[1], 4) ||
		   (strcmp(line_els[1], "c") == 0)) {
	  change_panel = VIS_PLOTPANEL_CLOSUREPHASE;
	}
        if (change_panel != VIS_PLOTPANEL_ALL) {
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
        if ((iarg >= 1) && (found_options != NULL)) {
          // Change the delay averaging for all the IFs, but only for
	  // the currently selected options.
	  for (i = 0; i < found_options->num_ifs; i++) {
	    found_options->delay_averaging[i] = iarg;
          }
          action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
        }
      } else if (nels == 3) {
        // Change the delay averaging for the two IFs in the calband.
	// That only works for the currently selected time.
        succ = false;
        for (i = 1; i < 3; i++) {
          iarg = atoi(line_els[i]);
          if ((iarg >= 1) && (found_options != NULL)) {
            found_options->delay_averaging[visband_idx[i - 1]] = iarg;
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
	  if (found_options != NULL) {
	    if (strncmp(line_els[k], "on", 2) == 0) {
	      // Turn median averaging on.
	      if (found_options->averaging_method[visband_idx[i]] & AVERAGETYPE_MEAN) {
		found_options->averaging_method[visband_idx[i]] -= AVERAGETYPE_MEAN;
	      }
	      found_options->averaging_method[visband_idx[i]] |= AVERAGETYPE_MEDIAN;
	    } else if (strncmp(line_els[k], "off", 3) == 0) {
	      // Turn mean averaging on.
	      if (found_options->averaging_method[visband_idx[i]] & AVERAGETYPE_MEDIAN) {
		found_options->averaging_method[visband_idx[i]] -= AVERAGETYPE_MEDIAN;
	      }
	      found_options->averaging_method[visband_idx[i]] |= AVERAGETYPE_MEAN;
	    }
	  }
        }
        action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
      } else {
        // Output the averaging type.
        printf(" Currently using averaging type:");
        for (i = 0; i < nvisbands; i++) {
	  if (found_options != NULL) {
	    if (found_options->averaging_method[visband_idx[i]] & AVERAGETYPE_MEAN) {
	      printf(" MEAN");
	    } else if (found_options->averaging_method[visband_idx[i]] & AVERAGETYPE_MEDIAN) {
	      printf(" MEDIAN");
	    } else {
	      printf(" UNKNOWN!");
	    }
	  } else {
	    printf(" NO OPTIONS FOUND!");
	  }
        }
        printf("\n");
      }
    } else if (minmatch("tvchannel", line_els[0], 4)) {
      // Change the tvchannels to compute with.
      if (nels == 4) {
        CHECKSIMULATOR;
        // The user should have given us a visband, followed by the
        // min and max.
        strncpy(tvchan_visband, line_els[1], 10);
        tvchan_change_min = atoi(line_els[2]);
        tvchan_change_max = atoi(line_els[3]);
        action_required = ACTION_TVCHANNELS_CHANGED;
      } else {
        // Output the current tvchannels.
        action_required = ACTION_AMPPHASE_OPTIONS_PRINT;
      }
    } else if (minmatch("onsource", line_els[0], 3)) {
      // Change whether we display data while off-source.
      CHECKSIMULATOR;
      for (i = 0; i < n_ampphase_options; i++) {
	if (i == 0) {
	  if (ampphase_options[i]->include_flagged_data == YES) {
	    ampphase_options[i]->include_flagged_data = NO;
	  } else {
	    ampphase_options[i]->include_flagged_data = YES;
	  }
	} else {
	  ampphase_options[i]->include_flagged_data =
	    ampphase_options[i - 1]->include_flagged_data;
	}
      }
      action_required = ACTION_AMPPHASE_OPTIONS_CHANGED;
    } else if (minmatch("tsys", line_els[0], 4)) {
      // Tell the simulator to recompute visibilities based on some
      // version of the system temperature.
      CHECKSIMULATOR;
      tsys_apply = -1;
      if (nels == 2) {
	// The user should supply a flag as to which Tsys to use.
	if (minmatch("off", line_els[1], 3)) {
	  // Don't correct visibilites, and reverse any corrections already made.
	  tsys_apply = STM_REMOVE;
	} else if (minmatch("correlator", line_els[1], 3)) {
	  // Use the systemp values calculated by the correlator to correct
	  // the visibilities.
	  tsys_apply = STM_APPLY_CORRELATOR;
	} else if (minmatch("computed", line_els[1], 4)) {
	  // Use the systemp values calculated by our library to correct
	  // the visibilities.
	  tsys_apply = STM_APPLY_COMPUTED;
	}
      }
      if (tsys_apply > -1) {
	action_required = ACTION_TSYSCORR_CHANGED;
      } else {
	action_required = ACTION_AMPPHASE_OPTIONS_PRINT;
      }
    } else if (minmatch("refant", line_els[0], 3)) {
      // Change or report the reference antenna used for closure phase
      // calculations.
      if (nels == 1) {
	// Report the reference antenna.
	printf(" Reference antenna is %d\n",
	       vis_plotcontrols.reference_antenna);
      } else {
	// Set the reference antenna. The user should specify the actual
	// antenna number.
	succ = string_to_integer(line_els[1], &temp_refant);
	if (succ) {
	  succ = false;
	  for (i = 0; i < vis_data.header_data[data_selected_index]->num_ants; i++) {
	    if (vis_data.header_data[data_selected_index]->ant_label[i] == temp_refant) {
	      // Found the antenna.
	      succ = true;
	      reference_antenna_index = i;
	      break;
	    }
	  }
	}
	if (succ) {
	  action_required = ACTION_COMPUTE_CLOSUREPHASE;
	} else {
	  printf(" Unable to find reference antenna %s\n", line_els[1]);
	}
      }
    } else if (minmatch("dump", line_els[0], 4)) {
      // Make a hardcopy version of the plot.
      if (nels == 1) {
	// We automatically generate a file name, based on the current time.
	current_time_string(ttime, TIMEFILE_LENGTH);
	snprintf(hardcopy_filename, VISBUFSHORT, "nvis_plot_%s", ttime);
      } else if (nels == 2) {
	// We've been given a file name to use.
	strncpy(hardcopy_filename, line_els[1], VISBUFSHORT);
      }
      action_required = ACTION_HARDCOPY_PLOT;
    } else if (minmatch("nncal", line_els[0], 3)) {
      // Set the number of cycles to use while computing calibration parameters.
      if (nels == 1) {
	// Report the number of cycles.
	printf(" NNCAL = %d\n", nncal);
      } else {
	succ = string_to_integer(line_els[1], &temp_nncal);
	if (succ && (temp_nncal > 0)) {
	  nncal = temp_nncal;
	  REALLOC(nncal_indices, nncal);
	  REALLOC(nncal_seconds, nncal);
	  // Re-compute the indices if we can.
	  time_index();
	  action_required = ACTION_REFRESH_PLOT;
	}
      }
    } else if (minmatch("dcal", line_els[0], 4)) {
      CHECKSIMULATOR;
      // Compute the delays required to correct the data and then tell the server
      // to incorporate them.
      // Check if a time range has been specified.
      if (data_seconds < 0) {
	printf(" Unable to dcal: please select a time range with data and nncal\n");
      } else {
	// Check that we have enough cycles of data.
	temp_nncal = 0;
	for (i = 0; i < nncal; i++) {
	  if (nncal_indices[i] >= 0) {
	    temp_nncal += 1;
	  }
	}
	if (temp_nncal != nncal) {
	  printf(" Unable to dcal: need %d consecutive cycles of data, please change data and/or nncal\n",
		 nncal);
	} else {
	  action_required = ACTION_COMPUTE_DELAYS;
	  // Check if we have any modifiers.
	  if (nels == 2) {
	    if (minmatch("all", line_els[1], 3)) {
	      // We make the correction for the entire dataset.
	      action_modifier = ACTIONMOD_DELAYCORRECT_ALL;
	    } else if (minmatch("after", line_els[1], 3)) {
	      action_modifier = ACTIONMOD_DELAYCORRECT_AFTER;
	    } else if (minmatch("before", line_els[1], 3)) {
	      action_modifier = ACTIONMOD_DELAYCORRECT_BEFORE;
	    }
	  }
	}
      }
    } else if (minmatch("reset", line_els[0], 3)) {
      CHECKSIMULATOR;
      if (nels == 1) {
	// Can't reset nothing, need another argument.
	printf(" Reset needs an argument (delays)\n");
      } else if (minmatch("delays", line_els[1], 3)) {
	// Remove any delay modifiers from our correction list, and then tell the server
	// to do its thing.
	action_required = ACTION_RESET_DELAYS;
      }
    } else {
      if (nels == 1) {
        // We try to interpret the string as the panels to show.
        // If the second-to-last character is - then the x-axis is at
        // the end.
        plen = strlen(line_els[0]);
        product_backwards = false;
        if (line_els[0][plen - 2] == '-') {
          product_backwards = true;
          temp_xaxis_type = char_to_product(line_els[0][plen - 1]);
        } else {
          // Otherwise the x-axis is specified as the first char.
          temp_xaxis_type = char_to_product(line_els[0][0]);
        }
        // Now go through all the other characters.
        if (product_backwards) {
          idx_low = 0;
          idx_high = plen - 1;
        } else {
          idx_low = 1;
          idx_high = plen;
        }
        for (i = idx_low, temp_nypanels = 0; i < idx_high; i++) {
          ty = char_to_product(line_els[0][i]);
          if (ty >= 0) {
            temp_nypanels += 1;
            REALLOC(temp_yaxis_type, temp_nypanels);
            temp_yaxis_type[temp_nypanels - 1] = ty;
          }
        }
        // Make the change if we have enough usable information.
        if ((product_can_be_x(temp_xaxis_type)) &&
            (temp_nypanels > 0)) {
          change_vis_plotcontrols_panels(&vis_plotcontrols, temp_xaxis_type,
                                         temp_nypanels, temp_yaxis_type,
                                         &vis_panelspec);
          action_required = ACTION_REFRESH_PLOT;
        } else {
	  action_required = ACTION_UNKNOWN_COMMAND;
	}
        // And free our memory.
        FREE(temp_yaxis_type);
      } else {
	action_required = ACTION_UNKNOWN_COMMAND;
      }
    }
    
    FREE(line_els);
  } 

  // We need to free the memory.
  FREE(line);
}


int main(int argc, char *argv[]) {
  struct nvis_arguments arguments;
  int i, j, k, l, m, r, bytes_received, nmesg = 0, bidx = -1, num_timelines = 0;
  int dump_type = FILETYPE_UNKNOWN, dump_device_number = -1, vis_device_number = -1;
  int *timeline_types = NULL, cycidx, visidx, a1, a2;
  cmp_ctx_t cmp;
  cmp_mem_access_t mem;
  struct requests server_request;
  struct responses server_response;
  SOCKET socket_peer, max_socket = -1;
  char *recv_buffer = NULL, send_buffer[SENDBUFSIZE], htime[20];
  char **mesgout = NULL, client_id[CLIENTIDLENGTH];
  char header_string[VISBUFSIZE], dump_device[VISBUFMEDIUM], dump_file[VISBUFSIZE];
  fd_set watchset, reads;
  bool vis_device_opened = false, dump_device_opened = false;
  size_t recv_buffer_length;
  float *timelines = NULL, *timeline_deltas = NULL, dsign = 1;
  double tmjd;
  struct vis_quantities ***described_ptr = NULL;
  struct scan_header_data *described_hdr = NULL;
  struct panelspec dump_panelspec;
  struct ampphase_modifiers *modptr = NULL;

  // Allocate and initialise some memory.
  MALLOC(mesgout, MAX_N_MESSAGES);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    CALLOC(mesgout[i], VISBUFLONG);
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
  n_ampphase_options = 0;
  ampphase_options = NULL;
  found_options = NULL;
  reference_antenna_index = 0;

  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.input_file[0] = 0;
  arguments.vis_device[0] = 0;
  arguments.server_name[0] = 0;
  arguments.port_number = 8880;
  arguments.network_operation = false;
  arguments.username[0] = 0;
  arguments.default_dump = FILETYPE_PNG;

  // And defaults for some of the parameters.
  nxpanels = 1;
  nypanels = 3;
  CALLOC(yaxis_type, nypanels);
  data_seconds = -1;
  nncal = 3;
  MALLOC(nncal_indices, nncal);
  MALLOC(nncal_seconds, nncal);
  for (i = 0; i < nncal; i++) {
    nncal_indices[i] = -1;
    nncal_seconds[i] = -1;
  }
  
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
    strncpy(server_request.client_username, arguments.username, CLIENTIDLENGTH);
    server_request.client_type = CLIENTTYPE_NVIS;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
    
    // Send a request for the currently available VIS data.
    server_request.request_type = REQUEST_CURRENT_VISDATA;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
  }

  // Open the plotting device.
  prepare_vis_device(arguments.vis_device, &vis_device_number, &vis_device_opened,
		     &vis_panelspec);

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
  yaxis_type[0] = VIS_PLOTPANEL_AMPLITUDE;
  yaxis_type[1] = VIS_PLOTPANEL_PHASE;
  yaxis_type[2] = VIS_PLOTPANEL_DELAY;
  //yaxis_type = PLOT_AMPLITUDE | PLOT_PHASE | PLOT_DELAY;
  init_vis_plotcontrols(&vis_plotcontrols, xaxis_type, nypanels, yaxis_type,
                        nvisbands, visband, vis_device_number, &vis_panelspec);
  vis_plotcontrols.array_spec = interpret_array_string("1,2,3,4,5,6");
  CALLOC(vis_plotcontrols.vis_products, 1);
  vis_plotcontrols.nproducts = 1;
  vis_interpret_product("aa", &(vis_plotcontrols.vis_products[0]));
  vis_plotcontrols.cycletime = 10;
  action_required = 0;
  action_modifier = ACTIONMOD_NOMOD;
  while(true) {
    reads = watchset;
    if (action_required & ACTION_NEW_DATA_RECEIVED) {
      action_required -= ACTION_NEW_DATA_RECEIVED;
      action_required |= ACTION_VISBANDS_CHANGED;
      action_required |= ACTION_COMPUTE_CLOSUREPHASE;
      // Reset the selected index from the data.
      nmesg = 0;
      /* snprintf(mesgout[nmesg++], VISBUFSIZE, " new data arrives\n"); */
      if (data_seconds > 0) {
	// We've already got a time setting, which we should try to keep
	// if possible.
	data_selected_index = time_index();
	/* snprintf(mesgout[nmesg++], VISBUFSIZE, " time %.2f has selected index %d\n", */
	/* 	 data_seconds, data_selected_index); */
      }
      if (data_seconds <= 0) {
	// Couldn't keep it, or has never been set.
	data_selected_index = vis_data.nviscycles - 1;
	/* snprintf(mesgout[nmesg++], VISBUFSIZE, " last index selected %d\n", */
	/* 	 data_selected_index); */
      }
      described_hdr = vis_data.header_data[data_selected_index];
      print_information_scan_header(described_hdr, header_string, VISBUFSIZE);
      /* snprintf(mesgout[nmesg++], VISBUFSIZE, " header at index:\n%s\n", header_string); */
      found_options = find_ampphase_options(n_ampphase_options, ampphase_options,
					    described_hdr);
      /* print_options_set(1, &found_options, header_string, VISBUFSIZE); */
      /* snprintf(mesgout[nmesg++], VISBUFSIZE, "%s", header_string); */
      readline_print_messages(nmesg, mesgout);
      
    }

    if (action_required & ACTION_COMPUTE_CLOSUREPHASE) {
      action_required -= ACTION_COMPUTE_CLOSUREPHASE;
      action_required |= ACTION_REFRESH_PLOT;
      vis_plotcontrols.reference_antenna =
	vis_data.header_data[data_selected_index]->ant_label[reference_antenna_index];
      for (i = 0; i < vis_data.nviscycles; i++) {
	for (j = 0; j < vis_data.num_ifs[i]; j++) {
	  for (k = 0; k < vis_data.num_pols[i][j]; k++) {
	    compute_closure_phase(vis_data.header_data[i],
				  vis_data.vis_quantities[i][j][k],
				  reference_antenna_index);
	  }
	}
      }
    }

    if ((action_required & ACTION_COMPUTE_DELAYS) ||
	(action_required & ACTION_RESET_DELAYS)) {
      action_required |= ACTION_AMPPHASE_OPTIONS_CHANGED;
      nmesg = 0;
      for (j = 0; j < nvisbands; j++) {
	visidx = visband_idx[j] - 1;
	if (action_required & ACTION_RESET_DELAYS) {
	  // Remove all modifiers in the options.
	  for (i = 0; i < found_options->num_modifiers[visband_idx[j]]; i++) {
	    free_ampphase_modifiers(found_options->modifiers[visband_idx[j]][i]);
	    FREE(found_options->modifiers[visband_idx[j]][i]);
	  }
	  found_options->num_modifiers[visband_idx[j]] = 0;
	  FREE(found_options->modifiers[visband_idx[j]]);
	} else if (action_required & ACTION_COMPUTE_DELAYS) {
	  // Create a new modifier in the options.
	  found_options->num_modifiers[visband_idx[j]] += 1;
	  REALLOC(found_options->modifiers[visband_idx[j]],
		  found_options->num_modifiers[visband_idx[j]]);
	  CALLOC(found_options->modifiers[visband_idx[j]]
		 [found_options->num_modifiers[visband_idx[j]] - 1], 1);
	  modptr = found_options->modifiers[visband_idx[j]]
	    [found_options->num_modifiers[visband_idx[j]] - 1];
	  modptr->add_delay = true;
	  modptr->delay_num_antennas = 7;
	  modptr->delay_num_pols = POL_XY + 1;
	  CALLOC(modptr->delay, modptr->delay_num_antennas);
	  for (i = 0; i < modptr->delay_num_antennas; i++) {
	    CALLOC(modptr->delay[i], modptr->delay_num_pols);
	  }
	  if ((action_modifier == ACTIONMOD_DELAYCORRECT_ALL) ||
	      (action_modifier == ACTIONMOD_DELAYCORRECT_BEFORE)) {
	    modptr->delay_start_mjd = 0;
	  } else {
	    modptr->delay_start_mjd =
	      date2mjd(vis_data.vis_quantities[nncal_indices[0]][0][0]->obsdate,
		       vis_data.vis_quantities[nncal_indices[0]][0][0]->ut_seconds);
	  }
	  if ((action_modifier == ACTIONMOD_DELAYCORRECT_ALL) ||
	      (action_modifier == ACTIONMOD_DELAYCORRECT_AFTER)) {
	    modptr->delay_end_mjd = 100000; // This is 2132-SEP-01.
	  } else {
	    modptr->delay_end_mjd =
	      date2mjd(vis_data.vis_quantities[nncal_indices[nncal - 1]][0][0]->obsdate,
		       vis_data.vis_quantities[nncal_indices[nncal - 1]][0][0]->ut_seconds);
	  }
	  if (modptr->delay_end_mjd < modptr->delay_start_mjd) {
	    tmjd = modptr->delay_start_mjd;
	    modptr->delay_start_mjd = modptr->delay_end_mjd;
	    modptr->delay_end_mjd = tmjd;
	  }
	  
	  for (i = 0; i < nncal; i++) {
	    cycidx = nncal_indices[i];
	    for (k = 0; k < vis_data.num_pols[cycidx][visidx]; k++) {
	      if ((vis_data.vis_quantities[cycidx][visidx][k]->pol == POL_XX) ||
		  (vis_data.vis_quantities[cycidx][visidx][k]->pol == POL_YY)) {
		// Gather the delay data for the interferometer.
		for (l = 0; l < vis_data.header_data[cycidx]->num_ants; l++) {
		  if (l == reference_antenna_index) {
		    continue;
		  }
		  if (l < reference_antenna_index) {
		    // The sign needs reversing.
		    dsign = -1;
		  } else {
		    dsign = 1;
		  }
		  for (m = 0; m < vis_data.vis_quantities[cycidx][visidx][k]->nbaselines; m++) {
		    base_to_ants(vis_data.vis_quantities[cycidx][visidx][k]->baseline[m],
				 &a1, &a2);
		    if (((a1 == vis_data.header_data[cycidx]->ant_label[reference_antenna_index]) ||
			 (a2 == vis_data.header_data[cycidx]->ant_label[reference_antenna_index])) &&
			((a1 == vis_data.header_data[cycidx]->ant_label[l]) ||
			 (a2 == vis_data.header_data[cycidx]->ant_label[l]))) {
		      // We can read off the delay here.
		      /* snprintf(mesgout[nmesg++], VISBUFLONG, */
		      /* 	     " CYC %d BAND %d POL %d DELAY %d-%d = %.5f ns\n", */
		      /* 	     cycidx, visidx, */
		      /* 	     vis_data.vis_quantities[cycidx][visidx][k]->pol, a1, a2, */
		      /* 	     vis_data.vis_quantities[cycidx][visidx][k]->delay[m][0]); */
		      modptr->delay[vis_data.header_data[cycidx]->ant_label[l]]
			[vis_data.vis_quantities[cycidx][visidx][k]->pol - 2] += dsign *
			vis_data.vis_quantities[cycidx][visidx][k]->delay[m][0] / (float)nncal;
		    }
		  }
		}
	      } else if (vis_data.vis_quantities[cycidx][visidx][k]->pol == POL_XY) {
		// Get the cross-pol delay per antenna.
		for (l = 0; l < vis_data.header_data[cycidx]->num_ants; l++) {
		  for (m = 0; m < vis_data.vis_quantities[cycidx][visidx][k]->nbaselines; m++) {
		    base_to_ants(vis_data.vis_quantities[cycidx][visidx][k]->baseline[m],
				 &a1, &a2);
		    if ((a1 == vis_data.header_data[cycidx]->ant_label[l]) &&
			(a2 == vis_data.header_data[cycidx]->ant_label[l])) {
		      // We get the delay and add it to the Y pol.
		      modptr->delay[vis_data.header_data[cycidx]->ant_label[l]][POL_XY] +=
			vis_data.vis_quantities[cycidx][visidx][k]->delay[m][1] / (float)nncal;
		    }
		  }
		}
	      }
	    }
	  }
	  // Print out the delay corrections found.
	  snprintf(mesgout[nmesg++], VISBUFLONG, " BAND %d, MJD %.6f - %.6f:\n", visband_idx[j],
		   modptr->delay_start_mjd, modptr->delay_end_mjd);
	  for (l = 0; l < vis_data.header_data[nncal_indices[0]]->num_ants; l++) {
	    snprintf(mesgout[nmesg++], VISBUFLONG, "   ANT %d: X = %.3f Y = %.3f XY = %.3f ns\n",
		     vis_data.header_data[nncal_indices[0]]->ant_label[l],
		     modptr->delay[vis_data.header_data[nncal_indices[0]]->ant_label[l]][POL_X],
		     modptr->delay[vis_data.header_data[nncal_indices[0]]->ant_label[l]][POL_Y],
		     modptr->delay[vis_data.header_data[nncal_indices[0]]->ant_label[l]][POL_XY]);
	  }
	}
      }
      readline_print_messages(nmesg, mesgout);
      if (action_required & ACTION_COMPUTE_DELAYS) {
	action_required -= ACTION_COMPUTE_DELAYS;
      }
      if (action_required & ACTION_RESET_DELAYS) {
	action_required -= ACTION_RESET_DELAYS;
      }
    }
    
    if (action_required & ACTION_DESCRIBE_DATA) {
      // Describe the data.
      nmesg = 0;
      if (data_selected_index < 0) {
	snprintf(mesgout[nmesg++], VISBUFLONG, "NO DATA SELECTED\n");
      } else {
	described_ptr = vis_data.vis_quantities[data_selected_index];
	described_hdr = vis_data.header_data[data_selected_index];
	found_options = find_ampphase_options(n_ampphase_options, ampphase_options,
					      described_hdr);
	seconds_to_hourlabel(described_ptr[0][0]->ut_seconds, htime);
	snprintf(mesgout[nmesg++], VISBUFLONG, "DATA AT %s %s:\n",
		 described_ptr[0][0]->obsdate, htime);
	snprintf(mesgout[nmesg++], VISBUFLONG, "  HAS %d IFS CYCLE TIME %d\n\r",
		 vis_data.num_ifs[data_selected_index],
		 described_hdr->cycle_time);
	snprintf(mesgout[nmesg++], VISBUFLONG, "  SOURCE %s OBSTYPE %s\n",
		 described_hdr->source_name[described_ptr[0][0]->source_no],
		 described_hdr->obstype);
	for (i = 0; i < vis_data.num_ifs[data_selected_index]; i++) {
	  snprintf(mesgout[nmesg++], VISBUFLONG, " IF %d: CF %.2f MHz NCHAN %d BW %.0f MHz",
		   (i + 1), described_hdr->if_centre_freq[i], described_hdr->if_num_channels[i],
		   described_hdr->if_bandwidth[i]);
	  // Check if this is one of the calbands.
	  for (j = 0; j < nvisbands; j++) {
	    if (find_if_name(described_hdr, visband[j]) == (i + 1)) {
	      snprintf(mesgout[nmesg++], VISBUFLONG, " (%c%c %c%c %c%c)",
		       ('A' + (2 * j)), ('A' + (2 * j)),
		       ('B' + (2 * j)), ('B' + (2 * j)),
		       ('A' + (2 * j)), ('B' + (2 * j)));
	    }
	  }
	  snprintf(mesgout[nmesg++], VISBUFLONG, "\n");
	}
      }
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_DESCRIBE_DATA;
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

    if ((action_required & ACTION_HARDCOPY_PLOT) ||
	(action_required & ACTION_REFRESH_PLOT)) {
      if (data_seconds > 0) {
	num_timelines = 1;
	MALLOC(timelines, num_timelines);
	MALLOC(timeline_types, num_timelines);
	MALLOC(timeline_deltas, num_timelines);
	timelines[0] = data_seconds;
	timeline_types[0] = TIMEDISPLAY_DASHED;
	timeline_deltas[0] = 0;
      }
      for (i = 0; i < nncal; i++) {
	if (nncal_indices[i] >= 0) {
	  num_timelines++;
	  REALLOC(timelines, num_timelines);
	  REALLOC(timeline_types, num_timelines);
	  REALLOC(timeline_deltas, num_timelines);
	  timelines[i + 1] = nncal_seconds[i];
	  timeline_types[i + 1] = TIMEDISPLAY_CYCLE;
	  timeline_deltas[i + 1] = -1 * nncal_cycletime;
	}
      }

      if (action_required & ACTION_HARDCOPY_PLOT) {
	nmesg = 0;
	// Open a new PGPLOT device.
	dump_type = filename_to_pgplot_device(hardcopy_filename, dump_device,
					      VISBUFSIZE, arguments.default_dump,
					      dump_file, VISBUFSIZE);
	if (dump_type != FILETYPE_UNKNOWN) {
	  // Open the device.
	  //snprintf(mesgout[nmesg++], VISBUFLONG, " PGPLOT device %s\n", dump_device);
	  prepare_vis_device(dump_device, &dump_device_number, &dump_device_opened,
			     &dump_panelspec);
	  if (dump_device_opened) {
	    // Tell the plotter to use the device.
	    vis_plotcontrols.pgplot_device = dump_device_number;
	    // Do some prep work.
	    splitpanels(1, vis_plotcontrols.num_panels, dump_device_number, 1, 1, 0,
			&dump_panelspec);
	    // Make the plot.
	    make_vis_plot(vis_data.vis_quantities, vis_data.nviscycles,
			  vis_data.num_ifs, 4, sort_baselines,
			  &dump_panelspec, &vis_plotcontrols, vis_data.header_data,
			  vis_data.metinfo, vis_data.syscal_data, num_timelines, timelines,
			  timeline_types, timeline_deltas);
	    // Close the device.
	    release_vis_device(&dump_device_number, &dump_device_opened, &dump_panelspec);
	    // Reset the plot controls.
	    vis_plotcontrols.pgplot_device = vis_device_number;
	    snprintf(mesgout[nmesg++], VISBUFLONG, " NVIS output to file %s\n",
		     dump_file);
	  } else {
	    // Print an error.
	    snprintf(mesgout[nmesg++], VISBUFLONG, " NVIS NOT ABLE TO OUPUT TO %s\n",
		     hardcopy_filename);
	  }
	} else {
	  // Print an error.
	  snprintf(mesgout[nmesg++], VISBUFLONG, " UNKNOWN OUTPUT %s\n", hardcopy_filename);
	}
	action_required -= ACTION_HARDCOPY_PLOT;
	readline_print_messages(nmesg, mesgout);
      }
      
      if (action_required & ACTION_REFRESH_PLOT) {
	// Let's make a plot.
	make_vis_plot(vis_data.vis_quantities, vis_data.nviscycles,
		      vis_data.num_ifs, 4, sort_baselines,
		      &vis_panelspec, &vis_plotcontrols, vis_data.header_data,
		      vis_data.metinfo, vis_data.syscal_data, num_timelines, timelines,
		      timeline_types, timeline_deltas);
	action_required -= ACTION_REFRESH_PLOT;
      }

      FREE(timelines);
      FREE(timeline_deltas);
      FREE(timeline_types);
      num_timelines = 0;
    }
    

    if (action_required & ACTION_AMPPHASE_OPTIONS_PRINT) {
      // Output the ampphase_options that were used for the
      // computation of the data at the selected index.
      nmesg = 0;
      snprintf(mesgout[nmesg++], VISBUFLONG, "VIS DATA COMPUTED WITH OPTIONS:\n");
      print_options_set(1, &found_options, mesgout[nmesg++], VISBUFLONG);
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_AMPPHASE_OPTIONS_PRINT;
    }

    if (action_required & ACTION_TVCHANNELS_CHANGED) {
      // Change the tvchannels as requested after finding the correct
      // specified band to change.
      bidx = find_if_name_nosafe(described_hdr, tvchan_visband);
      nmesg = 0;
      if (bidx < 0) {
        // Didn't find the specified band.
        snprintf(mesgout[nmesg++], VISBUFLONG, "Band %s not found in data.\n",
                 tvchan_visband);
      } else {
        found_options->min_tvchannel[bidx] = tvchan_change_min;
        found_options->max_tvchannel[bidx] = tvchan_change_max;
        action_required |= ACTION_AMPPHASE_OPTIONS_CHANGED;
      }

      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_TVCHANNELS_CHANGED;
    }

    if (action_required & ACTION_TSYSCORR_CHANGED) {
      // Change the way the Tsys should be applied to the data.
      switch (tsys_apply) {
      case STM_REMOVE:
	found_options->systemp_reverse_online = true;
	found_options->systemp_apply_computed = false;
	break;
      case STM_APPLY_CORRELATOR:
	found_options->systemp_reverse_online = false;
	found_options->systemp_apply_computed = false;
	break;
      case STM_APPLY_COMPUTED:
	found_options->systemp_reverse_online = true;
	found_options->systemp_apply_computed = true;
	break;
      }
      action_required |= ACTION_AMPPHASE_OPTIONS_CHANGED;
      // Ensure we send our options.
      if (action_required & ACTION_OMIT_OPTIONS) {
	action_required -= ACTION_OMIT_OPTIONS;
      }

      action_required -= ACTION_TSYSCORR_CHANGED;
    }
    
    if (action_required & ACTION_AMPPHASE_OPTIONS_CHANGED) {
      // Send the new options to the server and ask for the
      // vis data to be recomputed.
      server_request.request_type = REQUEST_COMPUTE_VISDATA;
      init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
      pack_requests(&cmp, &server_request);
      if (action_required & ACTION_OMIT_OPTIONS) {
	// Some other client has changed the options, and we tell the server
	// to use those options.
	pack_write_sint(&cmp, 0);
	action_required -= ACTION_OMIT_OPTIONS;
      } else {
	pack_write_sint(&cmp, n_ampphase_options);
	for (i = 0; i < n_ampphase_options; i++) {
	  pack_ampphase_options(&cmp, ampphase_options[i]);
	}
      }
      socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      action_required -= ACTION_AMPPHASE_OPTIONS_CHANGED;
    }

    if (action_required & ACTION_USERNAME_OBTAINED) {
      // Change our prompt back.
      rl_callback_handler_remove();
      rl_callback_handler_install(prompt, interpret_command);
      action_required -= ACTION_USERNAME_OBTAINED;
      action_required |= ACTION_SEND_USERNAME;
      // Store the username in the arguments.
      strncpy(arguments.username, username, CLIENTIDLENGTH);
      // Print a message.
      nmesg = 1;
      snprintf(mesgout[0], VISBUFLONG, " Thankyou and hello %s\n", arguments.username);
      readline_print_messages(nmesg, mesgout);
    }

    if (action_required & ACTION_SEND_USERNAME) {
      // Send the username back to the server.
      server_request.request_type = REQUEST_SUPPLY_USERNAME;
      strncpy(server_request.client_username, arguments.username, CLIENTIDLENGTH);
      init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
      pack_requests(&cmp, &server_request);
      pack_write_string(&cmp, arguments.username, CLIENTIDLENGTH);
      socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      action_required -= ACTION_SEND_USERNAME;
    }

    if (action_required & ACTION_UNKNOWN_COMMAND) {
      nmesg = 0;
      snprintf(mesgout[nmesg++], VISBUFLONG, "  UNKNOWN COMMAND!\n");
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_UNKNOWN_COMMAND;
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
      /* nmesg = 1; */
      /* snprintf(mesgout[0], VISBUFLONG, "Received %d bytes\n", bytes_received); */
      /* readline_print_messages(nmesg, mesgout); */
      init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
      unpack_responses(&cmp, &server_response);
      // Ignore this if we somehow get a message not addressed to us.
      if (strncmp(server_response.client_id, client_id, CLIENTIDLENGTH) != 0) {
        continue;
      }
      // Check we're getting what we expect.
      if ((server_response.response_type == RESPONSE_CURRENT_VISDATA) ||
          (server_response.response_type == RESPONSE_COMPUTED_VISDATA)) {
	// Receive the ampphase options first, and free old ones if we need to.
	if (n_ampphase_options > 0) {
	  for (i = 0; i < n_ampphase_options; i++) {
	    free_ampphase_options(ampphase_options[i]);
	    FREE(ampphase_options[i]);
	  }
	  FREE(ampphase_options);
	}
	pack_read_sint(&cmp, &n_ampphase_options);
	MALLOC(ampphase_options, n_ampphase_options);
	for (i = 0; i < n_ampphase_options; i++) {
	  CALLOC(ampphase_options[i], 1);
	  unpack_ampphase_options(&cmp, ampphase_options[i]);
	}
	// Before we get the new vis data, free the old.
	for (i = 0; i < vis_data.nviscycles; i++) {
	  free_scan_header_data(vis_data.header_data[i]);
	  FREE(vis_data.header_data[i]);
	}
	free_vis_data(&vis_data);
        unpack_vis_data(&cmp, &vis_data);
        action_required = ACTION_NEW_DATA_RECEIVED;
	/* nmesg = 0; */
	/* snprintf(mesgout[nmesg++], VISBUFLONG, " Data received\n"); */
	/* print_options_set(n_ampphase_options, ampphase_options, header_string, VISBUFLONG); */
	/* snprintf(mesgout[nmesg++], VISBUFLONG, "%s", header_string); */
	/* readline_print_messages(nmesg, mesgout); */
      } else if (server_response.response_type == RESPONSE_VISDATA_COMPUTED) {
        // We're being told new data is available after we asked for a new
        // computation. We request this new data.
        server_request.request_type = REQUEST_COMPUTED_VISDATA;
        init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
        pack_requests(&cmp, &server_request);
        socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      } else if (server_response.response_type == RESPONSE_SERVERTYPE) {
        // We're being told what type of server we've connected to.
        pack_read_sint(&cmp, &server_type);
        nmesg = 1;
        snprintf(mesgout[0], VISBUFLONG, "Connected to %s server.\n",
                 get_servertype_string(server_type));
        readline_print_messages(nmesg, mesgout);
      } else if (server_response.response_type == RESPONSE_REQUEST_USERNAME) {
        // The server wants us to find out who the user is.
        // We want to change the prompt first.
        rl_callback_handler_remove();
        rl_callback_handler_install(uprompt, interpret_username);
        username_tries = 0;
        // Print a message.
        nmesg = 1;
        snprintf(mesgout[0], VISBUFLONG, "PLEASE INPUT ATNF USER NAME\n");
        // Change the prompt.
        readline_print_messages(nmesg, mesgout);
      } else if ((server_response.response_type == RESPONSE_USERREQUEST_SPECTRUM) ||
		 (server_response.response_type == RESPONSE_USERNAME_EXISTS)) {
	// Another NSPD client the same user is controlling has requested data with
	// different options, so we re-request the same data we're viewing now but
	// with those same new options.
	action_required = ACTION_AMPPHASE_OPTIONS_CHANGED | ACTION_OMIT_OPTIONS;
      } else if (server_response.response_type == RESPONSE_SHUTDOWN) {
	// The server is shutting down and wants us to die first so it can close
	// its listening port cleanly.
	action_required = ACTION_QUIT;
      }
      FREE(recv_buffer);
    }
  }

  // Remove our callbacks.
  rl_callback_handler_remove();
  rl_clear_history();
  printf("\n\n NVIS EXITS\n");

  // Release the plotting device.
  release_vis_device(&vis_device_number, &vis_device_opened, &vis_panelspec);

  // Release all the memory.
  // We have to handle freeing header data.
  for (i = 0; i < vis_data.nviscycles; i++) {
    free_scan_header_data(vis_data.header_data[i]);
    FREE(vis_data.header_data[i]);
  }
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
  FREE(visband_idx);
  FREE(yaxis_type);
  for (i = 0; i < n_ampphase_options; i++) {
    free_ampphase_options(ampphase_options[i]);
    FREE(ampphase_options[i]);
  }
  FREE(ampphase_options);
  FREE(nncal_indices);
  FREE(nncal_seconds);
}
