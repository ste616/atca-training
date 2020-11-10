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
