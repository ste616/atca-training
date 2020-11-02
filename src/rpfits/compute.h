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
