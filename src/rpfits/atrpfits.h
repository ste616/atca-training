/** \file atrpfits.h
 *  \brief Definitions for low-level RPFITS-related parameters.
 *
 * ATCA Training Library
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
  /*! \var num_sources
   *  \brief The number of sources in this scan
   *
   * The number of sources is normally 1 for most scans, but can be
   * larger if it's a mosaic.
   */
  int num_sources;
  /*! \var source_name
   *  \brief The name of each source, supplied by the observer.
   *
   * This array of strings have length `num_sources`, and is indexed
   * starting at 0. Each string has length SOURCE_LENGTH.
   *
   * The observer can name each scan whatever they like as the
   * source name, and this name is represented here.
   */
  char **source_name;
  // Source coordinates.
  /*! \var rightascension_hours
   *  \brief The right ascension of the correlated phase centre, in hours,
   *         for each source
   */
  float *rightascension_hours;
  /*! \var declination_degrees
   *  \brief The declination of the correlated phase centre, in degrees,
   *         for each source
   */
  float *declination_degrees;
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
/*! \def SYSCAL_VALID
 *  \brief Magic number to indicate that either the SYSCAL weather parameters or the
 *         seeing monitor parameters are valid and useful.
 *
 * This flag is used by the correlator to indicate that the weather or seeing monitor
 * parameters in the SYSCAL additional record are valid and useful. These parameters
 * can be obtained using the SYSCAL_ADDITIONAL_* macros.
 */
#define SYSCAL_VALID   0
/*! \def SYSCAL_INVALID
 *  \brief Magic number to indicate that either the SYSCAL weather parameters or the
 *         seeing monitor parameters are invalid and not useful.
 *
 * This flag is used by the correlator to indicate that the weather or seeing monitor
 * parameters in the SYSCAL additional record are not valid or useful. These parameters
 * can be obtained using the SYSCAL_ADDITIONAL_* macros.
 */
#define SYSCAL_INVALID 1

/*! \struct cycle_data
 *  \brief This structure holds all the data from a single cycle from the telescope
 *
 * When reading from the RPFITS file, each cycle's data is read at once, along
 * with all the necessary metadata. The reader routine takes the cycle data and stores
 * it in this structure for easy access.
 */
struct cycle_data {
  // Time variables.
  /*! \var ut_seconds
   *  \brief The number of seconds past midnight on the base date of the file at this
   *         cycle's mid-point
   *
   * The mid-point of each cycle is recorded as the number of seconds that have passed
   * since UTC midnight on the date that the file was opened. This base date is recorded
   * in the obsdate variable in the scan header associated with this cycle.
   */
  float ut_seconds;
  // Antennas and baselines.
  /*! \var num_points
   *  \brief The number of points contained within this cycle structure
   *
   * This structure's data variables are all index by "point", where a single point
   * represents a single antenna pair (and an autocorrelation is an antenna paired
   * with itself) at a single time, in a single bin and IF. This variable indicates
   * the total number of points in this cycle, and thus allows for iteration over
   * the arrays.
   */
  int num_points;
  /*! \var all_baselines
   *  \brief The index of where baselines are in the points of this cycle
   *
   * This stores the order of the baselines found in this cycle. As the data is read
   * in, the first baseline number encountered is given the index of 1, the next
   * baseline that is different to the first is given the index of 2, the next
   * baseline that is different to all the baselines previously read is given the
   * index of 3, and so on.
   *
   * If any particular value in this array is 0, then no data about that baseline
   * number is present in this structure.
   */
  int all_baselines[MAX_BASELINENUM];
  /*! \var n_baselines
   *  \brief The total number of different baselines found in the cycle
   */
  int n_baselines;
  /*! \var u
   *  \brief The u coordinate of the baseline [m]
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  float *u;
  /*! \var v
   *  \brief The v coordinate of the baseline [m]
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  float *v;
  /*! \var w
   *  \brief The w coordinate of the baseline [m]
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  float *w;
  /*! \var ant1
   *  \brief The number of the first antenna in the baseline
   *
   * This array has length of `num_points` and is indexed starting at 0.
   *
   * This value is computed from the baseline number stored in the RPFITS file.
   */
  int *ant1;
  /*! \var ant2
   *  \brief The number of the second antenna in the baseline
   *
   * This array has length of `num_points` and is indexed starting at 0.
   *
   * This value is computed from the baseline number stored in the RPFITS file.
   */
  int *ant2;

  // Flagging.
  /*! \var flag
   *  \brief Whether this point should be flagged; 0 here implies perfect data
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  int *flag;

  // Data.
  /*! \var vis_size
   *  \brief The return value from max_size_of_vis during this read operation,
   *         which is also the size of the vis and wgt arrays
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  int *vis_size;
  /*! \var vis
   *  \brief The complex channel data
   *
   * This 2-dimensional array has size `num_points` * `vis_size[i]`, where
   * `i` is the index along the `num_points` axis (which is the first array
   * index), and is indexed starting at 0 in each axis.
   */
  float complex **vis;
  /*! \var wgt
   *  \brief The channel weighting data
   *
   * This 2-dimensional array has size `num_points` * `vis_size[i]`, where
   * `i` is the index along the `num_points` axis (which is the first array
   * index), and is indexed starting at 0 in each axis.
   *
   * **This parameter is likely completely pointless and will probably not
   * be kept in this structure for very long, as it takes up memory for no
   * useful reason.**
   */
  float **wgt;

  // Labelling.
  /*! \var bin
   *  \brief The bin number pertaining to this point
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  int *bin;
  /* \var if_no
   * \brief The IF number pertaining to this point
   *
   * This array has length of `num_points` and is indexed starting at 0.
   */
  int *if_no;
  /*! \var source_no
   *  \brief The index of the source for this cycle
   *
   * This array has length of `num_points`, and is indexed starting at 0.
   *
   * This source index corresponds with a source entry in the scan header's
   * source table.
   */
  int *source_no;

  // Metadata.
  /*! \var num_cal_ifs
   *  \brief The number of IFs about which SYSCAL data was found in this
   *         cycle, and thus the length of the first index of all the SYSCAL-related
   *         parameters in this structure
   */
  int num_cal_ifs;
  /*! \var num_cal_ants
   *  \brief The number of antennas which have data in the SYSCAL data entries
   *         in this cycle, and thus the length of the second index of all the
   *         SYSCAL-related parameters in this structure
   */
  int num_cal_ants;
  /*! \var cal_ifs
   *  \brief The IF number of the SYSCAL entry at this index
   *
   * This array has length of `num_cal_ifs`, and is indexed starting at 0.
   */
  int *cal_ifs;
  /*! \var cal_ants
   *  \brief The antenna number of the SYSCAL entry at this index
   *
   * This array has length of `num_cal_ants`, and is indexed starting at 0.
   */
  int *cal_ants;

  // These are indexed [IF][ANT][POL]
  /*! \var tsys
   *  \brief The system temperature measured by the correlator for each IF, antenna
   *         and polarisation combination, in Kelvin
   *
   * This array has length `num_cal_ifs` * `num_cal_ants` * 2. The first index
   * is over the IFs, the second index is over the antennas, and the third index is
   * over the polarisations, with the indexing there controlled by the magic
   * numbers CAL_XX and CAL_YY. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float ***tsys;
  /*! \var tsys_applied
   *  \brief Flag indicating whether the data in this structure has been corrected for
   *         the system temperature measured by the correlator for each IF, antenna and 
   *         polarisation combination; SYSCAL_TSYS_APPLIED means that the data has been
   *         corrected, whereas SYSCAL_TSYS_NOT_APPLIED means the opposite
   *
   * This array has length `num_cal_ifs` * `num_cal_ants` * 2. The first index
   * is over the IFs, the second index is over the antennas, and the third index is
   * over the polarisations, with the indexing there controlled by the magic
   * numbers CAL_XX and CAL_YY. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  int ***tsys_applied;
  /*! \var computed_tsys
   *  \brief The system temperature computed by this library for each IF, antenna
   *         and polarisation combination, in Kelvin
   *
   * This array has length `num_cal_ifs` * `num_cal_ants` * 2. The first index
   * is over the IFs, the second index is over the antennas, and the third index is
   * over the polarisations, with the indexing there controlled by the magic
   * numbers CAL_XX and CAL_YY. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float ***computed_tsys;
  /*! \var computed_tsys_applied
   *  \brief Flag indicating whether the data in this structure has been corrected for
   *         the system temperature computed by this library for each IF, antenna and 
   *         polarisation combination; SYSCAL_TSYS_APPLIED means that the data has been
   *         corrected, whereas SYSCAL_TSYS_NOT_APPLIED means the opposite
   *
   * This array has length `num_cal_ifs` * `num_cal_ants` * 2. The first index
   * is over the IFs, the second index is over the antennas, and the third index is
   * over the polarisations, with the indexing there controlled by the magic
   * numbers CAL_XX and CAL_YY. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  int ***computed_tsys_applied;

  // These are indexed [IF][ANT].
  /*! \var xyphase
   *  \brief The XY-phase measured by the correlator for each IF and antenna combination,
   *         in degrees
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **xyphase;
  /*! \var xyamp
   *  \brief The XY-amplitude measured by the correlator for each IF and antenna combination,
   *         in Jy
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **xyamp;
  /*! \var parangle
   *  \brief The parallactic angle for each IF and antenna combination, in degrees
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **parangle;
  /*! \var tracking_error_max
   *  \brief The maximum antenna tracking error observed during this cycle, for each IF 
   *         and antenna combination, in arcsec
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **tracking_error_max;
  /*! \var tracking_error_rms
   *  \brief The RMS antenna tracking error observed during this cycle, for each IF 
   *         and antenna combination, in arcsec
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **tracking_error_rms;
  /*! \var gtp_x
   *  \brief The gated total power (GTP) observed during this cycle for the X pol, for each IF 
   *         and antenna combination
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **gtp_x;
  /*! \var gtp_y
   *  \brief The gated total power (GTP) observed during this cycle for the Y pol, for each IF 
   *         and antenna combination
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **gtp_y;
  /*! \var sdo_x
   *  \brief The synchronously-demodulated output (SDO) observed during this cycle for the
   *         X pol, for each IF and antenna combination
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **sdo_x;
  /*! \var sdo_y
   *  \brief The synchronously-demodulated output (SDO) observed during this cycle for the
   *         Y pol, for each IF and antenna combination
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **sdo_y;
  /*! \var caljy_x
   *  \brief The noise diode signal level assumed by the correlator during this cycle for the
   *         X pol, for each IF and antenna combination, in Jy
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **caljy_x;
  /*! \var caljy_y
   *  \brief The noise diode signal level assumed by the correlator during this cycle for the
   *         Y pol, for each IF and antenna combination, in Jy
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  float **caljy_y;
  /*! \var flagging
   *  \brief An indication of whether this cycle was flagged as bad by the correlator,
   *         for each IF and antenna combination; the flagging indicators are explained
   *         in the documentation for the SYSCAL_FLAG_BAD macro
   *
   * This array has length `num_cal_ifs` * `num_cal_ants`. The first index is over the IFs,
   * and the second index is over the antennas. Each array index starts at 0.
   *
   * The order of the IF and antenna indices is the same as for the arrays `cal_ifs` and
   * `cal_ants` respectively.
   */
  int **flagging;

  // Weather metadata.
  /*! \var temperature
   *  \brief The ambient temperature measured by the site weather station during this
   *         cycle, in C
   */
  float temperature;
  /*! \var air_pressure
   *  \brief The air pressure measured by the site weather station during this
   *         cycle, in mBar
   */
  float air_pressure;
  /*! \var humidity
   *  \brief The relative humidity measured by the site weather station during this
   *         cycle, in %
   */
  float humidity;
  /*! \var wind_speed
   *  \brief The wind speed measured by the site weather station during this
   *         cycle, in km/h
   */
  float wind_speed;
  /*! \var wind_direction
   *  \brief The wind direction measured by the site weather station during this
   *         cycle, in deg
   */
  float wind_direction;
  /*! \var rain_gauge
   *  \brief The rain gauge level measured by the site weather station during this
   *         cycle, in mm
   */
  float rain_gauge;
  /*! \var weather_valid
   *  \brief An indicator of the quality of data from the site weather station during this
   *         cycle; consider this data useful only if this variable has value SYSCAL_VALID
   */
  int weather_valid;
  /*! \var seemon_phase
   *  \brief The average phase value observed by the atmospheric seeing monitor during this
   *         cycle, in deg
   */
  float seemon_phase;
  /*! \var seemon_rms
   *  \brief The RMS phase variation observed by the atmospheric seeing monitor during this
   *         cycle, in deg
   */
  float seemon_rms;
  /*! \var seemon_valid
   *  \brief An indicator of the quality of data from the site atmospheric seeing monitor
   *         during this cycle; consider this data useful only if this variable has value 
   *         SYSCAL_VALID
   */
  int seemon_valid;
};

/*! \struct scan_data
 *  \brief A structure to hold all the cycles together for a scan, along with the
 *         corresponding header
 */
struct scan_data {
  /*! \var header_data
   *  \brief The header parameters for this scan
   */
  struct scan_header_data header_data;
  /*! \var num_cycles
   *  \brief The number of cycles read in for this scan
   */
  int num_cycles;
  /*! \var cycles
   *  \brief An array of pointers to cycle_data structures, one for each cycle in this scan
   *
   * This array has size `num_cycles`, and is indexed starting at 0.
   */
  struct cycle_data **cycles;
};

/*! \struct rpfits_index
 *  \brief A structure which describes how to quickly access data in an RFPITS file.
 *
 * **UNUSED AND NOT WORKING, HOPEFULLY AVAILABLE IN FUTURE VERSIONS**
 */
struct rpfits_index {
  /*! \var filename
   *  \brief The name of the RPFITS file that this index pertains to
   */
  char filename[FILENAME_LENGTH];
  /*! \var num_headers
   *  \brief The number of headers contained in the RPFITS file
   */
  int num_headers;
  /*! \var header_pos
   *  \brief An array of binary file positions corresponding to each header
   *
   * This array has size `num_headers`, and is indexed starting at 0.
   */
  long int *header_pos;
  /*! \var num_cycles
   *  \brief A record of how many cycles each scan (remembering there is one scan
   *         per header) has
   *
   * This array has size `num_headers`, and is indexed starting at 0.
   */
  int *num_cycles;
  /*! \var cycle_pos
   *  \brief An array of binary file positions corresponding to each cycle
   *
   * This 2-D array has size `num_headers` * `num_cycles[i]`, where `i` is the position
   * along the `num_headers` axis (the first axis), and both indices start at 0.
   */
  long int **cycle_pos;
};

void base_to_ants(int baseline, int *ant1, int *ant2);
int ants_to_base(int ant1, int ant2);

#include "reader.h"
#include "compute.h"
