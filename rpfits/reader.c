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
 * Routine to read a single cycle from the open file and return some
 * parameters.
 * The return value is whether the file has more data in it after this cycle.
 */
int read_cycle(struct cycle_data *cycle_data) {
  int keep_reading = 1, this_jstat = 0, rpfits_result = 0;
  int vis_size = 0, flag = 0, bin = 0, if_no = 0, sourceno = 0, baseline = 0;
  float *vis = NULL, *wgt = NULL, ut = 0, u = 0, v = 0, w = 0;
  
  this_jstat = JSTAT_READNEXTHEADER;
  rpfits_result = rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL, NULL);
  if (this_jstat == JSTAT_SUCCESSFUL) {
    // We can read the data in this header.

    // Get the date of the observation.
    (void)strncpy(cycle_data->obsdate, names_.datobs, OBSDATE_LENGTH);
    cycle_data->obsdate[10] = '\0';

    // Allocate some memory.
    vis_size = size_of_vis();
    vis = realloc(vis, vis_size * sizeof(float));
    wgt = realloc(wgt, vis_size * sizeof(float));
    
    // Read the data.
    this_jstat = JSTAT_READDATA;
    rpfits_result = rpfitsin_(&this_jstat, vis, wgt, &baseline, &ut,
			      &u, &v, &w, &flag, &bin, &if_no, &sourceno);

    // Put all the data into the structure.
    (void)strncpy(cycle_data->obstype, names_.obstype, OBSTYPE_LENGTH);
    //(void)strncpy(cycle_data->source_name, SOURCENAME(sourceno), SOURCE_LENGTH);
    (void)strncpy(cycle_data->source_name, names_.su_name + (sourceno - 1) *
		 SOURCE_LENGTH, SOURCE_LENGTH);
    cycle_data->source_name[SOURCE_LENGTH - 1] = '\0';
    
    // Free the memory we allocated.
    free(vis);
    free(wgt);
    
  } else if (this_jstat == JSTAT_ENDOFFILE) {
    keep_reading = 0;
  }

  return(keep_reading);
}
