/**
 * ATCA Training Library: rpfitsfile_server.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This application makes one or more RPFITS files available to
 * the network control and view tasks.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <argp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "atrpfits.h"
#include "memory.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#define SOCKET int

const char *argp_program_version = "rpfitsfile_server 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "RPFITS file reader for network tasks";

// Argument description.
static char args_doc[] = "[options] RPFITS_FILES...";

// Our options.
static struct argp_option options[] = {
  { 0 }
};

// The arguments structure.
struct arguments {
  int n_rpfits_files;
  char **rpfits_files;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  
  switch (key) {
  case ARGP_KEY_ARG:
    arguments->n_rpfits_files += 1;
    REALLOC(arguments->rpfits_files, arguments->n_rpfits_files);
    arguments->rpfits_files[arguments->n_rpfits_files - 1] = arg;
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

#define BUFSIZE 2048

struct rpfits_file_information {
  char filename[BUFSIZE];
  int n_scans;
  struct scan_header_data *scan_headers;
  double *scan_start_mjd;
  double *scan_end_mjd;
};

struct rpfits_file_information *new_rpfits_file(void) {
  struct rpfits_file_information *rv;

  MALLOC(rv, 1);
  rv->scan_headers = NULL;
  rv->n_scans = 0;
  rv->filename[0] = 0;
  rv->scan_start_mjd = NULL;
  rv->scan_end_mjd = NULL;

  return (rv);
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

// The ways in which we can read data.
#define READ_SCAN_METADATA 1

void data_reader(int read_type, int n_rpfits_files,
		 struct rpfits_file_information **info_rpfits_files) {
  int i, res, open_file, keep_reading, n, read_cycles, keep_cycling;
  int cycle_free, header_free;
  struct scan_header_data *sh = NULL;
  struct cycle_data *cycle_data = NULL;
  
  for (i = 0; i < n_rpfits_files; i++) {
    // HERE WILL GO ANY CHECKS TO SEE IF WE ACTUALLY WANT TO
    // READ THIS FILE
    open_file = 0;
    if (read_type == READ_SCAN_METADATA) {
      open_file = 1;
    }
    if (!open_file) {
      continue;
    }
    res = open_rpfits_file(info_rpfits_files[i]->filename);
    if (res) {
      fprintf(stderr, "OPEN FAILED FOR FILE %s, CODE %d\n",
	      info_rpfits_files[i]->filename, res);
      continue;
    }
    keep_reading = 1;
    while (keep_reading) {
      n = info_rpfits_files[i]->n_scans;
      if (read_type == READ_SCAN_METADATA) {
	// We need to expand our metadata arrays as we go.
	REALLOC(info_rpfits_files[i]->scan_headers, (n + 1));
	REALLOC(info_rpfits_files[i]->scan_start_mjd, (n + 1));
	REALLOC(info_rpfits_files[i]->scan_end_mjd, (n + 1));
      }
      // We always need to read the scan headers to move through
      // the file, but where we direct the information changes.
      if (read_type != READ_SCAN_METADATA) {
	MALLOC(sh, 1);
      } else {
	sh = &(info_rpfits_files[i]->scan_headers[n]);
      }
      res = read_scan_header(sh);
      header_free = 1;
      if (read_type == READ_SCAN_METADATA) {
	// Keep track of the times covered by each scan.
	if (sh->ut_seconds > 0) {
	  header_free = 0;
	  info_rpfits_files[i]->scan_start_mjd[n] =
	    info_rpfits_files[i]->scan_end_mjd[n] = date2mjd(sh->obsdate, sh->ut_seconds);
	  info_rpfits_files[i]->n_scans += 1;
	}
      }
      // HERE WILL GO THE LOGIC TO WORK OUT IF WE NEED TO READ
      // CYCLES FROM THIS SCAN
      read_cycles = 0;
      if (read_type == READ_SCAN_METADATA) {
	read_cycles = 1;
      }
      if (read_cycles && (res & READER_DATA_AVAILABLE)) {
	keep_cycling = 1;
	while (keep_cycling) {
	  cycle_data = prepare_new_cycle_data();
	  res = read_cycle_data(sh, cycle_data);
	  cycle_free = 1;
	  if (!(res & READER_DATA_AVAILABLE)) {
	    keep_cycling = 0;
	  }
	  // HERE WILL GO THE LOGIC TO WORK OUT IF WE WANT TO DO
	  // WITH THIS CYCLE
	  if (read_type == READ_SCAN_METADATA) {
	    info_rpfits_files[i]->scan_end_mjd[n] = date2mjd(sh->obsdate, cycle_data->ut_seconds);
	  }
	  // Do we need to free this memory now?
	  if (cycle_free) {
	    free_cycle_data(cycle_data);
	    FREE(cycle_data);
	  }
	}
      }
      if (header_free) {
	free_scan_header_data(sh);
      }
      if (res == READER_EXHAUSTED) {
	keep_reading = 0;
      }
    }
    // If we get here we must have opened the RPFITS file.
    res = close_rpfits_file();
    if (res) {
      fprintf(stderr, "CLOSE FAILED FOR FILE %s, CODE %d\n",
	      info_rpfits_files[i]->filename, res);
      return;
    }
  }
}

int main(int argc, char *argv[]) {
  struct arguments arguments;
  int i, j;
  struct rpfits_file_information **info_rpfits_files = NULL;

  // Set the defaults for the arguments.
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;

  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Stop here if we don't have any RPFITS files.
  if (arguments.n_rpfits_files == 0) {
    fprintf(stderr, "NO RPFITS FILES SPECIFIED, EXITING\n");
    exit(-1);
  }

  // Do a scan of each RPFITS file to get time information.
  MALLOC(info_rpfits_files, arguments.n_rpfits_files);
  // Put the names of the RPFITS files into the info structure.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    info_rpfits_files[i] = new_rpfits_file();
    strncpy(info_rpfits_files[i]->filename, arguments.rpfits_files[i], BUFSIZE);
  }
  data_reader(READ_SCAN_METADATA, arguments.n_rpfits_files, info_rpfits_files);

  // Print out the summary.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    printf("RPFITS FILE: %s (%d scans):\n",
	   info_rpfits_files[i]->filename, info_rpfits_files[i]->n_scans);
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      printf("  scan %d (%s, %s) MJD range %.6f -> %.6f\n", (j + 1),
	     info_rpfits_files[i]->scan_headers[j].source_name,
	     info_rpfits_files[i]->scan_headers[j].obstype,
	     info_rpfits_files[i]->scan_start_mjd[j],
	     info_rpfits_files[i]->scan_end_mjd[j]);
    }
    printf("\n");
  }
  
  // We're finished, free all our memory.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      free_scan_header_data(&(info_rpfits_files[i]->scan_headers[j]));
    }
    FREE(info_rpfits_files[i]->scan_headers);
    FREE(info_rpfits_files[i]->scan_start_mjd);
    FREE(info_rpfits_files[i]->scan_end_mjd);
    FREE(info_rpfits_files[i]);
  }
  FREE(info_rpfits_files);
  FREE(arguments.rpfits_files);
  
}
