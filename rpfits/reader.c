#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "RPFITS.h"
#include "reader.h"

/**
 * ATCA Training Library: reader.c
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

/**
 * Routine to convert an RPFITS baseline number into the component antennas.
 */
void base_to_ants(int baseline,int *ant1,int *ant2){
  /* the baseline number is just 256*a1 + a2, where
     a1 is antenna 1, and a2 is antenna 2, and a1<=a2 */

  *ant2 = baseline % 6;
  *ant1 = (baseline - *ant2) / 256;
  
}

/**
 * Routine to assess how large the visibility set is.
 * Unfortunately, RPFITS makes a bunch of data visible via global variables,
 * which is why we don't take any arguments.
 */
int size_of_vis(void) {
  int i, vis_size = 0;

  for (i = 0; i < if_.n_if; i++){
    vis_size += ((if_.if_nstok[i] + 1) * (if_.if_nfreq[i] + 1));
  }

  return(vis_size);
}

/**
 * This routine attempts to open an RPFITS file.
 */
int open_rpfits_file(char *filename) {
  int this_jstat = JSTAT_UNSUCCESSFUL, rpfits_result = 0;
  size_t flen = 0;
  
  // We load in the name of the file to read.
  flen = strlen(filename);
  if (flen > 0) {
    strncpy(names_.file, filename, flen);
    this_jstat = JSTAT_OPENFILE;
    rpfits_result = rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
			      NULL, NULL, NULL, NULL, NULL, NULL);
  }
  if (this_jstat == JSTAT_UNSUCCESSFUL) {
    fprintf(stderr, "Cannot open RPFITS file %s\n", filename);
  }

  return(this_jstat);
}

/**
 * This routine attempts to close the currently open RPFITS file.
 */
int close_rpfits_file(void) {
  int this_jstat = JSTAT_CLOSEFILE, rpfits_result = 0;

  rpfits_result = rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL, NULL);

  return(this_jstat);
}

/**
 * Routine to copy some length of string from a pointer, and ensure
 * it's terminated correctly.
 */
void string_copy(char *start, int length, char *dest) {
  (void)strncpy(dest, start, length);
  dest[length - 1] = '\0';
};

/**
 * Routine to read a single cycle from the open file and return some
 * parameters.
 * The return value is whether the file has more data in it after this cycle.
 */
int read_scan_header(struct scan_header_data *scan_header_data) {
  int keep_reading = READER_HEADER_AVAILABLE, this_jstat = 0, that_jstat = 0;
  int rpfits_result = 0;
  int vis_size = 0, flag = 0, bin = 0, if_no = 0, sourceno = 0, baseline = 0;
  int i = 0, read_data = 0;
  float *vis = NULL, *wgt = NULL, ut = 0, u = 0, v = 0, w = 0;
  
  this_jstat = JSTAT_READNEXTHEADER;
  rpfits_result = rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL, NULL);
  if (this_jstat == JSTAT_SUCCESSFUL) {
    // We can read the data in this header.

    // Get the date of the observation.
    string_copy(names_.datobs, OBSDATE_LENGTH, scan_header_data->obsdate);

    // Read the data.
    that_jstat = JSTAT_READDATA;
    rpfits_result = rpfitsin_(&that_jstat, NULL, NULL, &baseline, &ut,
			      &u, &v, &w, &flag, &bin, &if_no, &sourceno);

    // Put all the data into the structure.
    scan_header_data->ut_seconds = ut;
    string_copy(names_.obstype, OBSTYPE_LENGTH, scan_header_data->obstype);
    string_copy(CALCODE(sourceno), CALCODE_LENGTH, scan_header_data->calcode);
    scan_header_data->cycle_time = param_.intime;

    string_copy(SOURCENAME(sourceno), SOURCE_LENGTH, scan_header_data->source_name);
    scan_header_data->rightascension_hours = RIGHTASCENSION(sourceno) * 180 /
      (15 * M_PI);
    scan_header_data->declination_degrees = DECLINATION(sourceno) * 180 / M_PI;

    scan_header_data->num_ifs = if_.n_if;
    scan_header_data->if_centre_freq = (float *)malloc(scan_header_data->num_ifs *
						 sizeof(float));
    scan_header_data->if_bandwidth = (float *)malloc(scan_header_data->num_ifs *
					       sizeof(float));
    scan_header_data->if_num_channels = (int *)malloc(scan_header_data->num_ifs *
						sizeof(int));
    scan_header_data->if_num_stokes = (int *)malloc(scan_header_data->num_ifs *
					      sizeof(int));
    for (i = 0; i < scan_header_data->num_ifs; i++) {
      scan_header_data->if_centre_freq[i] = FREQUENCYMHZ(i);
      scan_header_data->if_bandwidth[i] = BANDWIDTHMHZ(i);
      scan_header_data->if_num_channels[i] = NCHANNELS(i);
      scan_header_data->if_num_stokes[i] = NSTOKES(i);
    }

    // We read the data if there is data to read.
    if ((scan_header_data->if_num_stokes[0] *
	 scan_header_data->if_num_channels[0]) > 0) {
      // Allocate some memory for the data.
      //vis = (float *)realloc(vis, 2 * vis_size * sizeof(float));
      //wgt = (float *)realloc(wgt, 2 * vis_size * sizeof(float));
      keep_reading = keep_reading | READER_DATA_AVAILABLE;
    }

    
  } else if (this_jstat == JSTAT_ENDOFFILE) {
    keep_reading = READER_EXHAUSTED;
  }

  return(keep_reading);
}

/**
 * This routine makes an empty cycle_data structure ready to be
 * filled by the reader.
 */
struct cycle_data* prepare_new_cycle_data(void) {
  struct cycle_data *cycle_data = NULL;

  cycle_data = (struct cycle_data *)malloc(sizeof(struct cycle_data));
  cycle_data->ut_seconds = 0;
  cycle_data->num_points = 0;
  cycle_data->u = NULL;
  cycle_data->v = NULL;
  cycle_data->w = NULL;
  cycle_data->ant1 = NULL;
  cycle_data->ant2 = NULL;

  return(cycle_data);
}

/**
 * This routine makes an empty scan_data structure ready to be
 * filled by the reader.
 */
struct scan_data* prepare_new_scan_data(void) {
  struct scan_data *scan_data = NULL;

  scan_data = (struct scan_data*)malloc(sizeof(struct scan_data));
  
  scan_data->num_cycles = 0;
  scan_data->cycles = NULL;

  return(scan_data);
}

/**
 * This routine adds a cycle to a scan and returns the cycle.
 */
struct cycle_data* scan_add_cycle(struct scan_data *scan_data) {
  scan_data->num_cycles += 1;
  scan_data->cycles =
    (struct cycle_data**)realloc(scan_data->cycles,
				 scan_data->num_cycles * sizeof(struct cycle_data*));
  scan_data->cycles[scan_data->num_cycles - 1] =
    prepare_new_cycle_data();

  return(scan_data->cycles[scan_data->num_cycles - 1]);
}

/**
 * This routine reads in a full cycle's worth of data.
 */
int read_cycle_data(struct scan_header_data *scan_header_data,
		    struct cycle_data *cycle_data) {
  int this_jstat = JSTAT_READDATA, read_data = 1, rpfits_result = 0;
  int flag, bin, if_no, sourceno, vis_size = 0, rv = READER_HEADER_AVAILABLE;
  int baseline, last_ut = -1;
  float *vis = NULL, *wgt = NULL, ut, u, v, w;

  // Allocate some memory.
  vis_size = size_of_vis();
  vis = (float *)malloc(2 * vis_size * sizeof(float));
  wgt = (float *)malloc(2 * vis_size * sizeof(float));
  
  while (read_data) {
    this_jstat = JSTAT_READDATA;
    rpfits_result = rpfitsin_(&this_jstat, vis, wgt, &baseline, &ut,
			      &u, &v, &w, &flag, &bin, &if_no, &sourceno);
    if (last_ut == -1) {
      // Set it here.
      last_ut = ut;
      cycle_data->ut_seconds = ut;
    }
    
    // Check for success.
    if (this_jstat != JSTAT_SUCCESSFUL) {
      // Ignore illegal data.
      if (this_jstat == JSTAT_ILLEGALDATA) {
	continue;
      }
      // Stop reading.
      read_data = 0;
      if (this_jstat == JSTAT_ENDOFFILE) {
	rv = READER_EXHAUSTED;
      } else if (this_jstat == JSTAT_FGTABLE) {
	// We've hit the end of this data.
	rv = READER_HEADER_AVAILABLE;
      }
    } else {
      /*fprintf(stderr, "jstat = %d ut = %.1f baseline = %d\n", this_jstat,
	      ut, baseline);
      fprintf(stderr, "if_no = %d sourceno = %d flag = %d bin = %d\n",
      if_no, sourceno, flag, bin);*/
      // Check for a time change.
      if ((baseline == -1) && (ut > last_ut)) {
	// We've gone to a new cycle.
	rv = READER_HEADER_AVAILABLE | READER_DATA_AVAILABLE;
	read_data = 0;
      } else {
	// Store this data.
	cycle_data->num_points += 1;
	// Allocate the memory.
	cycle_data->u = (float *)realloc(cycle_data->u,
					 cycle_data->num_points * sizeof(float));
	cycle_data->u[cycle_data->num_points - 1] = u;
	cycle_data->v = (float *)realloc(cycle_data->v,
					 cycle_data->num_points * sizeof(float));
	cycle_data->v[cycle_data->num_points - 1] = v;
	cycle_data->w = (float *)realloc(cycle_data->w,
					 cycle_data->num_points * sizeof(float));
	cycle_data->w[cycle_data->num_points - 1] = w;
	cycle_data->ant1 = (int *)realloc(cycle_data->ant1,
					  cycle_data->num_points * sizeof(int));
	cycle_data->ant2 = (int *)realloc(cycle_data->ant2,
					  cycle_data->num_points * sizeof(int));
	base_to_ants(baseline, &(cycle_data->ant1[cycle_data->num_points - 1]),
		     &(cycle_data->ant2[cycle_data->num_points - 1]));

      }
    }
  }

  // Free our memory.
  free(vis);
  free(wgt);

  return(rv);
  
}
