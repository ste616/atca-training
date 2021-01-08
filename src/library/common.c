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

/*! \var station_names
 *  \brief All the station names we know about
 *
 * Along with the station_coord_* variables, this allows antenna station coordinates
 * to be resolved into which station they were at.
 */
const char station_names[NUM_STATIONS][STATION_NAME_LENGTH] = {
  "W0",   "W2",   "W4",   "W6",   "W8",   "W10",  "W12",  "W14",  "W16",  "W32",  "W45",
  "W64",  "W84",  "W98",  "W100", "W102", "W104", "W106", "W109", "W110", "W111", "W112",
  "W113", "W124", "W125", "W128", "W129", "W140", "W147", "W148", "W163", "W168", "W172",
  "W173", "W182", "W189", "W190", "W195", "W196", "W392", "N2",   "N5",   "N7",   "N11",
  "N14"
};
const float station_coordinate_X[NUM_STATIONS] = {
  -4752438.459, -4752422.922, -4752407.385, -4752391.848, -4752376.311, -4752360.774, 
  -4752345.237, -4752329.700, -4752314.163, -4752189.868, -4752088.877, -4751941.276, 
  -4751785.907, -4751677.148, -4751661.611, -4751646.074, -4751630.537, -4751615.000, 
  -4751591.695, -4751583.926, -4751576.158, -4751568.389, -4751560.621, -4751475.168, 
  -4751467.399, -4751444.094, -4751436.325, -4751350.872, -4751296.492, -4751288.724, 
  -4751172.197, -4751133.354, -4751102.281, -4751094.512, -4751024.596, -4750970.216, 
  -4750962.448, -4750923.605, -4750915.837, -4749393.198, -4751628.291, -4751648.226, 
  -4751661.517, -4751688.098, -4751708.034   
};
const float station_coordinate_Y[NUM_STATIONS] = {
  2790321.299, 2790347.675, 2790374.052, 2790400.428, 2790426.804, 2790453.181,
  2790479.557, 2790505.934, 2790532.310, 2790743.321, 2790914.767, 2791165.342,
  2791429.106, 2791613.741, 2791640.117, 2791666.493, 2791692.870, 2791719.246,
  2791758.810, 2791771.999, 2791785.187, 2791798.375, 2791811.563, 2791956.633,
  2791969.821, 2792009.386, 2792022.574, 2792167.644, 2792259.961, 2792273.149,
  2792470.972, 2792536.913, 2792589.666, 2792602.854, 2792721.547, 2792813.865,
  2792827.053, 2792892.994, 2792906.182, 2795491.050, 2791727.075, 2791738.818,
  2791746.647, 2791762.304, 2791774.047
};
const float station_coordinate_Z[NUM_STATIONS] = {
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747, -3200483.747,
  -3200483.747, -3200483.747, -3200483.747, -3200483.694, -3200457.305, -3200417.642,
  -3200391.200, -3200338.316, -3200298.653  
};
const char array_names[NUM_ARRAYS][ARRAY_NAME_LENGTH] = {
  "6A", "6B", "6C", "6D", "1.5A", "1.5B", "1.5C", "1.5D", "750A", "750B", "750C", "750D",
  "EW367", "EW352", "H214", "H168", "H75", "EW214", "NS214", "122C", "375", "210",
  "122B", "122A"
};
const char array_stations[NUM_ARRAYS][6][STATION_NAME_LENGTH] = {
  { "W4", "W45", "W102", "W173", "W195", "W392" },
  { "W2", "W64", "W147", "W182", "W196", "W392" },
  { "W0", "W10", "W113", "W140", "W182", "W392" },
  { "W8", "W32", "W84", "W168", "W173", "W392" },
  { "W100", "W110", "W147", "W168", "W196", "W392" },
  { "W111", "W113", "W163", "W182", "W195", "W392" },
  { "W98", "W128", "W173", "W190", "W195", "W392" },
  { "W102", "W109", "W140", "W182", "W196", "W392" },
  { "W147", "W163", "W172", "W190", "W195", "W392" },
  { "W98", "W109", "W113", "W140", "W148", "W392" },
  { "W64", "W84", "W100", "W110", "W113", "W392" },
  { "W100", "W102", "W128", "W140", "W147", "W392" },
  { "W104", "W110", "W113", "W124", "W128", "W392" },
  { "W102", "W104", "W109", "W112", "W125", "W392" },
  { "W98", "W104", "W113", "N5", "N14", "W392" },
  { "W100", "W104", "W111", "N7", "N11", "W392" },
  { "W104", "W106", "W109", "N2", "N5", "W392" },
  { "W98", "W102", "W104", "W109", "W112", "W392" },
  { "W106", "N2", "N7", "N11", "N14", "W392" },
  { "W98", "W100", "W102", "W104", "W106", "W392" },
  { "W2", "W10", "W14", "W16", "W32", "W392" },
  { "W98", "W100", "W102", "W109", "W112", "W392" },
  { "W8", "W10", "W12", "W14", "W16", "W392" },
  { "W0", "W2", "W4", "W6", "W8", "W392" }
};


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

/*!
 *  \brief Find the index of the named IF
 *  \param scan_header_data header information about the scan
 *  \param name the name of the window you're searching for, something like f1 or z2
 *  \return the index of the named IF in the header data, or -1 if it isn't found
 */
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

  return res;
}

/*!
 *  \brief Find the index of the named IF, ensuring a valid index is always
 *         returned
 *  \param scan_header_data header information about the scan
 *  \param name the name of the window you're searching for, something like f1 or z2
 *  \return the index of the named IF in the header data, or 1 (which is always
 *          present) if the named IF isn't found
 */
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
  seconds -= d * 86400;
  
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

  FREE(tels);
  
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
  char oform_second[10], oform_third[10], oform_all[40], signchar[2];
  
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
    secondlength = (precision > 0) ? 3 + precision : 2;
    snprintf(oform_second, 9, "%%0%d.%df%%s", secondlength, precision);
    oform_third[0] = 0;
  } else {
    strcpy(oform_second, "%02d%s");
    thirdlength = (precision > 0) ? 3 + precision : 2;
    snprintf(oform_third, 9, "%%0%d.%df%%s", thirdlength, precision);
  }
  snprintf(oform_all, 39, "%%s%s%s%s", oform_first, oform_second, oform_third);
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

/*!
 *  \brief From an antenna's XYZ coordinates, determine which station it was
 *         on
 *  \param x the X coordinate of the antenna, in m
 *  \param y the Y coordinate of the antenna, in m
 *  \param z the Z coordinate of the antenna, in m
 *  \param station_name this string (which must have available length of at least
 *                      STATION_NAME_LENGTH) will be filled by this routine with 
 *                      the name of the station corresponding to the supplied
 *                      coordinates
 *  \return an indication of whether a station was found or not, either:
 *          - FINDSTATION_FOUND if a match was found
 *          - FINDSTATION_NOT_FOUND if a match was not found
 */
int find_station(float x, float y, float z, char *station_name) {
  int i, min_match;
  float min_dist, diff_x, diff_y, diff_z, diff;

  // Work out the minimum distance.
  for (i = 0; i < NUM_STATIONS; i++) {
    diff_x = x - station_coordinate_X[i];
    diff_y = y - station_coordinate_Y[i];
    diff_z = z - station_coordinate_Z[i];
    diff = sqrtf(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
    if ((i == 0) || (diff < min_dist)) {
      min_dist = diff;
      min_match = i;
    }
  }

  // Check this is small enough.
  if (min_dist < 1) {
    // Good enough match.
    strncpy(station_name, station_names[min_match], STATION_NAME_LENGTH);
    return FINDSTATION_FOUND;
  }
  station_name[0] = 0;
  return FINDSTATION_NOT_FOUND;
}

/*!
 *  \brief From all the antenna station names, determine which configuration the
 *         array was in
 *  \param station_names an array of 6 strings, with each string being the name
 *                       of a station
 *  \param array_configuration this string (which must have available length of at
 *                             least ARRAY_NAME_LENGTH) will be filled by this 
 *                             routine with the name of the array configuration
 *                             corresponding to the supplied stations
 *  \return an indication of whether an array configuration was found or not, either:
 *          - FINDARRAYCONFIG_FOUND if a match was found
 *          - FINDARRAYCONFIG_NOT_FOUND if a match was not found
 */ 
int find_array_configuration(char **station_names, char *array_configuration) {
  int i, j, k, max_matches = 0, match_array, curr_match_count;
  
  for (i = 0; i < NUM_ARRAYS; i++) {
    curr_match_count = 0;
    for (j = 0; j < 6; j++) {
      for (k = 0; k < 6; k++) {
	if (strcmp(station_names[j], array_stations[i][k]) == 0) {
	  curr_match_count++;
	  break;
	}
      }
    }
    if (curr_match_count > max_matches) {
      max_matches = curr_match_count;
      match_array = i;
    }
  }

  if (max_matches == 6) {
    // Need a perfect match.
    strncpy(array_configuration, array_names[match_array], ARRAY_NAME_LENGTH);
    return FINDARRAYCONFIG_FOUND;
  }
  array_configuration[0] = 0;
  return FINDARRAYCONFIG_NOT_FOUND;
}

/*!
 *  \brief From a scan header which includes information about antenna positions,
 *         fill an array_information structure
 *  \param scan_header_data a pre-filled structure containing information from
 *                          a scan header 
 *  \param array_information this routine will fill this structure using information
 *                           from \a scan_header_data; if the dereferenced pointer is
 *                           NULL upon entry, a structure will be allocated
 *  \return a return code from the last routine this routine calls:
 *          - FINDSTATION_NOT_FOUND: one of the antenna positions did not match a
 *                                   known station location
 *          - FINDARRAYCONFIG_NOT_FOUND: the station locations did not match a known
 *                                       array configuration
 *          - FINDARRAYCONFIG_FOUND: everything worked
 */
int scan_header_to_array(struct scan_header_data *scan_header_data,
			 struct array_information **array_information) {
  int i, r;
  
  if (*array_information == NULL) {
    MALLOC(*array_information, 1);
  }

  // Initialise the output structure.
  (*array_information)->num_ants = scan_header_data->num_ants;
  CALLOC((*array_information)->X, (*array_information)->num_ants);
  CALLOC((*array_information)->Y, (*array_information)->num_ants);
  CALLOC((*array_information)->Z, (*array_information)->num_ants);
  CALLOC((*array_information)->station_name, (*array_information)->num_ants);
  for (i = 0; i < (*array_information)->num_ants; i++) {
    CALLOC((*array_information)->station_name[i], STATION_NAME_LENGTH);
  }

  for (i = 0; i < (*array_information)->num_ants; i++) {
    // Copy information.
    (*array_information)->X[i] = scan_header_data->ant_cartesian[i][0];
    (*array_information)->Y[i] = scan_header_data->ant_cartesian[i][1];
    (*array_information)->Z[i] = scan_header_data->ant_cartesian[i][2];

    // Get the station name.
    r = find_station((*array_information)->X[i], (*array_information)->Y[i],
		     (*array_information)->Z[i], (*array_information)->station_name[i]);
    if (r == FINDSTATION_NOT_FOUND) {
      return r;
    }
  }

  // Work out the array configuration.
  r = find_array_configuration((*array_information)->station_name,
			       (*array_information)->array_configuration);

  return r;
}

/*!
 *  \brief Free memory from an array_information structure
 *  \param array_information a pointer to an array_information structure; upon
 *                           exit, both the internal memory and the structure memory
 *                           will be freed, and the pointer should point to NULL
 */
void free_array_information(struct array_information **array_information) {
  int i;
  
  if (*array_information == NULL) {
    return;
  }

  for (i = 0; i < (*array_information)->num_ants; i++) {
    FREE((*array_information)->station_name[i]);
  }
  FREE((*array_information)->X);
  FREE((*array_information)->Y);
  FREE((*array_information)->Z);
  FREE((*array_information)->station_name);

  FREE(*array_information);

  return;
}

/*!
 *  \brief Strip all spaces from the end of a string and return a new string
 *         without those spaces
 *  \param s the string which might have spaces at the end
 *  \param r a string with no spaces after valid non-space characters, after being
 *           copied from \a s
 *  \param rlen the maximum length of \a r
 *
 * This routine is useful for several RPFITS strings which have fixed size padded
 * with spaces at the end. Printing these padded strings looks dumb and takes up
 * unnecessary terminal space.
 */
void strip_end_spaces(char *s, char *r, size_t rlen) {
  unsigned int i, last_char;

  strncpy(r, s, rlen);
  for (i = 0, last_char = 0; i < strlen(r); i++) {
    if (r[i] != ' ') {
      last_char = i;
    }
  }

  if (++last_char < rlen) {
    r[last_char] = 0;
  }
}
