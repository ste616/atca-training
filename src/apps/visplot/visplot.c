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
#include <ctype.h>
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
  { "array", 'a', "ARRAY", 0,
    "Which antennas to plot, as a comma-separated list" },
  { "device", 'd', "PGPLOT_DEVICE", 0, "Direct plots to this PGGPLOT device" },
  { "frequency", 'f', 0, 0, "Plots frequency on x-axis" },
  { "ifs", 'I', "IFS", 0,
    "Which IFs to plot, as a comma-separated list" },
  { "non-interactive", 'N', 0, 0, "Do not run in interactive mode" },
  { "phase", 'p', 0, 0, "Plots phase on y-axis" },
  { "pols", 'P', "POLS", 0,
    "Which polarisations to plot, as a comma-separated list" },
  { 0 }
};

// Argp stuff.
struct arguments {
  char pgplot_device[BUFSIZE];
  char **rpfits_files;
  char array_spec[BUFSIZE];
  int n_rpfits_files;
  int plot_phase;
  int plot_frequency;
  int plot_pols;
  int npols;
  int plot_ifs[MAXIFS];
  int nifs;
  int interactive;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  int i;
  char *token;
  const char s[2] = ",";

  switch (key) {
  case 'a':
    strncpy(arguments->array_spec, arg, BUFSIZE);
    break;
  case 'd':
    strncpy(arguments->pgplot_device, arg, BUFSIZE);
    break;
  case 'f':
    arguments->plot_frequency = YES;
    break;
  case 'I':
    // Reset the IFs.
    memset(arguments->plot_ifs, 0, sizeof(arguments->plot_ifs));
    arguments->nifs = 0;
    token = strtok(arg, s);
    while (token != NULL) {
      i = atoi(token);
      if ((i >= 1) && (i <= MAXIFS) && (arguments->nifs < MAXIFS)) {
	arguments->plot_ifs[arguments->nifs] = i - 1;
	arguments->nifs++;
      }
      token = strtok(NULL, s);
    }
    break;
  case 'N':
    // Not interactive.
    arguments->interactive = NO;
    break;
  case 'p':
    arguments->plot_phase = YES;
    break;
  case 'P':
    // Reset the polarisations.
    arguments->plot_pols = 0;
    arguments->npols = 0;
    // Convert to lower case.
    for (i = 0; arg[i]; i++) {
      arg[i] = tolower(arg[i]);
    }
    // Tokenise.
    token = strtok(arg, s);
    while (token != NULL) {
      if (strcmp(token, "xx") == 0) {
	arguments->plot_pols |= PLOT_POL_XX;
	arguments->npols++;
      } else if (strcmp(token, "yy") == 0) {
	arguments->plot_pols |= PLOT_POL_YY;
	arguments->npols++;
      } else if (strcmp(token, "xy") == 0) {
	arguments->plot_pols |= PLOT_POL_XY;
	arguments->npols++;
      } else if (strcmp(token, "yx") == 0) {
	arguments->plot_pols |= PLOT_POL_YX;
	arguments->npols++;
      }
      token = strtok(NULL, s);
    }
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
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no = 0, read_response = 0;
  int plotif = 0, r = 0, ant1, ant2, num_ifs = 2;
  int p = 0, pp = 0, q = 0, sp = 0, rp, ri, rx, ry, yaxis_type, xaxis_type;
  int old_num_ifs = 0, old_npols = 0;
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
  strcpy(arguments.array_spec, "1,2,3,4,5,6");
  arguments.plot_phase = NO;
  arguments.plot_frequency = NO;
  arguments.plot_pols = PLOT_POL_XX | PLOT_POL_YY;
  arguments.npols = 2;
  arguments.interactive = YES;
  memset(arguments.plot_ifs, 0, sizeof(arguments.plot_ifs));
  for (i = 0; i < MAXIFS; i++) {
    arguments.plot_ifs[i] = i;
  }
  arguments.nifs = MAXIFS;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Check for nonsense arguments.
  if (arguments.nifs == 0) {
    printf("Must specify at least 1 IF to plot!\n");
    exit(-1);
  }
  
  cpgopen(arguments.pgplot_device);
  cpgask(1);

  // Get phase in degrees.
  ampphase_options.phase_in_degrees = true;
  ampphase_options.delay_averaging = 1;
  ampphase_options.min_tvchannel = 513;
  ampphase_options.max_tvchannel = 1537;
  ampphase_options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_SCALAR;

  // Initialise the plotting space and options.
  splitpanels(5, 5, &panelspec);
  if (arguments.plot_phase == NO) {
    yaxis_type = PLOT_AMPLITUDE | PLOT_AMPLITUDE_LINEAR;
  } else {
    yaxis_type = PLOT_PHASE;
  }
  if (arguments.plot_frequency == NO) {
    xaxis_type = PLOT_CHANNEL;
  } else {
    xaxis_type = PLOT_FREQUENCY;
  }
  
  init_plotcontrols(&plotcontrols, xaxis_type, yaxis_type,
		    arguments.plot_pols, DEFAULT);
  plotcontrols.array_spec = interpret_array_string(arguments.array_spec);
  plotcontrols.interactive = arguments.interactive;
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    // Try to open the RPFITS file.
    res = open_rpfits_file(arguments.rpfits_files[i]);
    printf("Attempt to open RPFITS file %s, %d\n", arguments.rpfits_files[i], res);
    keep_reading = 1;
    while (keep_reading) {
      // Make a new scan and add it to the list.
      scan_data = prepare_new_scan_data();
      nscans += 1;
      ARRAY_APPEND(all_scans, nscans, scan_data);

      // Read in the scan header.
      read_response = read_scan_header(&(scan_data->header_data));
      // Adjust the number of IFs.
      num_ifs = (scan_data->header_data.num_ifs < arguments.nifs) ?
	scan_data->header_data.num_ifs : arguments.nifs;
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
      printf("  number of IFs = %d\n", scan_data->header_data.num_ifs);
      if (read_response & READER_DATA_AVAILABLE) {
	// Now start reading the cycle data.
	read_cycle = 1;
	while (read_cycle) {
	  //printf("reading cycle\n");
	  cycle_data = scan_add_cycle(scan_data);
	  read_response = read_cycle_data(&(scan_data->header_data),
					  cycle_data);
	  /* fprintf(stderr, "found read response %d\n", read_response); */
	  /* printf("cycle has %d points\n", cycle_data->num_points); */
	  if (!(read_response & READER_DATA_AVAILABLE)) {
	    read_cycle = 0;
	    //keep_reading = 0;
	    /* printf("something went wrong while reading cycle: %d\n", */
	    /* 	   read_response); */
	  }
	}
      }
      // We allocate memory if we need to.
      if (num_ifs != old_num_ifs) {
	/* printf("Reallocating cycle memory...\n"); */
	for (j = 0; j < old_num_ifs; j++) {
	  for (k = 0; k < arguments.npols; k++) {
	    FREE(cycle_ampphase[j][k]);
	  }
	  FREE(cycle_ampphase[j]);
	}
	FREE(cycle_ampphase);
	MALLOC(cycle_ampphase, num_ifs);
	for (j = 0; j < num_ifs; j++) {
	  MALLOC(cycle_ampphase[j], arguments.npols);
	  for (k = 0; k < arguments.npols; k++) {
	    cycle_ampphase[j][k] = NULL;
	  }
	}
	old_num_ifs = num_ifs;
	// Change the plot control as well.
	memset(plotcontrols.if_num_spec, 0, sizeof(plotcontrols.if_num_spec));
      }

      /* printf("working with %d cycles\n", scan_data->num_cycles); */
      for (k = 0; k < scan_data->num_cycles; k++) {
	cycle_data = scan_data->cycles[k];
	/* printf("cycle %d, number of IFs = %d\n", k, num_ifs); */
	for (q = 0; q < num_ifs; q++) {
	  if_no = arguments.plot_ifs[q];
	  plotcontrols.if_num_spec[if_no] = 1;
	  for (p = 0; p < arguments.npols; p++) {
	    pp = p;
	    if (pp == 0) {
	      if (arguments.plot_pols & PLOT_POL_XX) {
		sp = POL_XX;
	      } else {
		pp++;
	      }
	    }
	    if (pp == 1) {
	      if (arguments.plot_pols & PLOT_POL_YY) {
		sp = POL_YY;
	      } else {
		pp++;
	      }
	    }
	    if (pp == 2) {
	      if (arguments.plot_pols & PLOT_POL_XY) {
		sp = POL_XY;
	      } else {
		pp++;
	      }
	    }
	    if (pp == 3) {
	      if (arguments.plot_pols & PLOT_POL_YX) {
		sp = POL_YX;
	      } else {
		sp = POL_XX; // Safety.
	      }
	    }
	    r = vis_ampphase(&(scan_data->header_data), cycle_data,
			     &(cycle_ampphase[q][p]), sp, if_no,
			     &ampphase_options);
	    if (r < 0) {
	      printf("error encountered while calculating amp and phase\n");
	      free_ampphase(&(cycle_ampphase[q][p]));
	      exit(0);
	    /* } else { */
	    /*   printf("frequency determined to be %.6f - %.6f\n", */
	    /* 	     cycle_ampphase[q][p]->frequency[0], */
	    /* 	     cycle_ampphase[q][p]->frequency */
	    /* 	     [cycle_ampphase[q][p]->nchannels - 1]); */
	    /*   fflush(stdout); */
	    }
	  }
	}
	make_plot(cycle_ampphase, &panelspec, &plotcontrols);
	for (q = 0; q < num_ifs; q++) {
	  for (p = 0; p < arguments.npols; p++) {
	    free_ampphase(&(cycle_ampphase[q][p]));
	  }
	}
      }
    
      if (read_response == READER_EXHAUSTED) {
	// No more data in this file.
	keep_reading = 0;
      } // Otherwise we've probably hit another header.
      printf("scan had %d cycles\n", scan_data->num_cycles);

    }
    for (j = 0; j < old_num_ifs; j++) {
      for (k = 0; k < arguments.npols; k++) {
	FREE(cycle_ampphase[j][k]);
      }
      FREE(cycle_ampphase[j]);
    }
    FREE(cycle_ampphase);
    old_num_ifs = 0;

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);

  }
  free_panelspec(&panelspec);
  cpgclos();
  
  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  FREE(all_scans);
  // And the last little bit of memory.
  FREE(arguments.rpfits_files);
  
  exit(0);
  
}
