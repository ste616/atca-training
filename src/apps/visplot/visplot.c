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

#define BUFSIZE 1024
#define MAXANTS 6
#define MAXIFS 34
#define YES 1
#define NO 0

// Some option flags.
#define PLOT_AMPLITUDE            1<<0
#define PLOT_PHASE                1<<1
#define PLOT_CHANNEL              1<<2
#define PLOT_FREQUENCY            1<<3
#define PLOT_AUTOCORRELATIONS     1<<4
#define PLOT_CROSSCORRELATIONS    1<<5
#define PLOT_POL_XX               1<<6
#define PLOT_POL_YY               1<<7
#define PLOT_POL_XY               1<<8
#define PLOT_POL_YX               1<<9
#define PLOT_AMPLITUDE_LINEAR     1<<10
#define PLOT_AMPLITUDE_LOG        1<<11
#define PLOT_CONSISTENT_YRANGE    1<<12

const char *argp_program_version = "visplot 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "visplot plotter for RPFITS files";

// Argument description.
static char args_doc[] = "[options] RPFITS_FILES...";

// Our options.
static struct argp_option options[] = {
  { "device", 'd', "PGPLOT_DEVICE", 0, "Direct plots to this PGGPLOT device" },
  { 0 }
};

// Argp stuff.
struct arguments {
  char pgplot_device[BUFSIZE];
  char **rpfits_files;
  int n_rpfits_files;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
  case 'd':
    strncpy(arguments->pgplot_device, arg, BUFSIZE);
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

// This structure holds pre-calculated panel positions for a PGPLOT
// device with nx x ny panels.
struct panelspec {
  int nx;
  int ny;
  float **x1;
  float **x2;
  float **y1;
  float **y2;
  float orig_x1;
  float orig_x2;
  float orig_y1;
  float orig_y2;
};

// This structure holds all the details about user plot control.
struct plotcontrols {
  // General plot options.
  long int plot_options;
  // Has the user specified a channel range to look at.
  int channel_range_limit;
  int channel_range_min;
  int channel_range_max;
  // Has the user specified a y-axis range.
  int yaxis_range_limit;
  float yaxis_range_min;
  float yaxis_range_max;
  // The IF numbers to plot.
  long int if_num_spec;
  // The antennas to plot.
  int array_spec;
};

void init_plotcontrols(struct plotcontrols *plotcontrols) {
  int i;

  // Initialise the plotcontrols structure.
  plotcontrols->plot_options =
    PLOT_AMPLITUDE | PLOT_CHANNEL | PLOT_AUTOCORRELATIONS |
    PLOT_CROSSCORRELATIONS | PLOT_POL_XX | PLOT_POL_YY |
    PLOT_AMPLITUDE_LINEAR;
  plotcontrols->channel_range_limit = NO;
  plotcontrols->yaxis_range_limit = NO;
  plotcontrols->if_num_spec = 0;
  for (i = 0; i <= MAXIFS; i++) {
    plotcontrols->if_num_spec |= 1<<i;
  }
  plotcontrols->array_spec = 0;
  for (i = 0; i <= MAXANTS; i++) {
    plotcontrols->array_spec |= 1<<i;
  }
}

void splitpanels(int nx, int ny, struct panelspec *panelspec) {
  int i, j;
  float panel_width, panel_height, margin_reduction = 5;
  float padding_fraction = 2, padding_x, padding_y;
  float orig_charheight, charheight;

  // Allocate some memory.
  panelspec->nx = nx;
  panelspec->ny = ny;
  MALLOC(panelspec->x1, nx);
  MALLOC(panelspec->x2, nx);
  MALLOC(panelspec->y1, nx);
  MALLOC(panelspec->y2, nx);
  for (i = 0; i < nx; i++) {
    MALLOC(panelspec->x1[i], ny);
    MALLOC(panelspec->x2[i], ny);
    MALLOC(panelspec->y1[i], ny);
    MALLOC(panelspec->y2[i], ny);
  }

  // Get the original settings.
  cpgqvp(0, &(panelspec->orig_x1), &(panelspec->orig_x2),
	 &(panelspec->orig_y1), &(panelspec->orig_y2));
  // Reduce the margins.
  panelspec->orig_x1 /= margin_reduction;
  panelspec->orig_x2 = 1 - panelspec->orig_x1;
  panelspec->orig_y1 /= margin_reduction;
  panelspec->orig_y2 = 1 - panelspec->orig_y1;
  printf("viewport is x = %.2f -> %.2f, y = %.2f -> %.2f\n",
	 panelspec->orig_x1, panelspec->orig_x2,
	 panelspec->orig_y1, panelspec->orig_y2);
  // Space between the panels should be some fraction of the margin.
  padding_x = panelspec->orig_x1 * padding_fraction;
  padding_y = panelspec->orig_y1 * padding_fraction;
  
  panel_width = (panelspec->orig_x2 - panelspec->orig_x1 -
		 (float)(nx - 1) * padding_x) / (float)nx;
  panel_height = (panelspec->orig_y2 - panelspec->orig_y1 -
		  (float)(ny - 1) * padding_y) / (float)ny;

  for (i = 0; i < nx; i++) {
    for (j = 0; j < ny; j++) {
      panelspec->x1[i][j] = panelspec->orig_x1 + i * (panel_width + padding_x);
      panelspec->x2[i][j] = panelspec->x1[i][j] + panel_width;
      panelspec->y2[i][j] = panelspec->orig_y2 - j * (panel_height + padding_y);
      panelspec->y1[i][j] = panelspec->y2[i][j] - panel_height;
    }
  }

  // Change the character height.
  cpgqch(&orig_charheight);
  charheight = orig_charheight / 2;
  cpgsch(charheight);
}

void changepanel(int x, int y, struct panelspec *panelspec) {
  if ((x >= 0) && (x < panelspec->nx) &&
      (y >= 0) && (y < panelspec->ny)) {
    cpgsvp(panelspec->x1[x][y], panelspec->x2[x][y],
	   panelspec->y1[x][y], panelspec->y2[x][y]);
  } else if ((x == -1) && (y == -1)) {
    // Set it back to original.
    cpgsvp(panelspec->orig_x1, panelspec->orig_x2,
	   panelspec->orig_y1, panelspec->orig_y2);
  }
}

void plotnum_to_xy(struct panelspec *panelspec, int plotnum,
		   int *px, int *py) {
  *px = plotnum % panelspec->nx;
  *py = (int)((plotnum - *px) / panelspec->nx);
}

#define MINASSIGN(a, b) a = (b < a) ? b : a
#define MAXASSIGN(a, b) a = (b > a) ? b : a

void plotpanel_minmax(struct ampphase **plot_ampphase,
		      struct plotcontrols *plot_controls,
		      int plot_baseline_idx, int npols, int *polidx,
		      float *plotmin_x, float *plotmax_x,
		      float *plotmin_y, float *plotmax_y) {
  // This routine does the computation required to work out what the
  // axis ranges of a single plot with plotoptions will be. It takes into
  // account whether it's an auto or cross correlation, and the polarisations
  // that are being plotted.
  int i = 0, j = 0, bltype, ant1, ant2;

  // Get the x-axis range first.
  if (plot_controls->plot_options & PLOT_CHANNEL) {
    *plotmin_x = 0;
    *plotmax_x = plot_ampphase[0]->nchannels;
    if (plot_controls->channel_range_limit == YES) {
      if ((plot_controls->channel_range_min >= 0) &&
	  (plot_controls->channel_range_min < plot_ampphase[0]->nchannels)) {
	*plotmin_x = plot_controls->channel_range_min;
      }
      if ((plot_controls->channel_range_max > 0) &&
	  (plot_controls->channel_range_max <= plot_ampphase[0]->nchannels) &&
	  (plot_controls->channel_range_max > *plotmin_x)) {
	*plotmax_x = plot_controls->channel_range_max;
      }
    } else if (plot_controls->plot_options & PLOT_FREQUENCY) {
      // TODO: frequency conversions.
    }

    // Now the y-axis. If we've been given a range by the user,
    // we can set that and return immediately.
    if (plot_controls->yaxis_range_limit == YES) {
      *plotmin_y = plot_controls->yaxis_range_min;
      *plotmax_y = plot_controls->yaxis_range_max;
      return;
    }
    

    // If we have no pols to plot, we're done.
    if (npols == 0) {
      *plotmin_y = 0;
      *plotmax_y = 1;
      return;
    }
    
    // Get the initial value for min and max.
    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
      *plotmin_y = plot_ampphase[polidx[0]]->min_amplitude[plot_baseline_idx];
      *plotmax_y = plot_ampphase[polidx[0]]->max_amplitude[plot_baseline_idx];
    } else if (plot_controls->plot_options & PLOT_PHASE) {
      *plotmin_y = plot_ampphase[polidx[0]]->min_phase[plot_baseline_idx];
      *plotmax_y = plot_ampphase[polidx[0]]->max_phase[plot_baseline_idx];
    } 

    // Account for all the other polarisations.
    for (i = 1; i < npols; i++) {
      if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	MINASSIGN(*plotmin_y,
		  plot_ampphase[polidx[i]]->min_amplitude[plot_baseline_idx]);
	MAXASSIGN(*plotmax_y,
		  plot_ampphase[polidx[i]]->max_amplitude[plot_baseline_idx]);
      } else if (plot_controls->plot_options & PLOT_PHASE) {
	MINASSIGN(*plotmin_y,
	  plot_ampphase[polidx[i]]->min_phase[plot_baseline_idx]);
	MAXASSIGN(*plotmax_y,
	  plot_ampphase[polidx[i]]->max_phase[plot_baseline_idx]);
      }
    }
	
    
    // Refine the min/max values.
    if (plot_controls->plot_options & PLOT_CONSISTENT_YRANGE) {
      // The user wants us to keep a consistent Y axis range for
      // all the plots. But we also need a different Y axis range for
      // auto and cross correlations. So we need to know what type of
      // correlation the specified baseline is first.
      base_to_ants(plot_ampphase[0]->baseline[plot_baseline_idx], &ant1, &ant2);
      if (ant1 == ant2) {
	// Autocorrelation.
	bltype = 0;
      } else {
	bltype = 1;
      }
      for (i = 0; i < plot_ampphase[0]->nbaselines; i++) {
	base_to_ants(plot_ampphase[0]->baseline[i], &ant1, &ant2);
	// TODO: check the antennas are in the array spec.
	if (((ant1 == ant2) && (bltype == 0)) ||
	    ((ant1 != ant2) && (bltype == 1))) {
	  for (j = 0; j < npols; j++) {
	    if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	      MINASSIGN(*plotmin_y,
			plot_ampphase[polidx[j]]->min_amplitude[i]);
	      MAXASSIGN(*plotmax_y,
			plot_ampphase[polidx[j]]->max_amplitude[i]);
	    } else if (plot_controls->plot_options & PLOT_PHASE) {
	      MINASSIGN(*plotmin_y,
			plot_ampphase[polidx[j]]->min_phase[i]);
	      MAXASSIGN(*plotmax_y,
			plot_ampphase[polidx[j]]->max_phase[i]);
	    }
	  }
	}
      }
    }
  }
}

void make_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
	       struct plotcontrols *plot_controls) {
  int x = 0, y = 0, i, j, ant1, ant2, nants = 0, px, py, iauto = 0, icross = 0;
  int npols = 0, *polidx = NULL, if_num = 0;
  float xaxis_min, xaxis_max, yaxis_min, yaxis_max, theight = 0.4;
  char ptitle[BUFSIZE], ptype[BUFSIZE];
  struct ampphase **ampphase_if = NULL;

  // Work out how many antennas we will show.
  for (i = 1, nants = 0; i <= MAXANTS; i++) {
    if ((1 << i) & plot_controls->array_spec) {
      nants++;
    }
  }
  if (nants == 0) {
    // Nothing to plot!
    return;
  }

  // Which polarisations are we plotting?
  if (plot_controls->plot_options & PLOT_POL_XX) {
	REALLOC(polidx, ++npols);
	polidx[npols - 1] = 0;
  }
  if (plot_controls->plot_options & PLOT_POL_YY) {
	REALLOC(polidx, ++npols);
	polidx[npols - 1] = 1;
  }
  if (plot_controls->plot_options & PLOT_POL_XY) {
	REALLOC(polidx, ++npols);
	polidx[npols - 1] = 2;
  }
  if (plot_controls->plot_options & PLOT_POL_YX) {
	REALLOC(polidx, ++npols);
	polidx[npols - 1] = 3;
  }
  
  changepanel(-1, -1, panelspec);
  cpgpage();
  ampphase_if = cycle_ampphase[if_num];

  for (i = 0; i < ampphase_if[0]->nbaselines; i++) {
    // Work out the antennas in this baseline.
    base_to_ants(ampphase_if[0]->baseline[i], &ant1, &ant2);
    // Check if we are plotting both of these antenna.
    if (((1 << ant1) & plot_controls->array_spec) &&
	((1 << ant2) & plot_controls->array_spec)) {
      // Work out which panel to use.
      if ((ant1 == ant2) &&
	  (plot_controls->plot_options & PLOT_AUTOCORRELATIONS)) {
	px = iauto % panelspec->nx;
	py = (int)((iauto - px) / panelspec->nx);
	iauto++;
      } else if ((ant1 != ant2) &&
		 (plot_controls->plot_options & PLOT_CROSSCORRELATIONS)) {
	if (plot_controls->plot_options & PLOT_AUTOCORRELATIONS) {
	  px = (nants + icross) % panelspec->nx;
	  py = (int)((nants + icross - px) / panelspec->nx);
	} else {
	  px = icross % panelspec->nx;
	  py = (int)((icross - px) / panelspec->nx);
	}
	icross++;
      }
      // Check if we've exceeded the space for this plot.
      if (py >= panelspec->ny) {
	continue;
      }
      // Set the panel.
      changepanel(px, py, panelspec);
      // Set the title for the plot.
      if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	snprintf(ptype, BUFSIZE, "AMPL.");
      } else if (plot_controls->plot_options & PLOT_PHASE) {
	snprintf(ptype, BUFSIZE, "PHASE");
      }
      snprintf(ptitle, BUFSIZE, "%s: FQ:%d BSL%d%d",
	       ptype, (if_num + 1), ant1, ant2);
      plotpanel_minmax(ampphase_if, plot_controls, i, npols, polidx,
		       &xaxis_min, &xaxis_max, &yaxis_min, &yaxis_max);
      cpgsci(1);
      cpgswin(xaxis_min, xaxis_max, yaxis_min, yaxis_max);
      cpgbox("BCNTS", 0, 0, "BCNTS", 0, 0);
      cpgmtxt("T", theight, 0.5, 0.5, ptitle);
      for (j = 0; j < npols; j++) {
	cpgsci(polidx[j] + 1);
	if (plot_controls->plot_options & PLOT_AMPLITUDE) {
	  if (plot_controls->plot_options & PLOT_CHANNEL) {
	    cpgline(ampphase_if[polidx[j]]->f_nchannels[i],
		    ampphase_if[polidx[j]]->f_channel[i],
		    ampphase_if[polidx[j]]->f_amplitude[i]);
	  }
	}
      }
    }
  }
  
}

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, j = 0, k = 0, l = 0, m = 0, res = 0, keep_reading = 1;
  int read_cycle = 1, nscans = 0, vis_length = 0, if_no, read_response = 0;
  int plotpol = POL_XX, plotif = 0, r = 0, ant1, ant2, num_ifs = 2, num_pols = 2;
  int p = 0, q = 0, sp = 0, rp, ri, rx, ry;
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
  init_plotcontrols(&plotcontrols);
  
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
	  /* r = ampphase_average(cycle_ampphase[q][p], &cycle_vis_quantities, */
	  /* 			 &ampphase_options); */
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
