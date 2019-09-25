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

#pragma once
#include "atrpfits.h"

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

/* Macros to get information. */
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

void base_to_ants(int baseline, int *ant1, int *ant2);
int size_of_vis(void);
int size_of_if_vis(int if_no);
int max_size_of_vis(void);
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
