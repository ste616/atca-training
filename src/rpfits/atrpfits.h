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
 * This header describes the structures necessary for all the other routines.
 */

#pragma once

// Some check values.
#define RPFITS_FLAG_BAD 1
#define RPFITS_FLAG_GOOD 0

#define OBSDATE_LENGTH 12
#define OBSTYPE_LENGTH 16
#define SOURCE_LENGTH 16
#define CALCODE_LENGTH 4

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
};

/**
 * This structure holds all the cycle data.
 */
struct cycle_data {
  // Time variables.
  float ut_seconds;
  // Antennas and baselines.
  int num_points;
  float *u;
  float *v;
  float *w;
  int *ant1;
  int *ant2;
  // Flagging.
  int *flag;
  // Data.
  float **vis;
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
