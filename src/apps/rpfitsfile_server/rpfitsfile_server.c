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
#include <stdbool.h>
#include "atrpfits.h"
#include "memory.h"
#include "cmp.h"
#include "packing.h"

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


void data_reader(int read_type, int n_rpfits_files,
		 double mjd_required, struct ampphase_options *ampphase_options,
		 struct rpfits_file_information **info_rpfits_files,
		 struct spectrum_data *spectrum_data,
		 struct vis_quantities ****vis_quantities) {
  int i, res, open_file, keep_reading, n, read_cycles, keep_cycling, calcres;
  int cycle_free, header_free, curr_header, idx_if, idx_pol;
  int pols[4] = { POL_XX, POL_YY, POL_XY, POL_YX };
  double cycle_mjd, cycle_start, cycle_end, half_cycle;
  struct scan_header_data *sh = NULL;
  struct cycle_data *cycle_data = NULL;

  if (vis_quantities == NULL) {
    // Resist warnings.
    fprintf(stderr, "ignore this message\n");
  }
  
  for (i = 0; i < n_rpfits_files; i++) {
    // HERE WILL GO ANY CHECKS TO SEE IF WE ACTUALLY WANT TO
    // READ THIS FILE
    open_file = false;
    if (read_type & READ_SCAN_METADATA) {
      open_file = true;
    }
    if (read_type & GRAB_SPECTRUM) {
      // Does this file encompass the time we want?
      half_cycle = (double)info_rpfits_files[i]->scan_headers[0].cycle_time / (2.0 * 86400.0);
      if ((mjd_required >= (info_rpfits_files[i]->scan_start_mjd[0] - half_cycle)) &&
	  (mjd_required <= (info_rpfits_files[i]->scan_end_mjd[info_rpfits_files[i]->n_scans - 1]
			    + half_cycle))) {
	open_file = true;
      /* } else { */
      /* 	printf("File %s does not cover the requested MJD %.6f\n", */
      /* 	       info_rpfits_files[i]->filename, mjd_required); */
      }
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
    keep_reading = true;
    curr_header = -1;
    while (keep_reading) {
      n = info_rpfits_files[i]->n_scans;
      if (read_type & READ_SCAN_METADATA) {
	// We need to expand our metadata arrays as we go.
	REALLOC(info_rpfits_files[i]->scan_headers, (n + 1));
	REALLOC(info_rpfits_files[i]->scan_start_mjd, (n + 1));
	REALLOC(info_rpfits_files[i]->scan_end_mjd, (n + 1));
      }
      // We always need to read the scan headers to move through
      // the file, but where we direct the information changes.
      if (!(read_type & READ_SCAN_METADATA)) {
	MALLOC(sh, 1);
      } else {
	sh = &(info_rpfits_files[i]->scan_headers[n]);
      }
      res = read_scan_header(sh);
      header_free = true;
      if (sh->ut_seconds > 0) {
	curr_header += 1;
	if (read_type & READ_SCAN_METADATA) {
	  // Keep track of the times covered by each scan.
	  header_free = false;
	  info_rpfits_files[i]->scan_start_mjd[n] =
	    info_rpfits_files[i]->scan_end_mjd[n] = date2mjd(sh->obsdate, sh->ut_seconds);
	  info_rpfits_files[i]->n_scans += 1;
	}
	// HERE WILL GO THE LOGIC TO WORK OUT IF WE NEED TO READ
	// CYCLES FROM THIS SCAN
	read_cycles = false;
	if ((read_type & READ_SCAN_METADATA) ||
	    (read_type & COMPUTE_VIS_PRODUCTS)) {
	  // In both of these cases we have to look at all data.
	  read_cycles = true;
	}
	if (read_type & GRAB_SPECTRUM) {
	  // Check if this scan contains the cycle with MJD matching
	  // the required.
	  /* printf("assessing scan header %d: %.6f / %.6f / %.6f\n", curr_header, */
	  /* 	 info_rpfits_files[i]->scan_start_mjd[curr_header], */
	  /* 	 mjd_required, */
	  /* 	 info_rpfits_files[i]->scan_end_mjd[curr_header]); */
	  if ((mjd_required >= info_rpfits_files[i]->scan_start_mjd[curr_header]) &&
	      (mjd_required <= info_rpfits_files[i]->scan_end_mjd[curr_header])) {
	    /* printf("scan should contain what we're looking for\n"); */
	    read_cycles = true;
	  /* } else { */
	  /*   printf("ignoring this scan\n"); */
	  }
	}
	if (read_cycles && (res & READER_DATA_AVAILABLE)) {
	  keep_cycling = true;
	  while (keep_cycling) {
	    cycle_data = prepare_new_cycle_data();
	    res = read_cycle_data(sh, cycle_data);
	    cycle_free = true;
	    if (!(res & READER_DATA_AVAILABLE)) {
	      keep_cycling = false;
	    }
	    // HERE WILL GO THE LOGIC TO WORK OUT IF WE WANT TO DO
	    // SOMETHING WITH THIS CYCLE
	    cycle_mjd = date2mjd(sh->obsdate, cycle_data->ut_seconds);
	    if (read_type & READ_SCAN_METADATA) {
	      info_rpfits_files[i]->scan_end_mjd[n] = cycle_mjd;
	    }
	    if (read_type & GRAB_SPECTRUM) {
	      // We consider this the correct cycle if the requested
	      // MJD is within half a cycle time of this cycle's time.
	      cycle_start = cycle_mjd - ((double)sh->cycle_time / (2 * 86400.0));
	      cycle_end = cycle_mjd + ((double)sh->cycle_time / (2 * 86400.0));
	      /* printf("%.6f / %.6f / %.6f\n", cycle_start, mjd_required, cycle_end); */
	      if ((mjd_required >= cycle_start) &&
		  (mjd_required <= cycle_end)) {
		// Convert this cycle into the spectrum that we return.
		// Allocate all the memory we need.
		printf("cycle found!\n");
		spectrum_data->num_ifs = sh->num_ifs;
		MALLOC(spectrum_data->spectrum, spectrum_data->num_ifs);
		for (idx_if = 0; idx_if < spectrum_data->num_ifs; idx_if++) {
		  spectrum_data->num_pols = sh->if_num_stokes[idx_if];
		  CALLOC(spectrum_data->spectrum[idx_if], spectrum_data->num_pols);
		  for (idx_pol = 0; idx_pol < spectrum_data->num_pols; idx_pol++) {
		    calcres = vis_ampphase(sh, cycle_data,
					   &(spectrum_data->spectrum[idx_if][idx_pol]),
					   pols[idx_pol], sh->if_label[idx_if],
					   ampphase_options);
		    if (calcres < 0) {
		      fprintf(stderr, "CALCULATING AMP AND PHASE FAILED FOR IF %d POL %d, CODE %d\n",
			      sh->if_label[idx_if], pols[idx_pol], calcres);
		    } else {
		      printf("CONVERTED SPECTRUM FOR CYCLE IF %d POL %d AT MJD %.6f\n",
			     sh->if_label[idx_if], pols[idx_pol], cycle_mjd);
		    }
		  }
		}
		// We don't need to search any more.
		keep_cycling = false;
		keep_reading = false;
	      }
	    }
	    // Do we need to free this memory now?
	    if (cycle_free) {
	      free_cycle_data(cycle_data);
	      FREE(cycle_data);
	    }
	  }
	}
      }
      if (header_free) {
	free_scan_header_data(sh);
      }
      if (res == READER_EXHAUSTED) {
	keep_reading = false;
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
  struct ampphase_options ampphase_options;
  struct spectrum_data spectrum_data;
  struct vis_quantities ***vis_quantities = NULL;
  FILE *fh = NULL;
  cmp_ctx_t cmp;
  
  // Set the defaults for the arguments.
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;

  // And the default for the calculator options.
  ampphase_options.phase_in_degrees = true;
  ampphase_options.delay_averaging = 1;
  ampphase_options.min_tvchannel = 1;
  ampphase_options.max_tvchannel = 2048;
  ampphase_options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_SCALAR;
  ampphase_options.include_flagged_data = 1;
  
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
  data_reader(READ_SCAN_METADATA, arguments.n_rpfits_files, 0.0, &ampphase_options,
	      info_rpfits_files, &spectrum_data, &vis_quantities);

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

  // Let's try to load one of the spectra.
  printf("Trying to grab a spectrum.\n");
  data_reader(GRAB_SPECTRUM, arguments.n_rpfits_files, 58501.470312,
	      &ampphase_options, info_rpfits_files, &spectrum_data, &vis_quantities);
  // Save these spectra to a file after packing it.
  fh = fopen("test.dat", "w+b");
  if (fh == NULL) {
    error_and_exit("Error opening output file");
  }
  cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
  pack_spectrum_data(&cmp, &spectrum_data);
  fclose(fh);
  
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
