/**
 * ATCA Training Library: plotting.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module contains functions that need to plot things with
 * PGPLOT.
 */

#pragma once
#include "common.h"

/**
 * Some option flags.
 */
#define DEFAULT                   -1
#define PLOT_ALL_PANELS           -1
#define PLOT_AMPLITUDE            1<<0
#define PLOT_PHASE                1<<1
#define PLOT_CHANNEL              1<<2
#define PLOT_FREQUENCY            1<<3
#define PLOT_POL_XX               1<<4
#define PLOT_POL_YY               1<<5
#define PLOT_POL_XY               1<<6
#define PLOT_POL_YX               1<<7
#define PLOT_AMPLITUDE_LINEAR     1<<8
#define PLOT_AMPLITUDE_LOG        1<<9
#define PLOT_CONSISTENT_YRANGE    1<<10
#define PLOT_DELAY                1<<11
#define PLOT_TIME                 1<<12
#define PLOT_REAL                 1<<13
#define PLOT_IMAG                 1<<14

#define PLOT_FLAG_POL_XX             1<<1
#define PLOT_FLAG_POL_YY             1<<2
#define PLOT_FLAG_POL_XY             1<<3
#define PLOT_FLAG_POL_YX             1<<4
#define PLOT_FLAG_AUTOCORRELATIONS   1<<5
#define PLOT_FLAG_CROSSCORRELATIONS  1<<6
#define PLOT_FLAGS_AVAILABLE         6

#define VIS_PLOT_IF1     1<<0
#define VIS_PLOT_IF2     1<<1

#define VIS_PLOTPANEL_AMPLITUDE    1
#define VIS_PLOTPANEL_PHASE        2
#define VIS_PLOTPANEL_DELAY        3
#define VIS_PLOTPANEL_TEMPERATURE  4
#define VIS_PLOTPANEL_PRESSURE     5
#define VIS_PLOTPANEL_HUMIDITY     6
#define VIS_PLOTPANEL_SYSTEMP      7

#define PANEL_ORIGINAL    -1
#define PANEL_INFORMATION -2

// This structure holds pre-calculated panel positions for a PGPLOT
// device with nx x ny panels.
struct panelspec {
  // The number of panels in each direction.
  int nx;
  int ny;
  // The coordinates of each panel, first coordinate X, second Y.
  float **x1;
  float **x2;
  float **y1;
  float **y2;
  float **px_x1;
  float **px_x2;
  float **px_y1;
  float **px_y2;
  // The limits of the window in normalised device coordinates.
  float orig_x1;
  float orig_x2;
  float orig_y1;
  float orig_y2;
  // The limits of the window in pixels.
  float orig_px_x1;
  float orig_px_x2;
  float orig_px_y1;
  float orig_px_y2;
  // A flag which prevents the window coordinates from being
  // recomputed, which screws things up since PGPLOT remembers.
  int measured;
  // The coordinates of the information area at the top.
  float information_x1;
  float information_x2;
  float information_y1;
  float information_y2;
  // And how many lines are given in the information area.
  int num_information_lines;
};

// This structure holds all the details about user plot control
// for an SPD-type plot.
struct spd_plotcontrols {
  // General plot options.
  long int plot_options;
  long int plot_flags;
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

// This structure informs vis what to plot.
struct vis_product {
  // The antennas to plot.
  int antenna_spec;
  // The IFs to plot.
  int if_spec;
  // The pols to plot.
  int pol_spec;
};

// This structure holds all the details about use plot control
// for a VIS-type plot.
struct vis_plotcontrols {
  // General plot options.
  long int plot_options;
  // The products to show.
  struct vis_product **vis_products;
  int nproducts;
  // The array specification.
  int array_spec;
  // The number of panels needed.
  int num_panels;
  // And their type.
  int *panel_type;
  // And any axis limits.
  bool *use_panel_limits;
  float *panel_limits_min;
  float *panel_limits_max;
  // The maximum history to plot (minutes).
  float history_length;
  // The history start point (minutes).
  float history_start;
  // The PGPLOT device number used.
  int pgplot_device;
  // The bands to assign as IFs.
  int nvisbands;
  char **visbands;
  // The cycle time.
  int cycletime;
};

// This structure holds details about the lines we are asked to
// plot in the VIS-type plots.
struct vis_line {
  // The antennas.
  int ant1;
  int ant2;
  // The IF.
  char if_label[BUFSIZE];
  // Polarisation.
  int pol;
  // Label.
  char label[BUFSIZE];
  // The colour.
  int pgplot_colour;
  // To come: baseline length for sorting.
  float baseline_length;
};

void count_polarisations(struct spd_plotcontrols *plotcontrols);
int cmpfunc_baseline_length(const void *a, const void *b);
void change_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
                             int *xaxis_type, int *yaxis_type, int *pols);
int change_spd_plotflags(struct spd_plotcontrols *plotcontrols,
                         long int changed_flags, int add_remove);
void init_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
			   int xaxis_type, int yaxis_type, int pols,
			   int pgplot_device);
void change_vis_plotcontrols_visbands(struct vis_plotcontrols *plotcontrols,
                                      int nvisbands, char **visbands);
void change_vis_plotcontrols_limits(struct vis_plotcontrols *plotcontrols,
                                    int paneltype, bool use_limits,
                                    float limit_min, float limit_max);
void init_vis_plotcontrols(struct vis_plotcontrols *plotcontrols,
                           int xaxis_type, int num_panels,
                           int *paneltypes, int nvisbands, char **visbands,
                           int pgplot_device,
                           struct panelspec *panelspec);
void free_vis_plotcontrols(struct vis_plotcontrols *plotcontrols);
void free_panelspec(struct panelspec *panelspec);
void splitpanels(int nx, int ny, int pgplot_device, int abut,
                 float margin_reduction, int info_area_num_lines,
                 struct panelspec *panelspec);
void changepanel(int x, int y, struct panelspec *panelspec);
void plotnum_to_xy(struct panelspec *panelspec, int plotnum, int *px, int *py);
void plotpanel_minmax(struct ampphase **plot_ampphase,
		      struct spd_plotcontrols *plot_controls,
		      int plot_baseline_idx, int npols, int *polidx,
		      float *plotmin_x, float *plotmax_x,
		      float *plotmin_y, float *plotmax_y);
void add_vis_line(struct vis_line ***vis_lines, int *n_vis_lines,
                  int ant1, int ant2, int ifnum, char *if_label, int pol,
                  struct scan_header_data *scan_header_data);
void vis_interpret_pol(char *pol, struct vis_product *vis_product);
int vis_interpret_product(char *product, struct vis_product **vis_product);
float fracwidth(struct panelspec *panelspec,
		float axis_min_x, float axis_max_x,
		int x, int y, char *label);
void make_vis_plot(struct vis_quantities ****cycle_vis_quantities,
                   int ncycles, int *cycle_numifs, int npols,
                   bool sort_baselines,
                   struct panelspec *panelspec,
                   struct vis_plotcontrols *plot_controls,
                   struct scan_header_data **header_data);
void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
                   struct spd_plotcontrols *plot_controls,
                   struct scan_header_data *scan_header_data,
		   struct syscal_data *compiled_tsys_data,
                   int max_tsys_ifs, bool all_data_present);
