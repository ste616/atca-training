/**
 * ATCA Training Library: reader.h
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library is designed to read an RPFITS file and for each cycle make available
 * the products which would be available online: spectra, amp, phase, delay,
 * u, v, w, etc.
 * This is so we can make tools which will let observers muck around with what they 
 * would see online in all sorts of real situations, as represented by real data 
 * files.
 * 
 * This module handles reading an RPFITS file.
 */

/* RPFITS commands numerical definitions to make the code more readable */
#define JSTAT_OPENFILE -3
#define JSTAT_OPENFILE_READHEADER -2
#define JSTAT_READNEXTHEADER -1
#define JSTAT_READDATA 0
#define JSTAT_CLOSEFILE 1
#define JSTAT_SKIPTOEND 2

/* RPFITS return codes definitions to make the code more readable */
#define JSTAT_UNSUCCESSFUL -1
#define JSTAT_SUCCESSFUL 0
#define JSTAT_HEADERNOTDATA 1
#define JSTAT_ENDOFSCAN 2
#define JSTAT_ENDOFFILE 3
#define JSTAT_FGTABLE 4
#define JSTAT_ILLEGALDATA 5

#define OBSDATE_LENGTH 12
#define OBSTYPE_LENGTH 16
#define SOURCE_LENGTH 16
#define CALCODE_LENGTH 4
#define NUM_ANTS 6
#define SOURCENAME(s) names_.su_name + (s) * SOURCE_LENGTH
#define RIGHTASCENSION(s) doubles_.su_ra[s]
#define DECLINATION(s) doubles_.su_dec[s]
#define CALCODE(s) names_.su_cal + (s) * CALCODE_LENGTH
#define FREQUENCYMHZ(f) doubles_.if_freq[f] / 1e6
#define BANDWIDTHMHZ(f) doubles_.if_bw[f] / 1e6
#define NCHANNELS(f) if_.if_nfreq[f]
#define NSTOKES(f) if_.if_nstok[f]


// Return values from the read routines.
#define READER_EXHAUSTED 0
#define READER_DATA_AVAILABLE 1
#define READER_HEADER_AVAILABLE 2

// Some check values.
#define RPFITS_FLAG_BAD 1
#define RPFITS_FLAG_GOOD 0

/**
 * This structure holds all the header information data.
 */
struct scan_header_data {
  // Time variables.
  char obsdate[OBSDATE_LENGTH];
  float ut_seconds;
  // Details about the observation.
  char obstype[OBSTYPE_LENGTH];
  char calcode[CALCODE_LENGTH];
  int cycle_time;
  // Name of the source.
  char source_name[SOURCE_LENGTH];
  // Source coordinates.
  float rightascension_hours;
  float declination_degrees;
  // Frequency configuration.
  int num_ifs;
  float *if_centre_freq;
  float *if_bandwidth;
  int *if_num_channels;
  int *if_num_stokes;
};

/**
 * This structure holds all the cycle data.
 */
struct cycle_data {
  // Time variables.
  float ut_seconds;
  // Antennas and baselines.
  int num_points;
  float *u;
  float *v;
  float *w;
  int *ant1;
  int *ant2;
  // Flagging.
  int *flag;
  // Data.
  float **vis;
  float **wgt;
  // Metadata.
  int *bin;
  int *if_no;
  char **source;
};

/**
 * A structure to hold everything together for a scan.
 */
struct scan_data {
  // Needs a header.
  struct scan_header_data header_data;
  // And any number of cycles.
  int num_cycles;
  struct cycle_data **cycles;
};

void base_to_ants(int baseline, int *ant1, int *ant2);
int size_of_vis(void);
int open_rpfits_file(char *filename);
int close_rpfits_file(void);
void string_copy(char *start, int length, char *dest);
int read_scan_header(struct scan_header_data *scan_header_data);
struct cycle_data* prepare_new_cycle_data(void);
struct scan_data* prepare_new_scan_data(void);
struct cycle_data* scan_add_cycle(struct scan_data *scan_data);
void free_scan_header_data(struct scan_header_data *scan_header_data);
void free_cycle_data(struct cycle_data *cycle_data);
void free_scan_data(struct scan_data *scan_data);
int read_cycle_data(struct scan_header_data *scan_header_data,
		    struct cycle_data *cycle_data);
