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
#include "plotting.h"
#include "atnetworking.h"
#include "atreadline.h"
#include "compute.h"

const char *argp_program_version = "nspd 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char nspd_doc[] = "new/network SPD";

// Argument description.
static char nspd_args_doc[] = "[options]";

// Our options.
static struct argp_option nspd_options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "The PGPLOT device to use" },
  { "default-dump", 'D', "DUMP_TYPE", 0,
    "The plot type to use as default for output files (default: PNG)" },
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { "port", 'p', "PORTNUM", 0, "The port number on the server to connect to" },
  { "server", 's', "SERVER", 0, "The server name or address to connect to" },
  { "username", 'u', "USERNAME", 0, "The username to communicate to the server" },
  { "verbose", 'v', 0, 0, "Output debugging information" },
  { 0 }
};

#define SPDBUFSIZE 1024
#define SPDBUFSHORT 512
#define SPDBUFMEDIUM 768
#define SPDBUFLONG 1280

// The arguments structure.
struct nspd_arguments {
  bool use_file;
  bool debugging_output;
  char input_file[SPDBUFSIZE];
  char spd_device[SPDBUFSIZE];
  int port_number;
  char server_name[SPDBUFSIZE];
  bool network_operation;
  char username[CLIENTIDLENGTH];
  int default_dump;
};

// And some fun, totally necessary, global state variables.
int action_required, server_type, n_ampphase_options;
int xaxis_type, yaxis_type, plot_pols, yaxis_scaling, nxpanels, nypanels, plot_decorations;
double mjd_request, mjd_base;
char hardcopy_filename[SPDBUFSHORT];
struct spd_plotcontrols spd_plotcontrols;
struct panelspec spd_panelspec;
struct spectrum_data spectrum_data;
struct ampphase_options **ampphase_options, *found_options;

// The maximum number of divisions supported on the plot.
#define MAX_XPANELS 7
#define MAX_YPANELS 7
// The number of information lines to use on the plot.
#define NUM_INFO_LINES 4

static error_t nspd_parse_opt(int key, char *arg, struct argp_state *state) {
  struct nspd_arguments *arguments = state->input;

  switch (key) {
  case 'd':
    strncpy(arguments->spd_device, arg, SPDBUFSIZE);
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
    strncpy(arguments->input_file, arg, SPDBUFSIZE);
    break;
  case 'p':
    arguments->port_number = atoi(arg);
    break;
  case 's':
    arguments->network_operation = true;
    strncpy(arguments->server_name, arg, SPDBUFSIZE);
    break;
  case 'u':
    strncpy(arguments->username, arg, CLIENTIDLENGTH);
    break;
  case 'v':
    arguments->debugging_output = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { nspd_options, nspd_parse_opt, nspd_args_doc, nspd_doc };

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

void prepare_spd_device(char *device_name, int *spd_device_number,
			bool *device_opened, struct panelspec *panelspec) {

  if (!(*device_opened)) {
    // Open the device.
    *spd_device_number = cpgopen(device_name);
    *device_opened = true;
  }

  // Set up the device with the current settings.
  cpgask(0);
  panelspec->measured = NO;
  splitpanels(nxpanels, nypanels, *spd_device_number, 0, 5,
	      (NUM_INFO_LINES + 1), panelspec);
  
}

void release_spd_device(int *spd_device_number, bool *device_opened,
			struct panelspec *panelspec) {

  if (*device_opened) {
    // Close the device.
    cpgslct(*spd_device_number);
    cpgclos();
    *device_opened = false;
    *spd_device_number = -1;
  }

  free_panelspec(panelspec);
  
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

#define ACTION_REFRESH_PLOT              1<<0
#define ACTION_QUIT                      1<<1
#define ACTION_CHANGE_PLOTSURFACE        1<<2
#define ACTION_NEW_DATA_RECEIVED         1<<3
#define ACTION_CYCLE_FORWARD             1<<4
#define ACTION_CYCLE_BACKWARD            1<<5
#define ACTION_LIST_CYCLES               1<<6
#define ACTION_TIME_REQUEST              1<<7
#define ACTION_OMIT_OPTIONS              1<<8
#define ACTION_UNKNOWN_COMMAND           1<<9
#define ACTION_HARDCOPY_PLOT             1<<10

// Make a shortcut to stop action for those actions which can only be
// done by a simulator.
#define CHECKSIMULATOR				\
  do						\
    {						\
      if (server_type != SERVERTYPE_SIMULATOR)	\
	{					\
	  FREE(line_els);			\
	  FREE(line);				\
	  return;				\
	}					\
    }						\
  while(0)

#define TIMEFILE_LENGTH 18
// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  char **line_els = NULL, *cvt = NULL, *cycomma = NULL;
  char delim[] = " ", ttime[TIMEFILE_LENGTH];
  int nels = -1, i, j, k, pols_specified, if_no, iarg, bidx;
  int if_num_spec[MAXIFS], yaxis_change_type, array_change_spec;
  int flag_change, flag_change_mode, change_nxpanels, change_nypanels;
  int min_chan, max_chan, drange_swap;
  bool pols_selected = false, if_selected = false, range_valid = false;
  bool mjdr_timeparsed, decorations_changed = false, chan_range_valid = false;
  float range_limit_low, range_limit_high, range_swap, mjdr_seconds;
  double mjdr_base;
  
  if (line == NULL) {
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
    next_history();

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
    if (minmatch("exit", line_els[0], 4) ||
	minmatch("quit", line_els[0], 4)) {
      action_required = ACTION_QUIT;
    } else if (minmatch("select", line_els[0], 3)) {
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
        change_spd_plotcontrols(&spd_plotcontrols, NULL, NULL, &pols_specified, NULL);
        action_required = ACTION_REFRESH_PLOT;
      }
      if (if_selected) {
        // Reset the IF selection.
        for (i = 0; i < MAXIFS; i++) {
          spd_plotcontrols.if_num_spec[i] = if_num_spec[i];
        }
        action_required = ACTION_REFRESH_PLOT;
      }
    } else if (minmatch("channel", line_els[0], 2)) {
      // We've been asked to change channel range.
      if (nels == 1) {
	// Reset all channel ranges back to their defaults.
	for (i = 0; i < MAXIFS; i++) {
	  spd_plotcontrols.channel_range_limit[i] = 0;
	}
	action_required = ACTION_REFRESH_PLOT;
      } else if ((nels == 3) || (nels == 4)) {
	// Change the channel range of some IFs.
	cvt = NULL;
	chan_range_valid = true;
	min_chan = strtod(line_els[nels - 2], &cvt);
	if ((cvt == line_els[nels - 2]) || (errno == ERANGE)) {
	  chan_range_valid = false;
	}
	cvt = NULL;
	max_chan = strtod(line_els[nels - 1], &cvt);
	if ((cvt == line_els[nels - 1]) || (errno == ERANGE)) {
	  chan_range_valid = false;
	}
	if (chan_range_valid == true) {
	  if (min_chan > max_chan) {
	    // Silly user, swap the numbers for them.
	    drange_swap = min_chan;
	    min_chan = max_chan;
	    max_chan = drange_swap;
	  }
	  if (nels == 3) {
	    // Change all channel ranges for the currently displayed IFs.
	    for (i = 0; i < MAXIFS; i++) {
	      if (spd_plotcontrols.if_num_spec[i] == 1) {
		spd_plotcontrols.channel_range_limit[i] = 1;
		spd_plotcontrols.channel_range_min[i] = min_chan;
		spd_plotcontrols.channel_range_max[i] = max_chan;
		action_required = ACTION_REFRESH_PLOT;
	      }
	    }
	  } else if (nels == 4) {
	    // We've been given an IF to change, so we find it first.
	    if_no = find_if_name(spectrum_data.header_data, line_els[1]);
	    if ((if_no >= 0) && (if_no < MAXIFS)) {
	      spd_plotcontrols.channel_range_limit[if_no - 1] = 1;
	      spd_plotcontrols.channel_range_min[if_no - 1] = min_chan;
	      spd_plotcontrols.channel_range_max[if_no - 1] = max_chan;
	      action_required = ACTION_REFRESH_PLOT;
	    }
	  }
	}
      }
    
    } else if ((strcasecmp(line_els[0], "x") == 0) &&
               (nels == 1)) {
      // Change between frequency and channel X axes.
      if (xaxis_type == PLOT_FREQUENCY) {
        xaxis_type = PLOT_CHANNEL;
      } else if (xaxis_type == PLOT_CHANNEL) {
        xaxis_type = PLOT_FREQUENCY;
      }
      change_spd_plotcontrols(&spd_plotcontrols, &xaxis_type, NULL, NULL, NULL);
      action_required = ACTION_REFRESH_PLOT;
    } else if ((minmatch("phase", line_els[0], 1)) ||
               (minmatch("amplitude", line_els[0], 1)) ||
               (minmatch("real", line_els[0], 1)) ||
               (minmatch("imaginary", line_els[0], 1)) ||
	       (minmatch("delay", line_els[0], 5))) {
      // We've been asked to show phase, amplitude, real or imaginary, or delay.
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
      } else if (minmatch("real", line_els[0], 1)) {
        yaxis_type = PLOT_REAL;
        yaxis_change_type = yaxis_type | yaxis_scaling;
      } else if (minmatch("imaginary", line_els[0], 1)) {
        yaxis_type = PLOT_IMAG;
        yaxis_change_type = yaxis_type | yaxis_scaling;
      } else if (minmatch("delay", line_els[0], 5)) {
	yaxis_type = PLOT_DELAY;
	yaxis_change_type = yaxis_type;
      }
      change_spd_plotcontrols(&spd_plotcontrols, NULL, &yaxis_change_type, NULL, NULL);
      action_required = ACTION_REFRESH_PLOT;
    } else if (minmatch("scale", line_els[0], 3)) {
      if (nels == 2) {
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
	  change_spd_plotcontrols(&spd_plotcontrols, NULL, &yaxis_change_type, NULL, NULL);
	  action_required = ACTION_REFRESH_PLOT;
	}
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
    } else if (minmatch("forward", line_els[0], 4)) {
      // Move forward one cycle in time.
      CHECKSIMULATOR;
      action_required |= ACTION_CYCLE_FORWARD;
    } else if (minmatch("backward", line_els[0], 4)) {
      // Move backward one cycle in time.
      CHECKSIMULATOR;
      action_required |= ACTION_CYCLE_BACKWARD;
    } else if (minmatch("list", line_els[0], 3)) {
      // List all the cycle times we know about from the server.
      CHECKSIMULATOR;
      action_required |= ACTION_LIST_CYCLES;
    } else if (minmatch("get", line_els[0], 3)) {
      // Get something.
      if (minmatch("time", line_els[1], 3)) {
        // The user wants to request a time in the format [yyyy-mm-dd] HH:MM:SS.
        if (nels == 4) {
          // Both date and time are given.
          mjdr_base = date2mjd(line_els[2], 0);
          if (mjdr_base == 0) {
            // The date wasn't specified correctly.
            fprintf(stderr, " DATE SPECIFIED INCORRECTLY, MUST BE YYYY-MM-DD\n");
            return;
          }
        } else if (nels == 3) {
          // Just the time is given. So we reference it to the date of the
          // first scan.
          mjdr_base = mjd_base;
        }
        // Scan the time now.
        mjdr_timeparsed = string_to_seconds(line_els[nels - 1], &mjdr_seconds);
        if (mjdr_timeparsed == false) {
          fprintf(stderr, " TIME SPECIFIED INCORRECTLY, MUST BE HH:MM[:SS]\n");
          return;
        }
        mjd_request = mjdr_base + (double)mjdr_seconds / 86400.0;
        action_required |= ACTION_TIME_REQUEST;
        /* fprintf(stderr, " PARSED REQUESTED TIME AS %.8f\n", mjd_request); */
      }
    } else if (minmatch("show", line_els[0], 3)) {
      // Change the plot decorations.
      if (nels > 1) {
	// Read what they want to show.
	if (minmatch("tvchannels", line_els[1], 4)) {
	  // Show the tvchannel decorations.
	  plot_decorations |= PLOT_TVCHANNELS;
	  decorations_changed = true;
	} else if (minmatch("averaged", line_els[1], 2)) {
	  // Show the averaged channels.
	  plot_decorations |= PLOT_AVERAGED_DATA;
	  decorations_changed = true;
	}

	if (decorations_changed) {
	  change_spd_plotcontrols(&spd_plotcontrols, NULL, NULL, NULL,
				  &plot_decorations);
	  action_required = ACTION_REFRESH_PLOT;
	}
      }
    } else if (minmatch("hide", line_els[0], 3)) {
      if (nels > 1) {
	// Read what they want to hide.
	if (minmatch("tvchannels", line_els[1], 4)) {
	  // Hide the tvchannel decorations.
	  if (plot_decorations & PLOT_TVCHANNELS) {
	    plot_decorations -= PLOT_TVCHANNELS;
	    decorations_changed = true;
	  }
	} else if (minmatch("averaged", line_els[1], 2)) {
	  // Hide the averaged channels.
	  if (plot_decorations & PLOT_AVERAGED_DATA) {
	    plot_decorations -= PLOT_AVERAGED_DATA;
	    decorations_changed = true;
	  }
	}

	if (decorations_changed) {
	  change_spd_plotcontrols(&spd_plotcontrols, NULL, NULL, NULL,
				  &plot_decorations);
	  action_required = ACTION_REFRESH_PLOT;
	}
      }
    } else if (minmatch("delavg", line_els[0], 5)) {
      CHECKSIMULATOR;
      // Change the delay averaging.
      if (nels == 2) {
	iarg = atoi(line_els[1]);
	if ((iarg >= 1) && (found_options != NULL)) {
	  // Change the delay averaging for all the IFs, but only for
	  // the currently displayed options.
	  for (i = 0; i < found_options->num_ifs; i++) {
	    found_options->delay_averaging[i] = iarg;
	  }
	  action_required = ACTION_TIME_REQUEST;
	}
      } else if (nels == 3) {
	// The first argument should be the name of the band to change.
	bidx = find_if_name_nosafe(spectrum_data.header_data, line_els[1]);
	if (bidx >= 0) {
	  iarg = atoi(line_els[2]);
	  if ((iarg >= 1) && (found_options != NULL)) {
	    found_options->delay_averaging[bidx] = iarg;
	    action_required = ACTION_TIME_REQUEST;
	  }
	} else {
	  fprintf(stderr, "Couldn't find %s\n", line_els[1]);
	}
      }
    } else if (minmatch("dump", line_els[0], 4)) {
      // Make a hardcopy version of the plot.
      if (nels == 1) {
	// We automatically generate a file name, based on the current time.
	current_time_string(ttime, TIMEFILE_LENGTH);
	snprintf(hardcopy_filename, SPDBUFSHORT, "nspd_plot_%s", ttime);
      } else if (nels == 2) {
	// We've been given a file name to use.
	strncpy(hardcopy_filename, line_els[1], SPDBUFSHORT);
      }
      action_required = ACTION_HARDCOPY_PLOT;
    } else {
      action_required = ACTION_UNKNOWN_COMMAND;
    }
    FREE(line_els);
  }

  // We need to free the memory.
  FREE(line);
}

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
    // Also, set which channels to plot.
    data_plotcontrols->channel_range_limit[i] = 0;
    if (user_plotcontrols->channel_range_limit[i]) {
      // The user wants to limit the channels.
      if (i < spectrum_data->num_ifs) {
	// This IF is in the data.
	data_plotcontrols->channel_range_limit[i] = 1;
	STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_min[i]);
	STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_max[i]);
      }
    }
  }

  // Copy over the other parameters.
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, plot_options);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, plot_flags);
  /* STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_limit); */
  /* STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_min); */
  /* STRUCTCOPY(user_plotcontrols, data_plotcontrols, channel_range_max); */
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_limit);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_min);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, yaxis_range_max);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, array_spec);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, npols);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, interactive);
  STRUCTCOPY(user_plotcontrols, data_plotcontrols, pgplot_device);
  
}

int main(int argc, char *argv[]) {
  struct nspd_arguments arguments;
  bool spd_device_opened = false, action_proceed = false, dump_device_opened = false;
  struct spd_plotcontrols spd_alteredcontrols;
  fd_set watchset, reads;
  int i, r, bytes_received, max_socket = -1, nmesg = 0, n_cycles = 0;
  int nlistlines = 0, mjd_year, mjd_month, mjd_day, min_dmjd_idx = -1;
  int pending_action = -1, dump_type = FILETYPE_UNKNOWN, dump_device_number = -1;
  int spd_device_number = -1;
  float mjd_utseconds;
  size_t recv_buffer_length;
  struct requests server_request;
  struct responses server_response;
  char send_buffer[SENDBUFSIZE], client_id[CLIENTIDLENGTH];
  char *recv_buffer = NULL, **mesgout = NULL, tstring[20];
  char dump_device[SPDBUFMEDIUM], dump_file[SPDBUFSIZE];
  SOCKET socket_peer;
  cmp_ctx_t cmp;
  cmp_mem_access_t mem;
  double earliest_mjd, latest_mjd, mjd_cycletime, cmjd, min_dmjd, dmjd;
  double *all_cycle_mjd = NULL;
  struct syscal_data *tsys_data;
  struct panelspec dump_panelspec;
  
  // Allocate some memory.
  MALLOC(mesgout, MAX_N_MESSAGES);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    CALLOC(mesgout[i], SPDBUFLONG);
  }
  
  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.debugging_output = false;
  arguments.input_file[0] = 0;
  arguments.spd_device[0] = 0;
  arguments.server_name[0] = 0;
  arguments.port_number = 8880;
  arguments.network_operation = false;
  arguments.username[0] = 0;
  arguments.default_dump = FILETYPE_PNG;
  
  // And defaults for some of the parameters.
  nxpanels = 5;
  nypanels = 5;
  n_ampphase_options = 0;
  ampphase_options = NULL;

  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Generate our ID.
  generate_client_id(client_id, CLIENTIDLENGTH);
  printf("client ID = %s\n", client_id);
  
  // Handle window size changes.
  signal(SIGWINCH, sighandler);

  // Open and unpack the file if we have one.
  if (arguments.use_file) {
    read_data_from_file(arguments.input_file, &spectrum_data);
  } else if (arguments.network_operation) {
    // Prepare our connection.
    if (!prepare_client_connection(arguments.server_name, arguments.port_number,
                                   &socket_peer, arguments.debugging_output)) {
      return(1);
    }
    // Ask what type of server we're connecting to.
    server_request.request_type = REQUEST_SERVERTYPE;
    strncpy(server_request.client_id, client_id, CLIENTIDLENGTH);
    strncpy(server_request.client_username, arguments.username, CLIENTIDLENGTH);
    server_request.client_type = CLIENTTYPE_NSPD;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
    // Send a request for the currently available spectrum.
    server_request.request_type = REQUEST_CURRENT_SPECTRUM;
    init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
    pack_requests(&cmp, &server_request);
    socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
  }

  // Open the plotting device.
  prepare_spd_device(arguments.spd_device, &spd_device_number, &spd_device_opened,
		     &spd_panelspec);

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
  xaxis_type = PLOT_FREQUENCY;
  yaxis_type = PLOT_AMPLITUDE;
  yaxis_scaling = PLOT_AMPLITUDE_LINEAR;
  plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  plot_decorations = PLOT_TVCHANNELS;
  init_spd_plotcontrols(&spd_plotcontrols, xaxis_type, yaxis_type | yaxis_scaling,
                        plot_pols, plot_decorations, spd_device_number);
  // The number of pols is set by the data though, not the selection.
  action_required = 0;
  while(true) {
    reads = watchset;
    /* nmesg = 0; */
    /* snprintf(mesgout[nmesg++], SPDBUFSIZE, " ACTION_REQUIRED = %d  PENDING_ACTION = %d\n", */
    /* 	     action_required, pending_action); */
    /* readline_print_messages(nmesg, mesgout); */
    if (action_required & ACTION_NEW_DATA_RECEIVED) {
      // Adjust the number of pols.
      spd_plotcontrols.npols = spectrum_data.num_pols;
      action_required -= ACTION_NEW_DATA_RECEIVED;
      action_required |= ACTION_CHANGE_PLOTSURFACE;
      // Work out the MJD.
      if (spectrum_data.header_data != NULL) {
        cmjd = date2mjd(spectrum_data.header_data->obsdate,
                        spectrum_data.spectrum[0][0]->ut_seconds);
        /* nmesg = 1; */
        /* snprintf(mesgout[0], SPDBUFSIZE, " Data has MJD %.8f\n", cmjd); */
	// Store this as if we had requested that MJD.
	mjd_request = cmjd;
        /* readline_print_messages(nmesg, mesgout); */
      }
    }
    
    if (action_required & ACTION_CHANGE_PLOTSURFACE) {
      // The plotting window needs to be split into a different set of windows.
      if (arguments.debugging_output) {
        fprintf(stderr, "Changing plot surface.\n");
      }
      free_panelspec(&spd_panelspec);
      splitpanels(nxpanels, nypanels, spd_device_number, 0, 5,
		  (NUM_INFO_LINES + 1), &spd_panelspec);
      action_required -= ACTION_CHANGE_PLOTSURFACE;
      action_required |= ACTION_REFRESH_PLOT;
    }

    if ((action_required & ACTION_HARDCOPY_PLOT) ||
	(action_required & ACTION_REFRESH_PLOT)) {
      // Let's make a plot.
      if (arguments.debugging_output) {
	fprintf(stderr, "Refreshing plot.\n");
      }
      // First, ensure the plotter doesn't try to do something it can't.
      reconcile_spd_plotcontrols(&spectrum_data, &spd_plotcontrols, &spd_alteredcontrols);
      // Get the Tsys.
      spectrum_data_compile_system_temperatures(&spectrum_data, &tsys_data);

      if (action_required & ACTION_HARDCOPY_PLOT) {
	nmesg = 0;
	// Open a new PGPLOT device.
	dump_type = filename_to_pgplot_device(hardcopy_filename, dump_device,
					      SPDBUFSIZE, arguments.default_dump,
					      dump_file, SPDBUFSIZE);
	if (dump_type != FILETYPE_UNKNOWN) {
	  // Open the device.
	  prepare_spd_device(dump_device, &dump_device_number, &dump_device_opened,
			     &dump_panelspec);
	  if (dump_device_opened) {
	    // The the plotter to use the device.
	    spd_alteredcontrols.pgplot_device = dump_device_number;
	    // Make the plot.
	    make_spd_plot(spectrum_data.spectrum, &dump_panelspec, &spd_alteredcontrols,
			  spectrum_data.header_data, tsys_data, 2, true);
	    // Close the device.
	    release_spd_device(&dump_device_number, &dump_device_opened, &dump_panelspec);
	    // Reset the plot controls.
	    spd_alteredcontrols.pgplot_device = spd_device_number;
	    snprintf(mesgout[nmesg++], SPDBUFLONG, " NSPD output to file %s\n",
		     dump_file);
	  } else {
	    // Print an error.
	    snprintf(mesgout[nmesg++], SPDBUFLONG, " NSPD NOT ABLE TO OUTPUT TO %s\n",
		     hardcopy_filename);
	  }
	} else {
	  // Print an error.
	  snprintf(mesgout[nmesg++], SPDBUFLONG, " UNKNOWN OUTPUT %s\n", hardcopy_filename);
	}
	action_required -= ACTION_HARDCOPY_PLOT;
	readline_print_messages(nmesg, mesgout);
      }
      
      if (action_required & ACTION_REFRESH_PLOT) {
	make_spd_plot(spectrum_data.spectrum, &spd_panelspec, &spd_alteredcontrols,
		      spectrum_data.header_data, tsys_data, 2, true);
	action_required -= ACTION_REFRESH_PLOT;
      }

      free_syscal_data(tsys_data);
      FREE(tsys_data);
    }

    if ((pending_action >= 0) && (n_cycles > 0)) {
      // Go back to request the new time.
      action_required |= pending_action;
      pending_action = -1;
    }
    
    if ((action_required & ACTION_CYCLE_FORWARD) ||
        (action_required & ACTION_CYCLE_BACKWARD) ||
        (action_required & ACTION_TIME_REQUEST)) {
      if (n_cycles <= 0) {
	if (action_required & ACTION_CYCLE_FORWARD) {
	  pending_action = ACTION_CYCLE_FORWARD;
	} else if (action_required & ACTION_CYCLE_BACKWARD) {
	  pending_action = ACTION_CYCLE_BACKWARD;
	} else if (action_required & ACTION_TIME_REQUEST) {
	  pending_action = ACTION_TIME_REQUEST;
	}
	if (action_required & ACTION_OMIT_OPTIONS) {
	  pending_action |= ACTION_OMIT_OPTIONS;
	}
	action_required -= pending_action;
      }
      // Ask for different data.
      if ((action_required & ACTION_CYCLE_FORWARD) ||
          (action_required & ACTION_CYCLE_BACKWARD)) {
        // First, work out which cycle we're currently looking at.
        for (i = 0, action_proceed = false; i < n_cycles; i++) {
          if ((cmjd >= (all_cycle_mjd[i] - mjd_cycletime / 2.0)) &&
              (cmjd < (all_cycle_mjd[i] + mjd_cycletime / 2.0))) {
            // Now select the MJD from the appropriate cycle.
            if ((action_required & ACTION_CYCLE_FORWARD) &&
                (i < (n_cycles -1))) {
              mjd_request = all_cycle_mjd[i + 1];
              action_proceed = true;
              break;
            } else if ((action_required & ACTION_CYCLE_BACKWARD) &&
                       (i > 0)) {
              mjd_request = all_cycle_mjd[i - 1];
              action_proceed = true;
              break;
            }
          }
        }
        // Don't do it again, no matter what.
        if (action_required & ACTION_CYCLE_FORWARD) {
          action_required -= ACTION_CYCLE_FORWARD;
        } else if (action_required & ACTION_CYCLE_BACKWARD) {
          action_required -= ACTION_CYCLE_BACKWARD;
        }
      } else if (action_required & ACTION_TIME_REQUEST) {
        // Check that it isn't outside the range.
        nmesg = 0;
        action_proceed = false;
        if (mjd_request < (earliest_mjd - 2 * mjd_cycletime)) {
          snprintf(mesgout[nmesg++], SPDBUFSIZE, " REQUESTED MJD %.8f IS TOO EARLY\n",
                   mjd_request);
        } else if (mjd_request > (latest_mjd + 2 * mjd_cycletime)) {
          snprintf(mesgout[nmesg++], SPDBUFSIZE, " REQUESTED MJD %.8f IS TOO LATE\n",
                   mjd_request);
        } else {
          // Try to find the nearest cycle.
          min_dmjd = fabs(mjd_request - all_cycle_mjd[0]);
          min_dmjd_idx = 0;
          for (i = 1; i < n_cycles; i++) {
            dmjd = fabs(mjd_request - all_cycle_mjd[i]);
            if (dmjd < min_dmjd) {
              min_dmjd = dmjd;
              min_dmjd_idx = i;
            }
          }
          if (min_dmjd < (2 * mjd_cycletime)) {
            action_proceed = true;
            mjd_request = all_cycle_mjd[min_dmjd_idx];
          }
        }
        action_required -= ACTION_TIME_REQUEST;
        if (nmesg > 0) {
          readline_print_messages(nmesg, mesgout);
        }
      }
      if (action_proceed) {
        // We could check this against the known range here, but for now
        // let's just request the data (the server also has range checking).
        server_request.request_type = REQUEST_SPECTRUM_MJD;
        init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
        pack_requests(&cmp, &server_request);
        /* nmesg = 1; */
        /* snprintf(mesgout[0], SPDBUFSIZE, " Requesting data at MJD %.8f\n", mjd_request); */
        /* readline_print_messages(nmesg, mesgout); */
        pack_write_double(&cmp, mjd_request);
	if (action_required & ACTION_OMIT_OPTIONS) {
	  // We will not send any options, so that options being used by our
	  // user in some other client will be used automatically by the server.
	  pack_write_sint(&cmp, 0);
	  action_required -= ACTION_OMIT_OPTIONS;
	} else {
	  // We have to send along the ampphase options as well.
	  pack_write_sint(&cmp, n_ampphase_options);
	  for (i = 0; i < n_ampphase_options; i++) {
	    pack_ampphase_options(&cmp, ampphase_options[i]);
	  }
	}
        socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      }
    }
    
    if (action_required & ACTION_LIST_CYCLES) {
      // List all the cycles we've been told about by the server.
      for (i = 0, nmesg = 0, nlistlines = 2; i < n_cycles; i++) {
        if ((i > 0) && (nlistlines == -1) &&
            ((all_cycle_mjd[i] - all_cycle_mjd[i - 1]) > (2 * mjd_cycletime))) {
          // We go back and print out the last two cycles and then the first
          // two new cycles.
          i -= 2;
          nlistlines = 4;
        } else if ((i == (n_cycles - 2)) && (nlistlines == -1)) {
          // Print out the last two cycles.
          nlistlines = 2;
        }
        if (nlistlines > 0) {
          mjd2cal(all_cycle_mjd[i], &mjd_year, &mjd_month, &mjd_day, &mjd_utseconds);
          seconds_to_hourlabel((mjd_utseconds * 86400), tstring);
          snprintf(mesgout[nmesg++], SPDBUFSIZE, " CYCLE %d: %4d-%02d-%02d %s\n",
                   (i + 1), mjd_year, mjd_month, mjd_day, tstring);
          nlistlines--;
        } else if (nlistlines == 0) {
          snprintf(mesgout[nmesg++], SPDBUFSIZE, " ....\n");
          nlistlines--;
        }
      }
      readline_print_messages(nmesg, mesgout);
      
      action_required -= ACTION_LIST_CYCLES;
    }

    if (action_required & ACTION_UNKNOWN_COMMAND) {
      nmesg = 0;
      snprintf(mesgout[nmesg++], SPDBUFSIZE, "  UNKNOWN COMMAND!\n");
      readline_print_messages(nmesg, mesgout);
      action_required -= ACTION_UNKNOWN_COMMAND;
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
      if ((server_response.response_type == RESPONSE_CURRENT_SPECTRUM) ||
          (server_response.response_type == RESPONSE_LOADED_SPECTRUM)) {
	// First the options. Free the old ones if we need to first.
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
	// Print out the options.
	/* nmesg = 0; */
	/* snprintf(mesgout[nmesg++], SPDBUFSIZE, "OPTIONS RECEIVED:\n"); */
	/* print_options_set(n_ampphase_options, ampphase_options, mesgout[nmesg++], */
	/* 		  SPDBUFSIZE); */
	// Free any old spectrum data.
	free_spectrum_data(&spectrum_data);
	free_scan_header_data(spectrum_data.header_data);
	FREE(spectrum_data.header_data);
	// And then get the spectrum data.
        unpack_spectrum_data(&cmp, &spectrum_data);
	// Find the options relevant to the displayed data.
	found_options = find_ampphase_options(n_ampphase_options, ampphase_options,
					      spectrum_data.header_data);
	/* snprintf(mesgout[nmesg++], SPDBUFSIZE, "RELEVANT OPTIONS:\n"); */
	/* print_options_set(1, &found_options, mesgout[nmesg++], SPDBUFSIZE); */
	/* readline_print_messages(nmesg, mesgout); */
        // Refresh the plot next time through.
        action_required = ACTION_NEW_DATA_RECEIVED;
      } else if (server_response.response_type == RESPONSE_SERVERTYPE) {
        // We're being told what type of server we've connected to.
        pack_read_sint(&cmp, &server_type);
        nmesg = 1;
        snprintf(mesgout[0], SPDBUFSIZE, "Connected to %s server.\n",
                 get_servertype_string(server_type));
        readline_print_messages(nmesg, mesgout);
        if (server_type == SERVERTYPE_SIMULATOR) {
          // Send a request for the time information.
          server_request.request_type = REQUEST_TIMERANGE;
          init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
          pack_requests(&cmp, &server_request);
          socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
          server_request.request_type = REQUEST_CYCLE_TIMES;
          init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
          pack_requests(&cmp, &server_request);
          socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
        }
      } else if (server_response.response_type == RESPONSE_TIMERANGE) {
        // Just store these values for later use.
        pack_read_double(&cmp, &mjd_cycletime);
        pack_read_double(&cmp, &earliest_mjd);
        pack_read_double(&cmp, &latest_mjd);
        /* nmesg = 3; */
        /* snprintf(mesgout[0], SPDBUFSIZE, " Server earliest MJD %.8f\n", earliest_mjd); */
        /* snprintf(mesgout[1], SPDBUFSIZE, " Server   latest MJD %.8f\n", latest_mjd); */
        /* snprintf(mesgout[2], SPDBUFSIZE, " Data cycle time %.1f sec\n", */
        /*          (mjd_cycletime * 86400.0)); */
        /* readline_print_messages(nmesg, mesgout); */
        mjd_base = floor(earliest_mjd);
      } else if (server_response.response_type == RESPONSE_SPECTRUM_OUTSIDERANGE) {
        // We couldn't get a new spectrum.
        nmesg = 1;
        snprintf(mesgout[0], SPDBUFSIZE, " SERVER UNABLE TO SUPPLY SPECTRUM\n");
        readline_print_messages(nmesg, mesgout);
      } else if (server_response.response_type == RESPONSE_SPECTRUM_LOADED) {
        // Our new spectrum is ready, so we request it.
        server_request.request_type = REQUEST_MJD_SPECTRUM;
        init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)SENDBUFSIZE);
        pack_requests(&cmp, &server_request);
        socket_send_buffer(socket_peer, send_buffer, cmp_mem_access_get_pos(&mem));
      } else if (server_response.response_type == RESPONSE_CYCLE_TIMES) {
        // All the cycles the server knows about.
        pack_read_sint(&cmp, &n_cycles);
        REALLOC(all_cycle_mjd, n_cycles);
        pack_readarray_double(&cmp, n_cycles, all_cycle_mjd);
        /* nmesg = 1; */
        /* snprintf(mesgout[0], SPDBUFSIZE, " Received information about %d cycles.\n", */
        /*          n_cycles); */
        /* readline_print_messages(nmesg, mesgout); */
      } else if ((server_response.response_type == RESPONSE_USERREQUEST_VISDATA) ||
		 (server_response.response_type == RESPONSE_USERNAME_EXISTS)) {
	// Another NVIS client the same user is controlling has requested data with
	// different options, so we re-request the same data we're viewing now but
	// with those same new options.
	action_required = ACTION_TIME_REQUEST | ACTION_OMIT_OPTIONS;
      } else if (server_response.response_type == RESPONSE_SHUTDOWN) {
	// The server is shutting down and wants us to die first so it can close
	// its listening port cleanly.
	action_required = ACTION_QUIT;
      }
      // Free the socket buffer memory.
      FREE(recv_buffer);
    }
    
  }

  // Remove our callbacks.
  rl_callback_handler_remove();
  // And clear the history.
  rl_clear_history();
  printf("\n\n  NSPD EXITS\n");
  
  // Release the plotting device.
  release_spd_device(&spd_device_number, &spd_device_opened, &spd_panelspec);

  // Release all the memory.
  free_spectrum_data(&spectrum_data);
  free_scan_header_data(spectrum_data.header_data);
  FREE(spectrum_data.header_data);
  FREE(all_cycle_mjd);
  for (i = 0; i < MAX_N_MESSAGES; i++) {
    FREE(mesgout[i]);
  }
  FREE(mesgout);
  if (n_ampphase_options > 0) {
    for (i = 0; i < n_ampphase_options; i++) {
      free_ampphase_options(ampphase_options[i]);
      FREE(ampphase_options[i]);
    }
    FREE(ampphase_options);
  }
  
}
