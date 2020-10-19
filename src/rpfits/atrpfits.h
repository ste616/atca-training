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
  /*! \var obstype
   *  \brief The type of observation this scan describes.
   *
   * The observation type is something like:
   * - Dwell
   * - Point
   */
  char obstype[OBSTYPE_LENGTH];
  /*! \var calcode
   *  \brief The calibrator code setting for this scan.
   */
  char calcode[CALCODE_LENGTH];
  /*! \var cycle_time
   *  \brief The cycle time for this scan, in seconds.
   *
   * This parameter comes from the RPFITS variable `param_.intime`.
   */
  int cycle_time;
  /*! \var source_name
   *  \brief The name of the source, supplied by the observer.
   *
   * The observer can name each scan whatever they like as the
   * source name, and this name is represented here.
   */
  char source_name[SOURCE_LENGTH];
  // Source coordinates.
  /*! \var rightascension_hours
   *  \brief The right ascension of the correlated phase centre, in hours.
   */
  float rightascension_hours;
  /*! \var declination_degrees
   *  \brief The declination of the correlated phase centre, in degrees.
   */
  float declination_degrees;
  // Frequency configuration.
  /*! \var num_ifs
   *  \brief The number of IFs stored in this scan.
   *
   * The number of intermediate frequencies (IFs, or windows) is stored
   * in this variable, and can not change within a single scan.
   */
  int num_ifs;
  /*! \var if_centre_freq
   *  \brief The frequency at the centre of each IF, in MHz.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * The centre frequency of the central channel of the IF, in MHz.
   */
  float *if_centre_freq;
  /*! \var if_bandwidth
   *  \brief The total bandwidth of each IF, in MHz.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * The total bandwidth covered by all the channels of the IF, in MHz.
   */
  float *if_bandwidth;
  /*! \var if_num_channels
   *  \brief The number of channels of each IF.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   */
  int *if_num_channels;
  /*! \var if_num_stokes
   *  \brief The number of Stokes parameters of each IF.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * For CABB, this should always be 4 (XX, YY, XY, YX).
   */
  int *if_num_stokes;
  /*! \var if_sideband
   *  \brief The sideband of each IF.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * If the IF is upper sideband (USB), this variable will have value 1.
   * If the IF is lower sideband (LSB), this variable will have value -1.
   */
  int *if_sideband;
  /*! \var if_chain
   *  \brief The RF chain used for each IF.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * The RF signal from the receivers are first divided into broad IFs.
   * For CABB, the first division is into 2 IFs, and each zoom will come
   * from one of these two divisions. This variable describes which of the
   * broad divisions each IF comes from.
   *
   * **This variable has no 0 value. The first IF division is labelled 1.**
   */
  int *if_chain;
  /*! \var if_label
   *  \brief The label of each IF.
   *
   * Each IF in this scan has an entry in this array of size num_ifs.
   *
   * Each IF is given a numbered label, starting at 1, which is stored
   * in this variable.
   *
   * **This variable has no 0 value. The first IF is labelled 1.**
   */
  int *if_label;
  /*! \var if_name
   *  \brief A collection of names for each IF.
   *
   * Each IF in this scan has an entry in this array. This array has
   * dimensions [num_ifs][3][8]. That is, each IF has 3 names, each being
   * a string of length 8. These two magic numbers are hard-coded, and this
   * should be fixed.
   *
   * Each IF has the following three names:
   * 1. The simple name, like `f1` or `f3`; simply `f` followed by the
   *    number stored in if_label.
   * 2. The classified name, like `f1` or `z1`; if `f`, this is a band
   *    with coarse resolution (the continuum bands for CABB), and if `z`
   *    this is a zoom band. The numbering is incremental in order of the
   *    bands as specified in this header. That is, the first zoom encountered
   *    while traversing num_ifs in ascending order will be labelled `z1`.
   * 3. The informative name, like `f1` or `z1-1`. The same name as 2 will
   *    be used for continuum bands, but for each zoom, the chain number is
   *    given first, then the number of the zoom within that chain second.
   *    For example, if all 16 zooms are individual specified for each IF
   *    (CABB), then name 2 will go from `z1` to `z16` for the zooms in chain 1,
   *    and from `z17` to `z32` in chain 2. This name 3 however will go
   *    from `z1-1` to `z1-16` for the zooms in chain 1, and from `z2-1` to
   *    `z2-16` in chain 2.
   */
  char ***if_name;
  /*! \var if_stokes_names
   *  \brief The names of the Stokes parameters present for each IF.
   *
   * Each IF in this scan has an entry in this array. This array has
   * dimensions [num_ifs][if_num_stokes[n_if]][3], where n_if is the
   * index of the IF (each IF can have different numbers of Stokes parameters,
   * although for CABB this never occurs). The magic number 3 is hard-coded
   * and should be fixed.
   *
   * Each Stokes parameter of each IF has a name like `XX` or `YY` or `XY`.
   */
  char ***if_stokes_names;
  /*! \var num_ants
   *  \brief The number of antennas in the data in this scan.
   *
   * For CABB, all antennas are always stored regardless of whether they
   * were on-line or not. This number should always be 6 for CABB, but this
   * is not an RPFITS restriction.
   */
  int num_ants;
  /*! \var ant_label
   *  \brief The label of each antenna.
   *
   * Each antenna in this scan has an entry in this array of size num_ants.
   *
   * The antenna label is simply the number of the antenna, starting at 1 for
   * the ATCA antennas.
   */
  int *ant_label;
  /*! \var ant_name
   *  \brief The name of each antenna, which for ATCA is actually the
   *         name of the station each antenna is connected to.
   *
   * Each antenna in this scan has an entry in this array. This array has
   * dimensions [num_ants][9]. The magic number 9 is hard-coded and should
   * be fixed.
   *
   * Example names will be `W104` or `N2`.
   */
  char **ant_name;
  /*! \var ant_cartesian
   *  \brief The coordinates of each antenna on the WGS84 Cartesian plane, in m.
   *
   * Each antenna in this scan has an entry in this array. This array has
   * dimensions [num_ants][3]. The magic number 3 is hard-coded and should
   * be fixed.
   *
   * The coordinates are listed in X=0, Y=1, Z=2 index order of the second
   * array dimension, and are given in m. Each coordinate is that assumed by
   * the correlator, and thus (for ATCA) will represent the corrected location
   * calculated during the baseline length determination after each reconfiguration.
   */
  double **ant_cartesian;
};

/* Indexing for the metadata arrays. */
/*! \def CAL_XX
 *  \brief The array indexing magic number needed to access X-pol data in the SYSCAL arrays.
 */
#define CAL_XX 0
/*! \def CAL_YY
 *  \brief The array indexing magic number needed to access Y-pol data in the SYSCAL arrays.
 */
#define CAL_YY 1

/* Some integer values for the syscal data. */
#define SYSCAL_FLAGGED_GOOD 1
#define SYSCAL_FLAGGED_BAD  0
/*! \def SYSCAL_TSYS_APPLIED
 *  \brief Magic number to indicate that the data has been scaled according to the
 *         system temperature.
 *
 * This flag is intended to indicate that the data within a structure has been scaled
 * according to the system temperature. This flag should never be encountered as the value
 * for both the system temperature measured by the correlator, and the system temperature
 * measured by this library.
 */
#define SYSCAL_TSYS_APPLIED     1
/*! \def SYSCAL_TSYS_NOT_APPLIED
 *  \brief Magic number to indicate that the data has **not** been scaled according to
 *         the system temperature.
 *
 * This flag is intended to indicate that the data within a structure has not been scaled
 * according to the system temperature that the correlator measured at the time the
 * data was taken, or the system temperature calculated by this library, depending on which
 * variable has this value. Since the ATCA correlator is almost always instructed to scale the
 * data by the Tsys, if this flag is encountered for the online-Tsys variable, it almost 
 * certainly means that the scaling has been reversed by this software.
 */
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
