#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "RPFITS.h"
#include "atrpfits.h"
#include "reader.h"
#include "memory.h"

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
 * Routine to assess how large the visibility set is.
 * Unfortunately, RPFITS makes a bunch of data visible via global variables,
 * which is why we don't take any arguments.
 */
int size_of_vis(void) {
  int i, vis_size = 0;

  for (i = 0; i < if_.n_if; i++){
    vis_size += (if_.if_nstok[i] * if_.if_nfreq[i]);
  }

  return(vis_size);
}

/**
 * Routine to work out how big the visibility set is for
 * a nominated IF.
 */
int size_of_if_vis(int if_no) {
  int vis_size = 0, idx = -1;

  // The vis size is just the number of Stokes parameters
  // multiplied by the number of channels.
  // The if_no is what comes from the rpfitsin_ call, which is
  // 1-indexed, so we subtract 1.
  idx = if_no - 1;
  vis_size = if_.if_nstok[idx] * if_.if_nfreq[idx];

  return(vis_size);
}

/**
 * Routine to work out the maximum size required for a vis array.
 * We need this since we can't get the if_no until after the
 * rpfitsin_ call, but we need the vis allocated before it.
 */
int max_size_of_vis(void) {
  int i = 0, vis_size = 0, max_vis_size = 0;

  for (i = 1; i <= if_.n_if; i++) {
    vis_size = size_of_if_vis(i);
    max_vis_size = (vis_size > max_vis_size) ? vis_size : max_vis_size;
  }

  return(max_vis_size);
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
  int i = 0, read_data = 0, j = 0;
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
    MALLOC(scan_header_data->if_centre_freq, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_bandwidth, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_num_channels, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_num_stokes, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_sideband, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_stokes_names, scan_header_data->num_ifs);
    for (i = 0; i < scan_header_data->num_ifs; i++) {
      scan_header_data->if_centre_freq[i] = FREQUENCYMHZ(i);
      scan_header_data->if_bandwidth[i] = BANDWIDTHMHZ(i);
      scan_header_data->if_num_channels[i] = NCHANNELS(i);
      scan_header_data->if_num_stokes[i] = NSTOKES(i);
      scan_header_data->if_sideband[i] = SIDEBAND(i);
      MALLOC(scan_header_data->if_stokes_names[i], NSTOKES(i));
      for (j = 0; j < NSTOKES(i); j++) {
	MALLOC(scan_header_data->if_stokes_names[i][j], 3);
	(void)strncpy(scan_header_data->if_stokes_names[i][j],
		      CSTOKES(i, j), 2);
	scan_header_data->if_stokes_names[i][j][2] = '\0';
      }
    }

    // We read the data if there is data to read.
    if ((scan_header_data->if_num_stokes[0] *
	 scan_header_data->if_num_channels[0]) > 0) {
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
  int i;
  struct cycle_data *cycle_data = NULL;

  cycle_data = (struct cycle_data *)malloc(sizeof(struct cycle_data));
  cycle_data->ut_seconds = 0;
  cycle_data->num_points = 0;
  cycle_data->u = NULL;
  cycle_data->v = NULL;
  cycle_data->w = NULL;
  cycle_data->ant1 = NULL;
  cycle_data->ant2 = NULL;
  cycle_data->flag = NULL;
  cycle_data->vis = NULL;
  cycle_data->wgt = NULL;
  cycle_data->bin = NULL;
  cycle_data->if_no = NULL;
  cycle_data->source = NULL;

  // Zero the all_baselines array.
  memset(cycle_data->all_baselines, 0, sizeof(cycle_data->all_baselines));
  cycle_data->n_baselines = 0;
  
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
  REALLOC(scan_data->cycles, scan_data->num_cycles);
  scan_data->cycles[scan_data->num_cycles - 1] =
    prepare_new_cycle_data();

  return(scan_data->cycles[scan_data->num_cycles - 1]);
}

/**
 * Free memory from a scan header.
 */
void free_scan_header_data(struct scan_header_data *scan_header_data) {
  FREE(scan_header_data->if_centre_freq);
  FREE(scan_header_data->if_bandwidth);
  FREE(scan_header_data->if_num_channels);
  FREE(scan_header_data->if_num_stokes);
  FREE(scan_header_data->if_sideband);
}

/**
 * Free memory from a cycle.
 */
void free_cycle_data(struct cycle_data *cycle_data) {
  FREE(cycle_data->u);
  FREE(cycle_data->v);
  FREE(cycle_data->w);
  FREE(cycle_data->ant1);
  FREE(cycle_data->ant2);
}

/**
 * Free memory from a scan.
 */
void free_scan_data(struct scan_data *scan_data) {
  int i = 0;

  for (i = 0; i < scan_data->num_cycles; i++) {
    free_cycle_data(scan_data->cycles[i]);
  }
  FREE(scan_data->cycles);
  free_scan_header_data(&(scan_data->header_data));
}

/**
 * This routine reads in a full cycle's worth of data.
 */
int read_cycle_data(struct scan_header_data *scan_header_data,
		    struct cycle_data *cycle_data) {
  int this_jstat = JSTAT_READDATA, read_data = 1, rpfits_result = 0;
  int flag, bin, if_no, sourceno, vis_size = 0, rv = READER_HEADER_AVAILABLE;
  int baseline, ant1, ant2, i, bidx = 1;
  float *vis = NULL, *wgt = NULL, ut, last_ut = -1, u, v, w;
  float complex *cvis = NULL;
  
  while (read_data) {
    // Allocate some memory.
    vis_size = max_size_of_vis();
    MALLOC(vis, 2 * vis_size);
    MALLOC(wgt, vis_size);

    // Read in the data.
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
      /* fprintf(stderr, "jstat = %d ut = %.1f last = %.1f baseline = %d\n", this_jstat, */
      /* 	      ut, last_ut, baseline); */
      /* fprintf(stderr, "if_no = %d sourceno = %d flag = %d bin = %d\n", */
      /* 	      if_no, sourceno, flag, bin); */
      // Check for a time change.
      if ((baseline == -1) && (ut > last_ut)) {
	// We've gone to a new cycle.
	//printf("stopping because new cycle!\n");
	rv = READER_HEADER_AVAILABLE | READER_DATA_AVAILABLE;
	read_data = 0;
      } else {
	// Store this data.
	cycle_data->num_points += 1;
	base_to_ants(baseline, &ant1, &ant2);
	if (cycle_data->all_baselines[baseline] == 0) {
	  cycle_data->all_baselines[baseline] = bidx;
	  bidx += 1;
	}
	ARRAY_APPEND(cycle_data->u, cycle_data->num_points, u);
	ARRAY_APPEND(cycle_data->v, cycle_data->num_points, v);
	ARRAY_APPEND(cycle_data->w, cycle_data->num_points, w);
	ARRAY_APPEND(cycle_data->ant1, cycle_data->num_points, ant1);
	ARRAY_APPEND(cycle_data->ant2, cycle_data->num_points, ant2);
	ARRAY_APPEND(cycle_data->flag, cycle_data->num_points, flag);
	ARRAY_APPEND(cycle_data->bin, cycle_data->num_points, bin);
	ARRAY_APPEND(cycle_data->if_no, cycle_data->num_points, if_no);
	// We have to do something special for the source name.
	REALLOC(cycle_data->source, cycle_data->num_points);
	MALLOC(cycle_data->source[cycle_data->num_points - 1], SOURCE_LENGTH);
	string_copy(SOURCENAME(sourceno), SOURCE_LENGTH,
		    cycle_data->source[cycle_data->num_points - 1]);
	// Convert the vis array read into complex numbers.
	MALLOC(cvis, vis_size);
	for (i = 0; i < vis_size; i++) {
	  cvis[i] = vis[i * 2] + vis[(i * 2) + 1] * I;
	}
	// Keep a pointer to the vis and weight data.
	ARRAY_APPEND(cycle_data->vis, cycle_data->num_points, cvis);
	ARRAY_APPEND(cycle_data->wgt, cycle_data->num_points, wgt);
      }
    }
  }

  cycle_data->n_baselines = bidx - 1;
  
  return(rv);
  
}
