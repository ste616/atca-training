/**
 * ATCA Training Library: common.h
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
#define BUFSIZE 1024
#define BIGBUFSIZE (4 * BUFSIZE)
#define MAXANTS 6
#define MAXIFS 34
#define YES 1
#define NO 0
#define MAXCHAN 18000
#define MAXPOL  4
#define MAXBIN  32
#define VISBANDLEN 10

#define TESTTYPE_MULTIPLE_CHOICE 1
#define TESTTYPE_FREE_RESPONSE   2

#define MINASSIGN(a, b) a = (b < a) ? b : a
#define MAXASSIGN(a, b) a = (b > a) ? b : a

// Take the dB log of value v, compared with max value m, store
// the log value in s.
#define LOGAMP(v, m, s)                               \
  if (v < (m / 1.0E12)) {                             \
    s = -120.0;                                       \
  } else {                                            \
    s = (float)(10.0 * log10((double)v / (double)m)); \
  }

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
