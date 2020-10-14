/** \file atrpfits.h
 *  \brief Definitions for low-level RPFITS-related parameters.
 *
 * ATCA Training Library: atrpfits.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This file contains the structures and common-use functions necessary for all 
 * the other routines, along with some magic numbers.
 *
 */

#pragma once

#include <complex.h>

// Some check values.
/*! \def RPFITS_FLAG_BAD
 *  \brief A magic number for bad data. **UNUSED**
 *
 *  This magic number is currently unused.
 */
#define RPFITS_FLAG_BAD 1
/*! \def RPFITS_FLAG_GOOD
 *  \brief A magic number for good data. **UNUSED**
 *
 *  This magic number is currently unused.
 */
#define RPFITS_FLAG_GOOD 0

/*! \def OBSDATE_LENGTH
 *  \brief The length of an RPFITS date string.
 *
 *  This length is set in the RPFITS definition of `nx_date` to be 12.
 *
 *  All RPFITS date strings are formatted like 2020-11-20
 *  (10 characters).
 */
#define OBSDATE_LENGTH 12
/*! \def OBSTYPE_LENGTH
 *  \brief The length of an RPFITS observation type string.
 *
 * This length is set in the RPFITS definition of `obstype` to be 16.
 *
 * Examples of RPFITS observation types are:
 * - DWELL
 * - POINT
 */
#define OBSTYPE_LENGTH 16
/*! \def SOURCE_LENGTH
 *  \brief The length of an RPFITS source name string.
 *
 * This length is set in the RPFITS definition of `nx_source` to be 16.
 */
#define SOURCE_LENGTH 16
/*! \def CALCODE_LENGTH
 *  \brief The length of an RPFITS calibrator code string.
 *
 * This length is set in the RPFITS definition of `su_cal` to be 4.
 *
 * Examples of RPFITS calibrator codes are:
 * - C (usually for a known calibrator)
 * - B (observed during a baseline solution)
 */
#define CALCODE_LENGTH 4

/*! \def MAX_BASELINENUM
 *  \brief The largest number of baselines we support.
 *
 * This parameter sets the length of the all_baselines array in the
 * cycle_data structure. The baseline number is determined with the
 * ants_to_base routine. This number allows for up to 100 antennas,
 * which is not really necessary for ATCA use, but will mean it doesn't
 * need altering should this library be used for larger arrays.
 *
 * We use this array in this way so we can quickly and efficiently
 * identify which baseline pairs are available in the data (by marking
 * the array entry according to the unique baseline number).
 */
#define MAX_BASELINENUM 25700

/*! \def FILENAME_LENGTH
 *  \brief The maximum length of a filename we can use.
 *
 * This length is set in the RPFITS definition of `file` to be 256.
 */
#define FILENAME_LENGTH 256

/*! \struct scan_header_data
 *  \brief This structure holds all the header information data.
 *
 * When reading from the RPFITS file, each scan has a header which
 * describes the metadata for that scan. The reader routine takes
 * the header data and stores it in this structure for easy access.
 */
struct scan_header_data {
  // Time variables.
  /*! \var obsdate
   *  \brief The base date of the scan.
   *
   * The base date of the scan will be the date when the file
   * containing the scan was opened. It is therefore possible
   * that this variable does not represent the actual date of
   * the scan, but this can be determined with the ut_seconds
   * variable. All dates have the format YYYY-MM-DD, where MM
   * and DD start at 01, and are in UTC.
   */
  char obsdate[OBSDATE_LENGTH];
  /*! \var ut_seconds
   *  \brief The number of seconds after the base date that this
   *         scan was started.
   *
   * This scan started this number of seconds after UTC midnight
   * on the base date represented by the obsdate variable.
   */
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

/* Indexing for the metadata arrays. */
#define CAL_XX 0
#define CAL_YY 1

/* Some integer values for the syscal data. */
#define SYSCAL_FLAGGED_GOOD 1
#define SYSCAL_FLAGGED_BAD  0
#define SYSCAL_TSYS_APPLIED     1
#define SYSCAL_TSYS_NOT_APPLIED 0
#define SYSCAL_VALID   0
#define SYSCAL_INVALID 1

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
  // Labelling.
  int *bin;
  int *if_no;
  char **source;
  // Metadata.
  int num_cal_ifs;
  int num_cal_ants;
  int *cal_ifs;
  int *cal_ants;
  // This is indexed [IF][ANT][POL]
  float ***tsys;
  int ***tsys_applied;
  float ***computed_tsys;
  int ***computed_tsys_applied;
  // These are indexed [IF][ANT].
  float **xyphase;
  float **xyamp;
  float **parangle;
  float **tracking_error_max;
  float **tracking_error_rms;
  float **gtp_x;
  float **gtp_y;
  float **sdo_x;
  float **sdo_y;
  float **caljy_x;
  float **caljy_y;
  int **flagging;
  // Weather metadata.
  float temperature;
  float air_pressure;
  float humidity;
  float wind_speed;
  float wind_direction;
  float rain_gauge;
  int weather_valid;
  float seemon_phase;
  float seemon_rms;
  int seemon_valid;
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
