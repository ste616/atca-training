/** \file rpfitsfile_server.c
 *  \brief An application to make one or more RPFITS files available to the network
 *         control and view tasks
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
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
#include <time.h>
#include <signal.h>
#include "atrpfits.h"
#include "memory.h"
#include "packing.h"
#include "atnetworking.h"
#include "common.h"

/*! \def RPSBUFSIZE
 *  \brief Length used for "reasonable length" strings throughout
 */
#define RPSBUFSIZE 1024

/*! \var argp_program_version
 *  \brief Variable used by the argp library as the name of the application and
 *         its version number, when printing help
 */
const char *argp_program_version = "rpfitsfile_server 1.0";
/*! \var argp_program_bug_address
 *  \brief Variable used by the argp library as the email address to send a message
 *         to when reporting a bug
 */
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

/*! \var rpfitsfile_server_doc
 *  \brief Variable used by the argp library as a short description of what this
 *         application does, when printing help
 */
static char rpfitsfile_server_doc[] = "RPFITS file reader for network tasks";

// Argument description.
/*! \var rpfitsfile_server_args_doc
 *  \brief Variable used by the argp library as a description of how to call
 *         this application, when printing help
 */
static char rpfitsfile_server_args_doc[] = "[options] RPFITS_FILES...";

// Our options.
/*! \var rpfitsfile_server_options
 *  \brief An array of options we accept, required by the argp library to allow
 *         it to properly parse command line arguments
 */
static struct argp_option rpfitsfile_server_options[] = {
  { "minimum_mjd", 'j', "MJD", 0,
    "MJD before which no data will be read" },
  { "maximum_mjd", 'J', "MJD", 0,
    "MJD after which no data will be read" },
  { "networked", 'n', 0, 0,
    "Switch to operate as a network data server " },
  { "port", 'p', "PORTNUM", 0,
    "The port number to listen on" },
  { "testing", 't', "TESTFILE", 0,
    "Operate as a testing server with instructions given "
    "in this file (multiple accepted)" },
  { 0 }
};

/*! \struct rpfitsfile_server_arguments
 *  \brief The structure holding values representing the command line arguments
 *         we were called with; filled by the argp library
 */
struct rpfitsfile_server_arguments {
  /*! \var n_rpfits_files
   *  \brief The number of RPFITS files that were specified by the user
   */
  int n_rpfits_files;
  /*! \var rpfits_files
   *  \brief The name of each RPFITS file that was specified by the user
   *
   * This array of strings has length `n_rpfits_files`, and is indexed starting
   * at 0. Each string has its own length (argp handles this) but is properly
   * NULL terminated.
   */
  char **rpfits_files;
  /*! \var port_number
   *  \brief The TCP port number to listen on, default is 8880
   */
  int port_number;
  /*! \var network_operation
   *  \brief A flag to indicate that this application should operate as a
   *         network server
   */
  bool network_operation;
  /*! \var testing_operation
   *  \brief A flag to indicate that we offer an examination to connected
   *         users
   */
  bool testing_operation;
  /*! \var num_instruction_files
   *  \brief The number of instruction files specified by the user
   */
  int num_instruction_files;
  /*! \var testing_instruction_files
   *  \brief The name of each instruction file specified by the user
   *
   * This array of strings has length `num_instruction_files`, and is indexed
   * starting at 0. Each string has length RPSBUFSIZE.
   */
  char **testing_instruction_files;
  /*! \var minimum_read_mjd
   *  \brief MJD before which no data will be read from file
   */
  double minimum_read_mjd;
  /*! \var maximum_read_mjd
   *  \brief MJD after which no data will be read from file
   */
  double maximum_read_mjd;
};

/*!
 *  \brief Routine that helps argp parse the command line arguments and
 *         store the appropriate values in the rpfitsfile_server_arguments
 *         structure
 *  \param key a single character key, as specified in rpfitsfile_server_options,
 *             representing the option being parsed
 *  \param arg the argument string specified by the user alongside the key, or NULL if
 *             the option doesn't accept arguments
 *  \param state an argp internal state variable
 *  \return an indication of success, or error reason magic number, as known by argp
 */
static error_t rpfitsfile_server_parse_opt(int key, char *arg, struct argp_state *state) {
  struct rpfitsfile_server_arguments *arguments = state->input;
  
  switch (key) {
  case 'j':
    if (!string_to_double(arg, &(arguments->minimum_read_mjd))) {
      arguments->minimum_read_mjd = -INFINITY;
    }
    break;
  case 'J':
    if (!string_to_double(arg, &(arguments->maximum_read_mjd))) {
      arguments->maximum_read_mjd = INFINITY;
    }
    break;
  case 'n':
    arguments->network_operation = true;
    break;
  case 'p':
    arguments->port_number = atoi(arg);
    break;
  case 't':
    arguments->testing_operation = true;
    REALLOC(arguments->testing_instruction_files, ++(arguments->num_instruction_files));
    MALLOC(arguments->testing_instruction_files[arguments->num_instruction_files - 1], RPSBUFSIZE);
    strncpy(arguments->testing_instruction_files[arguments->num_instruction_files - 1],
            arg, RPSBUFSIZE);
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

static struct argp argp = {
  rpfitsfile_server_options,
  rpfitsfile_server_parse_opt,
  rpfitsfile_server_args_doc,
  rpfitsfile_server_doc
};


/*! \struct file_instructions
 *  \brief A linked list structure to contain instructions from a file
 */
struct file_instructions {
  // The files to read for this instruction.
  int num_files;
  char **files;
  // The time range of data to give to the user.
  double start_mjd;
  double end_mjd;
  // The options to use while loading the data.
  struct ampphase_options ampphase_options;
  // The initial data cache.
  struct spectrum_data *spectrum_data;
  struct vis_data *vis_data;
  // The message to give to the user when they ask for it.
  char *message;
  // The type of response to expect.
  int response_type;
  // A list of options if it's multiple choice.
  int num_choices;
  char **choices;
  // Prefix for output files.
  char output_prefix[RPSBUFSIZE];
  
  // The linked list pointers.
  struct file_instructions *next;
  struct file_instructions *prev;
};

#define SECTION_MESSAGE  1
#define SECTION_OPTION   2
#define SECTION_MJD      3
#define SECTION_FILES    4
#define SECTION_AMPPHASE 5

void read_instruction_file(char *file, struct file_instructions **head) {
  struct file_instructions *file_instructions = NULL, *prev = NULL;
  FILE *fh = NULL;
  char lstr[RPSBUFSIZE], mtype[RPSBUFSIZE];
  int section = 0;
  size_t l2;
  double mjd;
    
  // We read an instructions file and add any instruction
  // structures to the linked list.
  // Open the file.
  fh = fopen(file, "r");
  if (fh == NULL) {
    fprintf(stderr, "ERROR: Unable to read instructions file %s.\n",
            file);
    return;
  }
  while (fgets(lstr, RPSBUFSIZE, fh) != NULL) {
    // Get rid of the new line.
    lstr[strcspn(lstr, "\n")] = 0;
    // Is this line a comment?
    if (lstr[0] == '#') {
      // Ignore it.
      continue;
    }
    // Check for a section change.
    if (strncmp(lstr, "MESSAGE", 7) == 0) {
      section = SECTION_MESSAGE;
    } else if (strncmp(lstr, "END MESSAGE", 11) == 0) {
      section = 0;
    } else if (strncmp(lstr, "OPTION", 6) == 0) {
      section = SECTION_OPTION;
      // Increment the number of options.
      file_instructions->num_choices += 1;
      REALLOC(file_instructions->choices, file_instructions->num_choices);
      file_instructions->choices[file_instructions->num_choices - 1] = NULL;
    } else if (strncmp(lstr, "END OPTION", 10) == 0) {
      section = 0;
    } else if (strncmp(lstr, "MJD RANGE", 9) == 0) {
      section = SECTION_MJD;
    } else if (strncmp(lstr, "END MJD RANGE", 13) == 0) {
      section = 0;
    } else if (strncmp(lstr, "RPFITS FILES", 12) == 0) {
      section = SECTION_FILES;
    } else if (strncmp(lstr, "END RPFITS FILES", 16) == 0) {
      section = 0;
    }
    // Or a single line definition.
    else if (strncmp(lstr, "RESPONSE TYPE ", 14) == 0) {
      // What type of response should the user give us.
      if (strncmp((lstr + 14), "MULTIPLE_CHOICE", 15) == 0) {
        file_instructions->response_type = TESTTYPE_MULTIPLE_CHOICE;
      } else if (strncmp((lstr + 14), "FREE_RESPONSE", 13) == 0) {
        file_instructions->response_type = TESTTYPE_FREE_RESPONSE;
      }
    } else if (strncmp(lstr, "TEST NAME ", 10) == 0) {
      // The name of the test is used for the output prefix.
      strncpy(file_instructions->output_prefix, (lstr + 10), (RPSBUFSIZE - 10));
    }
    // Deal with each section.
    else if (section == SECTION_MESSAGE) {
      // Add this line to the message.
      stringappend(&(file_instructions->message), lstr);
    } else if (section == SECTION_OPTION) {
      stringappend(&(file_instructions->choices[file_instructions->num_choices - 1]),
                   lstr);
    } else if (section == SECTION_FILES) {
      // Add a new file to the list.
      file_instructions->num_files += 1;
      REALLOC(file_instructions->files, file_instructions->num_files);
      l2 = strlen(lstr);
      MALLOC(file_instructions->files[file_instructions->num_files - 1], (l2 + 2));
      strncpy(file_instructions->files[file_instructions->num_files - 1], lstr, l2);
    } else if (section == SECTION_MJD) {
      if (sscanf(lstr, "%s %lf", mtype, &mjd) == 2) {
        // A properly formatted MJD specifier.
        if (strncmp(mtype, "START", 5) == 0) {
          file_instructions->start_mjd = mjd;
        } else if (strncmp(mtype, "END", 3) == 0) {
          file_instructions->end_mjd = mjd;
        }
      }
    }
    // Deal when new instructions are found or ended.
    else if (strncmp(lstr, "INSTRUCTION", 11) == 0) {
      // Prepare the instruction.
      MALLOC(file_instructions, 1);
      file_instructions->num_files = 0;
      file_instructions->files = NULL;
      file_instructions->start_mjd = 0;
      file_instructions->end_mjd = 0;
      file_instructions->spectrum_data = NULL;
      file_instructions->vis_data = NULL;
      file_instructions->message = NULL;
      file_instructions->response_type = -1;
      file_instructions->num_choices = 0;
      file_instructions->choices = NULL;
      file_instructions->output_prefix[0] = 0;
    } else if (strncmp(lstr, "END INSTRUCTION", 15) == 0) {
      // Put the instruction on the list.
      if (*head == NULL) {
        // This is the first of the list.
        *head = file_instructions;
        file_instructions->prev = NULL;
      } else {
        prev = *head;
        while (prev->next != NULL) {
          prev = prev->next;
        }
        file_instructions->prev = prev;
        prev->next = file_instructions;
      }
      file_instructions->next = NULL;
    }
  }
  fclose(fh);
  
  
}

/*! \struct cache_vis_data
 *  \brief Cache for vis data
 *
 * After computing vis data with some set of options, we store it in this
 * cache structure so it can be recalled if the same set of options is
 * requested.
 */
struct cache_vis_data {
  /*! \var num_cache_vis_data
   *  \brief The number of cache entries present in this structure
   */
  int num_cache_vis_data;
  /*! \var num_options
   *  \brief The number of options supplied for each cache entry
   *
   * This array has length `num_cache_vis_data`, and is indexed starting at 0.
   */
  int *num_options;
  /*! \var ampphase_options
   *  \brief The set of options supplied for each cache entry
   *
   * This 2-D array of pointers has length `num_cache_vis_data` on the
   * first index, and `num_options[i]` for the second index, where `i` is the
   * position along the first index. Both indices start at 0.
   */
  struct ampphase_options ***ampphase_options;
  /*! \var vis_data
   *  \brief The cached vis data entries
   *
   * This array of pointers has length `num_cache_vis_data`, and is indexed
   * starting at 0.
   */
  struct vis_data **vis_data;
};

struct cache_vis_data cache_vis_data;

/*! \struct client_vis_data
 *  \brief Client cache of vis data
 */
struct client_vis_data {
  /*! \var num_clients
   *  \brief The number of clients we know about
   */
  int num_clients;
  /*! \var client_id
   *  \brief The ID of each client
   *
   * This array of strings has length `num_clients`, and is indexed starting
   * at 0. Each string has length CLIENTIDLENGTH.
   */
  char **client_id;
  /*! \var vis_data
   *  \brief The vis_data associated with each client
   *
   * This array has length `num_clients`, and is indexed starting at 0.
   */
  struct vis_data **vis_data;
};

/*! \struct cache_spd_data
 *  \brief Cache for SPD data
 *
 * After computing SPD data with some set of options, we store it in this
 * cache structure so it can be recalled if the same set of options is
 * requested.
 */
struct cache_spd_data {
  /*! \var num_cache_spd_data
   *  \brief The number of cache entries present in this structure
   */
  int num_cache_spd_data;
  /*! \var num_options
   *  \brief The number of options supplied for each cache entry
   *
   * This array has length `num_cache_spd_data`, and is indexed starting at 0.
   */
  int *num_options;
  /*! \var ampphase_options
   *  \brief The set of options supplied for each cache entry
   *
   * This 2-D array of pointers has length `num_cache_spd_data` on the first
   * index, and `num_options[i]` for the second index, where `i` is the
   * position along the first index. Both indices start at 0.
   */
  struct ampphase_options ***ampphase_options;
  /*! \var spectrum_data
   *  \brief The cached SPD data entries
   *
   * This array of pointers has length `num_cache_spd_data`, and is indexed
   * starting at 0.
   */
  struct spectrum_data **spectrum_data;
};

struct cache_spd_data cache_spd_data;

/*! \struct client_spd_data
 *  \param Client cache of SPD data
 */
struct client_spd_data {
  /*! \var num_clients
   *  \brief The number of clients we know about
   */
  int num_clients;
  /*! \var client_id
   *  \brief The ID of each client
   *
   * This array has length `num_clients`, and is indexed starting at 0.
   * Each string has length CLIENTIDLENGTH.
   */
  char **client_id;
  /*! \var spectrum_data
   *  \brief The spectrum_data associated with each client
   *
   * This array has length `num_clients`, and is indexed starting at 0.
   */
  struct spectrum_data **spectrum_data;
};

/*! \struct rpfits_file_information
 *  \brief Information about an RPFITS file we are asked to make available
 */
struct rpfits_file_information {
  /*! \var filename
   *  \brief The full relative path to the RPFITS file
   */
  char filename[RPSBUFSIZE];
  /*! \var n_scans
   *  \brief The number of scans in the file
   */
  int n_scans;
  /*! \var scan_headers
   *  \brief The header information for each scan
   *
   * This array has length `n_scans`, and is indexed starting at 0.
   */
  struct scan_header_data **scan_headers;
  /*! \var scan_start_mjd
   *  \brief The MJD of the first cycle of each scan
   *
   * This array has length `n_scans`, and is indexed starting at 0.
   */
  double *scan_start_mjd;
  /*! \var scan_end_mjd
   *  \brief The MJD of the last cycle of each scan
   *
   * This array has length `n_scans`, and is indexed starting at 0.
   */
  double *scan_end_mjd;
  /*! \var n_cycles
   *  \brief The number of cycles present in each scan
   *
   * This array has length `n_scans`, and is indexed starting at 0.
   */
  int *n_cycles;
  /*! \var cycle_mjd
   *  \brief The MJD of each cycle in each scan
   *
   * This 2-D array has length `n_scans` for the first index, and
   * `n_cycles[i]` for the second index, where `i` is the position along
   * the first index; both indices start at 0.
   */
  double **cycle_mjd;
};

/*!
 *  \brief Initialise and return a new rpfits_file_information structure
 *  \return the properly initialised structure
 */
struct rpfits_file_information *new_rpfits_file(void) {
  struct rpfits_file_information *rv;

  CALLOC(rv, 1);
  rv->scan_headers = NULL;
  rv->n_scans = 0;
  rv->filename[0] = 0;
  rv->scan_start_mjd = NULL;
  rv->scan_end_mjd = NULL;
  rv->n_cycles = NULL;
  rv->cycle_mjd = NULL;

  return (rv);
}

/*! \struct client_ampphase_options
 *  \brief Structure to hold the last-used ampphase_options set for each
 *         different user or client
 */
struct client_ampphase_options {
  /*! \var num_clients
   *  \brief The number of clients we hold data for
   */
  int num_clients;
  /*! \var client_id
   *  \brief The ID of each of the clients
   *
   * This array of strings has size `num_clients`, and is indexed starting
   * at 0. Each string has length CLIENTIDLENGTH.
   */
  char **client_id;
  /*! \var client_username
   *  \brief The username of each of the clients
   *
   * This array of strings has size `num_clients`, and is indexed starting
   * at 0. Each string has length CLIENTIDLENGTH.
   */
  char **client_username;
  /*! \var n_ampphase_options
   *  \brief The number of ampphase_options provided by each client
   *
   * This array has size `num_clients`, and is indexed starting at 0.
   */
  int *n_ampphase_options;
  /*! \var ampphase_options
   *  \brief The set of ampphase_options provided by each client
   *
   * This 2-D array has size `num_clients` for the first index, and
   * `n_ampphase_options[i]` for the second index, where `i` is the position
   * along the first index. Both indices start at 0.
   */
  struct ampphase_options ***ampphase_options;
};

/*!
 *  \brief Add a SPD cache entry, labelled with a provided set of options
 *  \param num_options the number of options in the set
 *  \param options the set of options with which to label the SPD cache entry
 *  \param data the data to store in the cache
 *  \return an indication of whether the data was added to the cache; false
 *          if an existing entry was found with the same options set, or true
 *          if the set was added
 */
bool add_cache_spd_data(int num_options, struct ampphase_options **options,
                        struct spectrum_data *data) {
  int i, j, n;
  bool match_found = false;
  char details[RPSBUFSIZE], time[RPSBUFSIZE];

  if (num_options == 0) {
    // What??
    return false;
  }
  
  /* fprintf(stderr, "[add_cache_spd_data] trying to cache data at %s %.1f %d\n", */
  /*         data->header_data->obsdate, data->spectrum[0][0]->ut_seconds, */
  /*         options->phase_in_degrees); */
  // Check that we don't already know about the data.
  for (i = 0; i < cache_spd_data.num_cache_spd_data; i++) {
    /* fprintf(stderr, "[add_cache_spd_data] checking cache entry %d %s %.1f %d\n", */
    /*         i, cache_spd_data.spectrum_data[i]->header_data->obsdate, */
    /*         cache_spd_data.spectrum_data[i]->spectrum[0][0]->ut_seconds, */
    /*         cache_spd_data.ampphase_options[i]->phase_in_degrees); */
    if (cache_spd_data.num_options[i] != num_options) {
      // Can't be this one.
      continue;
    }
    if ((strncmp(cache_spd_data.spectrum_data[i]->header_data->obsdate,
		 data->header_data->obsdate, OBSDATE_LENGTH) != 0) ||
	(cache_spd_data.spectrum_data[i]->spectrum[0][0]->ut_seconds !=
	 data->spectrum[0][0]->ut_seconds)) {
      // Not a match.
      continue;
    }
    match_found = true;
    for (j = 0; j < cache_spd_data.num_options[i]; j++) {
      /* if (options[j]->phase_in_degrees != */
      /* 	  cache_spd_data.ampphase_options[i][j]->phase_in_degrees) { */
      if (ampphase_options_match(options[j],
				 cache_spd_data.ampphase_options[i][j]) == false) {
	match_found = false;
	break;
      }
    }
    if (match_found) {
      /* fprintf(stderr, "[add_cache_spd_data] match found!\n"); */
      print_options_set(num_options, options, details, RPSBUFSIZE);
      angle_to_string((24.0 * data->spectrum[0][0]->ut_seconds / 86400.0),
		      time, ATS_ANGLE_IN_HOURS | ATS_STRING_IN_HOURS |
		      ATS_STRING_SEP_COLON | ATS_STRING_ZERO_PAD_FIRST, 0);
      fprintf(stderr, "[add_cache_spd_data] found SPD match for data at "
	      "time %s %s with options:\n%s", data->header_data->obsdate,
	      time, details);
      return false;
    }
  }

  // If we get here, this is new data.
  print_options_set(num_options, options, details, RPSBUFSIZE);
  angle_to_string((24.0 * data->spectrum[0][0]->ut_seconds / 86400.0),
		  time, ATS_ANGLE_IN_HOURS | ATS_STRING_IN_HOURS |
		  ATS_STRING_SEP_COLON | ATS_STRING_ZERO_PAD_FIRST, 0);
  fprintf(stderr, "[add_cache_spd_data] adding SPD cache entry for data at "
	  "time %s %s with options:\n%s", data->header_data->obsdate,
	  time, details);
  n = cache_spd_data.num_cache_spd_data + 1;
  REALLOC(cache_spd_data.num_options, n);
  REALLOC(cache_spd_data.ampphase_options, n);
  cache_spd_data.num_options[n - 1] = num_options;
  MALLOC(cache_spd_data.ampphase_options[n - 1], num_options);
  for (i = 0; i < num_options; i++) {
    MALLOC(cache_spd_data.ampphase_options[n - 1][i], 1);
    set_default_ampphase_options(cache_spd_data.ampphase_options[n - 1][i]);
    copy_ampphase_options(cache_spd_data.ampphase_options[n - 1][i], options[i]);
  }
  REALLOC(cache_spd_data.spectrum_data, n);
  MALLOC(cache_spd_data.spectrum_data[n - 1], 1);
  copy_spectrum_data(cache_spd_data.spectrum_data[n - 1], data);
  cache_spd_data.num_cache_spd_data = n;
  
  return true;
}

/*!
 *  \brief Add a vis cache entry, labelled with a provided set of options
 *  \param num_options the number of options in the set
 *  \param options the set of options with which to label the vis cache entry
 *  \param data the data to store in the cache
 *  \return an indication of whether the data was added to the cache; false
 *          if an existing entry was found with the same options set, or true
 *          if the set was added
 */
bool add_cache_vis_data(int num_options, struct ampphase_options **options,
                        struct vis_data *data) {
  int i, j, n;
  bool match_found = false;

  if (num_options == 0) {
    // What??
    return false;
  }

  // Check that we don't already know about the data.
  for (i = 0; i < cache_vis_data.num_cache_vis_data; i++) {
    if (cache_vis_data.num_options[i] != num_options) {
      // Can't be this one.
      continue;
    }
    match_found = true;
    for (j = 0; j < cache_vis_data.num_options[i]; j++) {
      if (ampphase_options_match(options[j],
				 cache_vis_data.ampphase_options[i][j]) == false) {
	// Not a perfect match.
	match_found = false;
	break;
      }
    }
    if (match_found) {
      // Don't need to add this data, we already have it.
      return false;
    }
  }

  // If we get here, this is new data.
  n = cache_vis_data.num_cache_vis_data + 1;
  REALLOC(cache_vis_data.num_options, n);
  REALLOC(cache_vis_data.ampphase_options, n);
  REALLOC(cache_vis_data.vis_data, n);
  cache_vis_data.num_options[n - 1] = num_options;
  MALLOC(cache_vis_data.ampphase_options[n - 1], num_options);
  for (i = 0; i < num_options; i++) {
    MALLOC(cache_vis_data.ampphase_options[n - 1][i], 1);
    set_default_ampphase_options(cache_vis_data.ampphase_options[n - 1][i]);
    copy_ampphase_options(cache_vis_data.ampphase_options[n - 1][i], options[i]);
  }
  MALLOC(cache_vis_data.vis_data[n - 1], 1);
  copy_vis_data(cache_vis_data.vis_data[n - 1], data);
  cache_vis_data.num_cache_vis_data = n;

  return true;
}

/*!
 *  \brief Get a SPD cache entry, labelled with a provided set of options
 *  \param num_options the number of options in the set
 *  \param options the set of options to consider while searching
 *  \param mjd the MJD of the cycle to search for
 *  \param tol the tolerance on the MJD (in days) in assessing a match
 *  \param data a pointer to a data variable which, if a match is found will
 *              be filled with the matching cache entry; if the pointer is to
 *              NULL, the pointer will be redirected to the actual cache
 *              entry, otherwise a copy of the data will be made
 *  \return an indication of whether the search found a match; true if a
 *          match was found, or false if not
 */
bool get_cache_spd_data(int num_options, struct ampphase_options **options,
                        double mjd, double tol,
                        struct spectrum_data **data) {
  int i, j;
  double tmjd;
  bool match_found = false;
  
  if (num_options == 0) {
    // No chance of a match.
    return false;
  }
  /* fprintf(stderr, "[get_cache_spd_data] looking for MJD %.8f within %.8f, %d\n", */
  /*         mjd, tol, options->phase_in_degrees); */
  for (i = 0; i < cache_spd_data.num_cache_spd_data; i++) {
    if (cache_spd_data.num_options[i] != num_options) {
      // Can't be this one.
      continue;
    }
    // Calculate MJD of this cache entry.
    tmjd = date2mjd(cache_spd_data.spectrum_data[i]->header_data->obsdate,
                    cache_spd_data.spectrum_data[i]->spectrum[0][0]->ut_seconds);
    /* fprintf(stderr, "[get_cache_spd_data] cache entry %d MJD %.8f %d %d\n", */
    /*         i, tmjd, (fabs(mjd - tmjd) <= tol), */
    /*         cache_spd_data.ampphase_options[i]->phase_in_degrees); */
    if (fabs(mjd - tmjd) > tol) {
      // Not a good enough match.
      continue;
    }
    match_found = true;
    for (j = 0; j < cache_spd_data.num_options[i]; j++) {
      if (ampphase_options_match(options[j],
				 cache_spd_data.ampphase_options[i][j]) == false) {
	// Not a perfect match.
	match_found = false;
	break;
      }
    }
    if (match_found) {
      /* fprintf(stderr, "[get_cache_spd_data] cache hit %s %.1f %d\n", */
      /*         cache_spd_data.spectrum_data[i]->header_data->obsdate, */
      /*         cache_spd_data.spectrum_data[i]->spectrum[0][0]->ut_seconds, */
      /*         cache_spd_data.ampphase_options[i]->phase_in_degrees); */
      if (*data == NULL) {
        // Occurs in the child computer usually.
        *data = cache_spd_data.spectrum_data[i];
      } else {
        copy_spectrum_data(*data, cache_spd_data.spectrum_data[i]);
      }
      return true;
    }
  }
  
  return false;
}

/*!
 *  \brief Search for a vis cache entry matching the provided set of options
 *  \param num_options the number of options in the set
 *  \param options the set of options to consider while searching
 *  \param data a pointer to a data variable which, if a match is found will
 *              be filled with the matching cache entry; if the pointer is
 *              to NULL, the pointer will be redirected to the actual cache
 *              entry, otherwise a copy of the data will be made
 *  \return an indication of whether the search found a match; true if a
 *          match was found, or false if not
 */
bool get_cache_vis_data(int num_options, struct ampphase_options **options,
                        struct vis_data **data) {
  int i, j;
  bool match_found = false;

  if (num_options == 0) {
    // No chance of a match.
    return false;
  }
  for (i = 0; i < cache_vis_data.num_cache_vis_data; i++) {
    if (cache_vis_data.num_options[i] != num_options) {
      // Can't be this one.
      continue;
    }
    match_found = true;
    for (j = 0; j < cache_vis_data.num_options[i]; j++) {
      if (ampphase_options_match(options[j],
				 cache_vis_data.ampphase_options[i][j]) == false) {
	// Not a perfect match.
	match_found = false;
	break;
      }
    }
    if (match_found) {
      if (*data == NULL) {
        // Occurs in the child computer usually.
        *data = cache_vis_data.vis_data[i];
      } else {
        copy_vis_data(*data, cache_vis_data.vis_data[i]);
      }
      return true;
    }
  }

  return false;
}

// The ways in which we can read data.
/*! \def READ_SCAN_METADATA
 *  \brief Magic number to tell data_reader that we would like to read the
 *         information about scans present in an RPFITS file
 *
 * This magic number can be combined in a bitwise-OR with COMPUTE_VIS_PRODUCTS
 * and GRAB_SPECTRUM.
 */
#define READ_SCAN_METADATA   1<<1
/*! \def COMPUTE_VIS_PRODUCTS
 *  \brief Magic number to tell data_reader that we would like to compute the
 *         products that will go to an NVIS client while reading the RPFITS file
 *
 * This magic number can be combined in a bitwise-OR with READ_SCAN_METADATA
 * and GRAB_SPECTRUM.
 */
#define COMPUTE_VIS_PRODUCTS 1<<2
/*! \def GRAB_SPECTRUM
 *  \brief Magic number to tell data_reader that we would like to compute the
 *         products that will go to an NSPD client while reading the RPFITS file
 *
 * This magic number can be combined in a bitwise-OR with READ_SCAN_METADATA
 * and COMPUTE_VIS_PRODUCTS.
 */
#define GRAB_SPECTRUM        1<<3
/*! \def GRAB_MJDS_SPECTRA
 *  \brief Magic number to tell data_reader that we would like to get several
 *         spectrum products, determined by a list of MJDs passed to the routine
 *
 * This magic number can be combined in a bitwise-OR with READ_SCAN_METADATA,
 * COMPUTE_VIS_PRODUCTS and GRAB_SPECTRUM.
 */
#define GRAB_MJDS_SPECTRA    1<<4

void data_reader(int read_type, int n_rpfits_files,
                 double mjd_required, double mjd_low, double mjd_high,
		 int num_mjds, double *mjds, int *num_options,
                 struct ampphase_options ***ampphase_options,
                 struct rpfits_file_information **info_rpfits_files,
                 struct spectrum_data **spectrum_data,
                 struct vis_data **vis_data,
		 struct spectrum_data ***spectrum_mjds) {
  int i, j, res, n, calcres, curr_header, idx_if, idx_pol, local_num_options;
  int pols[4] = { POL_XX, POL_YY, POL_XY, POL_YX }, idx_return, num_mjds_grabbed = 0;
  bool open_file, keep_reading, header_free, read_cycles, keep_cycling;
  bool cycle_free, spectrum_return, vis_cycled, cache_hit_vis_data;
  bool cache_hit_spectrum_data, nocompute, *mjds_cache_hit = NULL;
  double cycle_mjd, cycle_start, cycle_end, half_cycle;
  struct scan_header_data *sh = NULL;
  struct cycle_data *cycle_data = NULL;
  struct spectrum_data *temp_spectrum = NULL;
  struct ampphase_options **local_ampphase_options = NULL;

  if (vis_data == NULL) {
    // Resist warnings.
    fprintf(stderr, "ignore this message\n");
  }

  // Reset counters if required.
  cache_hit_vis_data = false;
  if (read_type & COMPUTE_VIS_PRODUCTS) {
    // Check whether we have a cached product for this.
    printf("[data_reader] checking for cached vis products...\n");
    /* printf("[data_reader] %s delavg %d %d\n", */
    /*        (ampphase_options->phase_in_degrees ? "degrees" : "radians"), */
    /*        ampphase_options->delay_averaging, ampphase_options->averaging_method); */

    cache_hit_vis_data = get_cache_vis_data(*num_options, *ampphase_options, vis_data);
    if (cache_hit_vis_data == false) {
      printf("[data_reader] no cache hit\n");
      if ((vis_data != NULL) && (*vis_data == NULL)) {
        // Need to allocate some memory.
        MALLOC(*vis_data, 1);
      }
      (*vis_data)->nviscycles = 0;
      (*vis_data)->mjd_low = mjd_low;
      (*vis_data)->mjd_high = mjd_high;
      (*vis_data)->header_data = NULL;
      (*vis_data)->num_ifs = NULL;
      (*vis_data)->num_pols = NULL;
      (*vis_data)->vis_quantities = NULL;
      (*vis_data)->metinfo = NULL;
      (*vis_data)->syscal_data = NULL;
      (*vis_data)->num_options = 0;
      (*vis_data)->options = NULL;
    } else {
      printf("[data_reader] found cache hit\n");
      read_type -= COMPUTE_VIS_PRODUCTS;
    }
  }

  half_cycle = -1;
  for (i = 0; i < n_rpfits_files; i++) {
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      if (info_rpfits_files[i]->n_cycles[j] > 0) {
	half_cycle = (double)info_rpfits_files[i]->scan_headers[j]->cycle_time / (2.0 * 86400.0);
	break;
      }
    }
    if (half_cycle > 0) {
      break;
    }
  }
  
  cache_hit_spectrum_data = false;
  if (read_type & GRAB_SPECTRUM) {
    // Check whether we have a cached product for this.
    //half_cycle = (double)info_rpfits_files[0]->scan_headers[0]->cycle_time / (2.0 * 86400.0);
    cache_hit_spectrum_data = get_cache_spd_data(*num_options, *ampphase_options, mjd_required,
						 half_cycle, spectrum_data);
    if (cache_hit_spectrum_data == true) {
      read_type -= GRAB_SPECTRUM;
    }
  }
  if (read_type & GRAB_MJDS_SPECTRA) {
    // Do some initialisation checks.
    if ((num_mjds <= 0) || (spectrum_mjds == NULL)) {
      // Can't do anything.
      read_type -= GRAB_MJDS_SPECTRA;
    }
  }
  if (read_type & GRAB_MJDS_SPECTRA) {
    // Are we allocating memory?
    if (*spectrum_mjds == NULL) {
      printf(" Allocating memory for %d MJD grabs\n", num_mjds);
      CALLOC(*spectrum_mjds, num_mjds);
    }
    CALLOC(mjds_cache_hit, num_mjds);
    // Check whether we have cached products for any of these MJDs.
    for (i = 0; i < num_mjds; i++) {
      mjds_cache_hit[i] = get_cache_spd_data(*num_options, *ampphase_options, mjds[i],
					     half_cycle, &((*spectrum_mjds)[i]));
      printf("  %s cache hit for MJD %.6f\n", (mjds_cache_hit[i] ? "GOT" : "NO"),
	     mjds[i]);
    }
  }

  /* printf("[data_reader] Entry to routine\n"); */
  /* print_options_set(*num_options, *ampphase_options); */
  
  for (i = 0; i < n_rpfits_files; i++) {
    // HERE WILL GO ANY CHECKS TO SEE IF WE ACTUALLY WANT TO
    // READ THIS FILE
    open_file = false;
    if (read_type & READ_SCAN_METADATA) {
      open_file = true;
    }
    // When we start storing things in memory later, we will need to alter
    // this section.
    if (read_type & GRAB_SPECTRUM) {
      // Does this file encompass the time we want?
      //half_cycle = (double)info_rpfits_files[i]->scan_headers[0]->cycle_time / (2.0 * 86400.0);
      if ((mjd_required >= (info_rpfits_files[i]->scan_start_mjd[0] - half_cycle)) &&
          (mjd_required <= (info_rpfits_files[i]->scan_end_mjd[info_rpfits_files[i]->n_scans - 1]
                            + half_cycle))) {
        open_file = true;
        /* } else { */
        /* 	printf("File %s does not cover the requested MJD %.6f\n", */
        /* 	       info_rpfits_files[i]->filename, mjd_required); */
      }
    }
    if (read_type & GRAB_MJDS_SPECTRA) {
      // Does this file encompass any of the times we want.
      for (j = 0; j < num_mjds; j++) {
	if ((mjds[j] >= (info_rpfits_files[i]->scan_start_mjd[0] - half_cycle)) &&
	    (mjds[j] <= (info_rpfits_files[i]->scan_end_mjd[info_rpfits_files[i]->n_scans - 1]
			 + half_cycle))) {
	  open_file = true;
	}
      }
      if (open_file) {
	printf(" opening file %d to grab MJDS\n", i);
      }
    }
    if (read_type & COMPUTE_VIS_PRODUCTS) {
      open_file = true;
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
        REALLOC(info_rpfits_files[i]->n_cycles, (n + 1));
        REALLOC(info_rpfits_files[i]->cycle_mjd, (n + 1));
      }
      // We always need to read the scan headers to move through
      // the file, but where we direct the information changes.
      if (!(read_type & READ_SCAN_METADATA)) {
        CALLOC(sh, 1);
        header_free = true;
      } else {
        CALLOC(info_rpfits_files[i]->scan_headers[n], 1);
        sh = info_rpfits_files[i]->scan_headers[n];
        header_free = false;
      }
      res = read_scan_header(sh);
      /* printf("[data_reader] read scan header\n"); */
      if (sh->num_sources > 0) {
        curr_header += 1;
	if (!(read_type & (READ_SCAN_METADATA)) &&
	    (curr_header >= info_rpfits_files[i]->n_scans)) {
	  // We need to do some memory freeing if required.
	  if (header_free == true) {
	    free_scan_header_data(sh);
	    FREE(sh);
	  }
	  continue;
	}
        if (read_type & READ_SCAN_METADATA) {
          // Keep track of the times covered by each scan.
          /* info_rpfits_files[i]->scan_start_mjd[n] = */
          /*   info_rpfits_files[i]->scan_end_mjd[n] = date2mjd(sh->obsdate, sh->ut_seconds); */
	  info_rpfits_files[i]->scan_start_mjd[n] =
	    info_rpfits_files[i]->scan_end_mjd[n] = 0;
          info_rpfits_files[i]->n_cycles[n] = 0;
          info_rpfits_files[i]->cycle_mjd[n] = NULL;
          info_rpfits_files[i]->n_scans += 1;
        }
        // HERE WILL GO THE LOGIC TO WORK OUT IF WE NEED TO READ
        // CYCLES FROM THIS SCAN
        read_cycles = false;
        if (read_type & READ_SCAN_METADATA) {
          // While building the index we have to look at all data.
          read_cycles = true;
        }
        if (read_type & COMPUTE_VIS_PRODUCTS) {
          // We may have to limit the data we read depending on whether
          // the MJD lies within our bounds. Low or high bounds can be disabled
          // if set to a negative number.
          if (((mjd_low < 0) || (mjd_low <= info_rpfits_files[i]->scan_end_mjd[curr_header])) &&
              ((mjd_high < 0) || (mjd_high >= info_rpfits_files[i]->scan_start_mjd[curr_header]))) {
            read_cycles = true;
          } else {
            fprintf(stderr, "[data_reader] skipping scan %d because outside of MJD range\n",
                    curr_header);
          }
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
	if (read_type & GRAB_MJDS_SPECTRA) {
	  // Check if this scan contains a cycle with an MJD we want.
	  for (j = 0; j < num_mjds; j++) {
	    if ((mjds_cache_hit[j] == false) &&
		(mjds[j] >= info_rpfits_files[i]->scan_start_mjd[curr_header]) &&
		(mjds[j] <= info_rpfits_files[i]->scan_end_mjd[curr_header])) {
	      read_cycles = true;
	    }
	  }
	  if (read_cycles) {
	    printf("  reading from scan header %d to grab MJDS\n", curr_header);
	  }
	}
        if (read_cycles && (res & READER_DATA_AVAILABLE)) {
          /* printf("[data_reader] reading cycles from this scan...\n"); */
          keep_cycling = true;
          while (keep_cycling) {
            /* fprintf(stderr, "[data_reader] preparing new cycle data...\n"); */
            cycle_data = prepare_new_cycle_data();
            /* fprintf(stderr, "[data_reader] reading cycle data...\n"); */
            res = read_cycle_data(sh, cycle_data);
            cycle_free = true;
            if (!(res & READER_DATA_AVAILABLE)) {
              keep_cycling = false;
	      if (res == READER_EXHAUSTED) {
		// No more data came from that read.
		free_cycle_data(cycle_data);
		FREE(cycle_data);
		continue;
	      }
            }
	    if (cycle_data->num_points == 0) {
	      // This isn't really usable data.
	      keep_cycling = false;
	      /* free_cycle_data(cycle_data); */
	      FREE(cycle_data);
	      continue;
	    }
	    
            // HERE WILL GO THE LOGIC TO WORK OUT IF WE WANT TO DO
            // SOMETHING WITH THIS CYCLE
            cycle_mjd = date2mjd(sh->obsdate, cycle_data->ut_seconds);
            if (read_type & READ_SCAN_METADATA) {
	      if ((cycle_mjd >= mjd_low) && (cycle_mjd <= mjd_high)) {
		// The early time of the scan is also now set.
		if (info_rpfits_files[i]->scan_start_mjd[n] == 0) {
		  info_rpfits_files[i]->scan_start_mjd[n] = cycle_mjd;
		}
		info_rpfits_files[i]->scan_end_mjd[n] = cycle_mjd;
		info_rpfits_files[i]->n_cycles[n] += 1;
		REALLOC(info_rpfits_files[i]->cycle_mjd[n], info_rpfits_files[i]->n_cycles[n]);
		info_rpfits_files[i]->cycle_mjd[n][info_rpfits_files[i]->n_cycles[n] - 1] =
		  cycle_mjd;
	      }
            }
            if ((read_type & GRAB_SPECTRUM) ||
		(read_type & GRAB_MJDS_SPECTRA) ||
                (read_type & COMPUTE_VIS_PRODUCTS)) {
              spectrum_return = false;
              if (read_type & COMPUTE_VIS_PRODUCTS) {
                nocompute = false;
              }
              // If we're trying to grab a specific spectrum,
              // we consider this the correct cycle if the requested
              // MJD is within half a cycle time of this cycle's time.
              cycle_start = cycle_mjd - ((double)sh->cycle_time / (2 * 86400.0));
              cycle_end = cycle_mjd + ((double)sh->cycle_time / (2 * 86400.0));
              // Check if we're out of bounds here.
              if ((read_type & COMPUTE_VIS_PRODUCTS) &&
                  ((mjd_low > cycle_end) || ((mjd_high > 0) && (mjd_high < cycle_start)))) {
                /* fprintf(stderr, "[data_reader] cycle skipped for COMPUTE_VIS_PRODUCTS %f\n", */
                /*         cycle_start); */
                nocompute = true;
              } 
              /* printf("%.6f / %.6f / %.6f  (%.6f)\n", cycle_start, cycle_mjd, cycle_end, */
              /* 	     mjd_required); */
              if ((read_type & GRAB_SPECTRUM) &&
                  (mjd_required >= cycle_start) &&
                  (mjd_required < cycle_end)) {
                spectrum_return = true;
              }
	      idx_return = -1;
	      if (read_type & GRAB_MJDS_SPECTRA) {
		for (j = 0; j < num_mjds; j++) {
		  if ((mjds_cache_hit[j] == false) &&
		      (mjds[j] >= cycle_start) &&
		      (mjds[j] < cycle_end)) {
		    spectrum_return = true;
		    idx_return = j;
		  }
		}
		if (spectrum_return) {
		  printf("   will return spectrum to index %d\n", idx_return);
		}
	      }
              if ((read_type & COMPUTE_VIS_PRODUCTS) ||
                  (spectrum_return)) {
                // We need this cycle.
		// Do a Tsys calculation.
		calculate_system_temperatures_cycle_data(cycle_data, sh, num_options,
							 ampphase_options);

		/* printf("[data_reader] system temperatures just calculated\n"); */
		/* print_options_set(*num_options, *ampphase_options); */

                // Allocate all the memory we need.
                /* printf("cycle found!\n"); */
                /* fprintf(stderr, "[data_reader] making a temporary spectrum...\n"); */
                MALLOC(temp_spectrum, 1);
                // Prepare the spectrum data structure.
                temp_spectrum->num_ifs = sh->num_ifs;
                temp_spectrum->header_data = info_rpfits_files[i]->scan_headers[curr_header];
                /* fprintf(stderr, "[data_reader] allocating some memory...\n"); */
                MALLOC(temp_spectrum->spectrum, temp_spectrum->num_ifs);
                if ((read_type & COMPUTE_VIS_PRODUCTS) && (nocompute == false)) {
                  REALLOC((*vis_data)->vis_quantities,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->header_data,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->num_ifs,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->num_pols,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->metinfo, ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->syscal_data, ((*vis_data)->nviscycles + 1));
                  CALLOC((*vis_data)->metinfo[(*vis_data)->nviscycles], 1);
                  //CALLOC((*vis_data)->syscal_data[(*vis_data)->nviscycles], 1);
                  (*vis_data)->num_ifs[(*vis_data)->nviscycles] = temp_spectrum->num_ifs;
                  (*vis_data)->header_data[(*vis_data)->nviscycles] =
                    info_rpfits_files[i]->scan_headers[curr_header];
                  MALLOC((*vis_data)->num_pols[(*vis_data)->nviscycles],
                         temp_spectrum->num_ifs);
                  MALLOC((*vis_data)->vis_quantities[(*vis_data)->nviscycles],
                         temp_spectrum->num_ifs);
                }
                vis_cycled = false;
                for (idx_if = 0; idx_if < temp_spectrum->num_ifs; idx_if++) {
                  temp_spectrum->num_pols = sh->if_num_stokes[idx_if];
                  CALLOC(temp_spectrum->spectrum[idx_if], temp_spectrum->num_pols);
                  if ((read_type & COMPUTE_VIS_PRODUCTS) && (nocompute == false)) {
                    (*vis_data)->num_pols[(*vis_data)->nviscycles][idx_if] =
                      temp_spectrum->num_pols;
                    CALLOC((*vis_data)->vis_quantities[(*vis_data)->nviscycles][idx_if],
                           temp_spectrum->num_pols);
                  }
                  for (idx_pol = 0; idx_pol < temp_spectrum->num_pols; idx_pol++) {
		    local_num_options = *num_options;
                    MALLOC(local_ampphase_options, local_num_options);
		    for (j = 0; j < local_num_options; j++) {
		      CALLOC(local_ampphase_options[j], 1);
		      set_default_ampphase_options(local_ampphase_options[j]);
		      copy_ampphase_options(local_ampphase_options[j], (*ampphase_options)[j]);
		    }
                    /* fprintf(stderr, "[data_reader] calculating amp products\n"); */
                    calcres = vis_ampphase(sh, cycle_data,
                                           &(temp_spectrum->spectrum[idx_if][idx_pol]),
                                           pols[idx_pol], sh->if_label[idx_if],
					   &local_num_options, &local_ampphase_options);
                    if (calcres < 0) {
                      fprintf(stderr, "CALCULATING AMP AND PHASE FAILED FOR IF %d POL %d, CODE %d\n",
                              sh->if_label[idx_if], pols[idx_pol], calcres);
                    /* } else if (read_type & COMPUTE_VIS_PRODUCTS) { */
                    /*     printf("CONVERTED SPECTRUM FOR CYCLE IF %d POL %d AT MJD %.6f\n", */
                    /*            sh->if_label[idx_if], pols[idx_pol], cycle_mjd); */
                    /* } else { */
                    /*   // TODO: perhaps make the Tsys calculations an option for */
                    /*   // when nobody cares. */
                    /*   calculate_system_temperatures(temp_spectrum->spectrum[idx_if][idx_pol], */
                    /*                                 local_ampphase_options); */
                    }
                    if ((read_type & COMPUTE_VIS_PRODUCTS) && (nocompute == false)) {
                      /* MALLOC(local_ampphase_options, 1); */
		      /* for (j = 0; j < local_num_options; j++) { */
		      /* 	free_ampphase_options(local_ampphase_options[j]); */
		      /* 	set_default_ampphase_options(local_ampphase_options[j]); */
		      /* 	copy_ampphase_options(local_ampphase_options[j], (*ampphase_options)[j]); */
		      /* } */
                      /* fprintf(stderr, "[data_reader] calculating vis products\n"); */
                      ampphase_average(sh, temp_spectrum->spectrum[idx_if][idx_pol],
                                       &((*vis_data)->vis_quantities[(*vis_data)->nviscycles][idx_if][idx_pol]),
                                       &local_num_options, &local_ampphase_options);
                      // Copy the ampphase_options back.
		      if (local_num_options > *num_options) {
			// Something got added.
			REALLOC(*ampphase_options, local_num_options);
			for (j = *num_options; j < local_num_options; j++) {
			  CALLOC((*ampphase_options)[j], 1);
			  copy_ampphase_options((*ampphase_options)[j],
						local_ampphase_options[j]);
			}
		      }
		      // And copy them to the vis data structure as well.
		      for (j = 0; j < (*vis_data)->num_options; j++) {
			free_ampphase_options((*vis_data)->options[j]);
			FREE((*vis_data)->options[j]);
		      }
		      (*vis_data)->num_options = local_num_options;
		      REALLOC((*vis_data)->options, (*vis_data)->num_options);
		      for (j = 0; j < (*vis_data)->num_options; j++) {
			CALLOC((*vis_data)->options[j], 1);
			copy_ampphase_options((*vis_data)->options[j],
					      local_ampphase_options[j]);
		      }
		      // Free the options we were sent.
		      for (j = 0; j < *num_options; j++) {
			free_ampphase_options((*ampphase_options)[j]);
		      }
		      *num_options = local_num_options;
		      for (j = 0; j < *num_options; j++) {
			copy_ampphase_options((*ampphase_options)[j], local_ampphase_options[j]);
		      }
                      /* fprintf(stderr, "[data_reader] calculated vis products\n"); */
                      vis_cycled = true;
                    }
		    // Free our local ampphase options.
		    for (j = 0; j < local_num_options; j++) {
		      free_ampphase_options(local_ampphase_options[j]);
		      FREE(local_ampphase_options[j]);
		    }
		    FREE(local_ampphase_options);
                  }
                }
                if (vis_cycled) {
                  // Compile the metinfo and calibration data.
                  copy_metinfo((*vis_data)->metinfo[(*vis_data)->nviscycles],
                               &(temp_spectrum->spectrum[0][0]->metinfo));
                  spectrum_data_compile_system_temperatures(temp_spectrum,
                                                            &((*vis_data)->syscal_data[(*vis_data)->nviscycles]));
                  (*vis_data)->nviscycles += 1;
                }
                if (spectrum_return) {
                  // Copy the pointer.
		  if (read_type & GRAB_SPECTRUM) {
		    *spectrum_data = temp_spectrum;
		  }
		  if ((read_type & GRAB_MJDS_SPECTRA) &&
		      (idx_return >= 0)) {
		    (*spectrum_mjds)[idx_return] = temp_spectrum;
		    num_mjds_grabbed++;
		  }
                } else {
                  // Free the temporary spectrum memory.
                  for (idx_if = 0; idx_if < temp_spectrum->num_ifs; idx_if++) {
                    for (idx_pol = 0; idx_pol < temp_spectrum->num_pols; idx_pol++) {
                      free_ampphase(&(temp_spectrum->spectrum[idx_if][idx_pol]));
                    }
                    FREE(temp_spectrum->spectrum[idx_if]);
                  }
                  FREE(temp_spectrum->spectrum);
                  FREE(temp_spectrum);
                }
                //if (!(read_type & COMPUTE_VIS_PRODUCTS)) {
		if (((read_type & GRAB_SPECTRUM) && !(read_type & COMPUTE_VIS_PRODUCTS)) ||
		    ((read_type & GRAB_MJDS_SPECTRA) &&
		     (num_mjds_grabbed == num_mjds))) {
                  // We don't need to search any more.
                  fprintf(stderr, "  NO MORE SEARCHING!\n");
                  keep_cycling = false;
                  keep_reading = false;
                }
              }
            }
            // Do we need to free this memory now?
            if (cycle_free) {
              free_cycle_data(cycle_data);
              FREE(cycle_data);
            }
          }
        }
      } else {
        header_free = true;
      }
      if (res == READER_EXHAUSTED) {
        keep_reading = false;
      }

      // Discard this "scan" if it didn't contain cycles.
      if ((read_type & READ_SCAN_METADATA) &&
	  (info_rpfits_files[i]->n_cycles[n] == 0)) {
	// We also need to deallocate the scan header.
	header_free = true;
	// Change the size of the arrays.
	if (n > 0) {
	  // Don't change the size of the arrays if this cycle is the
	  // first one!
	  REALLOC(info_rpfits_files[i]->scan_headers, n);
	  REALLOC(info_rpfits_files[i]->scan_start_mjd, n);
	  REALLOC(info_rpfits_files[i]->scan_end_mjd, n);
	  REALLOC(info_rpfits_files[i]->n_cycles, n);
	  REALLOC(info_rpfits_files[i]->cycle_mjd, n);
	  info_rpfits_files[i]->n_scans = n;
	}
      }

      if (header_free) {
        free_scan_header_data(sh);
        FREE(sh);
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

  if (read_type & GRAB_MJDS_SPECTRA) {
    FREE(mjds_cache_hit);
  }

  
}

/*! \def RPSENDBUFSIZE
 *  \brief A reasonable size to accommodate any possible send buffer,
 *         currently set to 100 MiB.
 */
#define RPSENDBUFSIZE 104857600

bool sigint_received;
bool sigpipe_received;

// Handle signals that we receive.
/*!
 *  \brief Our signal handler
 *  \param sig the signal being handled
 *
 * This handler handles the signals:
 * - SIGINT: when received, the global variable `sigint_received` is set to
 *   true
 * - SIGPIPE: when received, the global variable `sigpipe_received` is set to
 *   true
 */
static void rpfitsfile_server_sighandler(int sig) {
  if (sig == SIGINT) {
    sigint_received = true;
  }
  if (sig == SIGPIPE) {
    sigpipe_received = true;
  }
}

/*!
 *  \brief Associate some vis data to an individual client
 *  \param client_vis_data the cache structure
 *  \param client_id the client ID
 *  \param vis_data the vis data to cache, which will be copied
 */
void add_client_vis_data(struct client_vis_data *client_vis_data,
                         char *client_id, struct vis_data *vis_data) {
  int i, n;

  // This is the position to add this client data, by default at the end.
  n = client_vis_data->num_clients;
  
  // Check first to see if this client ID is already present.
  for (i = 0; i < client_vis_data->num_clients; i++) {
    if (strncmp(client_vis_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      // We will replace this data.
      n = i;
      /* fprintf(stderr, "[add_client_vis_data] client data %s will be replaced at %d\n", */
      /*         client_id, n); */
      break;
    }
  }
  
  // Store some vis data as identified by a client id.
  if (n >= client_vis_data->num_clients) {
    /* fprintf(stderr, "[add_client_vis_data] making new client cache %s\n", */
    /*         client_id); */
    REALLOC(client_vis_data->client_id, (n + 1));
    MALLOC(client_vis_data->client_id[n], CLIENTIDLENGTH);
    REALLOC(client_vis_data->vis_data, (n + 1));
    CALLOC(client_vis_data->vis_data[n], 1);
    client_vis_data->num_clients = (n + 1);
  }
  strncpy(client_vis_data->client_id[n], client_id, CLIENTIDLENGTH);
  copy_vis_data(client_vis_data->vis_data[n], vis_data);
}

/*!
 *  \brief Remove vis data associated with a client
 *  \param client_vis_data the cache structure
 *  \param client_id the client ID whose data should be cleared
 *  \return true if something was removed, false otherwise
 */
bool remove_client_vis_data(struct client_vis_data *client_vis_data,
			    char *client_id) {
  int i, cidx;

  for (i = 0, cidx = -1; i < client_vis_data->num_clients; i++) {
    if (strncmp(client_vis_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      cidx = i;
      break;
    }
  }

  if (cidx == -1) {
    // Nothing found.
    return (false);
  }
  // Free the vis_data memory for the client.
  //free_vis_data(client_vis_data->vis_data[cidx]);
  // And the pointer memory too.
  FREE(client_vis_data->vis_data[cidx]);
  if (cidx < (client_vis_data->num_clients - 1)) {
    // We will have to shift data down.
    for (i = (cidx + 1); i < client_vis_data->num_clients; i++) {
      strncpy(client_vis_data->client_id[i - 1],
	      client_vis_data->client_id[i], CLIENTIDLENGTH);
      // Redirect the pointer.
      client_vis_data->vis_data[i - 1] = client_vis_data->vis_data[i];
    }
  }
  // Free the memory of the last string.
  FREE(client_vis_data->client_id[client_vis_data->num_clients - 1]);
  // Reset our counter.
  client_vis_data->num_clients -= 1;
  if (client_vis_data->num_clients > 0) {
    // Reallocate the arrays.
    REALLOC(client_vis_data->client_id, client_vis_data->num_clients);
    REALLOC(client_vis_data->vis_data, client_vis_data->num_clients);
  } else {
    // No more clients, free everything.
    FREE(client_vis_data->client_id);
    FREE(client_vis_data->vis_data);
  }

  return (true);
}

/*!
 *  \brief Associate some SPD data to an individual client
 *  \param client_spd_data the cache structure
 *  \param client_id the client ID
 *  \param spectrum_data the SPD data to cache, which will be copied
 */
void add_client_spd_data(struct client_spd_data *client_spd_data,
			 char *client_id, struct spectrum_data *spectrum_data) {
  int i, n;

  // This is the positions to add this client data, by default at the end,
  n = client_spd_data->num_clients;

  // Check first to see if this client ID is already present.
  for (i = 0; i < client_spd_data->num_clients; i++) {
    if (strncmp(client_spd_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      // We will replace this data.
      n = i;
      fprintf(stderr, "[add_client_spd_data] client data %s will be replaced at %d\n",
	      client_id, n);
      break;
    }
  }

  // Store some spd data as identified by a client id.
  if (n >= client_spd_data->num_clients) {
    fprintf(stderr, "[add_client_spd_data] making new client cache %s\n",
	    client_id);
    REALLOC(client_spd_data->client_id, (n + 1));
    MALLOC(client_spd_data->client_id[n], CLIENTIDLENGTH);
    REALLOC(client_spd_data->spectrum_data, (n + 1));
    MALLOC(client_spd_data->spectrum_data[n], 1);
    client_spd_data->num_clients = (n + 1);
  }
  strncpy(client_spd_data->client_id[n], client_id, CLIENTIDLENGTH);
  copy_spectrum_data(client_spd_data->spectrum_data[n], spectrum_data);
}

/*!
 *  \brief Remove SPD data associated with a client
 *  \param client_spd_data the cache data structure
 *  \param client_id the client ID whose data should be cleared
 *  \return true if something was removed, false otherwise
 */
bool remove_client_spd_data(struct client_spd_data *client_spd_data,
			    char *client_id) {
  int i, cidx;

  for (i = 0, cidx = -1; i < client_spd_data->num_clients; i++) {
    if (strncmp(client_spd_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      cidx = i;
      break;
    }
  }

  if (cidx == -1) {
    // Nothing found.
    return (false);
  }
  // Free the spectrum_data memory for the client.
  //free_spectrum_data(client_spd_data->spectrum_data[cidx]);
  FREE(client_spd_data->spectrum_data[cidx]);
  if (cidx < (client_spd_data->num_clients - 1)) {
    // We will have to shift data down.
    for (i = (cidx + 1); i < client_spd_data->num_clients; i++) {
      strncpy(client_spd_data->client_id[i - 1],
	      client_spd_data->client_id[i], CLIENTIDLENGTH);
      // Redirect the pointer.
      client_spd_data->spectrum_data[i - 1] = client_spd_data->spectrum_data[i];
    }
  }
  // Free the memory of the last string.
  FREE(client_spd_data->client_id[client_spd_data->num_clients - 1]);
  // Reset our counter.
  client_spd_data->num_clients -= 1;
  if (client_spd_data->num_clients > 0) {
    // Reallocate the arrays.
    REALLOC(client_spd_data->client_id, client_spd_data->num_clients);
    REALLOC(client_spd_data->spectrum_data, client_spd_data->num_clients);
  } else {
    // No more clients, free everything.
    FREE(client_spd_data->client_id);
    FREE(client_spd_data->spectrum_data);
  }

  return (true);
}

/*!
 *  \brief Add a set of options from a client into a cache
 *  \param client_ampphase_options the cache structure that holds information about
 *                                 multiple clients
 *  \param client_id the ID of the client to associate with the supplied options;
 *                   only a single set of options may be cached against a single ID
 *  \param client_username the username of the client to associate with the supplied
 *                         options; only a single set of options may be cached against
 *                         a single username
 *  \param n_ampphase_options the number of options structures in the set
 *  \param ampphase_options the set of options to store
 *
 * The ampphase_options cache exists so that if a user (as identified by a client
 * username) is controlling multiple clients, but would like any changes they make
 * to the options on one client to be reflected in the others, this can happen
 * seamlessly.
 */
void add_client_ampphase_options(struct client_ampphase_options *client_ampphase_options,
				 char *client_id, char *client_username,
				 int n_ampphase_options,
				 struct ampphase_options **ampphase_options) {
  int i, j, n;
  // This is the position to add this client data, by default at the end.
  n = client_ampphase_options->num_clients;

  // Check first to see if this client ID or username is already present.
  for (i = 0; i < client_ampphase_options->num_clients; i++) {
    if ((strncmp(client_ampphase_options->client_id[i], client_id, CLIENTIDLENGTH) == 0) ||
	((strlen(client_ampphase_options->client_username[i]) > 0) &&
	 (strlen(client_username) > 0) &&
	 (strncmp(client_ampphase_options->client_username[i], client_username,
		  CLIENTIDLENGTH) == 0))) {
      // We will replace this data.
      n = i;
      break;
    }
  }

  // Store the ampphase_options.
  if (n >= client_ampphase_options->num_clients) {
    REALLOC(client_ampphase_options->client_id, (n + 1));
    REALLOC(client_ampphase_options->client_username, (n + 1));
    REALLOC(client_ampphase_options->n_ampphase_options, (n + 1));
    REALLOC(client_ampphase_options->ampphase_options, (n + 1));
    for (j = client_ampphase_options->num_clients; j < (n + 1); j++) {
      CALLOC(client_ampphase_options->client_id[j], CLIENTIDLENGTH);
      CALLOC(client_ampphase_options->client_username[j], CLIENTIDLENGTH);
      client_ampphase_options->n_ampphase_options[j] = 0;
      client_ampphase_options->ampphase_options[j] = NULL;
    }
    client_ampphase_options->num_clients = (n + 1);
  }
  // Clear memory if we're overwriting something.
  for (i = 0; i < client_ampphase_options->n_ampphase_options[n]; i++) {
    free_ampphase_options(client_ampphase_options->ampphase_options[n][i]);
    FREE(client_ampphase_options->ampphase_options[n][i]);
  }
  FREE(client_ampphase_options->ampphase_options[n]);
  // Store the metadata.
  strncpy(client_ampphase_options->client_id[n], client_id, CLIENTIDLENGTH);
  strncpy(client_ampphase_options->client_username[n], client_username, CLIENTIDLENGTH);
  client_ampphase_options->n_ampphase_options[n] = n_ampphase_options;
  CALLOC(client_ampphase_options->ampphase_options[n], n_ampphase_options);
  for (i = 0; i < client_ampphase_options->n_ampphase_options[n]; i++) {
    CALLOC(client_ampphase_options->ampphase_options[n][i], 1);
    copy_ampphase_options(client_ampphase_options->ampphase_options[n][i],
			  ampphase_options[i]);
  }
}

bool get_client_ampphase_options(struct client_ampphase_options *client_ampphase_options,
				 char *client_id, char *client_username,
				 int *n_ampphase_options,
				 struct ampphase_options ***ampphase_options) {
  int i, idx = -1;

  // Find the correct client ID or username.
  if (strlen(client_username) > 0) {
    // We do a search by username.
    for (i = 0; i < client_ampphase_options->num_clients; i++) {
      if ((strlen(client_ampphase_options->client_username[i]) > 0) &&
	  (strncmp(client_ampphase_options->client_username[i], client_username,
		   CLIENTIDLENGTH) == 0)) {
	// Found it.
	idx = i;
	break;
      }
    }
  }
  if ((strlen(client_username) == 0) || // If the username wasn't specified, or
      (idx < 0)) { // if the username wasn't found in the previous search...
    // Look for the client ID.
    for (i = 0; i < client_ampphase_options->num_clients; i++) {
      if (strncmp(client_ampphase_options->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
	idx = i;
	break;
      }
    }
  }

  if (idx < 0) {
    // No match.
    return false;
  }

  // Copy the options.
  *n_ampphase_options = client_ampphase_options->n_ampphase_options[idx];
  MALLOC(*ampphase_options, *n_ampphase_options);
  for (i = 0; i < *n_ampphase_options; i++) {
    CALLOC((*ampphase_options)[i], 1);
    copy_ampphase_options((*ampphase_options)[i],
			  client_ampphase_options->ampphase_options[idx][i]);
  }
  return true;
}


struct vis_data* get_client_vis_data(struct client_vis_data *client_vis_data,
                                     char *client_id) {
  // Return vis data associated with the specified client_id, or
  // the DEFAULT otherwise.
  struct vis_data *default_vis_data = NULL;
  int i;
  for (i = 0; i < client_vis_data->num_clients; i++) {
    if (strncmp(client_vis_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      fprintf(stderr, "[get_client_vis_data] found vis data for %s at %d\n",
              client_id, i);
      return(client_vis_data->vis_data[i]);
    } else if (strncmp(client_vis_data->client_id[i], "DEFAULT", 7) == 0) {
      default_vis_data = client_vis_data->vis_data[i];
    }
  }
  fprintf(stderr, "[get_client_vis_data] returning default data\n");
  return(default_vis_data);
}

struct spectrum_data* get_client_spd_data(struct client_spd_data *client_spd_data,
                                          char *client_id) {
  // Return spd data associated with the specified client_id, or
  // the DEFAULT otherwise.
  struct spectrum_data *default_spectrum_data = NULL;
  int i;
  for (i = 0; i < client_spd_data->num_clients; i++) {
    if (strncmp(client_spd_data->client_id[i], client_id, CLIENTIDLENGTH) == 0) {
      fprintf(stderr, "[get_client_spd_data] found spectrum data for %s at %d\n",
	      client_id, i);
      return(client_spd_data->spectrum_data[i]);
    } else if (strncmp(client_spd_data->client_id[i], "DEFAULT", 7) == 0) {
      default_spectrum_data = client_spd_data->spectrum_data[i];
    }
  }
  fprintf(stderr, "[get_client_spd_data] returning default data\n");
  return(default_spectrum_data);
}

int main(int argc, char *argv[]) {
  struct rpfitsfile_server_arguments arguments;
  int i, j, k, l, ri, rj, bytes_received, r, n_cycle_mjd = 0, n_client_options = 0;
  int n_alert_sockets = 0, n_ampphase_options = 0, *client_indices = NULL;
  int removed_client_type, total_n_scans = 0, loop_limit, n_acal_cycles = 0;
  int acal_window, acal_model_num_terms = 0, n_acal_fluxdensities = 0;
  int n_copied_options = 0, acal_options_idx;
  bool pointer_found = false, vis_cache_updated = false, notify_required = false;
  bool spd_cache_updated = false, outside_mjd_range = false, succ = false;
  bool client_added = false, determine_params = false;
  bool quit_when_closed = false, acal_source_recognised = false, acal_model_log = false;
  float acal_fd, *acal_model_terms = NULL, *acal_fluxdensities = NULL;
  double mjd_grab, earliest_mjd, latest_mjd, mjd_cycletime;
  double *all_cycle_mjd = NULL, *acal_cycle_mjds = NULL;
  struct rpfits_file_information **info_rpfits_files = NULL;
  struct ampphase_options **ampphase_options = NULL, **client_options = NULL;
  struct ampphase_options *spectrum_options = NULL, **copied_options = NULL;
  struct spectrum_data *spectrum_data = NULL, *child_spectrum_data = NULL;
  struct spectrum_data **acal_spectra = NULL;
  struct vis_data *vis_data = NULL, *child_vis_data = NULL;
  FILE *fh = NULL;
  cmp_ctx_t cmp, child_cmp;
  cmp_mem_access_t mem, child_mem;
  struct addrinfo hints, *bind_address, *addrinfo = NULL;
  char port_string[RPSBUFSIZE], address_buffer[RPSBUFSIZE], clienttype_string[RPSBUFSIZE];
  char *recv_buffer = NULL, *send_buffer = NULL, *child_send_buffer = NULL;
  char removed_id[CLIENTIDLENGTH], removed_username[CLIENTIDLENGTH];
  char *acal_source = NULL;
  SOCKET socket_listen, max_socket, loop_i, socket_client, child_socket;
  SOCKET *alert_socket = NULL;
  fd_set master, reads;
  struct sockaddr_storage client_address;
  socklen_t client_len;
  struct requests client_request, child_request;
  struct responses client_response;
  size_t recv_buffer_length, comp_buffer_length;
  ssize_t bytes_sent;
  struct client_vis_data client_vis_data;
  struct client_spd_data client_spd_data;
  pid_t pid;
  struct client_sockets clients;
  struct client_ampphase_options client_ampphase_options;
  struct file_instructions *testing_instructions = NULL, *file_instructions_ptr = NULL;
  struct fluxdensity_specification fd_spec;
  struct ampphase_modifiers *fd_modifier = NULL;
  
  // Set the defaults for the arguments.
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;
  arguments.port_number = 8880;
  arguments.network_operation = false;
  arguments.testing_operation = false;
  arguments.num_instruction_files = 0;
  arguments.testing_instruction_files = NULL;
  arguments.minimum_read_mjd = -INFINITY;
  arguments.maximum_read_mjd = INFINITY;
  
  // And the default for the calculator options.
  /* MALLOC(ampphase_options, 1); */
  /* set_default_ampphase_options(ampphase_options); */
  n_ampphase_options = 0;
  ampphase_options = NULL;
  
  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Stop here if we don't have any RPFITS files.
  if (arguments.n_rpfits_files == 0) {
    fprintf(stderr, "NO RPFITS FILES SPECIFIED, EXITING\n");
    return(-1);
  }
  // Check if our data limits make sense.
  if (arguments.minimum_read_mjd >= arguments.maximum_read_mjd) {
    fprintf(stderr, "SPECIFIED MJD RANGE IS INVALID, EXITING\n");
    return(-1);
  }

  // Reset our cache.
  cache_vis_data.num_cache_vis_data = 0;
  cache_vis_data.num_options = NULL;
  cache_vis_data.ampphase_options = NULL;
  cache_vis_data.vis_data = NULL;
  cache_spd_data.num_cache_spd_data = 0;
  cache_spd_data.num_options = NULL;
  cache_spd_data.ampphase_options = NULL;
  cache_spd_data.spectrum_data = NULL;

  // And initialise the clients.
  client_vis_data.num_clients = 0;
  client_vis_data.client_id = NULL;
  client_vis_data.vis_data = NULL;
  client_spd_data.num_clients = 0;
  client_spd_data.client_id = NULL;
  client_spd_data.spectrum_data = NULL;
  clients.num_sockets = 0;
  clients.socket = NULL;
  clients.client_id = NULL;
  clients.client_username = NULL;
  clients.client_type = NULL;
  client_ampphase_options.num_clients = 0;
  client_ampphase_options.client_id = NULL;
  client_ampphase_options.client_username = NULL;
  client_ampphase_options.n_ampphase_options = NULL;
  client_ampphase_options.ampphase_options = NULL;
  
  // Set up our signal handler.
  signal(SIGINT, rpfitsfile_server_sighandler);
  signal(SIGPIPE, rpfitsfile_server_sighandler);
  // Ignore the deaths of our children (they won't zombify).
  signal(SIGCHLD, SIG_IGN);

  // If we have instructions to read, we do it here so we can add to the
  // list of RPFITS files.
  if (arguments.testing_operation == true) {
    for (i = 0; i < arguments.num_instruction_files; i++) {
      read_instruction_file(arguments.testing_instruction_files[i],
                            &testing_instructions);
    }

    // Now go through the linked list and add the files.
    file_instructions_ptr = testing_instructions;
    while (file_instructions_ptr) {
      for (i = 0; i < file_instructions_ptr->num_files; i++) {
        REALLOC(arguments.rpfits_files, ++(arguments.n_rpfits_files));
        arguments.rpfits_files[arguments.n_rpfits_files - 1] =
          file_instructions_ptr->files[i];
      }
      file_instructions_ptr = file_instructions_ptr->next;
    }
  }
  
  // Do a scan of each RPFITS file to get time information.
  MALLOC(info_rpfits_files, arguments.n_rpfits_files);
  // Put the names of the RPFITS files into the info structure.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    info_rpfits_files[i] = new_rpfits_file();
    strncpy(info_rpfits_files[i]->filename, arguments.rpfits_files[i], RPSBUFSIZE);
  }
  data_reader(READ_SCAN_METADATA, arguments.n_rpfits_files, 0.0,
	      arguments.minimum_read_mjd, arguments.maximum_read_mjd, 0, NULL,
	      &n_ampphase_options, &ampphase_options,
              info_rpfits_files, &spectrum_data, &vis_data, NULL);
  // We can now work out which time range we cover and the cycle time.
  for (i = 0, determine_params = true; i < arguments.n_rpfits_files; i++) {
    total_n_scans += info_rpfits_files[i]->n_scans;
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      if (determine_params) {
	if (info_rpfits_files[i]->n_cycles[j] > 0) {
	  mjd_cycletime = (double)info_rpfits_files[i]->scan_headers[j]->cycle_time / 86400.0;
	  earliest_mjd = info_rpfits_files[i]->scan_start_mjd[j] - (mjd_cycletime / 2.0);
	  latest_mjd = info_rpfits_files[i]->scan_end_mjd[j] + (mjd_cycletime / 2.0);
	  determine_params = false;
	}
      } else {
        MINASSIGN(earliest_mjd, info_rpfits_files[i]->scan_start_mjd[j] - (mjd_cycletime / 2.0));
        MAXASSIGN(latest_mjd, info_rpfits_files[i]->scan_end_mjd[j] + (mjd_cycletime / 2.0));
      }
    }
  }

  if (total_n_scans <= 0) {
    fprintf(stderr, " FATAL: NO USABLE SCANS FOUND IN RPFITS FILES!\n");
    exit(1);
  }
  
  // Print out the summary.
  n_cycle_mjd = 0;
  all_cycle_mjd = NULL;
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    printf("RPFITS FILE: %s (%d scans):\n",
           info_rpfits_files[i]->filename, info_rpfits_files[i]->n_scans);
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      if (info_rpfits_files[i]->n_cycles[j] > 0) {
	printf("  scan %d (%s, %s) MJD range %.6f -> %.6f (%d c)\n", (j + 1),
	       info_rpfits_files[i]->scan_headers[j]->source_name[0],
	       info_rpfits_files[i]->scan_headers[j]->obstype,
	       info_rpfits_files[i]->scan_start_mjd[j],
	       info_rpfits_files[i]->scan_end_mjd[j],
	       info_rpfits_files[i]->n_cycles[j]);
	for (k = 0; k < info_rpfits_files[i]->n_cycles[j]; k++) {
	  n_cycle_mjd++;
	  REALLOC(all_cycle_mjd, n_cycle_mjd);
	  all_cycle_mjd[n_cycle_mjd - 1] = info_rpfits_files[i]->cycle_mjd[j][k];
	}
      	/* printf("    cycle %d: MJD %.8f\n", (k + 1), info_rpfits_files[i]->cycle_mjd[j][k]); */
      }
    }
    printf("\n");
  }

  // Let's try to load one of the spectra at startup, so we can send
  // something if required to.
  printf("Preparing for operation...\n");
  srand(time(NULL));
  mjd_grab = -1;
  loop_limit = 0;
  while ((mjd_grab < 0) && (loop_limit < 10)) {
    ri = rand() % arguments.n_rpfits_files;
    rj = rand() % info_rpfits_files[ri]->n_scans;
    if (info_rpfits_files[ri]->n_cycles[rj] > 0) {
      if ((info_rpfits_files[ri]->scan_start_mjd[rj] >= arguments.minimum_read_mjd) &&
	  (info_rpfits_files[ri]->scan_end_mjd[rj] <= arguments.maximum_read_mjd)) {
	mjd_grab = info_rpfits_files[ri]->scan_start_mjd[rj] + 10.0 / 86400.0;
      } else if ((info_rpfits_files[ri]->scan_start_mjd[rj] < arguments.minimum_read_mjd) &&
		 (info_rpfits_files[ri]->scan_end_mjd[rj] > arguments.minimum_read_mjd)) {
	mjd_grab = arguments.minimum_read_mjd;
      }
    }
    loop_limit++;
  }
  if (mjd_grab < 0) {
    printf(" Unable to find a usable random grab of data to begin!\n");
    exit(1);
  }
  printf(" grabbing from random scan %d from file %d, MJD %.6f\n", rj, ri, mjd_grab);
  data_reader(GRAB_SPECTRUM | COMPUTE_VIS_PRODUCTS, arguments.n_rpfits_files, mjd_grab,
	      arguments.minimum_read_mjd, arguments.maximum_read_mjd,
	      0, NULL, &n_ampphase_options, &ampphase_options, info_rpfits_files, &spectrum_data,
	      &vis_data, NULL);
  // This first grab of the vis_data goes into the default client slot.
  add_client_vis_data(&client_vis_data, "DEFAULT", vis_data);
  vis_cache_updated = add_cache_vis_data(n_ampphase_options, ampphase_options, vis_data);
  // Same with the spectrum_data.
  add_client_spd_data(&client_spd_data, "DEFAULT", spectrum_data);
  spd_cache_updated = add_cache_spd_data(n_ampphase_options, ampphase_options, spectrum_data);
  // And these are now the default ampphase options.
  add_client_ampphase_options(&client_ampphase_options, "DEFAULT", "",
			      n_ampphase_options, ampphase_options);
  
  // Now go through any testing data specifications and store those.
  
  
  if (!arguments.network_operation) {
    // Save these spectra to a file after packing it.
    printf("Writing SPD data output file...\n");
    fh = fopen("test_spd.dat", "wb");
    if (fh == NULL) {
      error_and_exit("Error opening spd output file");
    }
    cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
    pack_spectrum_data(&cmp, spectrum_data);
    fclose(fh);

    // Save the vis data to a file after packing it.
    fh = fopen("test_vis.dat", "wb");
    if (fh == NULL) {
      error_and_exit("Error opening vis output file");
    }
    cmp_init(&cmp, fh, file_reader, file_skipper, file_writer);
    pack_vis_data(&cmp, vis_data);
    fclose(fh);
  } else {

    printf("Configuring network server...\n");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    snprintf(port_string, RPSBUFSIZE, "%d", arguments.port_number);
    getaddrinfo(0, port_string, &hints, &bind_address);

    printf("Creating socket...\n");
    socket_listen = socket(bind_address->ai_family,
                           bind_address->ai_socktype,
                           bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
      fprintf(stderr, "socket() failed (%d)\n", GETSOCKETERRNO());
      return(1);
    }

    printf("Binding sockets...\n");
    addrinfo = bind_address;
    while (addrinfo != NULL) {
      printf("   to %s\n", addrinfo->ai_canonname);
      if (bind(socket_listen, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
	fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
	return(1);
      }
      addrinfo = addrinfo->ai_next;
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
      fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
      return(1);
    }

    // Enter our main loop.
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    max_socket = socket_listen;

    printf("Waiting for connections...\n");
    while (true) {
      // We wait for someone to ask for something.
      reads = master;

      if (quit_when_closed && (clients.num_sockets == 0)) {
	// All the clients have closed, and we want to shutdown, so we quit now.
	break;
      }

      r = select(max_socket + 1, &reads, 0, 0, 0);
      if ((r < 0) && (errno != EINTR)) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        break;
      }
      // Check we got a valid selection.
      if (sigint_received == true) {
	// We're quitting, user has ctrl-c-ed us.
	// Disconnect any clients that are still connected.
	for (i = 0; i < clients.num_sockets; i++) {
	  client_type_string(clients.client_type[i], clienttype_string);
	  client_response.response_type = RESPONSE_SHUTDOWN;
	  strncpy(client_response.client_id, clients.client_id[i], CLIENTIDLENGTH);
	  printf(" %s to %s client %s (%s)\n",
		 get_type_string(TYPE_RESPONSE, client_response.response_type),
		 clienttype_string, clients.client_id[i], clients.client_username[i]);
	  MALLOC(send_buffer, JUSTRESPONSESIZE);
	  init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
	  pack_responses(&cmp, &client_response);
	  bytes_sent = socket_send_buffer(clients.socket[i], send_buffer,
					  cmp_mem_access_get_pos(&mem));
	  FREE(send_buffer);
	  /* FD_CLR(clients.socket[i], &master); */
	  /* CLOSESOCKET(clients.socket[i]); */
	}
        sigint_received = false;
	quit_when_closed = true;
      }

      
      if (r < 0) {
        // Nothing got selected.
        continue;
      }
      
      for (loop_i = 1; loop_i <= max_socket; ++loop_i) {
        if (FD_ISSET(loop_i, &reads)) {
          // Handle this request.
          if (loop_i == socket_listen) {
            client_len = sizeof(client_address);
            socket_client = accept(socket_listen,
                                   (struct sockaddr*)&client_address,
                                   &client_len);
            if (!ISVALIDSOCKET(socket_client)) {
              fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
              continue;
            }

            FD_SET(socket_client, &master);
            MAXASSIGN(max_socket, socket_client);

            getnameinfo((struct sockaddr*)&client_address, client_len,
                        address_buffer, sizeof(address_buffer), 0, 0,
                        NI_NUMERICHOST);
            printf("New connection from %s\n", address_buffer);
          } else {
            // Get the requests structure.
            bytes_received = socket_recv_buffer(loop_i, &recv_buffer, &recv_buffer_length);
            if (bytes_received < 1) {
              // The connection failed. Delete the client from our list and clean up.
	      remove_client(&clients, loop_i, removed_id, removed_username,
			    &removed_client_type);
              FD_CLR(loop_i, &master);
              CLOSESOCKET(loop_i);
              FREE(recv_buffer);
	      client_type_string(removed_client_type, clienttype_string);
              printf("Closing connection to %s client %s (%s), no data received.\n",
		     clienttype_string, removed_id, removed_username);

	      // Remove any client-specific cache data.
	      if (removed_client_type == CLIENTTYPE_NVIS) {
		remove_client_vis_data(&client_vis_data, removed_id);
	      } else if (removed_client_type == CLIENTTYPE_NSPD) {
		remove_client_spd_data(&client_spd_data, removed_id);
	      }
              continue;
            }
            printf("Received %d bytes.\n", bytes_received);
            init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
            unpack_requests(&cmp, &client_request);

            // Do what we've been asked to do.
	    client_type_string(client_request.client_type, clienttype_string);
            printf(" %s from %s client %s.\n",
                   get_type_string(TYPE_REQUEST, client_request.request_type),
                   clienttype_string, client_request.client_id);
            // Add this client to our list.
            client_added = add_client(&clients, client_request.client_id,
				      client_request.client_username,
				      client_request.client_type, loop_i);
            if ((client_request.request_type == REQUEST_CURRENT_SPECTRUM) ||
                (client_request.request_type == REQUEST_MJD_SPECTRUM) ||
                (client_request.request_type == REQUEST_CURRENT_VISDATA) ||
                (client_request.request_type == REQUEST_COMPUTED_VISDATA)) {
              // We're going to send the currently cached data to this socket.
              // Make the buffers the size we may need.
              CALLOC(send_buffer, RPSENDBUFSIZE);
              
              // Set up the response.
              if (client_request.request_type == REQUEST_CURRENT_SPECTRUM) {
                client_response.response_type = RESPONSE_CURRENT_SPECTRUM;
              } else if (client_request.request_type == REQUEST_MJD_SPECTRUM) {
                client_response.response_type = RESPONSE_LOADED_SPECTRUM;
              } else if (client_request.request_type == REQUEST_CURRENT_VISDATA) {
                client_response.response_type = RESPONSE_CURRENT_VISDATA;
              } else if (client_request.request_type == REQUEST_COMPUTED_VISDATA) {
                client_response.response_type = RESPONSE_COMPUTED_VISDATA;
              }
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);

              // Now move to writing to the send buffer.
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)RPSENDBUFSIZE);
              pack_responses(&cmp, &client_response);
	      // Return the ampphase options used here.
	      if ((client_request.request_type == REQUEST_CURRENT_SPECTRUM) ||
		  (client_request.request_type == REQUEST_CURRENT_VISDATA)) {
		get_client_ampphase_options(&client_ampphase_options, "DEFAULT", "",
					    &n_client_options, &client_options);
	      } else if ((client_request.request_type == REQUEST_MJD_SPECTRUM) ||
			 (client_request.request_type == REQUEST_COMPUTED_VISDATA)) {
		succ = get_client_ampphase_options(&client_ampphase_options,
						   client_request.client_id,
						   client_request.client_username,
						   &n_client_options, &client_options);
		if (succ == false) {
		  // Revert to defaults.
		  get_client_ampphase_options(&client_ampphase_options, "DEFAULT", "",
					      &n_client_options, &client_options);
		  printf("DDDD Using DEFAULT options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		} else {
		  printf("DDDD Using cached options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		}
	      }
	      pack_write_sint(&cmp, n_client_options);
	      for (i = 0; i < n_client_options; i++) {
		pack_ampphase_options(&cmp, client_options[i]);
	      }
              if (client_request.request_type == REQUEST_CURRENT_SPECTRUM) {
                pack_spectrum_data(&cmp, get_client_spd_data(&client_spd_data, "DEFAULT"));
              } else if (client_request.request_type == REQUEST_MJD_SPECTRUM) {
                pack_spectrum_data(&cmp, get_client_spd_data(&client_spd_data,
                                                             client_request.client_id));
              } else if (client_request.request_type == REQUEST_CURRENT_VISDATA) {
                pack_vis_data(&cmp, get_client_vis_data(&client_vis_data, "DEFAULT"));
              } else if (client_request.request_type == REQUEST_COMPUTED_VISDATA) {
                pack_vis_data(&cmp, get_client_vis_data(&client_vis_data,
                                                        client_request.client_id));
              }
              
              // Send this data.
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_response.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer, cmp_mem_access_get_pos(&mem));
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;

            } else if (client_request.request_type == REQUEST_COMPUTE_VISDATA) {
              // We've been asked to recompute vis data with a different set of options.
              // Get the options.
	      pack_read_sint(&cmp, &n_client_options);
	      if (n_client_options > 0) {
		MALLOC(client_options, n_client_options);
		for (i = 0; i < n_client_options; i++) {
		  CALLOC(client_options[i], 1);
		  unpack_ampphase_options(&cmp, client_options[i]);
		}
		printf("DDDD Client supplied options\n");
		print_options_set(n_client_options, client_options, NULL, 0);
		// Store all the options in our cache.
		add_client_ampphase_options(&client_ampphase_options,
					    client_request.client_id,
					    client_request.client_username,
					    n_client_options, client_options);
		// And we will have to provide a notification later.
		notify_required = true;
	      } else {
		// Get the client options from our cache.
		succ = get_client_ampphase_options(&client_ampphase_options,
						   client_request.client_id,
						   client_request.client_username,
						   &n_client_options, &client_options);
		if (succ == false) {
		  // Revert to using the default.
		  get_client_ampphase_options(&client_ampphase_options,
					      "DEFAULT", "", &n_client_options,
					      &client_options);
		  printf("DDDD Using DEFAULT options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		} else {
		  printf("DDDD Using cached options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		}
		// No notification will be necessary.
		notify_required = false;
	      }

              // We're going to fork and compute in the background.
              pid = fork();
              if (pid == 0) {
                // We're the child.
                // First, we need to close our sockets.
                CLOSESOCKET(socket_listen);
                CLOSESOCKET(loop_i);
                // Now do the computation.
                printf("CHILD STARTED! Computing data...\n");
                
                data_reader(COMPUTE_VIS_PRODUCTS, arguments.n_rpfits_files, mjd_grab,
			    arguments.minimum_read_mjd, arguments.maximum_read_mjd,
			    0, NULL, &n_client_options, &client_options, info_rpfits_files,
			    &spectrum_data, &child_vis_data, NULL);
                // We send the data back to our parent over the network.
                if (prepare_client_connection("127.0.0.1", arguments.port_number,
                                              &child_socket, false)) {
                  // We have a connection.
                  child_request.request_type = CHILDREQUEST_VISDATA_COMPUTED;
                  strncpy(child_request.client_id, client_request.client_id, CLIENTIDLENGTH);
		  strncpy(child_request.client_username, client_request.client_username,
			  CLIENTIDLENGTH);
		  child_request.client_type = CLIENTTYPE_CHILD;
                  MALLOC(child_send_buffer, RPSENDBUFSIZE);
                  init_cmp_memory_buffer(&child_cmp, &child_mem, child_send_buffer,
                                         (size_t)RPSENDBUFSIZE);
                  pack_requests(&child_cmp, &child_request);
		  // Send all the options structures we used.
		  pack_write_sint(&child_cmp, n_client_options);
		  for (i = 0; i < n_client_options; i++) {
		    pack_ampphase_options(&child_cmp, client_options[i]);
		  }
		  // Next we indicate if a notification is required.
		  pack_write_bool(&child_cmp, notify_required);
		  // Now pack the data.
                  pack_vis_data(&child_cmp, child_vis_data);
                  printf("[CHILD] %s for client %s.\n",
                         get_type_string(TYPE_REQUEST, child_request.request_type),
                         client_request.client_id);
                  bytes_sent = socket_send_buffer(child_socket, child_send_buffer,
                                                  cmp_mem_access_get_pos(&child_mem));
                  FREE(child_send_buffer);
                }
                // Don't bother with the response.
                CLOSESOCKET(child_socket);
                // Clean up.
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
		FREE(client_options);
                // Die.
                printf("CHILD IS FINISHED!\n");
                exit(0);
              } else {
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
                FREE(client_options);
		n_client_options = 0;
                // Return a response saying that we are doing the computation.
                MALLOC(send_buffer, JUSTRESPONSESIZE);
                init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
                client_response.response_type = RESPONSE_VISDATA_COMPUTING;
                strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
                pack_responses(&cmp, &client_response);
                printf(" %s to client %s.\n",
                       get_type_string(TYPE_RESPONSE, client_response.response_type),
                       client_request.client_id);
                bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                                cmp_mem_access_get_pos(&mem));
                FREE(send_buffer);
              }
            } else if (client_request.request_type == CHILDREQUEST_VISDATA_COMPUTED) {
              // We're getting vis data back from our child after the computation has
              // finished.
              fprintf(stderr, "[PARENT] unpacking ampphase options\n");
              // Free the current client ampphase options.
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      // And read the new client options now.
	      pack_read_sint(&cmp, &n_client_options);
	      MALLOC(client_options, n_client_options);
	      for (i = 0; i < n_client_options; i++) {
		CALLOC(client_options[i], 1);
		unpack_ampphase_options(&cmp, client_options[i]);
	      }
	      // The child indicates whether this request requires us to notify
	      // other clients.
	      pack_read_bool(&cmp, &notify_required);
              /* fprintf(stderr, "[PARENT] unpacking vis data\n"); */
	      // And now get the data.
              unpack_vis_data(&cmp, vis_data);
              
              // Add this data to our cache.
              /* fprintf(stderr, "[PARENT] adding data to cache\n"); */
              vis_cache_updated = add_cache_vis_data(n_client_options,
						     client_options, vis_data);
              if (vis_cache_updated == false) {
                // The unpacked data can be freed.
                // This includes the header data.
                for (i = 0; i < vis_data->nviscycles; i++) {
                  free_scan_header_data(vis_data->header_data[i]);
                  FREE(vis_data->header_data[i]);
                }
                free_vis_data(vis_data);
                // Now get the cached vis data so we don't repeatedly store
                // new data into the client slot.
                get_cache_vis_data(n_client_options, client_options, &vis_data);
              }
              add_client_vis_data(&client_vis_data, client_request.client_id, vis_data);
              
              // Tell the client that their data is ready.
              // Find the client's socket.
              //alert_socket = find_client(&clients, client_request.client_id);
	      find_client(&clients, client_request.client_id, "",
			  &n_alert_sockets, &alert_socket, NULL);
	      for (i = 0; i < n_alert_sockets; i++) {
		if (ISVALIDSOCKET(alert_socket[i])) {
		  // Craft a response.
		  client_response.response_type = RESPONSE_VISDATA_COMPUTED;
		  strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
		  MALLOC(send_buffer, JUSTRESPONSESIZE);
		  init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
		  pack_responses(&cmp, &client_response);
		  printf(" %s to client %s.\n",
			 get_type_string(TYPE_RESPONSE, client_response.response_type),
			 client_request.client_id);
		  bytes_sent = socket_send_buffer(alert_socket[i], send_buffer,
						  cmp_mem_access_get_pos(&mem));
		  FREE(send_buffer);
		}
              }
	      FREE(alert_socket);

	      if (notify_required) {
		// Now send a message to other clients of the same user, to let them
		// know new data with different options is being generated.
		find_client(&clients, client_request.client_id,
			    client_request.client_username, &n_alert_sockets,
			    &alert_socket, &client_indices);
		for (i = 0; i < n_alert_sockets; i++) {
		  if (strncmp(clients.client_id[client_indices[i]],
			      client_request.client_id, CLIENTIDLENGTH) != 0) {
		    // Don't send the message to the client already connected.
		    client_response.response_type = RESPONSE_USERREQUEST_VISDATA;
		    strncpy(client_response.client_id,
			    clients.client_id[client_indices[i]], CLIENTIDLENGTH);
		    MALLOC(send_buffer, JUSTRESPONSESIZE);
		    init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
		    pack_responses(&cmp, &client_response);
		    printf(" %s to client %s.\n",
			   get_type_string(TYPE_RESPONSE, client_response.response_type),
			   client_response.client_id);
		    bytes_sent = socket_send_buffer(alert_socket[i], send_buffer,
						    cmp_mem_access_get_pos(&mem));
		    FREE(send_buffer);
		  }
		}
		// Free the client list.
		FREE(alert_socket);
		FREE(client_indices);
	      }
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      n_alert_sockets = 0;
            } else if (client_request.request_type == REQUEST_SERVERTYPE) {
              // Tell the client we're a simulator or a tester, depending on how
              // we were started.
              client_response.response_type = RESPONSE_SERVERTYPE;
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
              MALLOC(send_buffer, JUSTRESPONSESIZE);
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
              pack_responses(&cmp, &client_response);
              if (arguments.testing_operation) {
                pack_write_sint(&cmp, SERVERTYPE_TESTING);
              } else {
                pack_write_sint(&cmp, SERVERTYPE_SIMULATOR);
              }
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_request.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                              cmp_mem_access_get_pos(&mem));
              if (arguments.testing_operation) {
                // Also request the user name.
                init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
                client_response.response_type = RESPONSE_REQUEST_USERNAME;
                pack_responses(&cmp, &client_response);
                printf(" %s to client %s.\n",
                       get_type_string(TYPE_RESPONSE, client_response.response_type),
                       client_request.client_id);
                bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                                cmp_mem_access_get_pos(&mem));
              }
              FREE(send_buffer);
            } else if (client_request.request_type == REQUEST_SPECTRUM_MJD) {
              // This is asking us to grab a spectrum within some tolerance of
              // a specified MJD.
              // We unpack a double, being the MJD.
              pack_read_double(&cmp, &mjd_grab);
              printf(" Client has requested data at MJD %.8f\n", mjd_grab);
              // Get the options.
	      pack_read_sint(&cmp, &n_client_options);
	      if (n_client_options > 0) {
		// The options were supplied by the user.
		MALLOC(client_options, n_client_options);
		for (i = 0; i < n_client_options; i++) {
		  CALLOC(client_options[i], 1);
		  unpack_ampphase_options(&cmp, client_options[i]);
		}
		printf("DDDD Client supplied options\n");
		print_options_set(n_client_options, client_options, NULL, 0);
		// Store all the options in our cache.
		add_client_ampphase_options(&client_ampphase_options,
					    client_request.client_id,
					    client_request.client_username,
					    n_client_options, client_options);
		// And we will have to provide a notification later.
		notify_required = true;
	      } else {
		// Try to get the options that this user has used before.
		succ = get_client_ampphase_options(&client_ampphase_options,
						   client_request.client_id,
						   client_request.client_username,
						   &n_client_options, &client_options);
		if (succ == false) {
		  // No good, so we use the default options.
		  printf(" Search for user options failed, switching to DEFAULT\n");
		  get_client_ampphase_options(&client_ampphase_options,
					      "DEFAULT", "", &n_client_options,
					      &client_options);
		  printf("DDDD Using DEFAULT options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		} else {
		  printf("DDDD Using cached options\n");
		  print_options_set(n_client_options, client_options, NULL, 0);
		}
		// No notification will be necessary.
		notify_required = false;
	      }
              // Check that this MJD is within the range we know about.
              outside_mjd_range = false;
              if ((mjd_grab < earliest_mjd) || (mjd_grab > latest_mjd)) {
                outside_mjd_range = true;
              }
              if (outside_mjd_range == false) {
                // We're going to fork and compute in the background.
                pid = fork();
              } else {
                pid = 1;
              }
              if (pid == 0) {
                // We're the child.
                // First, we need to close our sockets.
                CLOSESOCKET(socket_listen);
                CLOSESOCKET(loop_i);
                // Now grab the spectrum.
                printf("CHILD STARTED! Grabbing spectrum...\n");
                
                data_reader(GRAB_SPECTRUM, arguments.n_rpfits_files, mjd_grab,
			    arguments.minimum_read_mjd, arguments.maximum_read_mjd, 0, NULL,
			    &n_client_options, &client_options, info_rpfits_files,
			    &child_spectrum_data, NULL, NULL);
                // We send the data back to our parent over the network.
                if (prepare_client_connection("127.0.0.1", arguments.port_number,
                                              &child_socket, false)) {
                  // We have a connection.
                  child_request.request_type = CHILDREQUEST_SPECTRUM_MJD;
                  strncpy(child_request.client_id, client_request.client_id, CLIENTIDLENGTH);
		  strncpy(child_request.client_username, client_request.client_username,
			  CLIENTIDLENGTH);
		  child_request.client_type = CLIENTTYPE_CHILD;
                  MALLOC(child_send_buffer, RPSENDBUFSIZE);
                  init_cmp_memory_buffer(&child_cmp, &child_mem, child_send_buffer,
                                         (size_t)RPSENDBUFSIZE);
                  pack_requests(&child_cmp, &child_request);
                  mjd_grab = date2mjd(child_spectrum_data->header_data->obsdate,
                                      child_spectrum_data->spectrum[0][0]->ut_seconds);
                  fprintf(stderr, "[CHILD] sending grab %s %.1f %.8f\n",
                          child_spectrum_data->header_data->obsdate,
                          child_spectrum_data->spectrum[0][0]->ut_seconds,
                          mjd_grab);
                  pack_write_double(&child_cmp, mjd_grab);
		  pack_write_sint(&child_cmp, n_client_options);
		  for (i = 0; i < n_client_options; i++) {
		    pack_ampphase_options(&child_cmp, client_options[i]);
		  }
		  // Next we indicate if a notification is required.
		  pack_write_bool(&child_cmp, notify_required);
		  // Now pack the data.
                  pack_spectrum_data(&child_cmp, child_spectrum_data);
                  printf("[CHILD] %s for client %s.\n",
                         get_type_string(TYPE_REQUEST, child_request.request_type),
                         client_request.client_id);
                  bytes_sent = socket_send_buffer(child_socket, child_send_buffer,
                                                  cmp_mem_access_get_pos(&child_mem));
                  FREE(child_send_buffer);
                }
                // Don't bother with the response.
                CLOSESOCKET(child_socket);
                // Clean up.
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
		FREE(client_options);
                // Die.
                printf("CHILD IS FINISHED!\n");
                exit(0);
              } else {
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
		FREE(client_options);
		n_client_options = 0;
                // Return a response saying that we are doing the grab.
                MALLOC(send_buffer, JUSTRESPONSESIZE);
                init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
                if (outside_mjd_range == false) {
                  client_response.response_type = RESPONSE_SPECTRUM_LOADING;
                } else {
                  client_response.response_type = RESPONSE_SPECTRUM_OUTSIDERANGE;
                }
                strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
                pack_responses(&cmp, &client_response);
                printf(" %s to client %s.\n",
                       get_type_string(TYPE_RESPONSE, client_response.response_type),
                       client_response.client_id);
                bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                                cmp_mem_access_get_pos(&mem));
                FREE(send_buffer);
              }
            } else if (client_request.request_type == CHILDREQUEST_SPECTRUM_MJD) {
              // We're getting a spectrum back from our child after it was grabbed.
              // Free the current client ampphase options.
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
              pack_read_double(&cmp, &mjd_grab);
	      pack_read_sint(&cmp, &n_client_options);
	      MALLOC(client_options, n_client_options);
	      for (i = 0; i < n_client_options; i++) {
		CALLOC(client_options[i], 1);
		unpack_ampphase_options(&cmp, client_options[i]);
	      }
	      pack_read_bool(&cmp, &notify_required);
              /* fprintf(stderr, " getting data from child for MJD %.8f, deg %d\n", */
              /*         mjd_grab, ampphase_options->phase_in_degrees); */
              unpack_spectrum_data(&cmp, spectrum_data);
              
              // Add this data to our cache.
              spd_cache_updated = add_cache_spd_data(n_client_options,
						     client_options, spectrum_data);
              if (spd_cache_updated == false) {
                // The unpacked data can be freed.
                free_scan_header_data(spectrum_data->header_data);
                FREE(spectrum_data->header_data);
                free_spectrum_data(spectrum_data);
                // Now get the cached spectrum data.
                fprintf(stderr, " matched data so grabbing cached data\n");
                get_cache_spd_data(n_client_options, client_options,
				   mjd_grab, (mjd_cycletime / 2.0),
                                   &spectrum_data);
              }
              fprintf(stderr, " associating data with client %s\n", client_request.client_id);
              add_client_spd_data(&client_spd_data, client_request.client_id, spectrum_data);
              
              // Tell the client that their data is ready.
              // Find the client's socket.
              find_client(&clients, client_request.client_id, "",
			  &n_alert_sockets, &alert_socket, NULL);
	      for (i = 0; i < n_alert_sockets; i++) {
		if (ISVALIDSOCKET(alert_socket[i])) {
		  // Craft a response.
		  fprintf(stderr, " alerting client %s\n", client_request.client_id);
		  client_response.response_type = RESPONSE_SPECTRUM_LOADED;
		  strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
		  MALLOC(send_buffer, JUSTRESPONSESIZE);
		  init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
		  pack_responses(&cmp, &client_response);
		  printf(" %s to client %s.\n",
			 get_type_string(TYPE_RESPONSE, client_response.response_type),
			 client_request.client_id);
		  bytes_sent = socket_send_buffer(alert_socket[i], send_buffer,
						  cmp_mem_access_get_pos(&mem));
		  FREE(send_buffer);
		}
	      }
	      FREE(alert_socket);
	      n_alert_sockets = 0;

	      if (notify_required) {
		// Now send a message to other clients of the same user, to let them
		// know new data with different options is being generated.
		find_client(&clients, client_request.client_id,
			    client_request.client_username, &n_alert_sockets,
			    &alert_socket, &client_indices);
		for (i = 0; i < n_alert_sockets; i++) {
		  if (strncmp(clients.client_id[client_indices[i]],
			      client_request.client_id, CLIENTIDLENGTH) != 0) {
		    // Don't send the message to the client already connected.
		    client_response.response_type = RESPONSE_USERREQUEST_SPECTRUM;
		    strncpy(client_response.client_id,
			    clients.client_id[client_indices[i]], CLIENTIDLENGTH);
		    MALLOC(send_buffer, JUSTRESPONSESIZE);
		    init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
		    pack_responses(&cmp, &client_response);
		    printf(" %s to client %s.\n",
			   get_type_string(TYPE_RESPONSE, client_response.response_type),
			   client_response.client_id);
		    bytes_sent = socket_send_buffer(alert_socket[i], send_buffer,
						    cmp_mem_access_get_pos(&mem));
		    FREE(send_buffer);
		  }
		}
		// Free everything.
		FREE(alert_socket);
		FREE(client_indices);
	      }
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      n_alert_sockets = 0;
            } else if (client_request.request_type == REQUEST_TIMERANGE) {
              // Something wants to know the time information like min/max
              // MJD and the cycle time.
              // Craft the response.
              client_response.response_type = RESPONSE_TIMERANGE;
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
              MALLOC(send_buffer, JUSTRESPONSESIZE);
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
              pack_responses(&cmp, &client_response);
              // We first write the cycle time in days.
              pack_write_double(&cmp, mjd_cycletime);
              // The the earliest time we handle, then the latest.
              pack_write_double(&cmp, earliest_mjd);
              pack_write_double(&cmp, latest_mjd);
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_response.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                              cmp_mem_access_get_pos(&mem));
              FREE(send_buffer);
            } else if (client_request.request_type == REQUEST_CYCLE_TIMES) {
              // We give back a list of all the cycle MJDs we know about so that
              // they can select from them exactly.
              client_response.response_type = RESPONSE_CYCLE_TIMES;
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
              comp_buffer_length = JUSTRESPONSESIZE + 2 * n_cycle_mjd * sizeof(double);
              MALLOC(send_buffer, comp_buffer_length);
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, comp_buffer_length);
              pack_responses(&cmp, &client_response);
              pack_write_sint(&cmp, n_cycle_mjd);
              pack_writearray_double(&cmp, n_cycle_mjd, all_cycle_mjd);
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_response.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                              cmp_mem_access_get_pos(&mem));
              FREE(send_buffer);
            } else if (client_request.request_type == REQUEST_SUPPLY_USERNAME) {
	      // We are being told that the username of this client has changed or
	      // supplied.
	      modify_client(&clients, client_request.client_id,
			    client_request.client_username, -1);
	      // We don't send any reply here.
	      bytes_sent = 0;
	    } else if (client_request.request_type == REQUEST_ACAL) {
	      // We need to compute the noise diode amplitudes from the data range
	      // supplied.
	      // We unpack some options structures.
	      pack_read_sint(&cmp, &n_client_options);
	      if (n_client_options > 0) {
		MALLOC(client_options, n_client_options);
		for (i = 0; i < n_client_options; i++) {
		  CALLOC(client_options[i], 1);
		  unpack_ampphase_options(&cmp, client_options[i]);
		}
	      } else {
		// Try to get the options that this user has used before.
		succ = get_client_ampphase_options(&client_ampphase_options,
						   client_request.client_id,
						   client_request.client_username,
						   &n_client_options, &client_options);
		if (succ == false) {
		  // Use the default options.
		  get_client_ampphase_options(&client_ampphase_options,
					      "DEFAULT", "", &n_client_options,
					      &client_options);
		}
	      }
	      // And then an array of MJDs to use for the calibration cycles.
	      pack_read_sint(&cmp, &n_acal_cycles);
	      if (n_acal_cycles > 0) {
		CALLOC(acal_cycle_mjds, n_acal_cycles);
		pack_readarray_double(&cmp, n_acal_cycles, acal_cycle_mjds);
	      }
	      // And then check if we have some user-specified amplitudes.
	      pack_read_sint(&cmp, &n_acal_fluxdensities);
	      if (n_acal_fluxdensities > 0) {
		CALLOC(acal_fluxdensities, n_acal_fluxdensities);
		pack_readarray_float(&cmp, n_acal_fluxdensities, acal_fluxdensities);
	      }
	      // Check all these cycles have MJDs are within the range we know about.
	      outside_mjd_range = false;
	      for (i = 0; i < n_acal_cycles; i++) {
		if ((acal_cycle_mjds[i] < earliest_mjd) || (acal_cycle_mjds[i] > latest_mjd)) {
		  outside_mjd_range = true;
		}
	      }
	      if ((outside_mjd_range == false) && (n_acal_cycles > 0)) {
		// We're going to fork and compute in the background.
		pid = fork();
	      } else {
		pid = 1;
	      }
	      if (pid == 0) {
		// We're the child.
		// First close our sockets.
		CLOSESOCKET(socket_listen);
		CLOSESOCKET(loop_i);
		printf("CHILD STARTED! Calculating acal parameters...\n");
		// Keep a copy of the options to send later.
		n_copied_options = n_client_options;
		MALLOC(copied_options, n_client_options);
		for (i = 0; i < n_copied_options; i++) {
		  CALLOC(copied_options[i], 1);
		  // Set the options to have no Tsys calibration applied.
		  client_options[i]->systemp_reverse_online = true;
		  client_options[i]->systemp_apply_computed = false;
		  copy_ampphase_options(copied_options[i], client_options[i]);
		}
		printf(" Getting %d cycles:\n", n_acal_cycles);
		for (i = 0; i < n_acal_cycles; i++) {
		  printf("   MJD %.6f\n", acal_cycle_mjds[i]);
		}
		// Get the cycles.
		data_reader(GRAB_MJDS_SPECTRA, arguments.n_rpfits_files, -1,
			    arguments.minimum_read_mjd, arguments.maximum_read_mjd,
			    n_acal_cycles, acal_cycle_mjds, &n_client_options,
			    &client_options, info_rpfits_files,
			    NULL, NULL, &acal_spectra);
		printf(" Data obtained, computing parameters...\n");
		// And compute the amplitude calibration parameters.
		if (n_acal_fluxdensities <= 0) {
		  acal_source =
		    acal_spectra[0]->header_data->source_name[acal_spectra[0]->spectrum[0][0]->source_no];
		  // We make a very simplistic flux density specifier here, but this will
		  // need to be changed when we want to move to a better method of acal.
		  // We do this now to match how CABB acal works.
		  fd_spec.num_models = acal_spectra[0]->num_ifs;
		} else {
		  // We use the numbers that came along with the request.
		  fd_spec.num_models = n_acal_fluxdensities;
		}
		/* printf(" Working with %d flux density models\n", fd_spec.num_models); */
		CALLOC(fd_spec.model_frequency, fd_spec.num_models);
		CALLOC(fd_spec.model_frequency_tolerance, fd_spec.num_models);
		CALLOC(fd_spec.model_num_terms, fd_spec.num_models);
		CALLOC(fd_spec.model_terms, fd_spec.num_models);
		// Find the correct set of options.
		spectrum_options = find_ampphase_options(n_client_options,
							 client_options,
							 acal_spectra[0]->header_data,
							 &acal_options_idx);
		if (n_acal_fluxdensities > 0) {
		  j = 0;
		  for (i = 0; i < n_acal_fluxdensities; i++) {
		    // We find the next available continuum band.
		    for (k = j; k < acal_spectra[0]->num_ifs; k++) {
		      if (acal_spectra[0]->header_data->if_bandwidth[k] > 1000) {
			j = k + 1;
			fd_spec.model_frequency[i] =
			  acal_spectra[0]->header_data->if_centre_freq[k];
			fd_spec.model_frequency_tolerance[i] =
			  acal_spectra[0]->header_data->if_bandwidth[k] / 2;
			break;
		      }
		    }
		    fd_spec.model_num_terms[i] = 1;
		    CALLOC(fd_spec.model_terms[i], fd_spec.model_num_terms[i]);
		    fd_spec.model_terms[i][0] = acal_fluxdensities[i];
		  }
		}
		for (i = 0; i < acal_spectra[0]->num_ifs; i++) {
		  // Find the correct window number.
		  acal_window = acal_spectra[0]->header_data->if_label[i];
		  if (n_acal_fluxdensities == 0) {
		    // Work out the correct flux density to use.
		    acal_source_recognised =
		      source_model(acal_source,
				   acal_spectra[0]->header_data->if_centre_freq[i],
				   &acal_model_num_terms, &acal_model_log,
				   &acal_model_terms);
		    fd_spec.model_frequency[i] = acal_spectra[0]->header_data->if_centre_freq[i];
		    fd_spec.model_frequency_tolerance[i] =
		      acal_spectra[0]->header_data->if_bandwidth[i] / 2;
		    fd_spec.model_num_terms[i] = 1;
		    REALLOC(fd_spec.model_terms[i], fd_spec.model_num_terms[i]);
		    if (acal_source_recognised) {
		      acal_fd =
			fluxdensity_model_evaluate(acal_model_num_terms, acal_model_log,
						   acal_model_terms,
						   acal_spectra[0]->header_data->if_centre_freq[i]);
		      fd_spec.model_terms[i][0] = acal_fd;
		    } else {
		      fd_spec.model_terms[i][0] = 1;
		    }
		  }
		  compute_noise_diode_amplitudes(&fd_spec, n_acal_cycles, acal_window,
						 spectrum_options,
						 acal_spectra, &fd_modifier);
		  // Attach the modifier to the options now.
		  add_modifier(spectrum_options, acal_window, fd_modifier);
		  free_ampphase_modifiers(fd_modifier);
		  FREE(fd_modifier);
		}
		
		// We send the data back to our parent over the network.
		if (prepare_client_connection("127.0.0.1", arguments.port_number,
					      &child_socket, false)) {
		  // We have a connection.
		  child_request.request_type = CHILDREQUEST_MJDS_SPECTRA;
		  strncpy(child_request.client_id, client_request.client_id, CLIENTIDLENGTH);
		  strncpy(child_request.client_username, client_request.client_username,
			  CLIENTIDLENGTH);
		  child_request.client_type = CLIENTTYPE_CHILD;
		  MALLOC(child_send_buffer, RPSENDBUFSIZE);
		  init_cmp_memory_buffer(&child_cmp, &child_mem, child_send_buffer,
					 (size_t)RPSENDBUFSIZE);
		  pack_requests(&child_cmp, &child_request);
		  // Send the original options back for caching.
		  pack_write_sint(&child_cmp, n_copied_options);
		  for (i = 0; i < n_copied_options; i++) {
		    pack_ampphase_options(&child_cmp, copied_options[i]);
		  }
		  // Send back the new options with the modifiers.
		  pack_write_sint(&child_cmp, n_client_options);
		  for (i = 0; i < n_client_options; i++) {
		    pack_ampphase_options(&child_cmp, client_options[i]);
		  }
		  // Specify the index of the options we modified.
		  pack_write_sint(&child_cmp, acal_options_idx);
		  // Send back the flux density specification for the user.
		  pack_fluxdensity_specification(&child_cmp, &fd_spec);
		  // Pack the MJDs of each of the data grabs. We send the grabs back so
		  // they can be cached.
		  pack_write_sint(&child_cmp, n_acal_cycles);
		  pack_writearray_double(&child_cmp, n_acal_cycles, acal_cycle_mjds);
		  for (i = 0; i < n_acal_cycles; i++) {
		    pack_spectrum_data(&child_cmp, acal_spectra[i]);
		  }
		  printf("[CHILD] %s for client %s.\n",
			 get_type_string(TYPE_REQUEST, child_request.request_type),
			 client_request.client_id);
		  bytes_sent = socket_send_buffer(child_socket, child_send_buffer,
						  cmp_mem_access_get_pos(&child_mem));
		  FREE(child_send_buffer);
		}
		// Don't bother with the response.
		CLOSESOCKET(child_socket);
		// Clean up.
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
		FREE(client_options);
		for (i = 0; i < n_copied_options; i++) {
		  free_ampphase_options(copied_options[i]);
		  FREE(copied_options[i]);
		}
		free_fluxdensity_specification(&fd_spec);
		// Die.
		printf("CHILD IS FINISHED!\n");
		exit(0);
	      } else {
		// Free the memory we used.
		for (i = 0; i < n_client_options; i++) {
		  free_ampphase_options(client_options[i]);
		  FREE(client_options[i]);
		}
		FREE(client_options);
		n_client_options = 0;
		FREE(acal_cycle_mjds);
		n_acal_cycles = 0;
		FREE(acal_fluxdensities);
		n_acal_fluxdensities = 0;
		if (pid == 1) {
		  // Return an error code.
		  client_response.response_type = RESPONSE_ACAL_REQUEST_INVALID;
		} else {
		  client_response.response_type = RESPONSE_ACAL_COMPUTING;
		}
		strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
		comp_buffer_length = JUSTRESPONSESIZE;
		MALLOC(send_buffer, comp_buffer_length);
		init_cmp_memory_buffer(&cmp, &mem, send_buffer, comp_buffer_length);
		pack_responses(&cmp, &client_response);
		printf(" %s to client %s.\n",
		       get_type_string(TYPE_RESPONSE, client_response.response_type),
		       client_response.client_id);
		bytes_sent = socket_send_buffer(loop_i, send_buffer,
						cmp_mem_access_get_pos(&mem));
		FREE(send_buffer);
	      }
	    } else if (client_request.request_type == CHILDREQUEST_MJDS_SPECTRA) {
	      // Receive some information coming back from an acal request.
	      // Free the current client ampphase options.
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      for (i = 0; i < n_copied_options; i++) {
		free_ampphase_options(copied_options[i]);
		FREE(copied_options[i]);
	      }
	      FREE(copied_options);
	      n_copied_options = 0;
	      // Read the original options now to label the cache.
	      pack_read_sint(&cmp, &n_copied_options);
	      MALLOC(copied_options, n_copied_options);
	      for (i = 0; i < n_copied_options; i++) {
		CALLOC(copied_options[i], 1);
		unpack_ampphase_options(&cmp, copied_options[i]);
	      }
	      // And read the new client options now.
	      pack_read_sint(&cmp, &n_client_options);
	      MALLOC(client_options, n_client_options);
	      for (i = 0; i < n_client_options; i++) {
		CALLOC(client_options[i], 1);
		unpack_ampphase_options(&cmp, client_options[i]);
	      }
	      // Read the index of the modified options.
	      pack_read_sint(&cmp, &acal_options_idx);
	      // Get the flux density specification.
	      unpack_fluxdensity_specification(&cmp, &fd_spec);
	      // Unpack the MJDs of each of the cycles that were requested, as
	      // we will cache them for later use.
	      pack_read_sint(&cmp, &n_acal_cycles);
	      CALLOC(acal_cycle_mjds, n_acal_cycles);
	      pack_readarray_double(&cmp, n_acal_cycles, acal_cycle_mjds);
	      for (i = 0; i < n_acal_cycles; i++) {
		unpack_spectrum_data(&cmp, spectrum_data);
		spd_cache_updated = add_cache_spd_data(n_copied_options,
						       copied_options, spectrum_data);
		if (spd_cache_updated == false) {
		  // The unpacked data can be freed.
		  free_scan_header_data(spectrum_data->header_data);
		  FREE(spectrum_data->header_data);
		  free_spectrum_data(spectrum_data);
		}
	      }
	      // Tell the client their acal information is ready.
	      // Find the client's socket.
	      find_client(&clients, client_request.client_id, "",
			  &n_alert_sockets, &alert_socket, NULL);
	      for (i = 0; i < n_alert_sockets; i++) {
		if (ISVALIDSOCKET(alert_socket[i])) {
		  fprintf(stderr, " alerting client %s\n", client_request.client_id);
		  client_response.response_type = RESPONSE_ACAL_COMPUTED;
		  strncpy(client_response.client_id, client_request.client_id,
			  CLIENTIDLENGTH);
		  MALLOC(send_buffer, RPSENDBUFSIZE);
		  init_cmp_memory_buffer(&cmp, &mem, send_buffer, RPSENDBUFSIZE);
		  pack_responses(&cmp, &client_response);
		  /* fprintf(stderr, "  response header packed\n"); */
		  // Send the new options with the modifiers.
		  pack_write_sint(&cmp, n_client_options);
		  /* fprintf(stderr, "  client option number %d packed\n", n_client_options); */
		  for (j = 0; j < n_client_options; j++) {
		    fprintf(stderr, "   ... %d\n", j);
		    pack_ampphase_options(&cmp, client_options[j]);
		  }
		  /* fprintf(stderr, "  all options packed\n"); */
		  // Tell them which index was modified.
		  /* fprintf(stderr, "  packing options index %d\n", acal_options_idx); */
		  pack_write_sint(&cmp, acal_options_idx);
		  // And send the flux density models.
		  pack_fluxdensity_specification(&cmp, &fd_spec);
		  printf(" %s to client %s.\n",
			 get_type_string(TYPE_RESPONSE, client_response.response_type),
			 client_request.client_id);
		  bytes_sent = socket_send_buffer(alert_socket[i], send_buffer,
						  cmp_mem_access_get_pos(&mem));
		  FREE(send_buffer);
		}
	      }
	      FREE(alert_socket);
	      n_alert_sockets = 0;
	      // Free the options again.
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      for (i = 0; i < n_copied_options; i++) {
		free_ampphase_options(copied_options[i]);
		FREE(copied_options[i]);
	      }
	      FREE(copied_options);
	      n_copied_options = 0;
	      // And free the other received memory.
	      FREE(acal_cycle_mjds);
	      n_acal_cycles = 0;
	      free_fluxdensity_specification(&fd_spec);
	    }
            printf(" Sent %ld bytes\n", bytes_sent);

	    if (client_added) {
	      // Now we check if another client from this user has already connected
	      // and made a request (which might have used different options) that might
	      // require recomputation. We do this here so that the immediately available
	      // data is passed quickly, and the computed data can be requested.
	      printf(" new client added %s, checking for user %s\n",
		     client_request.client_id, client_request.client_username);
	      find_client(&clients, client_request.client_id,
			  client_request.client_username, &n_alert_sockets,
			  &alert_socket, &client_indices);
	      notify_required = false;
	      for (i = 0; i < n_alert_sockets; i++) {
		if ((alert_socket[i] != loop_i) &&
		    (strncmp(clients.client_username[i], client_request.client_username,
			     CLIENTIDLENGTH) == 0)) {
		  // Check if we have options cached for this client.
		  if (get_client_ampphase_options(&client_ampphase_options,
						  client_request.client_id,
						  client_request.client_username,
						  &n_client_options, &client_options)) {
		    notify_required = true;
		  }
		}
	      }
	      // Free our memory.
	      FREE(alert_socket);
	      FREE(client_indices);
	      n_alert_sockets = 0;
	      for (i = 0; i < n_client_options; i++) {
		free_ampphase_options(client_options[i]);
		FREE(client_options[i]);
	      }
	      FREE(client_options);
	      n_client_options = 0;
	      if (notify_required) {
		client_response.response_type = RESPONSE_USERNAME_EXISTS;
		strncpy(client_response.client_id, client_request.client_id,
			CLIENTIDLENGTH);
		if (send_buffer != NULL) {
		  FREE(send_buffer);
		}
		MALLOC(send_buffer, JUSTRESPONSESIZE);
		init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
		pack_responses(&cmp, &client_response);
		printf(" %s to client %s.\n",
		       get_type_string(TYPE_RESPONSE, client_response.response_type),
		       client_response.client_id);
		bytes_sent = socket_send_buffer(loop_i, send_buffer,
						cmp_mem_access_get_pos(&mem));
		printf(" Sent %ld bytes\n", bytes_sent);
		FREE(send_buffer);
	      } else {
		printf("   first connection by this user\n");
	      }
	    }
	    
            // Free our memory.
            FREE(send_buffer);
            FREE(recv_buffer);
          }
        }
      }
    }
  }
  // Free the vis cache.
  for (l = 0; l < cache_vis_data.num_cache_vis_data; l++) {
    // Check if the header needs to be freed. If one header structure
    // is found to be allocated outside our RPFITS header cache, all of them
    // will have been.
    for (i = 0, pointer_found = false; i < arguments.n_rpfits_files; i++) {
      for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
        for (k = 0; k < cache_vis_data.vis_data[l]->nviscycles; k++) {
          if (info_rpfits_files[i]->scan_headers[j] ==
              cache_vis_data.vis_data[l]->header_data[k]) {
            pointer_found = true;
            break;
          }
        }
        if (pointer_found) break;
      }
      if (pointer_found) break;
    }
    if (!pointer_found) {
      // We have to free the memory.
      for (i = 0; i < cache_vis_data.vis_data[l]->nviscycles; i++) {
        free_scan_header_data(cache_vis_data.vis_data[l]->header_data[i]);
        FREE(cache_vis_data.vis_data[l]->header_data[i]);
      }
    }
    free_vis_data(cache_vis_data.vis_data[l]);
    FREE(cache_vis_data.vis_data[l]);
    for (i = 0; i < cache_vis_data.num_options[l]; i++) {
      free_ampphase_options(cache_vis_data.ampphase_options[l][i]);
      FREE(cache_vis_data.ampphase_options[l][i]);
    }
    FREE(cache_vis_data.ampphase_options[l]);
  }
  FREE(cache_vis_data.vis_data);
  FREE(cache_vis_data.ampphase_options);
  FREE(cache_vis_data.num_options);
  // Do the same for the spectrum cache.
  for (l = 0; l < cache_spd_data.num_cache_spd_data; l++) {
    for (i = 0, pointer_found = false; i < arguments.n_rpfits_files; i++) {
      for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
	if (info_rpfits_files[i]->scan_headers[j] ==
	    cache_spd_data.spectrum_data[l]->header_data) {
	  pointer_found = true;
	  break;
	}
	if (pointer_found) break;
      }
      if (pointer_found) break;
    }
    if (!pointer_found) {
      // We have to free the memory.
      free_scan_header_data(cache_spd_data.spectrum_data[l]->header_data);
      FREE(cache_spd_data.spectrum_data[l]->header_data);
    }
    free_spectrum_data(cache_spd_data.spectrum_data[l]);
    FREE(cache_spd_data.spectrum_data[l]);
    for (i = 0; i < cache_spd_data.num_options[l]; i++) {
      free_ampphase_options(cache_spd_data.ampphase_options[l][i]);
      FREE(cache_spd_data.ampphase_options[l][i]);
    }
    FREE(cache_spd_data.ampphase_options[l]);
  }
  FREE(cache_spd_data.spectrum_data);
  FREE(cache_spd_data.ampphase_options);
  FREE(cache_spd_data.num_options);
  // Free the spectrum memory.
  FREE(spectrum_data);

  // Free any testing instructions.
  while (testing_instructions != NULL) {
    file_instructions_ptr = testing_instructions;
    testing_instructions = testing_instructions->next;

    for (i = 0; i < file_instructions_ptr->num_files; i++) {
      FREE(file_instructions_ptr->files[i]);
    }
    FREE(file_instructions_ptr->files);

    for (i = 0; i < file_instructions_ptr->num_choices; i++) {
      FREE(file_instructions_ptr->choices[i]);
    }
    FREE(file_instructions_ptr->choices);

    FREE(file_instructions_ptr->message);

    FREE(file_instructions_ptr);
  }
  
  // We're finished, free all our memory.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      if (info_rpfits_files[i]->n_cycles[j] > 0) {
	free_scan_header_data(info_rpfits_files[i]->scan_headers[j]);
	FREE(info_rpfits_files[i]->scan_headers[j]);
	FREE(info_rpfits_files[i]->cycle_mjd[j]);
      }
    }
    FREE(info_rpfits_files[i]->scan_headers);
    FREE(info_rpfits_files[i]->scan_start_mjd);
    FREE(info_rpfits_files[i]->scan_end_mjd);
    FREE(info_rpfits_files[i]->n_cycles);
    FREE(info_rpfits_files[i]->cycle_mjd);
    FREE(info_rpfits_files[i]);
  }
  FREE(info_rpfits_files);
  FREE(arguments.rpfits_files);

  for (i = 0; i < n_ampphase_options; i++) {
    free_ampphase_options(ampphase_options[i]);
    FREE(ampphase_options[i]);
  }
  FREE(ampphase_options);
  FREE(vis_data);

  FREE(all_cycle_mjd);

  // Disconnect any clients that are still connected.
  free_client_sockets(&clients);
  
  // Free the clients.
  for (i = 0; i < client_vis_data.num_clients; i++) {
    FREE(client_vis_data.client_id[i]);
    FREE(client_vis_data.vis_data[i]);
  }
  FREE(client_vis_data.client_id);
  FREE(client_vis_data.vis_data);
  for (i = 0; i < client_spd_data.num_clients; i++) {
    FREE(client_spd_data.client_id[i]);
    FREE(client_spd_data.spectrum_data[i]);
  }
  FREE(client_spd_data.client_id);
  FREE(client_spd_data.spectrum_data);
  for (i = 0; i < client_ampphase_options.num_clients; i++) {
    FREE(client_ampphase_options.client_id[i]);
    FREE(client_ampphase_options.client_username[i]);
    for (j = 0; j < client_ampphase_options.n_ampphase_options[i]; j++) {
      free_ampphase_options(client_ampphase_options.ampphase_options[i][j]);
      FREE(client_ampphase_options.ampphase_options[i][j]);
    }
    FREE(client_ampphase_options.ampphase_options[i]);
  }
  FREE(client_ampphase_options.client_id);
  FREE(client_ampphase_options.client_username);
  FREE(client_ampphase_options.n_ampphase_options);
  FREE(client_ampphase_options.ampphase_options);
  
  if (arguments.network_operation) {
    // Close our socket.
    printf("\n\nClosing listening socket...\n");
    CLOSESOCKET(socket_listen);
  }

  
}
