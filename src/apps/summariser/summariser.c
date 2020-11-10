/** \file summariser.c
 *  \brief An application to summarise one or more RPFITS files
 *
 * ATCA Training Application
 * (C) Jamie Stevens CSIRO 2020
 *
 * This app summarises one or more RPFITS files, and can help interrogate data in those
 * files, and can help debug the library.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include "atrpfits.h"
#include "memory.h"
#include "common.h"

const char *argp_program_version = "summariser 1.0";
const char *arpg_program_bug_address = "<Jamie.Stevens@csiro.au>";

static char summariser_doc[] = "RPFITS file summariser and inspector";
static char summariser_args_doc[] = "[options] RPFITS_FILES...";

/*! \def VERBOSITY_QUIET
 *  \brief Magic number for the verbosity to indicate the least amount of output
 *
 * In this level we print information about: TODO
 */
#define VERBOSITY_QUIET   0
/*! \def VERBOSITY_NORMAL
 *  \brief Magic number for the verbosity to indicate the normal amount of output
 *
 * In this level we print information about: TODO
 */
#define VERBOSITY_NORMAL  1
/*! \def VERBOSITY_HIGH
 *  \brief Magic number for the verbosity to indicate a bit more output than normal
 *
 * In this level we print information about: TODO
 */
#define VERBOSITY_HIGH    2
/*! \def VERBOSITY_HIGHEST
 *  \brief Magic number for the verbosity to indicate the most amount of output
 *
 * In this level we print information about: TODO
 */
#define VERBOSITY_HIGHEST 3

static struct argp_option summariser_options[] = {
  { "verbose", 'v', 0, 0, "Increase verbosity of output" },
  { "quiet", 'q', 0, 0, "Set verbosity to quiet" },
  { 0 }
};

/*! \struct summariser_arguments
 *  \brief The structure holding values representing the command line arguments
 *         we were called with; filled by the argp library
 */
struct summariser_arguments {
  /*! \var verbosity
   *  \brief The verbosity level called for by the user
   */
  int verbosity;
  /*! \var n_rpfits_files
   *  \brief The number of RPFITS files that were specified by the user
   */
  int n_rpfits_files;
  /*! \var rpfits_files
   *  \brief The name of each RPFITS file that was specified by the user
   *
   * This array of strings has length `n_rpfits_files` and is indexed starting
   * at 0. Each string has its own length (argp handles this) but is properly
   * NULL terminated.
   */
  char **rpfits_files;
};

static error_t summariser_parse_opt(int key, char *arg, struct argp_state *state) {
  struct summariser_arguments *arguments = state->input;

  switch (key) {
  case 'v':
    if (arguments->verbosity < VERBOSITY_HIGHEST) {
      arguments->verbosity += 1;
    }
    break;
  case 'q':
    arguments->verbosity = VERBOSITY_QUIET;
    break;
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

static struct argp argp = { summariser_options, summariser_parse_opt,
			    summariser_args_doc, summariser_doc };

#define SBUFSIZE 1024

int main(int argc, char *argv[]) {
  // The argument list should all be RPFITS files.
  int i = 0, k = 0, res = 0, keep_reading = 1, read_response = 0;
  int read_cycle = 1, nscans = 0, n_continuum = 0, n_zooms = 0;
  int *continuum_bands = NULL, *zoom_bands = NULL;
  float channel_width;
  char emsg[SBUFSIZE], rastring[SBUFSIZE], decstring[SBUFSIZE];
  char sourcename[SBUFSIZE], scantype[BUFSIZE], antname[SBUFSIZE];
  struct scan_data *scan_data = NULL, **all_scans = NULL;
  struct cycle_data *cycle_data = NULL;
  struct summariser_arguments arguments;
  struct ampphase_options ampphase_options;
  struct array_information *array_information = NULL;

  // Set the defaults for the arguments.
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;
  arguments.verbosity = VERBOSITY_NORMAL;

  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Stop here if we don't have any RPFITS files.
  if (arguments.n_rpfits_files == 0) {
    fprintf(stderr, "NO RPFITS FILES SPECIFIED, EXITING\n");
    return (-1);
  }
  
  for (k = 0; k < arguments.n_rpfits_files; k++) {
    // Try to open the RPFITS file.
    res = open_rpfits_file(arguments.rpfits_files[k]);
    if (arguments.verbosity >= VERBOSITY_NORMAL) {
      snprintf(emsg, SBUFSIZE, " error code %d", res);
      printf("RPFITS file %s: %s\n", arguments.rpfits_files[k],
	     ((res == 0) ? "OK" : emsg));
    }
    keep_reading = 1;
    while (keep_reading) {
      // Make a new scan and add it to the list.
      scan_data = prepare_new_scan_data();
      nscans += 1;
      ARRAY_APPEND(all_scans, nscans, scan_data);

      // Read in the scan header.
      read_response = read_scan_header(&(scan_data->header_data));
      if (arguments.verbosity >= VERBOSITY_NORMAL) {
	seconds_to_hourlabel(scan_data->header_data.ut_seconds, emsg);
	printf("\n\nScan %d at %s %s\n", nscans,
	       scan_data->header_data.obsdate, emsg);
      }
      if (arguments.verbosity >= VERBOSITY_NORMAL) {
	angle_to_string(scan_data->header_data.rightascension_hours, rastring,
			(ATS_ANGLE_IN_HOURS | ATS_STRING_IN_HOURS | ATS_STRING_SEP_LETTER |
			 ATS_STRING_ZERO_PAD_FIRST), 1);
	angle_to_string(scan_data->header_data.declination_degrees, decstring,
			(ATS_ANGLE_IN_DEGREES | ATS_STRING_IN_DEGREES | ATS_STRING_SEP_LETTER |
			 ATS_STRING_ALWAYS_SIGN), 1);
	strip_end_spaces(scan_data->header_data.source_name, sourcename, SBUFSIZE);
	strip_end_spaces(scan_data->header_data.obstype, scantype, SBUFSIZE);
	printf("  Source %s [%s %s] (cc %s), Type %s\n",
	       sourcename, rastring, decstring,
	       scan_data->header_data.calcode, scantype);
      }

      // Work out how many bands and of what type.
      for (i = 0; i < scan_data->header_data.num_ifs; i++) {
	channel_width = scan_data->header_data.if_bandwidth[i] /
	  (float)(scan_data->header_data.if_num_channels[i]);
	if (channel_width >= 1) {
	  // Continuum resolution.
	  n_continuum += 1;
	  ARRAY_APPEND(continuum_bands, n_continuum, i);
	} else {
	  // Zoom resolution.
	  n_zooms += 1;
	  ARRAY_APPEND(zoom_bands, n_zooms, i);
	}
      }

      if (arguments.verbosity >= VERBOSITY_NORMAL) {
	printf("  # Continuum IFs = %d:\n", n_continuum);
	for (i = 0; i < n_continuum; i++) {
	  printf("   C%d: CF = %.1f BW = %.1f MHz # chan = %d, chain = %d (%s)\n",
		 scan_data->header_data.if_label[continuum_bands[i]],
		 scan_data->header_data.if_centre_freq[continuum_bands[i]],
		 scan_data->header_data.if_bandwidth[continuum_bands[i]],
		 scan_data->header_data.if_num_channels[continuum_bands[i]],
		 scan_data->header_data.if_chain[continuum_bands[i]],
		 scan_data->header_data.if_name[continuum_bands[i]][0]);
	}
      }
      if (arguments.verbosity >= VERBOSITY_NORMAL) {
	printf("  # Zoom IFS = %d", n_zooms);
	if (arguments.verbosity >= VERBOSITY_HIGH) {
	  printf(":");
	  for (i = 0; i < n_zooms; i++) {
	    printf("\n   Z%d: CF = %.1f BW = %.1f MHz # chan = %d, chain = %d (%s)",
		   scan_data->header_data.if_label[zoom_bands[i]],
		   scan_data->header_data.if_centre_freq[zoom_bands[i]],
		   scan_data->header_data.if_bandwidth[zoom_bands[i]],
		   scan_data->header_data.if_num_channels[zoom_bands[i]],
		   scan_data->header_data.if_chain[zoom_bands[i]],
		   scan_data->header_data.if_name[zoom_bands[i]][1]);
	  }
	}
	printf("\n");
      }
      /* for (i = 0; i < scan_data->header_data.num_ifs; i++) { */
      /* 	printf("   %d: num channels = %d, num stokes = %d, chain = %d (%s %s %s)\n", */
      /* 	       scan_data->header_data.if_label[i], */
      /* 	       scan_data->header_data.if_num_channels[i], */
      /* 	       scan_data->header_data.if_num_stokes[i], */
      /* 	       scan_data->header_data.if_chain[i], */
      /* 	       scan_data->header_data.if_name[i][0], */
      /* 	       scan_data->header_data.if_name[i][1], */
      /* 	       scan_data->header_data.if_name[i][2]); */
      /* 	printf("      "); */
      /* 	for (j = 0; j < scan_data->header_data.if_num_stokes[i]; j++) { */
      /* 	  printf("[%s] ", scan_data->header_data.if_stokes_names[i][j]); */
      /* 	} */
      /* 	printf("\n"); */
      /* } */

      if (arguments.verbosity >= VERBOSITY_HIGH) {
	printf("  %d antennas", scan_data->header_data.num_ants);
	scan_header_to_array(&(scan_data->header_data),
			     &array_information);
	for (i = 0; i < scan_data->header_data.num_ants; i++) {
	  strip_end_spaces(scan_data->header_data.ant_name[i], antname, SBUFSIZE);
	  printf(" %s", antname);
	  if (arguments.verbosity >= VERBOSITY_HIGHEST) {
	    printf(" [%s]", array_information->station_name[i]);
	  }
	}
	printf("\n    ARRAY %s\n", array_information->array_configuration);
	free_array_information(&array_information);
      }
      if (read_response & READER_DATA_AVAILABLE) {
	  // Now start reading the cycle data.
	read_cycle = 1;
	ampphase_options = ampphase_options_default();
	while (read_cycle) {
	  cycle_data = scan_add_cycle(scan_data);
	  read_response = read_cycle_data(&(scan_data->header_data),
					  cycle_data);
	  // Compute the system temperatures.
	  calculate_system_temperatures_cycle_data(cycle_data, &(scan_data->header_data),
						   &ampphase_options);
	  //fprintf(stderr, "found read response %d\n", read_response);
	  /*printf("cycle has %d baselines\n", cycle_data->n_baselines);*/
	  if (!(read_response & READER_DATA_AVAILABLE)) {
	    read_cycle = 0;
	  }
	}
      } 
      if (read_response == READER_EXHAUSTED) {
	// No more data in this file.
	keep_reading = 0;
      } // Otherwise we've probably hit another header.
      if (arguments.verbosity >= VERBOSITY_NORMAL) {
	printf(" Scan %d had %d cycles\n", nscans, scan_data->num_cycles);
      }

    }

    // Close it before moving on.
    res = close_rpfits_file();
    printf("Attempt to close RPFITS file, %d\n", res);
  }

  // Free all the scans.
  printf("Read in %d scans from all files.\n", nscans);
  for (i = 0; i < nscans; i++) {
    free_scan_data(all_scans[i]);
  }
  FREE(all_scans);
  
  exit(0);
  
}
