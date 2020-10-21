/** \file compute.h
 *  \brief Definitions for structures and parameters pertaining to low-level computations
 *         on the raw data
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module handles computing quantities like amplitude, phase, delay etc.
 * using the vis data, and system temperatures.
 */

#pragma once
#include <stdbool.h>
#include "atrpfits.h"

/**
 * Some polarisation flags.
 */
/*! \def POL_X
 *  \brief Magic number used for RPFITS pol X, listed in STRPOL_X
 */
#define    POL_X  1
/*! \def STRPOL_X
 *  \brief The string we're looking for from RPFITS data to assign as POL_X
 */
#define STRPOL_X  "X "
/*! \def POL_Y
 *  \brief Magic number used for RPFITS pol Y, listed in STRPOL_Y
 */
#define    POL_Y  2
/*! \def STRPOL_Y
 *  \brief The string we're looking for from RPFITS data to assign as POL_Y
 */
#define STRPOL_Y  "Y "
/*! \def POL_XX
 *  \brief Magic number used for RPFITS product XX, listed in STRPOL_XX
 */
#define    POL_XX 3
/*! \def STRPOL_XX
 *  \brief The string we're looking for from RPFITS data to assign as POL_XX
 */
#define STRPOL_XX "XX"
/*! \def POL_YY
 *  \brief Magic number used for RPFITS product YY, listed in STRPOL_YY
 */
#define    POL_YY 4
/*! \def STRPOL_YY
 *  \brief The string we're looking for from RPFITS data to assign as POL_YY
 */
#define STRPOL_YY "YY"
/*! \def POL_XY
 *  \brief Magic number used for RPFITS product XY, listed in STRPOL_XY
 */
#define    POL_XY 5
/*! \def STRPOL_XY
 *  \brief The string we're looking for from RPFITS data to assign as POL_XY
 */
#define STRPOL_XY "XY"
/*! \def POL_YX
 *  \brief Magic number used for RPFITS product YX, listed in STRPOL_YX
 */
#define    POL_YX 6
/*! \def STRPOL_YX
 *  \brief The string we're looking for from RPFITS data to assign as POL_YX
 */
#define STRPOL_YX "YX"

/**
 * Magic numbers for averaging types.
 */
/*! \def AVERAGETYPE_MEAN
 *  \brief A magic number used to indicate that the mean value should be used
 *         as the average value when computing vis properties
 *
 * This is a bitwise OR magic number along with the other AVERAGETYPE_* values.
 */
#define AVERAGETYPE_MEAN   1
/*! \def AVERAGETYPE_MEDIAN
 *  \brief A magic number used to indicate that the median value should be used
 *         as the average value when computing vis properties
 *
 * This is a bitwise OR magic number along with the other AVERAGETYPE_* values.
 */
#define AVERAGETYPE_MEDIAN 2
/*! \def AVERAGETYPE_VECTOR
 *  \brief A magic number used to indicate that the phase value should be considered
 *         while computing average values for vis
 *
 * This is a bitwise OR magic number along with the other AVERAGETYPE_* values.
 */
#define AVERAGETYPE_VECTOR 4
/*! \def AVERAGETYPE_SCALAR
 *  \brief A magic number used to indicate that the phase value should be discarded
 *         while computing average values for vis
 *
 * This is a bitwise OR magic number along with the other AVERAGETYPE_* values.
 */
#define AVERAGETYPE_SCALAR 8

/*! \struct ampphase_options
 *  \brief Structure to hold options controlling how to take raw data and produce
 *         more useful quantities like amplitude and phase.
 */
struct ampphase_options {
  /*! \var phase_in_degrees
   *  \brief Flag to instruct the library to calculate phases in degrees (if set to
   *         true) or radians (if set to false).
   */
  bool phase_in_degrees;

  // Whether to include flagged data in vis outputs.
  /*! \var include_flagged_data
   *  \brief Flag to instruct the library to include flagged channels when producing
   *         tvchannel-averaged data (if set to 1), or to discard flagged channels
   *         (if set to 0)
   */
  int include_flagged_data;
  
  // The "tv" channels can be set independently for
  // all the IFs.
  /*! \var num_ifs
   *  \brief The number of IFs that have information in the tvchannel arrays
   *         in this structure
   */
  int num_ifs;
  /*! \var min_tvchannel
   *  \brief The lowest numbered channel to include in the tvchannel range for
   *         each IF
   *
   * This array has size `num_ifs`, and is indexed starting at 0.
   */
  int *min_tvchannel;
  int *max_tvchannel;

  // The delay averaging factor (default is 1).
  int *delay_averaging;
  
  // The method to do the averaging (default is mean).
  int *averaging_method;

};

/**
 * Structure to hold meteorological information.
 */
struct metinfo {
  // Labelling.
  char obsdate[OBSDATE_LENGTH];
  float ut_seconds;

  // Quantities from the weather station.
  float temperature;
  float air_pressure;
  float humidity;
  float wind_speed;
  float wind_direction;
  float rain_gauge;
  bool weather_valid;

  // Quantities from the seeing monitor.
  float seemon_phase;
  float seemon_rms;
  bool seemon_valid;
};

/**
 * Structure to hold system calibration data.
 */
struct syscal_data {
  // Labelling.
  char obsdate[OBSDATE_LENGTH];
  float utseconds;

  // Array sizes.
  int num_ifs;
  int num_ants;
  int num_pols;

  // Array descriptors.
  int *if_num;
  int *ant_num;
  int *pol;

  // Parameters that only depend on antenna.
  float *parangle;
  float *tracking_error_max;
  float *tracking_error_rms;
  int *flagging;

  // Parameters that vary for each antenna and IF.
  float **xyphase;
  float **xyamp;

  // The system temperatures (varies by antenna, IF and pol).
  float ***online_tsys;
  int ***online_tsys_applied;
  float ***computed_tsys;
  int ***computed_tsys_applied;

  // Parameters measured by the correlator (varies by antenna, IF and pol).
  float ***gtp;
  float ***sdo;
  float ***caljy;
};

/*! \struct ampphase
 *  \brief Structure to hold amplitude and phase quantities and associated
 *         metadata
 *
 * This structure is the primary way to get access to all the data from a
 * particular product. Each structure represents a single IF and polarisation
 * from a single cycle, but contains all the baselines, channels and bins.
 * Both the raw complex data is stored here, along with the amplitudes and
 * phases computed from them. Two copies of the data are included, one which
 * has all the data, and another which excludes the data that was flagged bad.
 *
 * Metadata about the data ranges are available here to make it easier to plot
 * this data, The options used to compute this data is linked here, and the
 * meteorological and system calibration metadata is also available.
 */
struct ampphase {
  // The number of quantities in each array.
  /*! \var nchannels
   *  \brief The number of channels for all the unflagged arrays in this
   *         structure; this is how many channels were computed in this IF
   */
  int nchannels;
  /*! \var nbaselines
   *  \brief The number of baselines for all the arrays in this structure
   *
   * For CABB, this is almost always 21, which includes 6 auto-correlations
   * and 15 cross-correlations.
   */
  int nbaselines;

  // Now the arrays storing the labels for each quantity.
  /*! \var channel
   *  \brief The channel number for each channel
   *
   * This array has length `nchannels` and is indexed starting at 0.
   *
   * This is a float array so that it can be used while plotting scatter plots,
   * especially in PGPLOT which requires the arrays to be floats.
   */
  float *channel;
  /*! \var frequency
   *  \brief The central frequency of each channel, in GHz
   *
   * This array has length `nchannels` and is indexed starting at 0.
   */
  float *frequency;
  /*! \var baseline
   *  \brief The baseline number of each baseline
   *
   * This array has length `nbaselines` and is indexed starting at 0.
   */
  int *baseline;

  // The static quantities.
  /*! \var pol
   *  \brief The polarisation represented by these data
   *
   * The value of this variable will be one of the POL_* magic numbers
   * defined at the top of this header file.
   */
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
  float *min_real;
  float *max_real;
  float *min_imag;
  float *max_imag;
  struct ampphase_options *options;
  struct metinfo metinfo;
  struct syscal_data *syscal_data;
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

// This structure wraps around all the ampphase structures and
// will allow for easy transport.
struct spectrum_data {
  // The spectrum header.
  struct scan_header_data *header_data;
  // The number of IFs in this spectra set.
  int num_ifs;
  // The number of polarisations.
  int num_pols;
  // The ampphase structures.
  struct ampphase ***spectrum;
};

// This structure wraps around all the vis_quantities structures
// and will allow for easy transport.
struct vis_data {
  // The number of cycles contained here.
  int nviscycles;
  // The range of MJDs that were allowed when these data
  // were compiled.
  double mjd_low;
  double mjd_high;
  // The header data for each cycle.
  struct scan_header_data **header_data;
  // The number of IFs per cycle.
  int *num_ifs;
  // The number of pols per cycle per IF.
  int **num_pols;
  // The vis_quantities structures.
  struct vis_quantities ****vis_quantities;
  // The metinfo for each cycle.
  struct metinfo **metinfo;
  // And the calibration parameters.
  struct syscal_data **syscal_data;
};

float fmedianf(float *a, int n);
struct ampphase* prepare_ampphase(void);
struct vis_quantities* prepare_vis_quantities(void);
void free_ampphase(struct ampphase **ampphase);
void free_vis_quantities(struct vis_quantities **vis_quantities);
int polarisation_number(char *polstring);
struct ampphase_options ampphase_options_default(void);
void set_default_ampphase_options(struct ampphase_options *options);
void copy_ampphase_options(struct ampphase_options *dest,
                           struct ampphase_options *src);
void free_ampphase_options(struct ampphase_options *options);
void copy_metinfo(struct metinfo *dest,
                  struct metinfo *src);
void copy_syscal_data(struct syscal_data *dest,
		      struct syscal_data *src);
void free_syscal_data(struct syscal_data *syscal_data);
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
bool ampphase_options_match(struct ampphase_options *a,
                            struct ampphase_options *b);
void calculate_system_temperatures(struct ampphase *ampphase,
                                   struct ampphase_options *options);
void spectrum_data_compile_system_temperatures(struct spectrum_data *spectrum_data,
					       struct syscal_data **syscal_data);
