/**
 * ATCA Training Library: common.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains functions that are useful for many of the
 * applications.
 */

#pragma once
#include "atrpfits.h"

/**
 * Limits and magic numbers.
 */
#define BUFSIZE 1024
#define MAXANTS 6
#define MAXIFS 34
#define YES 1
#define NO 0
#define MAXCHAN 18000
#define MAXPOL  4
#define MAXBIN  32

/**
 * Some option flags.
 */
#define DEFAULT                   -1
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
#define PLOT_DELAY                1<<13
#define PLOT_TIME                 1<<14

#define MINASSIGN(a, b) a = (b < a) ? b : a
#define MAXASSIGN(a, b) a = (b > a) ? b : a



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

// This structure holds all the details about user plot control
// for an SPD-type plot.
struct spd_plotcontrols {
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
  int if_num_spec[MAXIFS];
  // The antennas to plot.
  int array_spec;
  // The number of polarisations to plot.
  int npols;
  // Whether to wait for user input when changing plots.
  int interactive;
  // The PGPLOT device number used.
  int pgplot_device;
};

// This structure holds all the details about use plot control
// for a VIS-type plot.
struct vis_plotcontrols {
  // General plot options.
  long int plot_options;
  // The products to show.
  long int *products;
  int nproducts;
  // The array specification.
  int array_spec;
  // The number of panels needed.
  int num_panels;
  // The maximum history to plot (minutes).
  float history_length;
  // The PGPLOT device number used.
  int pgplot_device;
};

// This structure holds details about the lines we are asked to
// plot in the VIS-type plots.
struct vis_line {
  // The antennas.
  int ant1;
  int ant2;
  // The IF.
  int if_number;
  // Polarisation.
  int pol;
  // Label.
  char label[BUFSIZE];
  // The colour.
  int pgplot_colour;
  // To come: baseline length for sorting.
  float baseline_length;
};

// Our routine definitions.
int interpret_array_string(char *array_string);
void init_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
			   int xaxis_type, int yaxis_type, int pols,
			   int corr_type, int pgplot_device);
void init_vis_plotcontrols(struct vis_plotcontrols *plotcontrols,
			   int xaxis_type, int paneltypes,
			   int pgplot_device,
			   struct panelspec *panelspec);
void free_panelspec(struct panelspec *panelspec);
void splitpanels(int nx, int ny, int pgplot_device,
		 struct panelspec *panelspec);
void changepanel(int x, int y, struct panelspec *panelspec);
void plotnum_to_xy(struct panelspec *panelspec, int plotnum, int *px, int *py);
void plotpanel_minmax(struct ampphase **plot_ampphase,
		      struct spd_plotcontrols *plot_controls,
		      int plot_baseline_idx, int npols, int *polidx,
		      float *plotmin_x, float *plotmax_x,
		      float *plotmin_y, float *plotmax_y);
int find_pol(struct ampphase ***cycle_ampphase, int npols, int ifnum, int poltype);
void add_vis_line(struct vis_line ***vis_lines, int *n_vis_lines,
		  int ant1, int ant2, int if_number, int pol);
long int vis_interpret_product(char *product);
int find_if_name(struct scan_header_data *scan_header_data, char *name);
void make_vis_plot(struct vis_quantities ****cycle_vis_quantities,
		   int ncycles, int *cycle_numifs, int npols,
		   struct panelspec *panelspec,
		   struct vis_plotcontrols *plot_controls);
void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
		   struct spd_plotcontrols *plot_controls);

