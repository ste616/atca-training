/** \file common.c
 *  \brief Useful routines used throughout the codebase
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains functions that are useful for many of the
 * applications.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <regex.h>
#include "common.h"
#include "memory.h"

int interpret_array_string(char *array_string) {
  // The array string should be a comma-separated list of antenna.
  int a = 0, r = 0;
  char *token, copy[BUFSIZE];
  const char s[2] = ",";

  // We make a copy of the string since strtok is destructive.
  (void)strncpy(copy, array_string, BUFSIZE);
  token = strtok(copy, s);
  while (token != NULL) {
    a = atoi(token);
    if ((a >= 1) && (a <= MAXANTS)) {
      r |= 1<<a;
    }
    token = strtok(NULL, s);
  }

  return r;
}

int find_pol(struct ampphase ***cycle_ampphase, int npols, int ifnum, int poltype) {
  int i;
  for (i = 0; i < npols; i++) {
    if (cycle_ampphase[ifnum][i]->pol == poltype) {
      return i;
    }
  }
  return -1;
}


int find_if_name_nosafe(struct scan_header_data *scan_header_data,
                        char *name) {
  int res = -1, i, j;
  /* fprintf(stderr, "[find_if_name] searching for band %s\n", name); */
  for (i = 0; i < scan_header_data->num_ifs; i++) {
    for (j = 0; j < 3; j++) {
      if (strcmp(scan_header_data->if_name[i][j], name) == 0) {
        res = scan_header_data->if_label[i];
        break;
      }
    }
    if (res >= 0) {
      break;
    }
  }

  /* if (res < 0) { */
  /*   res = 1; // For safety. */
  /* } */

  /* fprintf(stderr, "[find_if_name] found band %d %s\n", res, scan_header_data->if_name[res - 1][0]); */
  return res;
}

int find_if_name(struct scan_header_data *scan_header_data,
                 char *name) {
  int res = find_if_name_nosafe(scan_header_data, name);
  if (res < 0) {
    res = 1;
  }

  return res;
}

/*!
 *  \brief Generate HH:MM:SS label from a specified number of seconds
 *  \param seconds the number of seconds
 *  \param hourlabel this string will be filled by this routine, with either
 *                   HH:MM:SS corresponding to the parameter \a seconds, or
 *                   Dd HH:MM:SS if \a seconds > 86400
 */
void seconds_to_hourlabel(float seconds, char *hourlabel) {
  int d, h, m, s;
  float dayseconds;
  
  dayseconds = (int)seconds % 86400;
  d = (int)((seconds - dayseconds) / 86400);

  h = (int)floorf(dayseconds / 3600);
  seconds -= h * 3600;
  m = (int)floorf(seconds / 60);
  seconds -= m * 60;
  s = (int)seconds;

  if (d > 0) {
    (void)sprintf(hourlabel, "%dd %02d:%02d:%02d", d, h, m, s);
  } else {
    (void)sprintf(hourlabel, "%02d:%02d:%02d", h, m, s);
  }
}

bool minmatch(char *ref, char *chk, int minlength) {
  // This routine compares the string chk to the string
  // ref. If the entire string chk represents ref up the
  // the length of chk or ref, it returns true.
  // eg. if ref = "select", and chk = "sel", true
  //        ref = "select", and chk = "s", minlength = 3, false
  //        ref = "select", and chk = "selects", false
  int chklen, reflen;

  reflen = strlen(ref);
  chklen = strlen(chk);
  if ((minlength > chklen) || (minlength > reflen)) {
    return false;
  }

  if (chklen > reflen) {
    return false;
  }

  if (strncasecmp(chk, ref, chklen) == 0) {
    return true;
  }

  return false;
}

int split_string(char *s, char *delim, char ***elements) {
  int i = 0;
  char *token = NULL, *saveptr = NULL;

  // Skip any leading delimiters.
  while (*s == *delim) {
    s++;
  }
  while ((token = strtok_r(s, delim, &saveptr))) {
    s = NULL;
    REALLOC(*elements, (i + 1));
    (*elements)[i] = token;
    i++;
  }

  return i;
}

void minutes_representation(float minutes, char *representation) {
  // We look at the number of minutes, and output the best human representation
  // of that duration.
  if (minutes < 1) {
    // Output in seconds.
    sprintf(representation, "%.1f sec", (minutes * 60));
  } else if (minutes > 2800) {
    // Output in days.
    sprintf(representation, "%.2f days", (minutes / 1440));
  } else if (minutes > 120) {
    // Output in hours.
    sprintf(representation, "%.2f hours", (minutes / 60));
  } else {
    // Output in minutes.
    sprintf(representation, "%.1f min", minutes);
  }
}

bool string_to_float(char *s, float *v) {
  // This routine wraps around strtof to indicate if the parsing is
  // successful.
  bool succ;
  char *eptr = NULL;
  float tv;

  tv = strtof(s, &eptr);
  if (eptr == s) {
    // Failure.
    succ = false;
  } else {
    // Success.
    succ = true;
    *v = tv;
  }
  return succ;
}

bool string_to_seconds(char *s, float *seconds) {
  // Parse a string like 12:30:21 or 13:51 into the number of seconds
  // since midnight.
  float nels, tm, te;
  char delim[] = ":", **tels = NULL;
  int i;

  // Split by colons.
  nels = split_string(s, delim, &tels);

  if ((nels < 2) || (nels > 3)) {
    // We can't do anything with this.
    return false;
  }
  *seconds = 0;
  for (i = 0, tm = 3600; i < nels; i++, tm /= 60) {
    if (string_to_float(tels[i], &te)) {
      *seconds += te * tm;
    } else {
      return false;
    }
  }
  
  return true;
}

float string_to_minutes(char *s) {
  // Parse a string like 2.1m or 1.3h or 30s or 1h30m into the
  // number of minutes that string represents.
  float nmin = 0, num;
  regex_t regex;
  regmatch_t matches[4];
  int reti;
  char *cursor, numstr[100];

  reti = regcomp(&regex, "([0-9]+)([dhms])", REG_EXTENDED);

  if (reti) {
    fprintf(stderr, "[string_to_minutes] Unable to compile regex\n");
    return nmin;
  }
  cursor = s;
  while (!(regexec(&regex, cursor, 4, matches, 0))) {
    strncpy(numstr, cursor + matches[1].rm_so,
            (1 + matches[1].rm_eo - matches[1].rm_so));
    if (string_to_float(numstr, &num)) {
      // We got a conversion.
      switch ((cursor + matches[2].rm_so)[0]) {
      case 'd':
        nmin += num * 1440;
        break;
      case 'h':
        nmin += num * 60;
        break;
      case 'm':
        nmin += num;
        break;
      case 's':
        nmin += num / 60;
        break;
      }
    }
    
    cursor += matches[0].rm_eo;
  }

  regfree(&regex);
  
  return nmin;
}

void generate_client_id(char *client_id, size_t maxlen) {
  // Generate a client ID, which will be used to allow for tagging requests
  // to the server for later fulfilment.
  char *p = NULL;
  size_t i;
  srand(time(NULL));
  for (p = client_id, i = 0; i < (maxlen - 1); p++, i++) {
    *p = ' ' + (rand() % 94);
  }
  *p = 0;
}

int leap(int year) {
  // Return 1 if the year is a leap year.
  return (((!(year % 4)) &&
	   (year % 100)) ||
	  (!(year % 400)));
}

int dayOK(int day, int month, int year) {
  int ndays = 31;
  if ((month == 4) || (month == 6) ||
      (month == 9) || (month == 11)) {
    ndays = 30;
  } else if (month == 2) {
    ndays = 28;
    if (leap(year)) {
      ndays = 29;
    }
  }

  if ((day < 1) || (day > ndays)) {
    return 0;
  }
  return 1;
}

double cal2mjd(int day, int month, int year, float ut_seconds) {
  // Converts a calendar date (Universal Time) into modified
  // Julian day number.
  // day = day of the month (1 - 31)
  // month = month of the year (1 - 12)
  // year = year (full)
  // ut_seconds = number of seconds passed on the specified date
  // Returns the MJD
  int m, y, c, x1, x2, x3;
  
  if ((month < 1) || (month > 12)) {
    return 0;
  }

  if (!dayOK(day, month, year)) {
    return 0;
  }

  if (month <= 2) {
    m = month + 9;
    y = year - 1;
  } else {
    m = month - 3;
    y = year;
  }

  c = (int)((float)y / 100);
  y -= c * 100;
  x1 = (int)(146097.0 * (float)c / 4.0);
  x2 = (int)(1461.0 * (float)y / 4.0);
  x3 = (int)((153.0 * (float)m + 2.0) / 5.0);
  return ((float)(x1 + x2 + x3 + day - 678882) + (ut_seconds/ 86400.0));
}

double date2mjd(char *obsdate, float ut_seconds) {
  // Parse the RPFITS obsdate.
  int year, month, day;

  if (sscanf(obsdate, "%4d-%02d-%02d", &year, &month, &day) == 3) {
    return cal2mjd(day, month, year, ut_seconds);
  }

  return 0;
}

void mjd2cal(double mjd, int *year, int *month, int *day, float *ut_seconds) {
  // Converts an MJD into a calendar date (Universal Time).
  long j, c, y, m;
  
  *ut_seconds = (float)(mjd - floor(mjd));

  mjd -= (mjd - floor(mjd));
  j = (long)mjd + 2400001 + 68569;
  c = 4 * j / 146097;
  j -= (146097 * c + 3) / 4;
  y = 4000 * (j + 1) / 1461001;
  j = j - 1461 * y / 4 + 31;
  m = 80 * j / 2447;
  *day = (int)(j - 2447 * m / 80);
  j = m / 11;
  // We put month in the range 1 - 12.
  *month = (int)(m + 1 - (12 * j)) + 1;
  *year = (int)(100 * (c - 49) + y + j);
}

void stringappend(char **dest, const char *src) {
  // We resize dest to make it just long enough to add src to it.
  size_t l1 = 0, l2 = 0;

  if (*dest != NULL) {
    // Get the current size.
    l1 = strlen(*dest);
  }

  l2 = strlen(src);
  REALLOC(*dest, (l1 + l2 + 2));
  strncat(*dest, src, l2);
}

/*!
 *  \brief Convert an angle into a string representation
 *  \param angle the angle to convert to a string
 *  \param angle_string this string will be filled by this routine
 *  \param conversion_type a bitwise OR combination of flags to direct how
 *                         this routine will work
 *                         - ATS_ANGLE_IN_DEGREES: the input angle is in degrees
 *                         - ATS_ANGLE_IN_RADIANS: the input angle is in radians
 *                         - ATS_ANGLE_IN_HOURS: the input angle is in hours
 *                         - ATS_STRING_IN_DEGREES: the output string should be in
 *                           degrees
 *                         - ATS_STRING_IN_HOURS: the output string should be in hours
 *                         - ATS_STRING_DECIMAL_MINUTES: only show minutes of arc 
 *                           (or hours), and don't include seconds
 *                         - ATS_STRING_SEP_COLON: use : as the separator between the
 *                           string elements
 *                         - ATS_STRING_SEP_SPACE: use a space as the separator between
 *                           the string elements, or include a space along with the
 *                           letters or symbols
 *                         - ATS_STRING_SEP_LETTER: use hms or dms as the string element
 *                           labels
 *                         - ATS_STRING_SEP_SYMBOL: use d'" or h'" as the string element
 *                           labels
 *                         - ATS_STRING_ZERO_PAD_FIRST: always pad the hours to 2 figures,
 *                           or degrees to 3 figures, using 0 at the front
 *                         - ATS_STRING_ALWAYS_SIGN: always include the + if it is a
 *                           positive angle
 *  \param precision how many decimal places to include for the smallest element
 */
void angle_to_string(float angle, char *angle_string, int conversion_type,
		     int precision) {
  float out_angle, secondfloat, thirdfloat;
  int firstint, secondint, secondlength, thirdlength;
  bool isnegative;
  char firstlabel[3], secondlabel[3], thirdlabel[2], oform_first[10];
  char oform_second[10], oform_third[10], oform_all[30], signchar[2];
  
  // Begin by converting the angle to the output units.
  out_angle = angle;
  if (conversion_type & ATS_ANGLE_IN_DEGREES) {
    if (conversion_type & ATS_STRING_IN_HOURS) {
      out_angle /= 15.0;
    }
  } else if (conversion_type & ATS_ANGLE_IN_RADIANS) {
    if (conversion_type & ATS_STRING_IN_DEGREES) {
      out_angle *= (180.0 / M_PI);
    } else if (conversion_type & ATS_STRING_IN_HOURS) {
      out_angle *= (12.0 / M_PI);
    }
  } else if (conversion_type & ATS_ANGLE_IN_HOURS) {
    if (conversion_type & ATS_STRING_IN_DEGREES) {
      out_angle *= 15.0;
    }
  } else {
    // We don't actually know for sure what the units are of the input angle,
    // so we fail. TODO: we should probably return an error code.
    return;
  }

  // Assign the labels (default is colon).
  firstlabel[0] = ':'; firstlabel[1] = 0; firstlabel[2] = 0;
  secondlabel[0] = ':'; secondlabel[1] = 0; secondlabel[2] = 0;
  thirdlabel[0] = 0; thirdlabel[1] = 0;
  if (conversion_type & ATS_STRING_IN_DEGREES) {
    if (conversion_type & ATS_STRING_SEP_LETTER) {
      firstlabel[0] = 'd';
      secondlabel[0] = 'm';
      thirdlabel[0] = 's';
      if (conversion_type & ATS_STRING_SEP_SPACE) {
	firstlabel[1] = ' ';
	if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	  secondlabel[1] = ' ';
	} else {
	  thirdlabel[0] = 0;
	}
      }
    } else if (conversion_type & ATS_STRING_SEP_SYMBOL) {
      firstlabel[0] = 'd';
      secondlabel[0] = '\'';
      thirdlabel[0]= '\"';
      if (conversion_type & ATS_STRING_SEP_SPACE) {
	firstlabel[1] = ' ';
	if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	  secondlabel[1] = ' ';
	} else {
	  thirdlabel[0] = 0;
	}
      }
    } else if (conversion_type & ATS_STRING_SEP_SPACE) {
      firstlabel[0] = ' ';
      if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	secondlabel[0] = ' ';
      }
    }
  } else if (conversion_type & ATS_STRING_IN_HOURS) {
    if (conversion_type & ATS_STRING_SEP_LETTER) {
      firstlabel[0] = 'h';
      secondlabel[0] = 'm';
      thirdlabel[0] = 's';
      if (conversion_type & ATS_STRING_SEP_SPACE) {
	firstlabel[1] = ' ';
	if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	  secondlabel[1] = ' ';
	} else {
	  thirdlabel[0] = 0;
	}
      }
    } else if (conversion_type & ATS_STRING_SEP_SYMBOL) {
      firstlabel[0] = 'h';
      secondlabel[0] = 'm';
      thirdlabel[0]= 's';
      if (conversion_type & ATS_STRING_SEP_SPACE) {
	firstlabel[1] = ' ';
	if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	  secondlabel[1] = ' ';
	} else {
	  thirdlabel[0] = 0;
	}
      }
    } else if (conversion_type & ATS_STRING_SEP_SPACE) {
      firstlabel[0] = ' ';
      if (!(conversion_type & ATS_STRING_DECIMAL_MINUTES)) {
	secondlabel[0] = ' ';
      }
    }
  }

  // Make the computations.
  isnegative = (out_angle < 0);
  out_angle = fabsf(out_angle);
  firstint = (int)floorf(out_angle);
  secondfloat = (out_angle - (float)firstint) * 60.0;
  secondint = (int)floorf(secondfloat);
  thirdfloat = (secondfloat - (float)secondint) * 60.0;

  // Make the string.
  if (conversion_type & ATS_STRING_ZERO_PAD_FIRST) {
    if (conversion_type & ATS_STRING_IN_DEGREES) {
      strcpy(oform_first, "%03d%s");
    } else if (conversion_type & ATS_STRING_IN_HOURS) {
      strcpy(oform_first, "%02d%s");
    }
  }
  if (conversion_type & ATS_STRING_DECIMAL_MINUTES) {
    secondlength = 3 + precision;
    snprintf(oform_second, 9, "%%0%d.%df%%s", secondlength, precision);
    oform_third[0] = 0;
  } else {
    strcpy(oform_second, "%02d%s");
    thirdlength = 3 + precision;
    snprintf(oform_third, 9, "%%0%d.%df%%s", thirdlength, precision);
  }
  snprintf(oform_all, 29, "%%s%s%s%s", oform_first, oform_second, oform_third);
  signchar[0] = 0; signchar[1] = 0;
  if (isnegative) {
    signchar[0] = '-';
  } else if (conversion_type & ATS_STRING_ALWAYS_SIGN) {
    signchar[0] = '+';
  }
  if (conversion_type & ATS_STRING_DECIMAL_MINUTES) {
    sprintf(angle_string, oform_all, signchar, firstint, firstlabel, secondfloat,
	    secondlabel);
  } else {
    sprintf(angle_string, oform_all, signchar, firstint, firstlabel, secondint,
	    secondlabel, thirdfloat, thirdlabel);
  }
  
}
