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
  { "visband", 'c', "VISBAND", 0,
    "Two comma-separated IFs representing the visband setting" },
  { "device", 'd', "PGPLOT_DEVICE", 0, "Direct SPD plots to this PGGPLOT device" },
  { "delavg", 'D', "DELAVG", 0,
    "Number of channels to average together while calculating delay" },
  { "frequency", 'f', 0, 0, "Plots frequency on x-axis" },
  { "ifs", 'I', "IFS", 0,
    "Which IFs to plot, as a comma-separated list" },
  { "median", 'm', 0, 0,
    "Use median averaging instead of mean" },
  { "non-interactive", 'N', 0, 0, "Do not run in interactive mode" },
  { "phase", 'p', 0, 0, "Plots phase on y-axis" },
  { "pols", 'P', "POLS", 0,
    "Which polarisations to plot, as a comma-separated list" },
  { "select", 's', "VIS_SELECT", 0,
    "A string of products to plot on ths VIS plot" },
  { "tvchannel", 't', "TVCHANNEL", 0,
    "A comma separated list of tvchannels to use for all IFs (2 max)" },
  { "visdevice", 'V', "PGPLOT_DEVICE", 0, "Direct VIS plots to this PGPLOT device" },
  { 0 }
};

// Argp stuff.
struct arguments {
  char spd_device[BUFSIZE];
  char vis_device[BUFSIZE];
  char **rpfits_files;
  char array_spec[BUFSIZE];
  int n_rpfits_files;
  int plot_phase;
  int plot_frequency;
  int plot_pols;
  int npols;
  char plot_ifs[MAXIFS][BUFSIZE];
  int nifs;
  int interactive;
  int nselect;
  char **vis_select;
  int nvisbands;
  char **visband;
  int tvchannels[2];
  int median_averaging;
  int delavg;
};

void interpret_band_argument(char *dest, char *arg, size_t destlen) {
  long int ifnum;
  int i;
  char *eptr = NULL;
  // Check if the string can be straight converted to a number.
  ifnum = strtol(arg, &eptr, 10);
  if (*eptr != 0) {
    // This is a string, just copy it.
    strncpy(dest, arg, destlen);
  } else {
    // It's a number.
    i = (int)ifnum;
    if ((i >= 1) && (i <= MAXIFS)) {
      snprintf(dest, destlen, "f%d", i);
    }
  }
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  int i, j;
  long int ifnum;
  char *token, *eptr = NULL;
  const char s[2] = ",", sp[2] = " ";
  
  switch (key) {
  case 'a':
    strncpy(arguments->array_spec, arg, BUFSIZE);
    break;
  case 'c':
    token = strtok(arg, s);
    if (arguments->nvisbands > 0) {
      // We reallocate.
      for (j = 0; j < arguments->nvisbands; j++) {
        FREE(arguments->visband[j]);
      }
    }
    arguments->nvisbands = 0;
    FREE(arguments->visband);
    while (token != NULL) {
      REALLOC(arguments->visband, (arguments->nvisbands + 1));
      MALLOC(arguments->visband[arguments->nvisbands], VISBANDLEN);
      interpret_band_argument(arguments->visband[arguments->nvisbands++], token, VISBANDLEN);
      /* ifnum = atoi(token); */
      /* if ((ifnum >= 0) && (ifnum <= MAXIFS) && (i < 2)) { */
      /*   arguments->visband[i] = ifnum; */
      /* } */
      token = strtok(NULL, s);
    }
    break;
  case 'd':
    strncpy(arguments->spd_device, arg, BUFSIZE);
    break;
  case 'D':
    // Delavg.
    arguments->delavg = atoi(arg);
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
      // Check if the string can be straight converted to a number.
      ifnum = strtol(token, &eptr, 10);
      if (*eptr != 0) {
        // This is a string, just copy it.
        (void)strncpy(arguments->plot_ifs[arguments->nifs], token, BUFSIZE);
        arguments->nifs++;
      } else {
        // It's a number.
        i = (int)ifnum;
        if ((i >= 1) && (i <= MAXIFS) && (arguments->nifs < MAXIFS)) {
          (void)snprintf(arguments->plot_ifs[arguments->nifs], BUFSIZE,
                         "f%d", i);
          arguments->nifs++;
        }
      }
      token = strtok(NULL, s);
    }
    break;
  case 'm':
    // Use median averaging.
    arguments->median_averaging = YES;
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
  case 's':
    // Split any select string into space-separated components.
    if (arguments->nselect > 0) {
      FREE(arguments->vis_select);
    }
    arguments->nselect = 0;
    arguments->vis_select = NULL;
    // Convert to lower case.
    for (i = 0; arg[i]; i++) {
      arg[i] = tolower(arg[i]);
    }
    // Tokenise.
    token = strtok(arg, sp);
    while (token != NULL) {
      arguments->nselect += 1;
      REALLOC(arguments->vis_select, arguments->nselect);
      arguments->vis_select[arguments->nselect - 1] = token;
      token = strtok(NULL, sp);
    }
    break;
  case 't':
    // Split the string into comma-separated components.
    token = strtok(arg, s);
    i = 0;
    while ((token != NULL) && (i < 2)) {
      arguments->tvchannels[i++] = atoi(token);
      token = strtok(NULL, s);
    }
    break;
  case 'V':
    strncpy(arguments->vis_device, arg, BUFSIZE);
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
  int i = 0, j = 0, k = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, if_no = 0, read_response = 0;
  int r = 0, num_ifs = 2;
  int p = 0, pp = 0, q = 0, sp = 0, yaxis_type, xaxis_type;
  int old_num_ifs = 0, vis_num_cycles = 0, nviscycle = 0;
  int *vis_cycle_num_ifs = NULL, spd_pgplot = -1, vis_pgplot = -1;
  int global_max_cycletime = 0;
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  struct ampphase ***cycle_ampphase = NULL;
  struct vis_quantities ****cycle_vis_quantities = NULL;
  char defselect[BUFSIZE];
  struct ampphase_options ampphase_options;
  struct panelspec spd_panelspec, vis_panelspec;
  struct arguments arguments;
  struct spd_plotcontrols spd_plotcontrols;
  struct vis_plotcontrols vis_plotcontrols;
  
  // Set some defaults and parse our command line options.
  arguments.spd_device[0] = '\0';
  arguments.vis_device[0] = '\0';
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
    (void)snprintf(arguments.plot_ifs[i], BUFSIZE, "f%d", (i + 1));
  }
  arguments.nifs = MAXIFS;
  arguments.nselect = 1;
  MALLOC(arguments.vis_select, arguments.nselect);
  strcpy(defselect, "aa");
  arguments.vis_select[0] = defselect;
  arguments.nvisbands = 2;
  MALLOC(arguments.visband, arguments.nvisbands);
  for (i = 0; i < arguments.nvisbands; i++) {
    MALLOC(arguments.visband[i], VISBANDLEN);
    snprintf(arguments.visband[i], VISBANDLEN, "f%d", (i + 1));
  }
  arguments.tvchannels[0] = 513;
  arguments.tvchannels[1] = 1537;
  arguments.median_averaging = NO;
  arguments.delavg = 1;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Check for nonsense arguments.
  if (arguments.nifs == 0) {
    printf("Must specify at least 1 IF to plot!\n");
    exit(-1);
  }
  /* // Resolve the visbands. */
  /* if (arguments.visband[0] == 0) { */
  /*   arguments.visband[0] = 1; */
  /* } else if (arguments.visband[0] > arguments.nifs) { */
  /*   arguments.visband[0] = arguments.nifs; */
  /* } */
  /* if (arguments.visband[1] == 0) { */
  /*   if (arguments.nifs > 1) { */
  /*     arguments.visband[1] = 2; */
  /*   } else { */
  /*     arguments.visband[1] = 1; */
  /*   } */
  /* } else if (arguments.visband[1] > arguments.nifs) { */
  /*   arguments.visband[1] = arguments.nifs; */
  /* } */
  
  spd_pgplot = cpgopen(arguments.spd_device);
  cpgask(1);
  vis_pgplot = cpgopen(arguments.vis_device);
  
  // Get phase in degrees.
  ampphase_options.phase_in_degrees = true;
  ampphase_options.delay_averaging = arguments.delavg;
  // FIX THIS COMMAND LINE STUFF NOW THAT THE TVCHANNEL
  // SPECIFICATIONS HAVE CHANGED.
  ampphase_options.num_ifs = 0;
  ampphase_options.min_tvchannel = NULL;
    /* (arguments.tvchannels[0] < arguments.tvchannels[1]) ? */
    /* arguments.tvchannels[0] : arguments.tvchannels[1]; */
  ampphase_options.max_tvchannel = NULL;
    /* (arguments.tvchannels[0] > arguments.tvchannels[1]) ? */
    /* arguments.tvchannels[0] : arguments.tvchannels[1]; */
  if (arguments.median_averaging == NO) {
    ampphase_options.averaging_method = AVERAGETYPE_MEAN;
  } else {
    ampphase_options.averaging_method = AVERAGETYPE_MEDIAN;
  }
  ampphase_options.averaging_method |= AVERAGETYPE_SCALAR;

  // Initialise the plotting space and options.
  splitpanels(5, 5, spd_pgplot, 0, 5, 0, &spd_panelspec);
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
  
  init_spd_plotcontrols(&spd_plotcontrols, xaxis_type, yaxis_type,
                        arguments.plot_pols, spd_pgplot);
  init_vis_plotcontrols(&vis_plotcontrols, PLOT_TIME,
                        PLOT_AMPLITUDE | PLOT_PHASE | PLOT_DELAY,
                        arguments.nvisbands, arguments.visband,
                        vis_pgplot, &vis_panelspec);
  spd_plotcontrols.array_spec = interpret_array_string(arguments.array_spec);
  vis_plotcontrols.array_spec = interpret_array_string(arguments.array_spec);
  spd_plotcontrols.interactive = arguments.interactive;
  MALLOC(vis_plotcontrols.vis_products, arguments.nselect);
  vis_plotcontrols.nproducts = arguments.nselect;
  for (i = 0; i < vis_plotcontrols.nproducts; i++) {
    vis_interpret_product(arguments.vis_select[i], &(vis_plotcontrols.vis_products[i]));
  }
  nviscycle = 0;
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
      printf("  number of IFs = %d, cycle time = %d\n",
             scan_data->header_data.num_ifs, scan_data->header_data.cycle_time);
      if (scan_data->header_data.cycle_time > global_max_cycletime) {
        global_max_cycletime = scan_data->header_data.cycle_time;
      }
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
        memset(spd_plotcontrols.if_num_spec, 0,
               sizeof(spd_plotcontrols.if_num_spec));
      }
      
      //printf("working with %d cycles\n", scan_data->num_cycles);
      // Reallocate the vis data.
      vis_num_cycles += scan_data->num_cycles;
      //printf("now have %d total cycles\n", vis_num_cycles);
      REALLOC(cycle_vis_quantities, vis_num_cycles);
      REALLOC(vis_cycle_num_ifs, vis_num_cycles);
      for (k = 0; k < scan_data->num_cycles; k++) {
        cycle_data = scan_data->cycles[k];
        /* printf("cycle %d, number of IFs = %d\n", k, num_ifs); */
        //printf("   current cycle = %d\n", nviscycle);
        vis_cycle_num_ifs[nviscycle] = num_ifs;
        MALLOC(cycle_vis_quantities[nviscycle], num_ifs);
        for (q = 0; q < num_ifs; q++) {
          if_no = find_if_name(&(scan_data->header_data),
                               arguments.plot_ifs[q]);
          spd_plotcontrols.if_num_spec[if_no] = 1;
          MALLOC(cycle_vis_quantities[nviscycle][q], arguments.npols);
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
            // Calculate the vis quantities as well.
            cycle_vis_quantities[nviscycle][q][p] = NULL;
            r = ampphase_average(cycle_ampphase[q][p],
                                 &(cycle_vis_quantities[nviscycle][q][p]),
                                 &ampphase_options);
          }
        }
        make_spd_plot(cycle_ampphase, &spd_panelspec, &spd_plotcontrols, false);
        for (q = 0; q < num_ifs; q++) {
          for (p = 0; p < arguments.npols; p++) {
            free_ampphase(&(cycle_ampphase[q][p]));
          }
        }
        nviscycle++;
        
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

  // Make the vis plot now.
  vis_plotcontrols.cycletime = global_max_cycletime;
  make_vis_plot(cycle_vis_quantities, vis_num_cycles,
                vis_cycle_num_ifs, arguments.npols,
                &vis_panelspec, &vis_plotcontrols, NULL);
  
  // Close all the PGPLOT devices.
  cpgend();
  

  // Free all the memory we allocated.
  // The vis data.
  for (i = 0; i < vis_num_cycles; i++) {
    for (j = 0; j < vis_cycle_num_ifs[i]; j++) {
      for (k = 0; k < arguments.npols; k++) {
        free_vis_quantities(&cycle_vis_quantities[i][j][k]);
      }
      FREE(cycle_vis_quantities[i][j]);
    }
    FREE(cycle_vis_quantities[i]);
  }
  FREE(vis_cycle_num_ifs);
  FREE(cycle_vis_quantities);

  // The plot specifications.
  free_panelspec(&spd_panelspec);
  free_panelspec(&vis_panelspec);
  free_vis_plotcontrols(&vis_plotcontrols);
  
  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  FREE(all_scans);
  // And the last little bit of memory.
  FREE(arguments.rpfits_files);
  /* for (i = 0; i < arguments.nselect; i++) { */
  /*   FREE(arguments.vis_select[i]); */
  /* } */
  FREE(arguments.vis_select);
  
  exit(0);
  
}
