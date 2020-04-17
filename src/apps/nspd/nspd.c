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
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "memory.h"
#include "packing.h"
#include "cpgplot.h"
#include "common.h"

const char *argp_program_version = "nspd 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "new/network SPD";

// Argument description.
static char args_doc[] = "[options]";

// Our options.
static struct argp_option options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "The PGPLOT device to use" },
  { "file", 'f', "FILE", 0, "Use an output file as the input" },
  { 0 }
};

#define SPDBUFSIZE 1024

// The arguments structure.
struct arguments {
  bool use_file;
  char input_file[SPDBUFSIZE];
  char spd_device[SPDBUFSIZE];
};

// And some fun, totally necessary, global state variables.
int action_required;
int spd_device_number;
int xaxis_type, yaxis_type, plot_pols;
struct spd_plotcontrols spd_plotcontrols;
struct panelspec spd_panelspec;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'd':
    strncpy(arguments->spd_device, arg, SPDBUFSIZE);
    break;
  case 'f':
    arguments->use_file = true;
    strncpy(arguments->input_file, arg, SPDBUFSIZE);
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
  splitpanels(5, 5, spd_device_number, 0, 5, &spd_panelspec);
  
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

#define ACTION_REFRESH_PLOT 1<<0
#define ACTION_QUIT         1<<1

// Callback function called for each line when accept-line
// executed, EOF seen, or EOF character read.
static void interpret_command(char *line) {
  if ((line == NULL) || (strcmp(line, "exit") == 0) ||
      (strcmp(line, "quit") == 0)) {
    action_required = ACTION_QUIT;
    if (line == 0) {
      return;
    }
  }

  if (*line) {
    // Keep this history.
    add_history(line);

    // Figure out what we're supposed to do.
    
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
  struct spectrum_data spectrum_data;
  bool spd_device_opened = false;
  struct spd_plotcontrols spd_alteredcontrols;
  fd_set watchset;
  int r;
  
  // Set the default for the arguments.
  arguments.use_file = false;
  arguments.input_file[0] = 0;
  arguments.spd_device[0] = 0;

  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // At the moment we must have a file.
  if (arguments.use_file == false) {
    fprintf(stderr, "NSPD CURRENTLY ONLY SUPPORTS FILE INPUT!\n");
    exit(1);
  }

  // Handle window size changes.
  signal(SIGWINCH, sighandler);

  // Install the line handler.
  rl_callback_handler_install(prompt, interpret_command);
  
  // Open and unpack the file if we have one.
  read_data_from_file(arguments.input_file, &spectrum_data);

  // Open the plotting device.
  prepare_spd_device(arguments.spd_device, &spd_device_opened);
  
  // Let's print something to check.
  //printf("Data has %d IFs and %d pols\n", spectrum_data.num_ifs, spectrum_data.num_pols);

  // Set up our main loop.
  FD_ZERO(&watchset);
  // We'll be looking at the terminal.
  FD_SET(fileno(rl_instream), &watchset);

  // We will need to have a default plot upon entry.
  xaxis_type = PLOT_FREQUENCY;
  yaxis_type = PLOT_AMPLITUDE | PLOT_AMPLITUDE_LINEAR;
  plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  init_spd_plotcontrols(&spd_plotcontrols, xaxis_type, yaxis_type,
			plot_pols, DEFAULT, spd_device_number);

  action_required = ACTION_REFRESH_PLOT;
  while(true) {
    if (action_required & ACTION_REFRESH_PLOT) {
      // Let's make a plot.
      // First, ensure the plotter doesn't try to do something it can't.
      reconcile_spd_plotcontrols(&spectrum_data, &spd_plotcontrols, &spd_alteredcontrols);
      make_spd_plot(spectrum_data.spectrum, &spd_panelspec, &spd_alteredcontrols);
      action_required -= ACTION_REFRESH_PLOT;
    }

    if (action_required & ACTION_QUIT) {
      break;
    }

    // Have a look at our inputs.
    r = select(FD_SETSIZE, &watchset, NULL, NULL, NULL);
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
    if (FD_ISSET(fileno(rl_instream), &watchset)) {
      rl_callback_read_char();
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
