/** \file plotting.h
 *  \brief Structures and magic numbers used to make plots
 *
 * ATCA Training Library
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
#define PLOT_TVCHANNELS           1<<15
#define PLOT_AVERAGED_DATA        1<<16

#define PLOT_FLAG_POL_XX             1<<1
#define PLOT_FLAG_POL_YY             1<<2
#define PLOT_FLAG_POL_XY             1<<3
#define PLOT_FLAG_POL_YX             1<<4
#define PLOT_FLAG_AUTOCORRELATIONS   1<<5
#define PLOT_FLAG_CROSSCORRELATIONS  1<<6
#define PLOT_FLAGS_AVAILABLE         6

#define VIS_PLOT_IF1     1<<0
#define VIS_PLOT_IF2     1<<1

#define VIS_PLOTPANEL_ALL             -1
#define VIS_PLOTPANEL_AMPLITUDE        1
#define VIS_PLOTPANEL_PHASE            2
#define VIS_PLOTPANEL_DELAY            3
#define VIS_PLOTPANEL_TEMPERATURE      4
#define VIS_PLOTPANEL_PRESSURE         5
#define VIS_PLOTPANEL_HUMIDITY         6
#define VIS_PLOTPANEL_SYSTEMP          7
#define VIS_PLOTPANEL_WINDSPEED        8
#define VIS_PLOTPANEL_WINDDIR          9
#define VIS_PLOTPANEL_RAINGAUGE        10
#define VIS_PLOTPANEL_SEEMONPHASE      11
#define VIS_PLOTPANEL_SEEMONRMS        12
#define VIS_PLOTPANEL_SYSTEMP_COMPUTED 13
#define VIS_PLOTPANEL_GTP              14
#define VIS_PLOTPANEL_SDO              15
#define VIS_PLOTPANEL_CALJY            16
#define VIS_PLOTPANEL_CLOSUREPHASE     17

#define PANEL_ORIGINAL    -1
#define PANEL_INFORMATION -2

#define FILETYPE_PNG        -3
#define FILETYPE_POSTSCRIPT -4
#define FILETYPE_UNKNOWN     0

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
/*! \struct spd_plotcontrols
 *  \brief Details about how to plot an SPD-type plot
 */
struct spd_plotcontrols {
  // General plot options.
  /*! \var plot_options
   *  \brief Flags controlling what to plot on each axis and the polarisations
   *
   * This is a bitwise OR combination of the PLOT_* magic numbers
   * defined in this header.
   */
  long int plot_options;
  /*! \var plot_flags
   *  \brief Flags controlling which polarisations and products are available
   *         to be plotted
   *
   * This is a bitwise OR combination of PLOT_FLAG_* magic numbers defined
   * in this header.
   */
  long int plot_flags;
  // Has the user specified a channel range to look at.
  /*! \var channel_range_limit
   *  \brief An array specifying whether a particular IF's x-axis range
   *         should be limited
   *
   * This array has length MAXIFS, which is the maximum number of IFs which
   * can be supported in the data. Each index in this array corresponds to the
   * same index in the array of ampphase data, and as such is generally detached
   * from any definite correspondence betwene index and actual IF number.
   * If this index is set to 1, the plotting routine will limit the channel range
   * plotted for that IF, according to the values at the same index in
   * channel_range_min and channel_range_max.
   */
  int channel_range_limit[MAXIFS];
  /*! \var channel_range_min
   *  \brief If an index of channel_range_limit is set to YES, the value of
   *         this array at the same index will be used as the minimum channel
   *         plotted of each panel's y-axis for that corresponding IF
   */
  int channel_range_min[MAXIFS];
  /*! \var channel_range_max
   *  \brief If an index of channel_range_limit is set to YES, the value of
   *         this array at the same index will be used as the maximum channel
   *         plotted of each panel's y-axis for that corresponding IF
   */
  int channel_range_max[MAXIFS];
  // Has the user specified a y-axis range.
  /*! \var yaxis_range_limit
   *  \brief An indicator of whether the y-range of each plot should be limited
   *         by the yaxis_range_min and yaxis_range_max settings
   *
   * This variable should be set to either YES or NO, indicating whether the
   * yaxis_range_* variables should be used.
   */
  int yaxis_range_limit;
  /*! \var yaxis_range_min
   *  \brief If yaxis_range_limit is set to YES, this value will be used as
   *         the minimum value of each panel's y-axis
   */
  float yaxis_range_min;
  /*! \var yaxis_range_max
   *  \brief If yaxis_range_limit is set to YES, this value will be used as
   *         the maximum value of each panel's y-axis
   */
  float yaxis_range_max;
  // The IF numbers to plot.
  /*! \var if_num_spec
   *  \brief An array specifying which IFs to consider plotting
   *
   * This array has length MAXIFS, which is the maximum number of IFs which
   * can be supported in the data. Each index in this array corresponds to the
   * same index in the array of ampphase data, and as such is generally
   * detached from any definite correspondence between index and actual IF number.
   * If this index is set to 1, the plotting routine will try to plot it if
   * there is space.
   */
  int if_num_spec[MAXIFS];
  // The antennas to plot.
  /*! \var array_spec
   *  \brief A bitwise OR combination of antenna numbers to plot
   *
   * This is a single number indicating which antennas should be made
   * available to plot. Each antenna is represented by a 1<<ant number
   * (where ant starts at 1) in the bitwise OR. No antenna greater than
   * MAXANTS can be plotted.
   */
  int array_spec;
  // The number of polarisations to plot.
  /*! \var npols
   *  \brief The number of polarisations specified in the plot_options
   *         variable
   *
   * This variable does not need to be set by the user; it will be computed
   * by the count_polarisations routine during the plotting process.
   */
  int npols;
  // Whether to wait for user input when changing plots.
  /*! \var interactive
   *  \brief Indicator of whether the plot should wait for user input before
   *         refreshing the plot; YES or NO
   *
   * This indicator determines whether the SPD plot should stop after all
   * the panels have been filled (YES), or if it should automatically go to the
   * next page (NO).
   */
  int interactive;
  // The PGPLOT device number used.
  /*! \var pgplot_device
   *  \brief The PGPLOT device number used for the plot
   */
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
/*! \struct vis_plotcontrols
 *  \brief Parameter collection for controlling how a vis plot is made
 */
struct vis_plotcontrols {
  /*! \var plot_options
   *  \brief A bitwise-OR collection of options controlling which panels
   *         to plot, and which polarisations can be plotted, etc; 
   *         the PLOT_* binary numbers at the top of this header file
   */
  long int plot_options;
  /*! \var nproducts
   *  \brief The number of products selected for plotting by the user
   */
  int nproducts;
  /*! \var vis_products
   *  \brief A list of all the products selected for plotting by the user
   *
   * This is a 1-D array of pointers, with length `nproducts`, and is indexed
   * starting at 0.
   */
  struct vis_product **vis_products;
  /*! \var array_spec
   *  \brief An indicator of which antennas can be plotted, as a bitwise-OR
   *         combination of their antenna numbers as binary shifts
   */
  int array_spec;
  /*! \var num_panels
   *  \brief The number of panels needed on the vis plot
   */
  int num_panels;
  /*! \var panel_type
   *  \brief The type of panels to plot
   *
   * This 1-D array has length `num_panels` and is indexed starting at 0.
   * Each element is one of the VIS_PLOTPANEL_* magic numbers defined in this
   * header.
   */
  int *panel_type;
  /*! \var use_panel_limits
   *  \brief Indicator of whether each panel has user-defined limits
   *
   * This 1-D array has length `num_panels` and is indexed starting at 0.
   * Each element indicates whether to use `panel_limits_min` and `panel_limits_max`
   * when setting the axis range for this panel (true), or not (false).
   */
  bool *use_panel_limits;
  /*! \var panel_limits_min
   *  \brief The minimum value set by the user for each panel
   *
   * This 1-D array has length `num_panels` and is indexed starting at 0.
   */
  float *panel_limits_min;
  /*! \var panel_limits_max
   *  \brief The maximum value set by the user for each panel
   *
   * This 1-D array has length `num_panels` and is indexed starting at 0.
   */
  float *panel_limits_max;
  /*! \var x_axis_type
   *  \brief The variable to use on the x-axis, as one of the eligible
   *         PLOT_* magic numbers defined in this header
   */
  int x_axis_type;
  /*! \var history_length
   *  \brief The maximum amount of history to plot, in minutes
   */
  float history_length;
  /*! \var history_start
   *  \brief How long ago to start plotting `history_length`, in minutes
   */
  float history_start;
  /*! \var pgplot_device
   *  \brief The PGPLOT device number to use for the plotting
   */
  int pgplot_device;
  /*! \var nvisbands
   *  \brief The number of bands to plot
   */
  int nvisbands;
  /*! \var visbands
   *  \brief The name of the bands to plot
   *
   * This 1-D array of strings has length `nvisbands` and is indexed starting
   * at 0. Each string has length VISBANDLEN.
   */
  char **visbands;
  /*! \var cycletime
   *  \brief The cycle time, in seconds
   */
  int cycletime;
  /*! \var reference_antenna
   *  \brief The reference antenna to use while plotting closure phase, as the
   *         antenna number
   */
  int reference_antenna;
};

// This structure holds details about the lines we are asked to
// plot in the VIS-type plots.
/*! \struct vis_line
 *  \brief A descriptive set of parameters for each product displayed on
 *         a vis display
 */
struct vis_line {
  /*! \var ant1
   *  \brief The first antenna of the baseline pair
   */
  int ant1;
  /*! \var ant2
   *  \brief The second antenna of the pair, or the same antenna if it's
   *         an autocorrelation product or a single antenna parameter
   */
  int ant2;
  /*! \var if_label
   *  \brief The label of the IF that this product comes from
   */
  char if_label[BUFSIZE];
  /*! \var pol
   *  \brief The polarisation code for this product
   */
  int pol;
  /*! \var label
   *  \brief The label that describes this product on the plot
   */
  char label[BUFSIZE];
  /*! \var pgplot_colour
   *  \brief The PGPLOT colour index to use when plotting this line
   */
  int pgplot_colour;
  /*! \var line_style
   *  \brief The PGPLOT line style setting to use when plotting this line
   */
  int line_style;
  /*! \var baseline_length
   *  \brief The length of this baseline, to use when sorting the products
   */
  float baseline_length;
  /*! \var bin_index
   *  \brief The bin index from which to get the data to plot
   */
  int bin_index;
};

void count_polarisations(struct spd_plotcontrols *plotcontrols);
int cmpfunc_baseline_length(const void *a, const void *b);
int plot_colour_averaing(int plot_colour);
void change_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
                             int *xaxis_type, int *yaxis_type, int *pols,
			     int *decorations);
int change_spd_plotflags(struct spd_plotcontrols *plotcontrols,
                         long int changed_flags, int add_remove);
void init_spd_plotcontrols(struct spd_plotcontrols *plotcontrols,
                           int xaxis_type, int yaxis_type, int pols,
                           int dectorations, int pgplot_device);
void change_vis_plotcontrols_visbands(struct vis_plotcontrols *plotcontrols,
                                      int nvisbands, char **visbands);
void change_vis_plotcontrols_limits(struct vis_plotcontrols *plotcontrols,
                                    int paneltype, bool use_limits,
                                    float limit_min, float limit_max);
bool product_can_be_x(int product);
void change_vis_plotcontrols_panels(struct vis_plotcontrols *plotcontrols,
                                    int xaxis_type, int num_panels,
                                    int *paneltypes, struct panelspec *panelspec);
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
                      int plot_baseline_idx, int plot_if_idx,
		      int npols, int *polidx,
                      float *plotmin_x, float *plotmax_x,
                      float *plotmin_y, float *plotmax_y);
void add_vis_line(struct vis_line ***vis_lines, int *n_vis_lines,
                  int ant1, int ant2, int ifnum, char *if_label, int pol,
                  int colour, int line_style, int bin_index,
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
                   struct scan_header_data **header_data,
                   struct metinfo **metinfo, struct syscal_data **syscal_data,
		   int num_times, float *times);
void make_spd_plot(struct ampphase ***cycle_ampphase, struct panelspec *panelspec,
                   struct spd_plotcontrols *plot_controls,
                   struct scan_header_data *scan_header_data,
                   struct syscal_data *compiled_tsys_data,
                   int max_tsys_ifs, bool all_data_present);
int determine_filetype(char *f);
int filename_to_pgplot_device(char *f, char *d, size_t l, int type);
