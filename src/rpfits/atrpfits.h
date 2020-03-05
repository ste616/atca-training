/**
 * ATCA Training Library: reader.h
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library is designed to read an RPFITS file and for each cycle make available
 * the products which would be available online: spectra, amp, phase, delay,
 * u, v, w, etc.
 * This is so we can make tools which will let observers muck around with what they 
 * would see online in all sorts of real situations, as represented by real data 
 * files.
 * 
 * This module contains the structures and common-use functions necessary for all 
 * the other routines.
 */

#pragma once

#include <complex.h>

// Some check values.
#define RPFITS_FLAG_BAD 1
#define RPFITS_FLAG_GOOD 0

#define OBSDATE_LENGTH 12
#define OBSTYPE_LENGTH 16
#define SOURCE_LENGTH 16
#define CALCODE_LENGTH 4

// The largest number of baselines we support.
// This is for up to 100 antennas.
#define MAX_BASELINENUM 25700

#define FILENAME_LENGTH 2048

/**
 * This structure holds all the header information data.
 */
struct scan_header_data {
  // Time variables.
  char obsdate[OBSDATE_LENGTH];
  float ut_seconds;
  // Details about the observation.
  char obstype[OBSTYPE_LENGTH];
  char calcode[CALCODE_LENGTH];
  int cycle_time;
  // Name of the source.
  char source_name[SOURCE_LENGTH];
  // Source coordinates.
  float rightascension_hours;
  float declination_degrees;
  // Frequency configuration.
  int num_ifs;
  float *if_centre_freq;
  float *if_bandwidth;
  int *if_num_channels;
  int *if_num_stokes;
  int *if_sideband;
  int *if_chain;
  int *if_label;
  char ***if_name;
  char ***if_stokes_names;
  int num_ants;
  int *ant_label;
  char **ant_name;
  double **ant_cartesian;
};

/**
 * This structure holds all the cycle data.
 */
struct cycle_data {
  // Time variables.
  float ut_seconds;
  // Antennas and baselines.
  int num_points;
  int all_baselines[MAX_BASELINENUM];
  int n_baselines;
  float *u;
  float *v;
  float *w;
  int *ant1;
  int *ant2;
  // Flagging.
  int *flag;
  // Data.
  int *vis_size;
  float complex **vis;
  float **wgt;
  // Metadata.
  int *bin;
  int *if_no;
  char **source;
};

/**
 * A structure to hold everything together for a scan.
 */
struct scan_data {
  // Needs a header.
  struct scan_header_data header_data;
  // And any number of cycles.
  int num_cycles;
  struct cycle_data **cycles;
};

/**
 * A structure which describes how to quickly access data
 * in an RFPITS file.
 */
struct rpfits_index {
  char filename[FILENAME_LENGTH];
  int num_headers;
  long int *header_pos;
  int *num_cycles;
  long int **cycle_pos;
};

void base_to_ants(int baseline, int *ant1, int *ant2);
int ants_to_base(int ant1, int ant2);

#include "reader.h"
#include "compute.h"
