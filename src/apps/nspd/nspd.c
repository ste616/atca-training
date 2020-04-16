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

void prepare_spd_device(char *device_name, bool *device_opened,
			int *device_number, struct panelspec *panelspec) {

  if (!(*device_opened)) {
    // Open the device.
    *device_number = cpgopen(device_name);
    *device_opened = true;
  }

  // Set up the device with the current settings.
  cpgask(0);
  splitpanels(5, 5, *device_number, 0, 5, panelspec);
  
}

void release_spd_device(bool *device_opened, int *device_number,
			struct panelspec *panelspec) {

  if (*device_opened) {
    // Close the device.
    cpgslct(*device_number);
    cpgclos();
    *device_opened = false;
    *device_number = -1;
  }

  free_panelspec(panelspec);
  
}

int main(int argc, char *argv[]) {
  struct arguments arguments;
  struct spectrum_data spectrum_data;
  struct panelspec spd_panelspec;
  struct spd_plotcontrols spd_plotcontrols;
  bool spd_device_opened = false;
  int spd_device_number = -1, xaxis_type, yaxis_type, plot_pols;
  
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

  // Open and unpack the file if we have one.
  read_data_from_file(arguments.input_file, &spectrum_data);

  // Open the plotting device.
  prepare_spd_device(arguments.spd_device, &spd_device_opened,
		     &spd_device_number, &spd_panelspec);
  
  // Let's print something to check.
  printf("Data has %d IFs and %d pols\n", spectrum_data.num_ifs, spectrum_data.num_pols);

  // Let's make a plot.
  xaxis_type = PLOT_FREQUENCY;
  yaxis_type = PLOT_AMPLITUDE | PLOT_AMPLITUDE_LINEAR;
  plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  init_spd_plotcontrols(&spd_plotcontrols, xaxis_type, yaxis_type,
			plot_pols, DEFAULT, spd_device_number);
  make_spd_plot(spectrum_data.spectrum, &spd_panelspec, &spd_plotcontrols);
  
  // Release the plotting device.
  release_spd_device(&spd_device_opened, &spd_device_number, &spd_panelspec);

  // Release all the memory.

}
