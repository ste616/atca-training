/**
 * ATCA Training Library: compute.h
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library is designed to read an RPFITS file and for each cycle make available
 * the products which would be available online: spectra, amp, phase, delay,
 * u, v, w, etc.
 * This is so we can make tools which will let observers muck around with what they 
 * would see online in all sorts of real situations, as represented by real data 
 * files.
 * 
 * This module handles computing quantities like amplitude, phase, delay etc.
 * using the vis data.
 */

#pragma once
#include <stdbool.h>
#include "atrpfits.h"

/**
 * Some polarisation flags.
 */
#define    POL_X  1
#define STRPOL_X  "X "
#define    POL_Y  2
#define STRPOL_Y  "Y "
#define    POL_XX 3
#define STRPOL_XX "XX"
#define    POL_YY 4
#define STRPOL_YY "YY"
#define    POL_XY 5
#define STRPOL_XY "XY"
#define    POL_YX 6
#define STRPOL_YX "YX"

/**
 * Magic numbers for averaging types.
 */
#define AVERAGETYPE_MEAN   1
#define AVERAGETYPE_MEDIAN 2
#define AVERAGETYPE_VECTOR 4
#define AVERAGETYPE_SCALAR 8

/**
 * Structure to hold options to pass to the function
 * that computes the amplitude and phase.
 */
struct ampphase_options {
  // Return phase in degrees (default is radians).
  bool phase_in_degrees;

  // The delay averaging factor (default is 1).
  int delay_averaging;

  // The "tv" channels can be set independently for
  // all the IFs.
  int num_ifs;
  int *min_tvchannel;
  int *max_tvchannel;
  
  // The method to do the averaging (default is mean).
  int averaging_method;

  // Whether to include flagged data in vis outputs.
  int include_flagged_data;
};

/**
 * Structure to hold amplitude and phase quantities.
 */
struct ampphase {
  // The number of quantities in each array.
  int nchannels;
  int nbaselines;

  // Now the arrays storing the labels for each quantity.
  float *channel;
  float *frequency; // in GHz.
  int *baseline;

  // The static quantities.
  int pol;
  int window;
  char window_name[8];
  char obsdate[OBSDATE_LENGTH];
  float ut_seconds;
  char scantype[OBSTYPE_LENGTH];
  
  // The bin arrays have one element per baseline.
  int *nbins;

  // The flag array has the indexing:
  // array[baseline][bin]
  int **flagged_bad;
  
  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  // Weighting.
  float ***weight;
  // Amplitude.
  float ***amplitude;
  // Phase.
  float ***phase;
  // The raw data.
  float complex ***raw;

  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  int **f_nchannels;
  float ***f_channel;
  float ***f_frequency;
  float ***f_weight;
  float ***f_amplitude;
  float ***f_phase;
  float complex ***f_raw;
  
  // Some metadata.
  float min_amplitude_global;
  float max_amplitude_global;
  float min_phase_global;
  float max_phase_global;
  float *min_amplitude;
  float *max_amplitude;
  float *min_phase;
  float *max_phase;
  struct ampphase_options *options;
};

/**
 * Structure to hold average amplitude, phase and delay
 * quantities.
 */
struct vis_quantities {
  // The options that were used.
  struct ampphase_options *options;

  // Number of quantities in the array.
  int nbaselines;

  // The time.
  char obsdate[OBSDATE_LENGTH];
  float ut_seconds;
  
  // Labels.
  int pol;
  int window;
  int *nbins;
  int *baseline;
  int *flagged_bad;
  char scantype[OBSTYPE_LENGTH];
  
  // The arrays.
  float **amplitude;
  float **phase;
  float **delay;

  // Metadata.
  float min_amplitude;
  float max_amplitude;
  float min_phase;
  float max_phase;
  float min_delay;
  float max_delay;
};

struct ampphase* prepare_ampphase(void);
struct vis_quantities* prepare_vis_quantities(void);
void free_ampphase(struct ampphase **ampphase);
void free_vis_quantities(struct vis_quantities **vis_quantities);
int polarisation_number(char *polstring);
struct ampphase_options ampphase_options_default(void);
void copy_ampphase_options(struct ampphase_options *dest,
                           struct ampphase_options *src);
void default_tvchannels(int num_chan, float chan_width,
			float centre_freq, int *min_tvchannel,
			int *max_tvchannel);
void add_tvchannels_to_options(struct ampphase_options *ampphase_options,
			       int window, int min_tvchannel, int max_tvchannel);
int vis_ampphase(struct scan_header_data *scan_header_data,
		 struct cycle_data *cycle_data,
		 struct ampphase **ampphase, int pol, int ifno,
		 struct ampphase_options *options);
int cmpfunc_real(const void *a, const void *b);
int cmpfunc_complex(const void *a, const void *b);
int ampphase_average(struct ampphase *ampphase,
		     struct vis_quantities **vis_quantities,
		     struct ampphase_options *options);
