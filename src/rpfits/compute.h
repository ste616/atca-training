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
   *
   * Because all the arrays in this structure have size `num_ifs`, and all are
   * indexed directly by the RPFITS window parameter which starts at 1, this is
   * actually the largest window number we know about plus 1.
   */
  int num_ifs;
  /*! \var min_tvchannel
   *  \brief The lowest numbered channel to include in the tvchannel range for
   *         each IF
   *
   * This array has size `num_ifs`, and is indexed starting at 1.
   */
  int *min_tvchannel;
  /*! \var max_tvchannel
   *  \brief The highest numbered channel to include in the tvchannel range for
   *         each IF
   *
   * This array has size `num_ifs`, and is indexed starting at 1.
   */
  int *max_tvchannel;

  /*! \var delay_averaging
   *  \brief The number of channels to average together while computing the delay
   *         in each IF
   *
   * This array has size `num_ifs`, and is indexed starting at 1.
   */
  int *delay_averaging;
  
  /*! \var averaging_method
   *  \brief The type of averaging used when computing parameters within the
   *         tvchannel range.
   *
   * This array has size `num_ifs`, and is indexed starting at 1.
   *
   * The magic numbers to use when setting this parameter are listed at the top
   * of this header file: AVERAGETYPE_*.
   */
  int *averaging_method;

};

/*! \struct metinfo
 *  \brief Structure to hold meteorological information for a cycle
 */
struct metinfo {
  // Labelling.
  /*! \var obsdate
   *  \brief String representation of the base date of the file that
   *         the data in this structure represents
   */
  char obsdate[OBSDATE_LENGTH];
  /*! \var ut_seconds
   *  \brief The number of seconds past midnight UTC on `obsdate` at the midpoint
   *         of this cycle
   */
  float ut_seconds;

  // Quantities from the weather station.
  /*! \var temperature
   *  \brief The ambient temperature as measured by the site weather station, in C
   */
  float temperature;
  /*! \var air_pressure
   *  \brief The air pressure as measured by the site weather station, in mBar
   */
  float air_pressure;
  /*! \var humidity
   *  \brief The relative humidity as measured by the site weather station, in %
   */
  float humidity;
  /*! \var wind_speed
   *  \brief The wind speed as measured by the site weather station, in km/h
   */
  float wind_speed;
  /*! \var wind_direction
   *  \brief The wind direction as measured by the site weather station, in deg,
   *         where 0 deg is North and 90 deg is East
   */
  float wind_direction;
  /*! \var rain_gauge
   *  \brief The amount of rain in the rain gauge, in mm
   *
   * This is a quantity that resets to 0 each day 9am local time.
   */
  float rain_gauge;
  /*! \var weather_valid
   *  \brief Flag to indicate if the meteorological data for this cycle can be
   *         trusted (true) or not (false)
   */
  bool weather_valid;

  // Quantities from the seeing monitor.
  /*! \var seemon_phase
   *  \brief The average phase value measured by the site atmospheric seeing monitor,
   *         in radians
   */
  float seemon_phase;
  /*! \var seemon_rms
   *  \brief The RMS phase variation observed by the site atmospheric seeing monitor
   *         during this cycle, in radians
   */
  float seemon_rms;
  /*! \var seemon_valid
   *  \brief Flag to indicate if the measurements from the atmospheric seeing monitor
   *         can be trusted (true) or not (false)
   */
  bool seemon_valid;
};

/*! \struct syscal_data
 *  \brief Structure to hold system calibration data for a cycle
 *
 * This structure contains a mixture of information that is presented in the RPFITS
 * SYSCAL record, including antenna-based parameters, and correlator-computed system
 * temperatures. It can also hold any system temperatures this library computes from
 * the data.
 */
struct syscal_data {
  // Labelling.
  /*! \var obsdate
   *  \brief String representation of the base date of the file that the data in
   *         this structure represents
   */
  char obsdate[OBSDATE_LENGTH];
  /*! \var utseconds
   *  \brief The number of seconds past midnight UTC on `obsdate` at the midpoint
   *         of this cycle
   */
  float utseconds;

  // Array sizes.
  /*! \var num_ifs
   *  \brief The number of windows with information in this structure
   */
  int num_ifs;
  /*! \var num_ants
   *  \brief The number of antennas with information in this structure
   *
   * For CABB, this should always be 6.
   */
  int num_ants;
  /*! \var num_pols
   *  \brief The number of polarisations with information in this structure
   *
   * For CABB, this should always be 2, since calibration is relevant only for
   * the actual receptors, not the cross-pol products.
   */
  int num_pols;

  // Array descriptors.
  /*! \var if_num
   *  \brief The window number for all the windows we have information for
   *
   * This array has length `num_ifs` and is indexed starting at 0.
   */
  int *if_num;
  /*! \var ant_num
   *  \brief The antenna number for all the antennas we have information for
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  int *ant_num;
  /*! \var pol
   *  \brief The polarisation magic number for all the polarisations we have
   *         information for
   *
   * This array has length `num_pols` and is indexed starting at 0. For CABB it
   * should be one of POL_XX or POL_YY, and POL_XX should always be at index CAL_XX,
   * and POL_YY should always be at index CAL_YY.
   */
  int *pol;

  // Parameters that only depend on antenna.
  /*! \var parangle
   *  \brief The parallactic angle for each antenna
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *parangle;
  /*! \var tracking_error_max
   *  \brief The maximum tracking error for each antenna during this cycle, in
   *         arcsec
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *tracking_error_max;
  /*! \var tracking_error_rms
   *  \brief The RMS tracking error for each antenna during this cycle, in
   *         arcsec
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *tracking_error_rms;
  /*! \var flagging
   *  \brief Flag reasoning for each antenna for the whole of this cycle
   *
   * This array has length `num_ants` and is indexed starting at 0. The value for
   * each antenna is a bitwise OR indication of why the antenna was flagged, or 0
   * if the antenna was not flagged bad:
   * - 1 the antenna was not on source
   * - 2 the correlator flagged it for pol X
   * - 4 the correlator flagged it for pol Y
   */
  int *flagging;

  // Parameters that vary for each antenna and IF.
  /*! \var xyphase
   *  \brief The XY phase measured by the correlator for each antenna and window,
   *         in degrees
   *
   * This 2-D array has length `num_ants` for the first index and `num_ifs` for
   * the second index. Each index starts at 0.
   */
  float **xyphase;
  /*! \var xyamp
   *  \brief The XY amplitude measured by the correlator for each antenna and window,
   *         in Jy
   *
   * This 2-D array has length `num_ants` for the first index and `num_ifs` for
   * the second index. Each index starts at 0.
   */
  float **xyamp;

  // The system temperatures (varies by antenna, IF and pol).
  /*! \var online_tsys
   *  \brief The system temperature as measured by the correlator for each antenna,
   *         window and polarisation, in K
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs`
   * for the second index, and `num_pols` for the third index. Each index starts at 0.
   */
  float ***online_tsys;
  /*! \var online_tsys_applied
   *  \brief Indication of whether the raw data has been corrected for the system
   *         temperatures of the antennas as computed by the correlator; 0 means
   *         that the raw data is just correlation coefficients, 1 means that the
   *         system temperature corrections have been made
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   */
  int ***online_tsys_applied;
  /*! \var computed_tsys
   *  \brief The system temperature as measured by this library for each antenna,
   *         window and polarisation, in K
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   */
  float ***computed_tsys;
  /*! \var computed_tsys_applied
   *  \brief Indication of whether the raw data has been corrected for the system
   *         temperatures of the antennas as computed by this library; 0 means
   *         that the raw data is just correlation coefficients, 1 means that the
   *         system temperature corrections have been made
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   *
   * This library should ensure that only one of the online or computed
   * system temperatures is applied to the raw data.
   */
  int ***computed_tsys_applied;

  // Parameters measured by the correlator (varies by antenna, IF and pol).
  /*! \var gtp
   *  \brief The gated total power as computed by the correlator for each antenna,
   *         window and polarisation, in Jy
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   */
  float ***gtp;
  /*! \var sdo
   *  \brief The synchronously demodulated output as computed by the correlator 
   *         for each antenna, window and polarisation, in Jy
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   */
  float ***sdo;
  /*! \var caljy
   *  \brief The flux density of the switching noise source on each antenna, window
   *         and polarisation
   *
   * This 3-D array has length `num_ants` for the first index, `num_ifs` for the
   * second index and `num_pols` for the third index. Each index starts at 0.
   */
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
  /*! \var window
   *  \brief The window represented by these data
   *
   * The data windows start at 1 and increment for each set of data that
   * is correlated at a different frequency or with a different number of
   * channels. Usually, windows 1 and 2 are the continuum bands, and any additional
   * windows represent zoom bands, but this is not guaranteed to be the case.
   * Windows are distinct from IFs since IFs represent the signal after mixing
   * to different frequencies, and a single IF can have multiple windows within it,
   * but often the two terminologies are used interchangably.
   */
  int window;
  /*! \var window_name
   *  \brief The classified IF name, from the header data's `if_label` variable
   */
  char window_name[8];
  /*! \var obsdate
   *  \brief String representation of the base date of the file that the data in
   *         this structure represents
   */
  char obsdate[OBSDATE_LENGTH];
  /*! \var ut_seconds
   *  \brief The number of seconds past midnight UTC on `obsdate` at the midpoint
   *         of this cycle
   */
  float ut_seconds;
  /*! \var scantype
   *  \brief The type of observation this scan describes, from `obstype` in the
   *         scan header
   */
  char scantype[OBSTYPE_LENGTH];
  
  // The bin arrays have one element per baseline.
  /*! \var nbins
   *  \brief The number of bins present per baseline
   *
   * This array has length `nbaselines` and is indexed starting at 0.
   */
  int *nbins;

  // The flag array has the indexing:
  // array[baseline][bin]
  /*! \var flagged_bad
   *  \brief An indication of whether the entire baseline and bin data is to be
   *         flagged bad (value 0 means good data)
   *
   * This 2-D array has length `nbaselines` for the first index, and `nbins[i]`
   * for the second index, and where `i` is the position along the first index, 
   * because each baseline may have a different number of bins. Each index starts at 0.
   */
  int **flagged_bad;
  
  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  /*! \var weight
   *  \brief The weight of the data, which is currently unused
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]`
   * for the second index (where `i` is the position along the first index), and
   * `nchannels` for the third index. Each index starts at 0.
   */
  float ***weight;
  /*! \var amplitude
   *  \brief The computed amplitude
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]`
   * for the second index (where `i` is the position along the first index), and
   * `nchannels` for the third index. Each index starts at 0.
   */
  float ***amplitude;
  /*! \var phase
   *  \brief The computed phase, the units of which are controlled by the variable
   *         `phase_in_degrees` in the ampphase_options structure
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]`
   * for the second index (where `i` is the position along the first index), and
   * `nchannels` for the third index. Each index starts at 0.
   */
  float ***phase;
  /*! \var raw
   *  \brief A copy of the raw complex data, from which the computed parameters
   *         were derived
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]`
   * for the second index (where `i` is the position along the first index), and
   * `nchannels` for the third index. Each index starts at 0.
   */
  float complex ***raw;

  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  /*! \var f_nchannels
   *  \brief The number of good channels per baseline and bin
   *
   * This 2-D array has length `nbaselines` for the first index, and `nbins[i]`
   * for the second index (where `i` is the position along the first index).
   * Each index starts at 0.
   *
   * A channel from the raw complex data is bad if the real value of the channel
   * does not equal itself, ie. it has value NaN. Only those channels which are not
   * bad in this sense are included in the f_* arrays in this structure.
   */
  int **f_nchannels;
  /*! \var f_channel
   *  \brief The channel number of the raw data corresponding to this good channel
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float ***f_channel;
  /*! \var f_frequency
   *  \brief The frequency corresponding to this good channel, in GHz
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float ***f_frequency;
  /*! \var f_weight
   *  \brief The weight for this good channel, which is currently unused
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float ***f_weight;
  /*! \var f_amplitude
   *  \brief The computed amplitude of this good channel
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float ***f_amplitude;
  /*! \var f_phase
   *  \brief The computed phase corresponding to this good channel, the units of which
   *         are controlled by the variable `phase_in_degrees` in the ampphase_options
   *         structure
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float ***f_phase;
  /*! \var f_raw
   *  \brief The raw complex data corresponding to this good channel
   *
   * This 3-D array has length `nbaselines` for the first index, `nbins[i]` for
   * the second index (where `i` is the position along the first index) and
   * `f_nchannels[i][j]` for the third index (where `j` is the position along the
   * second index). Each index starts at 0.
   */
  float complex ***f_raw;
  
  // Some metadata.
  /*! \var min_amplitude_global
   *  \brief The minimum computed amplitude (including good data only) observed across
   *         all the baselines and bins
   *
   * This "global" minimum amplitude value is here to make it easy to plot multiple
   * baselines and bins on the same axis range.
   */
  float min_amplitude_global;
  /*! \var max_amplitude_global
   *  \brief The maximum computed amplitude (including good data only) observed across
   *         all the baselines and bins
   *
   * This "global" maximum amplitude value is here to make it easy to plot multiple
   * baselines and bins on the same axis range.
   */
  float max_amplitude_global;
  /*! \var min_phase_global
   *  \brief The minimum computed phase (including good data only) observed across
   *         all the baselines and bins
   *
   * This "global" minimum phase value is here to make it easy to plot multiple
   * baselines and bins on the same axis range.
   */
  float min_phase_global;
  /*! \var max_phase_global
   *  \brief The maximum computed phase (including good data only) observed across
   *         all the baselines and bins
   *
   * This "global" maximum phase value is here to make it easy to plot multiple
   * baselines and bins on the same axis range.
   */
  float max_phase_global;
  /*! \var min_amplitude
   *  \brief The minimum computed amplitude (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *min_amplitude;
  /*! \var max_amplitude
   *  \brief The maximum computed amplitude (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *max_amplitude;
  /*! \var min_phase
   *  \brief The minimum computed phase (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *min_phase;
  /*! \var max_phase
   *  \brief The maximum computed phase (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *max_phase;
  /*! \var min_real
   *  \brief The minimum raw real value (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *min_real;
  /*! \var max_real
   *  \brief The maximum raw real value (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *max_real;
  /*! \var min_imag
   *  \brief The minimum raw imaginary value (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *min_imag;
  /*! \var max_imag
   *  \brief The maximum raw imaginary value (including good data only) observed across
   *         all the bins on each baseline
   *
   * This array has length `nbaselines`, and is indexed starting at 0.
   */
  float *max_imag;
  /*! \var options
   *  \brief The options used while computing the parameters from the raw data
   *
   * At the moment, the only option relevant to the computations in this structure
   * is whether the phase is in degrees or radians.
   */
  struct ampphase_options *options;
  /*! \var metinfo
   *  \brief Information about the weather conditions on site during this cycle
   *
   * Storing this here means multiple copies, which might be a target for
   * efficiency if required.
   */
  struct metinfo metinfo;
  /*! \var syscal_data
   *  \brief Information about calibration data pertaining to this cycle
   *
   * Only the calibration data relevant for the listed polarisation and window
   * is stored in this structure, and this is stored at the 0-th index of all
   * the arrays when multiple polarisations or windows might be stored.
   */
  struct syscal_data *syscal_data;
};

/*! \struct vis_quantities
 *  \brief Structure to hold average amplitude, phase and delay quantities
 *
 * This structure is designed to have all the quantities you would normally see
 * in vis, which presents one amplitude, phase and delay number (and other quantities)
 * per baseline, per pol, per window. One structure holds information about a single
 * polarisation and window, but all baselines and bins. It also only contains
 * information about a single cycle; to make a vis plot requires many of these structures.
 */
struct vis_quantities {
  /*! \var options
   *  \brief The options that were used while computing the average quantities
   *         in this structure
   */
  struct ampphase_options *options;

  /*! \var nbaselines
   *  \brief The number of baselines with information in this structure
   *
   * This value is the length of the first index of the other arrays in this
   * structure.
   */
  int nbaselines;

  /*! \var obsdate
   *  \brief String representation of the base date of the file that the data in
   *         this structure represents
   */
  char obsdate[OBSDATE_LENGTH];
  /*! \var ut_seconds
   *  \brief The number of seconds past midnight UTC on `obsdate` at the midpoint
   *         of this cycle
   */
  float ut_seconds;
  
  // Labels.
  /*! \var pol
   *  \brief The polarisation represented by these data
   *
   * The value of this variable will be one of the POL_* magic numbers defined at
   * the tops of this header file.
   */
  int pol;
  /*! \var window
   *  \brief The window represented by these data
   *
   * The data windows start at 1 and increment for each set of data that is correlated
   * at a different frequency or with a different number of channels. Usually, windows
   * 1 and 2 are the continuum bands, and any additional windows represent zoom bands,
   * but this is not guaranteed to be the case. Windows are distinct from IFs since IFs
   * represent the signal after mixing to different frequencies, and a single IF can 
   * have multiple windows within it, but often the two terminologies are used
   * interchangeably.
   */
  int window;
  /*! \var nbins
   *  \brief The number of bins present for each baseline
   *
   * This array has length `nbaselines` and is indexed starting at 0.
   *
   * For CABB, `nbaselines` will usually be 21 (15 cross-correlation baselines and 6
   * auto-correlation "baselines"). The cross-correlation baselines will usually have
   * 1 bin (unless doing pulsar binning or HTR), and the auto-correlations will
   * normally have 2 bins (at least in the continuum bands) representing the on-off
   * noise diode states.
   */
  int *nbins;
  /*! \var baseline
   *  \brief The baseline number of each baseline
   *
   * This array has length `nbaselines` and is indexed starting at 0.
   */
  int *baseline;
  /*! \var flagged_bad
   *  \brief Indicator if each baseline should be flagged as bad
   *
   * This array has length `nbaselines` and is indexed starting at 0.
   *
   * This indicator will have value 0 if the baseline is good.
   */
  int *flagged_bad;
  /*! \var scantype
   *  \brief The type of observation this cycle describes
   */
  char scantype[OBSTYPE_LENGTH];
  
  // The arrays.
  /*! \var amplitude
   *  \brief The averaged amplitude for each baseline and bin, in Jy
   *
   * This 2-D array has length `nbaselines` for the first index, and `nbins[i]`
   * for the second index, where `i` is the position along the first index. Both indices
   * start at 0.
   *
   * This amplitude should be in Jy, but if the data has not been corrected for the
   * system temperatures, it will just be the correlation coefficient.
   */
  float **amplitude;
  /*! \var phase
   *  \brief The averaged phase for each baseline and bin, in units determined
   *         by the options
   *
   * This 2-D array has length `nbaselines` for the first index, and `nbins[i]`
   * for the second index, where `i` is the position along the first index. Both indices
   * start at 0.
   *
   * If the \a options parameter has its `phase_in_degrees` variable set to true, then
   * this value should be in degrees, or radians otherwise.
   */
  float **phase;
  /*! \var delay
   *  \brief The computed delay for each baseline and bin, in ns
   *
   * This 2-D array has length `nbaselines` for the first index, and `nbins[i]`
   * for the second index, where `i` is the position along the first index. Both indices
   * start at 0.
   *
   * Because the delay on an auto-correlation would be between two polarisations, and
   * this structure can only hold one, any auto-correlation delay will not be sensible.
   */
  float **delay;
  
  // Metadata.
  /*! \var min_amplitude
   *  \brief The minimum amplitude across all the baselines
   */
  float min_amplitude;
  /*! \var max_amplitude
   *  \brief The maximum amplitude across all the baselines
   */
  float max_amplitude;
  /*! \var min_phase
   *  \brief The minimum phase across all the baselines
   */
  float min_phase;
  /*! \var max_phase
   *  \brief The maximum phase across all the baselines
   */
  float max_phase;
  /*! \var min_delay
   *  \brief The minimum delay across all the baselines
   */
  float min_delay;
  /*! \var max_delay
   *  \brief The maximum delay across all the baselines
   */
  float max_delay;
};

/*! \struct spectrum_data
 *  \brief This structure wraps around all the ampphase structures allowing
 *         for easy transport
 *
 * Since a user very rarely wants only a single polarisation and window when
 * asking for a spectrum, this structure collects related spectra together.
 * It still is limited to containing a single cycle from a single scan.
 */
struct spectrum_data {
  /*! \var header_data
   *  \brief The spectrum header
   */
  struct scan_header_data *header_data;
  /*! \var num_ifs
   *  \brief The number of IFs in this spectra set
   */
  int num_ifs;
  /*! \var num_pols
   *  \brief The number of polarisations in this spectra set
   */
  int num_pols;
  /*! \var spectrum
   *  \brief All the spectra from this scan and cycle
   *
   * This 2-D array of pointers has length `num_ifs` for the first index,
   * and `num_pols` for the second index. Both indices start at 0.
   */
  struct ampphase ***spectrum;
};

/*! \struct vis_data
 *  \brief This structure wraps around all the vis_quantities structures
 *         allowing for easy transport
 *
 * Since a user very rarely wants only a single polarisation, window and time
 * when asking for vis data, this structure collects related vis quantities
 * together. It also comes along with meteorological and calibration data for
 * those same time ranges.
 */
struct vis_data {
  /*! \var nviscycles
   *  \brief The number of cycles contained in this vis set
   */
  int nviscycles;
  // The range of MJDs that were allowed when these data
  // were compiled.
  /*! \var mjd_low
   *  \brief The earliest MJD that was allowed when compiling these data
   */
  double mjd_low;
  /*! \var mjd_high
   *  \brief The latest MJD that was allowed when compiling these data
   */
  double mjd_high;
  /*! \var header_data
   *  \brief The header data for each cycle
   *
   * This array of pointers has length `nviscycles` and is indexed starting
   * at 0.
   */
  struct scan_header_data **header_data;
  /*! \var num_ifs
   *  \brief The number of IFs per cycle
   *
   * This array has length `nviscycles` and is indexed starting at 0.
   *
   * Since the window configuration can change at any time, this variable keeps
   * track of how many IFs are available as the time progresses.
   */
  int *num_ifs;
  /*! \var num_pols
   *  \brief The number of pols per cycle per IF
   *
   * This 2-D array has length `nviscycles` for the first index, and
   * `num_ifs[i]` for the second index, where `i` is the position along the
   * first index. Both indices start at 0.
   *
   * Although it is possible for the number of polarisations to change with
   * different windows and window configurations, for CABB it's unlikely. However,
   * not all polarisation combinations need to be included for each cycle, so
   * this variable is still useful.
   */
  int **num_pols;
  /*! \var vis_quantities
   *  \brief The vis_quantities structures representing each cycle, window and
   *         polarisation
   *
   * This 3-D array of pointers has length `nviscycles` for the first index,
   * `num_ifs[i]` for the second index (where `i` is the position along the
   * first index), and `num_pols[i][j]` for the third index (where `j` is the
   * position along the second index). All indices start at 0.
   */
  struct vis_quantities ****vis_quantities;
  /*! \var metinfo
   *  \brief Meteorological information for each cycle
   *
   * This array of pointers has length `nviscycles`, and is indexed starting at 0.
   */
  struct metinfo **metinfo;
  /*! \var syscal_data
   *  \brief Calibration parameters for each cycle
   *
   * This array of pointers has length `nviscycles`, and is indexed starting at 0.
   */
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
