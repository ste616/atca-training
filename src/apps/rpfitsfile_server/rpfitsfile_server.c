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
#include <time.h>
#include <signal.h>
#include "atrpfits.h"
#include "memory.h"
#include "packing.h"
#include "atnetworking.h"
#include "common.h"

#define RPSBUFSIZE 1024

const char *argp_program_version = "rpfitsfile_server 1.0";
const char *argp_program_bug_address = "<Jamie.Stevens@csiro.au>";

// Program documentation.
static char doc[] = "RPFITS file reader for network tasks";

// Argument description.
static char args_doc[] = "[options] RPFITS_FILES...";

// Our options.
static struct argp_option options[] = {
                                       { "networked", 'n', 0, 0,
                                         "Switch to operate as a network data server " },
                                       { "port", 'p', "PORTNUM", 0,
                                         "The port number to listen on" },
  { 0 }
};

// The arguments structure.
struct arguments {
  int n_rpfits_files;
  char **rpfits_files;
  int port_number;
  bool network_operation;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  
  switch (key) {
  case 'n':
    arguments->network_operation = true;
    break;
  case 'p':
    arguments->port_number = atoi(arg);
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

static struct argp argp = { options, parse_opt, args_doc, doc };

struct cache_vis_data {
  int num_cache_vis_data;
  struct ampphase_options **ampphase_options;
  struct vis_data **vis_data;
};

struct cache_vis_data cache_vis_data;

struct client_vis_data {
  int num_clients;
  char **client_id;
  struct vis_data **vis_data;
};

struct rpfits_file_information {
  char filename[RPSBUFSIZE];
  int n_scans;
  struct scan_header_data **scan_headers;
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





bool add_cache_vis_data(struct ampphase_options *options,
                        struct vis_data *data) {
  int i, n;
  // Check that we don't already know about the data.
  for (i = 0; i < cache_vis_data.num_cache_vis_data; i++) {
    if (ampphase_options_match(options,
                               cache_vis_data.ampphase_options[i])) {
      return false;
    }
  }
  // If we get here, this is new data.
  n = cache_vis_data.num_cache_vis_data + 1;
  REALLOC(cache_vis_data.ampphase_options, n);
  REALLOC(cache_vis_data.vis_data, n);
  MALLOC(cache_vis_data.ampphase_options[n - 1], 1);
  set_default_ampphase_options(cache_vis_data.ampphase_options[n - 1]);
  copy_ampphase_options(cache_vis_data.ampphase_options[n - 1], options);
  MALLOC(cache_vis_data.vis_data[n - 1], 1);
  copy_vis_data(cache_vis_data.vis_data[n - 1], data);
  cache_vis_data.num_cache_vis_data = n;

  return true;
}

bool get_cache_vis_data(struct ampphase_options *options,
			struct vis_data **data) {
  int i;
  for (i = 0; i < cache_vis_data.num_cache_vis_data; i++) {
    if (ampphase_options_match(options,
			       cache_vis_data.ampphase_options[i])) {
      if (*data == NULL ) {
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

void data_reader(int read_type, int n_rpfits_files,
                 double mjd_required, struct ampphase_options *ampphase_options,
                 struct rpfits_file_information **info_rpfits_files,
                 struct spectrum_data **spectrum_data,
                 struct vis_data **vis_data) {
  int i, res, n, calcres, curr_header, idx_if, idx_pol;
  int pols[4] = { POL_XX, POL_YY, POL_XY, POL_YX };
  bool open_file, keep_reading, header_free, read_cycles, keep_cycling;
  bool cycle_free, spectrum_return, vis_cycled, cache_hit_vis_data;
  double cycle_mjd, cycle_start, cycle_end, half_cycle;
  struct scan_header_data *sh = NULL;
  struct cycle_data *cycle_data = NULL;
  struct spectrum_data *temp_spectrum = NULL;
  struct ampphase_options *local_ampphase_options = NULL;

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

    cache_hit_vis_data = get_cache_vis_data(ampphase_options, vis_data);
    if (cache_hit_vis_data == false) {
      printf("[data_reader] no cache hit\n");
      if ((vis_data != NULL) && (*vis_data == NULL)) {
        // Need to allocate some memory.
        MALLOC(*vis_data, 1);
      }
      (*vis_data)->nviscycles = 0;
      (*vis_data)->header_data = NULL;
      (*vis_data)->num_ifs = NULL;
      (*vis_data)->num_pols = NULL;
      (*vis_data)->vis_quantities = NULL;
    } else {
      printf("[data_reader] found cache hit\n");
      read_type -= COMPUTE_VIS_PRODUCTS;
    }
  }
  
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
      half_cycle = (double)info_rpfits_files[i]->scan_headers[0]->cycle_time / (2.0 * 86400.0);
      if ((mjd_required >= (info_rpfits_files[i]->scan_start_mjd[0] - half_cycle)) &&
          (mjd_required <= (info_rpfits_files[i]->scan_end_mjd[info_rpfits_files[i]->n_scans - 1]
                            + half_cycle))) {
        open_file = true;
        /* } else { */
        /* 	printf("File %s does not cover the requested MJD %.6f\n", */
        /* 	       info_rpfits_files[i]->filename, mjd_required); */
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
      if (sh->ut_seconds > 0) {
        curr_header += 1;
        if (read_type & READ_SCAN_METADATA) {
          // Keep track of the times covered by each scan.
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
            }
            // HERE WILL GO THE LOGIC TO WORK OUT IF WE WANT TO DO
            // SOMETHING WITH THIS CYCLE
            cycle_mjd = date2mjd(sh->obsdate, cycle_data->ut_seconds);
            if (read_type & READ_SCAN_METADATA) {
              info_rpfits_files[i]->scan_end_mjd[n] = cycle_mjd;
            }
            if ((read_type & GRAB_SPECTRUM) ||
                (read_type & COMPUTE_VIS_PRODUCTS)) {
              spectrum_return = false;
              // If we're trying to grab a specific spectrum,
              // we consider this the correct cycle if the requested
              // MJD is within half a cycle time of this cycle's time.
              cycle_start = cycle_mjd - ((double)sh->cycle_time / (2 * 86400.0));
              cycle_end = cycle_mjd + ((double)sh->cycle_time / (2 * 86400.0));
              /* printf("%.6f / %.6f / %.6f\n", cycle_start, mjd_required, cycle_end); */
              if ((read_type & GRAB_SPECTRUM) &&
                  (mjd_required >= cycle_start) &&
                  (mjd_required <= cycle_end)) {
                spectrum_return = true;
              }
              if ((read_type & COMPUTE_VIS_PRODUCTS) ||
                  (spectrum_return)) {
                // We need this cycle.
                // Allocate all the memory we need.
                /* printf("cycle found!\n"); */
                /* fprintf(stderr, "[data_reader] making a temporary spectrum...\n"); */
                MALLOC(temp_spectrum, 1);
                // Prepare the spectrum data structure.
                temp_spectrum->num_ifs = sh->num_ifs;
                temp_spectrum->header_data = info_rpfits_files[i]->scan_headers[curr_header];
                /* fprintf(stderr, "[data_reader] allocating some memory...\n"); */
                MALLOC(temp_spectrum->spectrum, temp_spectrum->num_ifs);
                if (read_type & COMPUTE_VIS_PRODUCTS) {
                  REALLOC((*vis_data)->vis_quantities,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->header_data,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->num_ifs,
                          ((*vis_data)->nviscycles + 1));
                  REALLOC((*vis_data)->num_pols,
                          ((*vis_data)->nviscycles + 1));
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
                  if (read_type & COMPUTE_VIS_PRODUCTS) {
                    (*vis_data)->num_pols[(*vis_data)->nviscycles][idx_if] =
                      temp_spectrum->num_pols;
                    CALLOC((*vis_data)->vis_quantities[(*vis_data)->nviscycles][idx_if],
                           temp_spectrum->num_pols);
                  }
                  for (idx_pol = 0; idx_pol < temp_spectrum->num_pols; idx_pol++) {
                    MALLOC(local_ampphase_options, 1);
		    set_default_ampphase_options(local_ampphase_options);
                    copy_ampphase_options(local_ampphase_options, ampphase_options);
                    /* fprintf(stderr, "[data_reader] calculating amp products\n"); */
                    calcres = vis_ampphase(sh, cycle_data,
                                           &(temp_spectrum->spectrum[idx_if][idx_pol]),
                                           pols[idx_pol], sh->if_label[idx_if],
                                           local_ampphase_options);
                    if (calcres < 0) {
                      fprintf(stderr, "CALCULATING AMP AND PHASE FAILED FOR IF %d POL %d, CODE %d\n",
                              sh->if_label[idx_if], pols[idx_pol], calcres);
                    /* } else if (read_type & COMPUTE_VIS_PRODUCTS) { */
                    /*     printf("CONVERTED SPECTRUM FOR CYCLE IF %d POL %d AT MJD %.6f\n", */
                    /*            sh->if_label[idx_if], pols[idx_pol], cycle_mjd); */
                    }
                    if (read_type & COMPUTE_VIS_PRODUCTS) {
                      MALLOC(local_ampphase_options, 1);
		      set_default_ampphase_options(local_ampphase_options);
                      copy_ampphase_options(local_ampphase_options, ampphase_options);
                      /* fprintf(stderr, "[data_reader] calculating vis products\n"); */
                      ampphase_average(temp_spectrum->spectrum[idx_if][idx_pol],
                                       &((*vis_data)->vis_quantities[(*vis_data)->nviscycles][idx_if][idx_pol]),
                                       local_ampphase_options);
		      // Copy the ampphase_options back.
		      copy_ampphase_options(ampphase_options, local_ampphase_options);
                      /* fprintf(stderr, "[data_reader] calculated vis products\n"); */
                      vis_cycled = true;
                    }
                  }
                }
                if (vis_cycled) {
                  (*vis_data)->nviscycles += 1;
                }
                if (spectrum_return) {
                  // Copy the pointer.
                  *spectrum_data = temp_spectrum;
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
                if (!(read_type & COMPUTE_VIS_PRODUCTS)) {
                  // We don't need to search any more.
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
      if (header_free) {
        free_scan_header_data(sh);
        FREE(sh);
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

#define RPSENDBUFSIZE 104857600

bool sigint_received;

// Handle signals that we receive.
static void sighandler(int sig) {
  if (sig == SIGINT) {
    sigint_received = true;
  }
}

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
      fprintf(stderr, "[add_client_vis_data] client data %s will be replaced at %d\n",
              client_id, n);
      break;
    }
  }
  
  // Store some vis data as identified by a client id.
  if (n >= client_vis_data->num_clients) {
    fprintf(stderr, "[add_client_vis_data] making new client cache %s\n",
            client_id);
    REALLOC(client_vis_data->client_id, (n + 1));
    MALLOC(client_vis_data->client_id[n], CLIENTIDLENGTH);
    REALLOC(client_vis_data->vis_data, (n + 1));
    MALLOC(client_vis_data->vis_data[n], 1);
    client_vis_data->num_clients = (n + 1);
  }
  strncpy(client_vis_data->client_id[n], client_id, CLIENTIDLENGTH);
  copy_vis_data(client_vis_data->vis_data[n], vis_data);
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

int main(int argc, char *argv[]) {
  struct arguments arguments;
  int i, j, k, l, ri, rj, bytes_received, r;
  bool pointer_found = false, cache_updated = false;
  double mjd_grab;
  struct rpfits_file_information **info_rpfits_files = NULL;
  struct ampphase_options *ampphase_options = NULL, *client_options = NULL;
  struct spectrum_data *spectrum_data;
  struct vis_data *vis_data = NULL, *child_vis_data = NULL;
  FILE *fh = NULL;
  cmp_ctx_t cmp, child_cmp;
  cmp_mem_access_t mem, child_mem;
  struct addrinfo hints, *bind_address;
  char port_string[RPSBUFSIZE], address_buffer[RPSBUFSIZE];
  char *recv_buffer = NULL, *send_buffer = NULL, *child_send_buffer = NULL;
  SOCKET socket_listen, max_socket, loop_i, socket_client, child_socket;
  SOCKET alert_socket;
  fd_set master, reads;
  struct sockaddr_storage client_address;
  socklen_t client_len;
  struct requests client_request, child_request;
  struct responses client_response;
  size_t recv_buffer_length;
  ssize_t bytes_sent;
  struct client_vis_data client_vis_data;
  pid_t pid;
  struct client_sockets clients;
  
  // Set the defaults for the arguments.
  arguments.n_rpfits_files = 0;
  arguments.rpfits_files = NULL;
  arguments.port_number = 8880;
  arguments.network_operation = false;

  // And the default for the calculator options.
  MALLOC(ampphase_options, 1);
  set_default_ampphase_options(ampphase_options);
  
  // Parse the arguments.
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Stop here if we don't have any RPFITS files.
  if (arguments.n_rpfits_files == 0) {
    fprintf(stderr, "NO RPFITS FILES SPECIFIED, EXITING\n");
    return(-1);
  }

  // Reset our cache.
  cache_vis_data.num_cache_vis_data = 0;
  cache_vis_data.ampphase_options = NULL;
  cache_vis_data.vis_data = NULL;

  // And initialise the clients.
  client_vis_data.num_clients = 0;
  client_vis_data.client_id = NULL;
  client_vis_data.vis_data = NULL;
  clients.num_sockets = 0;
  clients.socket = NULL;
  clients.client_id = NULL;
  
  // Set up our signal handler.
  signal(SIGINT, sighandler);
  // Ignore the deaths of our children (they won't zombify).
  signal(SIGCHLD, SIG_IGN);
  
  // Do a scan of each RPFITS file to get time information.
  MALLOC(info_rpfits_files, arguments.n_rpfits_files);
  // Put the names of the RPFITS files into the info structure.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    info_rpfits_files[i] = new_rpfits_file();
    strncpy(info_rpfits_files[i]->filename, arguments.rpfits_files[i], RPSBUFSIZE);
  }
  data_reader(READ_SCAN_METADATA, arguments.n_rpfits_files, 0.0, ampphase_options,
              info_rpfits_files, &spectrum_data, &vis_data);

  // Print out the summary.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    printf("RPFITS FILE: %s (%d scans):\n",
           info_rpfits_files[i]->filename, info_rpfits_files[i]->n_scans);
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      printf("  scan %d (%s, %s) MJD range %.6f -> %.6f\n", (j + 1),
             info_rpfits_files[i]->scan_headers[j]->source_name,
             info_rpfits_files[i]->scan_headers[j]->obstype,
             info_rpfits_files[i]->scan_start_mjd[j],
             info_rpfits_files[i]->scan_end_mjd[j]);
    }
    printf("\n");
  }

  // Let's try to load one of the spectra at startup, so we can send
  // something if required to.
  printf("Preparing for operation...\n");
  srand(time(NULL));
  ri = rand() % arguments.n_rpfits_files;
  rj = rand() % info_rpfits_files[ri]->n_scans;
  mjd_grab = info_rpfits_files[ri]->scan_start_mjd[rj] + 10.0 / 86400.0;
  printf(" grabbing from random scan %d from file %d, MJD %.6f\n", rj, ri, mjd_grab);
  data_reader(GRAB_SPECTRUM | COMPUTE_VIS_PRODUCTS, arguments.n_rpfits_files, mjd_grab,
              ampphase_options, info_rpfits_files, &spectrum_data, &vis_data);
  // This first grab of the vis_data goes into the default client slot.
  add_client_vis_data(&client_vis_data, "DEFAULT", vis_data);
  cache_updated = add_cache_vis_data(ampphase_options, vis_data);
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

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
      fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
      return(1);
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
      r = select(max_socket + 1, &reads, 0, 0, 0);
      if ((r < 0) && (errno != EINTR)) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        break;
      }
      // Check we got a valid selection.
      if (sigint_received == true) {
        sigint_received = false;
        break;
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
              printf("Closing connection, no data received.\n");
              // The connection failed.
              FD_CLR(loop_i, &master);
              CLOSESOCKET(loop_i);
              FREE(recv_buffer);
              continue;
            }
            printf("Received %d bytes.\n", bytes_received);
            init_cmp_memory_buffer(&cmp, &mem, recv_buffer, recv_buffer_length);
            unpack_requests(&cmp, &client_request);

            // Do what we've been asked to do.
            printf(" %s from client %s.\n",
                   get_type_string(TYPE_REQUEST, client_request.request_type),
                   client_request.client_id);
            // Add this client to our list.
            add_client(&clients, client_request.client_id, loop_i);
            if ((client_request.request_type == REQUEST_CURRENT_SPECTRUM) ||
                (client_request.request_type == REQUEST_CURRENT_VISDATA) ||
                (client_request.request_type == REQUEST_COMPUTED_VISDATA)) {
              // We're going to send the currently cached data to this socket.
              // Make the buffers the size we may need.
              MALLOC(send_buffer, RPSENDBUFSIZE);

              // Set up the response.
              if (client_request.request_type == REQUEST_CURRENT_SPECTRUM) {
                client_response.response_type = RESPONSE_CURRENT_SPECTRUM;
              } else if (client_request.request_type == REQUEST_CURRENT_VISDATA) {
                client_response.response_type = RESPONSE_CURRENT_VISDATA;
              } else if (client_request.request_type == REQUEST_COMPUTED_VISDATA) {
                client_response.response_type = RESPONSE_COMPUTED_VISDATA;
              }
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);

              // Now move to writing to the send buffer.
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, (size_t)RPSENDBUFSIZE);
              pack_responses(&cmp, &client_response);
              if (client_request.request_type == REQUEST_CURRENT_SPECTRUM) {
                pack_spectrum_data(&cmp, spectrum_data);
              } else if (client_request.request_type == REQUEST_CURRENT_VISDATA) {
                pack_vis_data(&cmp, get_client_vis_data(&client_vis_data, "DEFAULT"));
              } else if (client_request.request_type == REQUEST_COMPUTED_VISDATA) {
                pack_vis_data(&cmp, get_client_vis_data(&client_vis_data,
                                                        client_request.client_id));
              }

              // Send this data.
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_request.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer, cmp_mem_access_get_pos(&mem));
            } else if (client_request.request_type == REQUEST_COMPUTE_VISDATA) {
              // We've been asked to recompute vis data with a different set of options.
              // Get the options.
              MALLOC(client_options, 1);
              unpack_ampphase_options(&cmp, client_options);
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
                            client_options, info_rpfits_files, &spectrum_data, &child_vis_data);
                // We send the data back to our parent over the network.
                if (prepare_client_connection("localhost", arguments.port_number,
                                              &child_socket, false)) {
                  // We have a connection.
                  child_request.request_type = CHILDREQUEST_VISDATA_COMPUTED;
                  strncpy(child_request.client_id, client_request.client_id, CLIENTIDLENGTH);
                  MALLOC(child_send_buffer, RPSENDBUFSIZE);
                  init_cmp_memory_buffer(&child_cmp, &child_mem, child_send_buffer,
                                         (size_t)RPSENDBUFSIZE);
                  pack_requests(&child_cmp, &child_request);
                  pack_ampphase_options(&child_cmp, client_options);
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
		free_ampphase_options(client_options);
                FREE(client_options);
                // Die.
                printf("CHILD IS FINISHED!\n");
                exit(0);
              } else {
		free_ampphase_options(client_options);
                FREE(client_options);
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
	      // Free the current ampphase options.
	      free_ampphase_options(ampphase_options);
              unpack_ampphase_options(&cmp, ampphase_options);

              fprintf(stderr, "[PARENT] unpacking vis data\n");
              unpack_vis_data(&cmp, vis_data);

              // Add this data to our cache.
              fprintf(stderr, "[PARENT] adding data to cache\n");
              cache_updated = add_cache_vis_data(ampphase_options, vis_data);
	      if (cache_updated == false) {
		// The unpacked data can be freed.
		// This includes the header data.
		for (i = 0; i < vis_data->nviscycles; i++) {
		  free_scan_header_data(vis_data->header_data[i]);
		  FREE(vis_data->header_data[i]);
		}
		free_vis_data(vis_data);
		// Now get the cached vis data so we don't repeatedly store
		// new data into the client slot.
		get_cache_vis_data(ampphase_options, &vis_data);
	      }
              add_client_vis_data(&client_vis_data, client_request.client_id, vis_data);

              // Tell the client that their data is ready.
              // Find the client's socket.
              alert_socket = find_client(&clients, client_request.client_id);
              if (ISVALIDSOCKET(alert_socket)) {
                // Craft a response.
                client_response.response_type = RESPONSE_VISDATA_COMPUTED;
                strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
                MALLOC(send_buffer, JUSTRESPONSESIZE);
                init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
                pack_responses(&cmp, &client_response);
                printf(" %s to client %s.\n",
                       get_type_string(TYPE_RESPONSE, client_response.response_type),
                       client_request.client_id);
                bytes_sent = socket_send_buffer(alert_socket, send_buffer,
                                                cmp_mem_access_get_pos(&mem));
                FREE(send_buffer);
              }
            } else if (client_request.request_type == REQUEST_SERVERTYPE) {
              // Tell the client we're a simulator.
              client_response.response_type = RESPONSE_SERVERTYPE;
              strncpy(client_response.client_id, client_request.client_id, CLIENTIDLENGTH);
              MALLOC(send_buffer, JUSTRESPONSESIZE);
              init_cmp_memory_buffer(&cmp, &mem, send_buffer, JUSTRESPONSESIZE);
              pack_responses(&cmp, &client_response);
              pack_write_sint(&cmp, SERVERTYPE_SIMULATOR);
              printf(" %s to client %s.\n",
                     get_type_string(TYPE_RESPONSE, client_response.response_type),
                     client_request.client_id);
              bytes_sent = socket_send_buffer(loop_i, send_buffer,
                                              cmp_mem_access_get_pos(&mem));
              FREE(send_buffer);
            }
            printf(" Sent %ld bytes\n", bytes_sent);
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
    free_ampphase_options(cache_vis_data.ampphase_options[l]);
    FREE(cache_vis_data.ampphase_options[l]);

  }
  FREE(cache_vis_data.vis_data);
  FREE(cache_vis_data.ampphase_options);

  // Free the spectrum memory.
  for (i = 0; i < spectrum_data->num_ifs; i++) {
    for (j = 0; j < spectrum_data->num_pols; j++) {
      free_ampphase(&(spectrum_data->spectrum[i][j]));
    }
    FREE(spectrum_data->spectrum[i]);
  }
  FREE(spectrum_data->spectrum);
  FREE(spectrum_data);

  // We're finished, free all our memory.
  for (i = 0; i < arguments.n_rpfits_files; i++) {
    for (j = 0; j < info_rpfits_files[i]->n_scans; j++) {
      free_scan_header_data(info_rpfits_files[i]->scan_headers[j]);
      FREE(info_rpfits_files[i]->scan_headers[j]);
    }
    FREE(info_rpfits_files[i]->scan_headers);
    FREE(info_rpfits_files[i]->scan_start_mjd);
    FREE(info_rpfits_files[i]->scan_end_mjd);
    FREE(info_rpfits_files[i]);
  }
  FREE(info_rpfits_files);
  FREE(arguments.rpfits_files);

  free_ampphase_options(ampphase_options);
  FREE(ampphase_options);
  FREE(vis_data);
  
  // Free the clients.
  for (i = 0; i < client_vis_data.num_clients; i++) {
    FREE(client_vis_data.client_id[i]);
    FREE(client_vis_data.vis_data[i]);
  }
  FREE(client_vis_data.client_id);
  FREE(client_vis_data.vis_data);
  free_client_sockets(&clients);
  
  if (arguments.network_operation) {
    // Close our socket.
    printf("\n\nClosing listening socket...\n");
    CLOSESOCKET(socket_listen);
  }

  
}
