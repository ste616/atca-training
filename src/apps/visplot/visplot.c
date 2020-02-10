/**
 * ATCA Training Application: summariser.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This app summarises plots out visibilities from an RPFITS file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>
#include <argp.h>
#include "atrpfits.h"
#include "memory.h"
#include "cpgplot.h"
#include "common.h"

const char *argp_program_version = "visplot 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "visplot plotter for RPFITS files";

// Argument description.
static char args_doc[] = "[options] RPFITS_FILES...";

// Our options.
static struct argp_option options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "Direct plots to this PGGPLOT device" },
  { "phase", 'p', 0, 0, "Plots phase on y-axis" },
  { 0 }
};

// Argp stuff.
struct arguments {
  char pgplot_device[BUFSIZE];
  char **rpfits_files;
  int n_rpfits_files;
  int plot_phase;
  int plot_pols;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'd':
    strncpy(arguments->pgplot_device, arg, BUFSIZE);
    break;
  case 'p':
    arguments->plot_phase = YES;
    break;
    
  case ARGP_KEY_ARG:
    arguments->n_rpfits_files += 1;
    REALLOC(arguments->rpfits_files, arguments->n_rpfits_files);
    arguments->rpfits_files[arguments->n_rpfits_files - 1] = arg;
    break;
    
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, l = 0, m = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no, read_response = 0;
  int plotif = 0, r = 0, ant1, ant2, num_ifs = 2, num_pols = 2;
  int p = 0, q = 0, sp = 0, rp, ri, rx, ry, yaxis_type;
  float min_vis, max_vis, *xpts = NULL, theight = 0.4;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  struct ampphase ***cycle_ampphase = NULL;
  struct vis_quantities *cycle_vis_quantities = NULL;
  char ptitle[BUFSIZE];
  struct ampphase_options ampphase_options;
  struct panelspec panelspec;
  struct arguments arguments;
  struct plotcontrols plotcontrols;
  
  // Set some defaults and parse our command line options.
  arguments.pgplot_device[0] = '\0';
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;
  arguments.plot_phase = NO;
  arguments.plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
  cpgopen(arguments.pgplot_device);
  cpgask(1);

  // Get phase in degrees.
  ampphase_options.phase_in_degrees = true;
  ampphase_options.delay_averaging = 1;
  ampphase_options.min_tvchannel = 513;
  ampphase_options.max_tvchannel = 1537;
  ampphase_options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_SCALAR;

  // We will get data for all IFs and pols.
  MALLOC(cycle_ampphase, num_ifs);
  for (i = 0; i < num_ifs; i++) {
    MALLOC(cycle_ampphase[i], num_pols);
    for (j = 0; j < num_pols; j++) {
      cycle_ampphase[i][j] = NULL;
    }
  }

  // Initialise the plotting space and options.
  splitpanels(5, 5, &panelspec);
  if (arguments.plot_phase == NO) {
    yaxis_type = PLOT_AMPLITUDE | PLOT_AMPLITUDE_LINEAR;
  } else {
    yaxis_type = PLOT_PHASE;
  }
  
  init_plotcontrols(&plotcontrols, DEFAULT, yaxis_type,
		    arguments.plot_pols, DEFAULT);
  
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    // Try to open the RPFITS file.
    res = open_rpfits_file(arguments.rpfits_files[i]);
    printf("Attempt to open RPFITS file %s, %d\n", argv[i], res);

    while (keep_reading) {
      // Make a new scan and add it to the list.
      scan_data = prepare_new_scan_data();
      nscans += 1;
      ARRAY_APPEND(all_scans, nscans, scan_data);

      // Read in the scan header.
      read_response = read_scan_header(&(scan_data->header_data));
      printf("scan has obs date %s, time %.1f\n",
	     scan_data->header_data.obsdate,
	     scan_data->header_data.ut_seconds);
      printf("  type %s, source %s, calcode %s\n",
	     scan_data->header_data.obstype,
	     scan_data->header_data.source_name,
	     scan_data->header_data.calcode);
      printf("  coordinates RA = %.4f, Dec = %.4f\n",
	     scan_data->header_data.rightascension_hours,
	     scan_data->header_data.declination_degrees);
      if (read_response & READER_DATA_AVAILABLE) {
	  // Now start reading the cycle data.
	read_cycle = 1;
	while (read_cycle) {
	  cycle_data = scan_add_cycle(scan_data);
	  read_response = read_cycle_data(&(scan_data->header_data),
					  cycle_data);
	  //fprintf(stderr, "found read response %d\n", read_response);
	  //printf("cycle has %d points\n", cycle_data->num_points);
	  if (!(read_response & READER_DATA_AVAILABLE)) {
	    read_cycle = 0;
	  }
	}
      }
      for (k = 0; k < scan_data->num_cycles; k++) {
	cycle_data = scan_data->cycles[k];
	for (q = 0; q < num_ifs; q++) {
	  for (p = 0; p < num_pols; p++) {
	    switch(p) {
	    case 0:
	      sp = POL_XX;
	      break;
	    case 1:
	      sp = POL_YY;
	      break;
	    default:
	      sp = POL_XX;
	    }
	    r = vis_ampphase(&(scan_data->header_data), cycle_data,
			     &(cycle_ampphase[q][p]), sp, q, 1,
			     &ampphase_options);
	    if (r < 0) {
	      printf("error encountered while calculating amp and phase\n");
	      free_ampphase(&(cycle_ampphase[q][p]));
	      exit(0);
	    }
	  }
	}
	make_plot(cycle_ampphase, &panelspec, &plotcontrols);
	for (q = 0; q < num_ifs; q++) {
	  for (p = 0; p < num_pols; p++) {
	    free_ampphase(&(cycle_ampphase[q][p]));
	  }
	}
	/* free_vis_quantities(&cycle_vis_quantities); */
      }
    
      if (read_response == READER_EXHAUSTED) {
	// No more data in this file.
	keep_reading = 0;
      } // Otherwise we've probably hit another header.
      printf("scan had %d cycles\n", scan_data->num_cycles);

    }

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);

  }

  cpgclos();
  
  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  
  exit(0);
  
}
