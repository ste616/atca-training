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
#define SOURCENAME(s) names_.su_name + ((s) - 1) * SOURCE_LENGTH

/**
 * This structure holds all the data from a single cycle.
 */
struct cycle_data {
  char obsdate[OBSDATE_LENGTH];
  char obstype[OBSTYPE_LENGTH];
  char source_name[SOURCE_LENGTH];
  //char source_name[SOURCE_NAME_LENGTH];
};

void base_to_ants(int baseline, int *ant1, int *ant2);
int size_of_vis(void);
int open_rpfits_file(char *filename);
int close_rpfits_file(void);
int read_cycle(struct cycle_data *cycle_data);
