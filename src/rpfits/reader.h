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
#define SIDEBAND(f) if_.if_invert[f]
#define CHAIN(f) if_.if_chain[f]
#define LABEL(f) if_.if_num[f]
#define CSTOKES(f,s) names_.if_cstok + (s * 2 + f * 4 * 2)
#define ANTNUM(a) anten_.ant_num[a]
#define ANTSTATION(a) names_.sta + (a * 8)
#define ANTX(a) doubles_.x[a]
#define ANTY(a) doubles_.y[a]
#define ANTZ(a) doubles_.z[a]

/* These macros allow for easier interrogation of the SYSCAL table. */
#define SYSCAL_NUM_ANTS   sc_.sc_ant
#define SYSCAL_NUM_IFS    sc_.sc_if
#define SYSCAL_NUM_PARAMS sc_.sc_q
#define SYSCAL_GRP(a, i) (a * (SYSCAL_NUM_IFS * SYSCAL_NUM_PARAMS) + i * SYSCAL_NUM_PARAMS)
#define SYSCAL_PARAM(a, i, o) (sc_.sc_cal[SYSCAL_GRP(a, i) + o])
#define SYSCAL_ANT(a, i) (int)SYSCAL_PARAM(a, i, 0)
#define SYSCAL_IF(a, i) (int)SYSCAL_PARAM(a, i, 1)
#define SYSCAL_XYPHASE(a, i) SYSCAL_PARAM(a, i, 2)
#define SYSCAL_TSYS_X(a, i) fabsf(SYSCAL_PARAM(a, i, 3))
#define SYSCAL_TSYS_Y(a, i) fabsf(SYSCAL_PARAM(a, i, 4))
#define SYSCAL_TSYS_X_APPLIED(a, i) (SYSCAL_PARAM(a, i, 3) < 0) ? 0 : 1
#define SYSCAL_TSYS_Y_APPLIED(a, i) (SYSCAL_PARAM(a, i, 4) < 0) ? 0 : 1
#define SYSCAL_GTP_X(a, i) SYSCAL_PARAM(a, i, 5)
#define SYSCAL_SDO_X(a, i) SYSCAL_PARAM(a, i, 6)
#define SYSCAL_CALJY_X(a, i) SYSCAL_PARAM(a, i, 7)
#define SYSCAL_GTP_Y(a, i) SYSCAL_PARAM(a, i, 8)
#define SYSCAL_SDO_Y(a, i) SYSCAL_PARAM(a, i, 9)
#define SYSCAL_CALJY_Y(a, i) SYSCAL_PARAM(a, i, 10)
#define SYSCAL_PARANGLE(a, i) SYSCAL_PARAM(a, i, 11)
#define SYSCAL_FLAG_BAD(a, i) (int)SYSCAL_PARAM(a, i, 12)
#define SYSCAL_XYAMP(a, i) SYSCAL_PARAM(a, i, 13)
#define SYSCAL_TRACKERR_MAX(a, i) SYSCAL_PARAM(a, i, 14)
#define SYSCAL_TRACKERR_RMS(a, i) SYSCAL_PARAM(a, i, 15)
/* These next macros will work if SYSCAL_ADDITIONAL_CHECK returns 0. */
#define SYSCAL_ADDITIONAL_CHECK (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 0)
#define SYSCAL_ADDITIONAL_TEMP SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 1)
#define SYSCAL_ADDITIONAL_AIRPRESS SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 2)
#define SYSCAL_ADDITIONAL_HUMI SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 3)
#define SYSCAL_ADDITIONAL_WINDSPEED SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 4)
#define SYSCAL_ADDITIONAL_WINDDIR SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 5)
#define SYSCAL_ADDITIONAL_WEATHERFLAG (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 6)
#define SYSCAL_ADDITIONAL_RAIN SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 7)
#define SYSCAL_ADDITIONAL_SEEMON_PHASE SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 8)
#define SYSCAL_ADDITIONAL_SEEMON_RMS SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 9)
#define SYSCAL_ADDITIONAL_SEEMON_FLAG (int)SYSCAL_PARAM((SYSCAL_NUM_ANTS - 1), (SYSCAL_NUM_IFS - 1), 10)

// Return values from the read routines.
#define READER_EXHAUSTED 0
#define READER_DATA_AVAILABLE 1
#define READER_HEADER_AVAILABLE 2

int size_of_vis(void);
int size_of_if_vis(int if_no);
int max_size_of_vis(void);
int open_rpfits_file(char *filename);
int close_rpfits_file(void);
void string_copy(char *start, int length, char *dest);
void get_card_value(char *header_name, char *value, int value_maxlength);
int read_scan_header(struct scan_header_data *scan_header_data);
struct cycle_data* prepare_new_cycle_data(void);
struct scan_data* prepare_new_scan_data(void);
struct cycle_data* scan_add_cycle(struct scan_data *scan_data);
void free_scan_header_data(struct scan_header_data *scan_header_data);
void free_cycle_data(struct cycle_data *cycle_data);
void free_scan_data(struct scan_data *scan_data);
int read_cycle_data(struct scan_header_data *scan_header_data,
		    struct cycle_data *cycle_data);
//int generate_rpfits_index(struct rpfits_index *rpfits_index);
