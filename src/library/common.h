/** \file common.h
 *  \brief Useful routines and magic numbers used through the codebase
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains functions that are useful for many of the
 * applications.
 */

#pragma once
#include "atrpfits.h"

/**
 * Limits and magic numbers.
 */
/*! \def BUFSIZE
 *  \brief A useful size for many strings
 */
#define BUFSIZE 1024
/*! \def BIGBUFSIZE
 *  \brief A useful size for strings that can contain strings of
 *         size BUFSIZE
 */
#define BIGBUFSIZE (4 * BUFSIZE)
/*! \def MAXANTS
 *  \brief The assumption of how many antennas can be used while
 *         still guaranteeing the code works
 *
 * All code is tested while dealing with the ATCA, which has 6 antennas.
 */
#define MAXANTS 6
/*! \def MAXIFS
 *  \brief The assumption of how many windows can be used while
 *         still guaranteeing the code works
 *
 * All code is tested while dealing with CABB, which supports 34 windows
 * (2 continuum IFs, and 16 zoom windows in each).
 */
#define MAXIFS 34
/*! \def YES
 *  \brief A convenience magic number for affirmative values
 */
#define YES 1
/*! \def NO
 *  \brief A convenience magic number for negative values
 */
#define NO 0
/*! \def MAXCHAN
 *  \brief The assumption of how many channels that can appear in a
 *         window while still guaranteeing the code works
 *
 * All code is tested while dealing with CABB, which supports up to 
 * 18000 channels in a single zoom.
 */
#define MAXCHAN 18000
/*! \def MAXPOL
 *  \brief The assumption of how many polarisations can be used while
 *         guaranteeing the code works
 *
 * All code is tested while dealing with CABB, which forms the polarisation
 * products XX, XY, YX and YY.
 */
#define MAXPOL  4
#define MAXBIN  32
#define VISBANDLEN 10

#define TESTTYPE_MULTIPLE_CHOICE 1
#define TESTTYPE_FREE_RESPONSE   2

/*! \def MINASSIGN
 *  \brief Compare a variable's value to another value, and store the
 *         minimum value in the variable
 *  \param a the variable, which should have a value assigned; after this
 *           macro, this variable will have the minimum value
 *  \param b a value to compare to
 */
#define MINASSIGN(a, b) a = (b < a) ? b : a
/*! \def MAXASSIGN
 *  \brief Compare a variable's value to another value, and store the
 *         maximum value in the variable
 *  \param a the variable, which should have a value assigned; after this
 *           macro, this variable will have the maximum value
 *  \param b a value to compare to
 */
#define MAXASSIGN(a, b) a = (b > a) ? b : a

/*! \def LOGAMP
 *  \brief Convert a value to dB, where 0 dB is set as some maximum value,
 *         and store it
 *  \param v the value in linear scale to convert to dB
 *  \param m the value to consider as 0 dB
 *  \param s the variable to hold the value in dB, or -120 if the value is
 *           too small
 */
#define LOGAMP(v, m, s)                               \
  if (v < (m / 1.0E12)) {                             \
    s = -120.0;                                       \
  } else {                                            \
    s = (float)(10.0 * log10((double)v / (double)m)); \
  }

#define ATS_ANGLE_IN_DEGREES       1<<0
#define ATS_ANGLE_IN_RADIANS       1<<1
#define ATS_ANGLE_IN_HOURS         1<<2
#define ATS_STRING_IN_DEGREES      1<<3
#define ATS_STRING_IN_HOURS        1<<4
#define ATS_STRING_DECIMAL_MINUTES 1<<5
#define ATS_STRING_SEP_COLON       1<<6
#define ATS_STRING_SEP_SPACE       1<<7
#define ATS_STRING_SEP_LETTER      1<<8
#define ATS_STRING_SEP_SYMBOL      1<<9
#define ATS_STRING_ZERO_PAD_FIRST  1<<10
#define ATS_STRING_ALWAYS_SIGN     1<<11

/*! \def STATION_NAME_LENGTH
 *  \brief The maximum length + 1 for all the names in the station_names array
 */
#define STATION_NAME_LENGTH 5
/*! \def NUM_STATIONS
 *  \brief The number of stations we know about
 */
#define NUM_STATIONS 45
/*! \def FINDSTATION_FOUND
 *  \brief Indication that a station was found matching the coordinates
 *
 * This is a possible return value from the find_station routine.
 */
#define FINDSTATION_FOUND 0
/*! \def FINDSTATION_NOT_FOUND
 *  \brief Indication that a station was not found matching the coordinates
 *
 * This is a possible return value from the find_station routine.
 */
#define FINDSTATION_NOT_FOUND 1
/*! \def NUM_ARRAYS
 *  \brief The number of arrays that we know about
 */
#define NUM_ARRAYS 24
/*! \def ARRAY_NAME_LENGTH
 *  \brief The maximum length + 1 for all the names in the array_names
 *         array
 */
#define ARRAY_NAME_LENGTH 6
/*! \def FINDARRAYCONFIG_FOUND
 *  \brief Indication that an array configuration was found matching the
 *         station names
 *
 * This is a possible return value from the find_array_configuration routine.
 */
#define FINDARRAYCONFIG_FOUND 2
/*! \def FINDARRAYCONFIG_NOT_FOUND
 *  \brief Indication that an array configuration was not found to match the
 *         station names
 *
 * This is a possible return value from the find_array_configuration routine.
 */
#define FINDARRAYCONFIG_NOT_FOUND 3

/*! \struct array_information
 *  \brief Information about each antenna position, station and the array
 *         configuration
 */
struct array_information {
  /*! \var num_ants
   *  \brief The number of antennas with information in this structure
   */
  int num_ants;
  /*! \var X
   *  \brief The X coordinate of each antenna, in m
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *X;
  /*! \var Y
   *  \brief The Y coordinate of each antenna, in m
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *Y;
  /*! \var Z
   *  \brief The Z coordinate of each antenna, in m
   *
   * This array has length `num_ants` and is indexed starting at 0.
   */
  float *Z;
  /*! \var station_name
   *  \brief The name of the station corresponding to the XYZ coordinates
   *
   * This array of strings has length `num_ants` and is indexed starting
   * at 0. Each string should have length STATION_NAME_LENGTH.
   */
  char **station_name;
  /*! \var array_configuration
   *  \brief The name of the array configuration matching the station names
   */
  char array_configuration[ARRAY_NAME_LENGTH];
};

// Our routine definitions.
int interpret_array_string(char *array_string);
int find_pol(struct ampphase ***cycle_ampphase, int npols, int ifnum, int poltype);
int find_if_name_nosafe(struct scan_header_data *scan_header_data, char *name);
int find_if_name(struct scan_header_data *scan_header_data, char *name);
void seconds_to_hourlabel(float seconds, char *hourlabel);
bool minmatch(char *ref, char *chk, int minlength);
int split_string(char *s, char *delim, char ***elements);
void minutes_representation(float minutes, char *representation);
bool string_to_float(char *s, float *v);
bool string_to_integer(char *s, int *v);
bool string_to_seconds(char *s, float *seconds);
float string_to_minutes(char *s);
void generate_client_id(char *client_id, size_t maxlen);
int leap(int year);
int dayOK(int day, int month, int year);
double cal2mjd(int day, int month, int year, float ut_seconds);
double date2mjd(char *obsdate, float ut_seconds);
void mjd2cal(double mjd, int *year, int *month, int *day, float *ut_seconds);
void stringappend(char **dest, const char *src);
void angle_to_string(float angle, char *angle_string, int conversion_type,
		     int precision);
int find_station(float x, float y, float z, char *station_name);
int find_array_configuration(char **station_names, char *array_configuration);
int scan_header_to_array(struct scan_header_data *scan_header_data,
			 struct array_information **array_information);
void free_array_information(struct array_information **array_information);
void strip_end_spaces(char *s, char *r, size_t rlen);
