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
 * Structure to hold amplitude and phase quantities.
 */
struct ampphase {
  // The number of quantities in each array.
  int nchannels;
  int nbaselines;

  // Now the arrays storing the labels for each quantity.
  int *channel;
  float *frequency; // in GHz.
  int *baseline;

  // The static quantities.
  int pol;
  int window;
  int bin;
  
  // The arrays here have the following indexing.
  // array[baseline][channel]
  // Weighting.
  float **weight;
  // Amplitude.
  float **amplitude;
  // Phase.
  float **phase;

  // Some metadata.
  float min_amplitude_global;
  float max_amplitude_global;
  float min_phase_global;
  float max_phase_global;
  float *min_amplitude;
  float *max_amplitude;
  float *min_phase;
  float *max_phase;
};

struct ampphase* prepare_ampphase(void);
void free_ampphase(struct ampphase *ampphase);
int polarisation_number(char *polstring);
int vis_ampphase(struct scan_header_data *scan_header_data,
		 struct cycle_data *cycle_data,
		 struct ampphase *ampphase, int pol, int ifno, int bin);
