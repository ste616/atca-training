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
  int if_num_spec[MAXIFS];
  // The antennas to plot.
  int array_spec;
  // The numer of polarisations to plot.
  int npols;
};

// Our routine definitions.
int interpret_array_string(char *array_string);
void init_plotcontrols(struct plotcontrols *plotcontrols,
		       int xaxis_type, int yaxis_type, int pols,
		       int corr_type);
void splitpanels(int nx, int ny, struct panelspec *panelspec);
void changepanel(int x, int y, struct panelspec *panelspec);
void plotnum_to_xy(struct panelspec *panelspec, int plotnum, int *px, int *py);
void plotpanel_minmax(struct ampphase **plot_ampphase,
		      struct plotcontrols *plot_controls,
		      int plot_baseline_idx, int npols, int *polidx,
		      float *plotmin_x, float *plotmax_x,
		      float *plotmin_y, float *plotmax_y);
int find_pol(struct ampphase ***cycle_ampphase, int npols, int ifnum, int poltype);
void make_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
	       struct plotcontrols *plot_controls);

