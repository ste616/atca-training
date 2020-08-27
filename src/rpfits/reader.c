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
  int this_jstat = JSTAT_UNSUCCESSFUL;
  size_t flen = 0;
  
  // We load in the name of the file to read.
  flen = strlen(filename);
  if (flen > 0) {
    strncpy(names_.file, filename, flen);
    this_jstat = JSTAT_OPENFILE;
    rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
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
  int this_jstat = JSTAT_CLOSEFILE;

  rpfitsin_(&this_jstat, NULL, NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL, NULL, NULL);
  if (this_jstat == JSTAT_UNSUCCESSFUL) {
    fprintf(stderr, "Problem when closing RPFITS file\n");
  }
  
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

void get_card_value(char *header_name, char *value, int value_maxlength) {
  int i, j;
  char *firstptr = NULL;
  
  for (i = 0; i < param_.ncard; i++) {
    firstptr = names_.card + i * 80;
    if (strncmp(firstptr, header_name, strlen(header_name)) == 0) {
      // Found the thing.
      // Find the first valid character.
      firstptr += strlen(header_name);
      while ((*firstptr == ' ') || (*firstptr == '=')) firstptr++;
      strncpy(value, firstptr, value_maxlength);
      value[value_maxlength - 1] = 0;

      // Remove trailing unnecessary characters.
      for (j = (value_maxlength - 2); j >= 0; j--) {
	if (value[j] == ' ') {
	  value[j] = 0;
	} else {
	  break;
	}
      }
      break;
    }
  }
}

/**
 * Routine to read a single cycle from the open file and return some
 * parameters.
 * The return value is whether the file has more data in it after this cycle.
 */
int read_scan_header(struct scan_header_data *scan_header_data) {
  int keep_reading = READER_HEADER_AVAILABLE, this_jstat = 0, that_jstat = 0;
  int rpfits_result = 0;
  int flag = 0, bin = 0, if_no = 0, sourceno = 0, baseline = 0;
  int i = 0, j = 0, *nfound_chain = NULL, nfound_if = 0, zn = 0;
  float ut = 0, u = 0, v = 0, w = 0;
  //char *ptr = NULL;
  
  this_jstat = JSTAT_READNEXTHEADER;
  // Get the cards so we can search for the scan type.
  param_.ncard = -1;
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
    get_card_value("SCANTYPE", scan_header_data->obstype, OBSTYPE_LENGTH);
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
    MALLOC(scan_header_data->if_chain, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_label, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_name, scan_header_data->num_ifs);
    MALLOC(scan_header_data->if_stokes_names, scan_header_data->num_ifs);
    
    for (i = 0; i < scan_header_data->num_ifs; i++) {
      scan_header_data->if_centre_freq[i] = FREQUENCYMHZ(i);
      scan_header_data->if_bandwidth[i] = BANDWIDTHMHZ(i);
      scan_header_data->if_num_channels[i] = NCHANNELS(i);
      scan_header_data->if_num_stokes[i] = NSTOKES(i);
      scan_header_data->if_sideband[i] = SIDEBAND(i);
      scan_header_data->if_chain[i] = CHAIN(i);
      scan_header_data->if_label[i] = LABEL(i);
      MALLOC(scan_header_data->if_name[i], 3);
      // The first name is the simple indicator like f1 or f3.
      MALLOC(scan_header_data->if_name[i][0], 8);
      (void)snprintf(scan_header_data->if_name[i][0], 8,
		     "f%d", scan_header_data->if_label[i]);
      // The second name classifies the band into f and z.
      MALLOC(scan_header_data->if_name[i][1], 8);
      // The third name is the complicated one to work out which
      // IF each zoom corresponds to.
      MALLOC(scan_header_data->if_name[i][2], 8);
      if (scan_header_data->if_chain[i] > nfound_if) {
        REALLOC(nfound_chain, scan_header_data->if_chain[i]);
        for (j = nfound_if; j < scan_header_data->if_chain[i]; j++) {
          nfound_chain[j] = 0;
        }
        nfound_if = scan_header_data->if_chain[i];
      }
      if (nfound_chain[scan_header_data->if_chain[i] - 1] == 0) {
        // This is the first one in this chain, it is the wideband.
        (void)snprintf(scan_header_data->if_name[i][1], 8,
                       "f%d", scan_header_data->if_chain[i]);
        (void)strncpy(scan_header_data->if_name[i][2],
                      scan_header_data->if_name[i][1], 8);
      } else {
        // This must be a zoom.
        zn++;
        (void)snprintf(scan_header_data->if_name[i][1], 8,
                       "z%d", zn);
        (void)snprintf(scan_header_data->if_name[i][2], 8,
                       "z%d-%d", scan_header_data->if_chain[i],
                       nfound_chain[scan_header_data->if_chain[i] - 1]);
      }
      nfound_chain[scan_header_data->if_chain[i] - 1] += 1;
      
      MALLOC(scan_header_data->if_stokes_names[i], NSTOKES(i));
      for (j = 0; j < NSTOKES(i); j++) {
        MALLOC(scan_header_data->if_stokes_names[i][j], 3);
        (void)strncpy(scan_header_data->if_stokes_names[i][j],
                      CSTOKES(i, j), 2);
        scan_header_data->if_stokes_names[i][j][2] = '\0';
      }
    }
    scan_header_data->num_ants = anten_.nant;
    MALLOC(scan_header_data->ant_label, scan_header_data->num_ants);
    MALLOC(scan_header_data->ant_name, scan_header_data->num_ants);
    MALLOC(scan_header_data->ant_cartesian, scan_header_data->num_ants);
    for (i = 0; i < scan_header_data->num_ants; i++) {
      scan_header_data->ant_label[i] = ANTNUM(i);
      MALLOC(scan_header_data->ant_name[i], 9);
      scan_header_data->ant_name[i][8] = '\0';
      (void)strncpy(scan_header_data->ant_name[i],
		    ANTSTATION(i), 8);
      MALLOC(scan_header_data->ant_cartesian[i], 3);
      scan_header_data->ant_cartesian[i][0] = ANTX(i);
      scan_header_data->ant_cartesian[i][1] = ANTY(i);
      scan_header_data->ant_cartesian[i][2] = ANTZ(i);
    }

    // We read the data if there is data to read.
    if ((scan_header_data->if_num_stokes[0] *
         scan_header_data->if_num_channels[0]) > 0) {
      keep_reading = keep_reading | READER_DATA_AVAILABLE;
    }

    
  } else if (this_jstat == JSTAT_ENDOFFILE) {
    keep_reading = READER_EXHAUSTED;
  } else if (this_jstat == JSTAT_FGTABLE) {
    // We've found the flagging table.
    printf("READER: found %d flag groups\n", fg_.n_fg);
    
  } else if (rpfits_result == 0) {
    fprintf(stderr, "While reading header, rpfitsin encountered an error\n");
  }

  FREE(nfound_chain);
  
  return(keep_reading);
}

/**
 * This routine makes an empty cycle_data structure ready to be
 * filled by the reader.
 */
struct cycle_data* prepare_new_cycle_data(void) {
  struct cycle_data *cycle_data = NULL;

  MALLOC(cycle_data, 1);
  cycle_data->ut_seconds = 0;
  cycle_data->num_points = 0;
  cycle_data->u = NULL;
  cycle_data->v = NULL;
  cycle_data->w = NULL;
  cycle_data->ant1 = NULL;
  cycle_data->ant2 = NULL;
  cycle_data->flag = NULL;
  cycle_data->vis_size = NULL;
  cycle_data->vis = NULL;
  cycle_data->wgt = NULL;
  cycle_data->bin = NULL;
  cycle_data->if_no = NULL;
  cycle_data->source = NULL;
  cycle_data->num_cal_ifs = 0;
  cycle_data->num_cal_ants = 0;
  cycle_data->cal_ifs = NULL;
  cycle_data->cal_ants = NULL;
  cycle_data->tsys = NULL;
  cycle_data->tsys_applied = NULL;
  cycle_data->xyphase = NULL;
  cycle_data->xyamp = NULL;
  cycle_data->parangle = NULL;
  cycle_data->tracking_error_max = NULL;
  cycle_data->tracking_error_rms = NULL;
  cycle_data->flagging = NULL;
  cycle_data->weather_valid = SYSCAL_INVALID;
  cycle_data->seemon_valid = SYSCAL_INVALID;

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

  MALLOC(scan_data, 1);
  
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
  int i, j;
  if ((scan_header_data == NULL) ||
      (scan_header_data->if_num_stokes == NULL) ||
      (scan_header_data->ant_name == NULL) ||
      (scan_header_data->ant_cartesian == NULL)) {
    // Probably wasn't actually allocated, just formatted as the struct.
    return;
  }

  for (i = 0; i < scan_header_data->num_ifs; i++) {
    for (j = 0; j < scan_header_data->if_num_stokes[i]; j++) {
      FREE(scan_header_data->if_stokes_names[i][j]);
    }
    FREE(scan_header_data->if_stokes_names[i]);
    for (j = 0; j < 3; j++) {
      FREE(scan_header_data->if_name[i][j]);
    }
    FREE(scan_header_data->if_name[i]);
  }
  FREE(scan_header_data->if_centre_freq);
  FREE(scan_header_data->if_bandwidth);
  FREE(scan_header_data->if_num_channels);
  FREE(scan_header_data->if_num_stokes);
  FREE(scan_header_data->if_sideband);
  FREE(scan_header_data->if_chain);
  FREE(scan_header_data->if_label);
  FREE(scan_header_data->if_name);
  FREE(scan_header_data->if_stokes_names);
  if (scan_header_data->ant_name != NULL) {
    for (i = 0; i < scan_header_data->num_ants; i++) {
      FREE(scan_header_data->ant_name[i]);
      FREE(scan_header_data->ant_cartesian[i]);
    }
  }
  FREE(scan_header_data->ant_label);
  FREE(scan_header_data->ant_name);
  FREE(scan_header_data->ant_cartesian);
}

/**
 * Free memory from a cycle.
 */
void free_cycle_data(struct cycle_data *cycle_data) {
  int i, j;
  if (cycle_data == NULL) {
    return;
  }
  
  FREE(cycle_data->u);
  FREE(cycle_data->v);
  FREE(cycle_data->w);
  FREE(cycle_data->ant1);
  FREE(cycle_data->ant2);
  FREE(cycle_data->flag);
  FREE(cycle_data->bin);
  FREE(cycle_data->if_no);
  for (i = 0; i < cycle_data->num_points; i++) {
    FREE(cycle_data->source[i]);
    FREE(cycle_data->vis[i]);
    FREE(cycle_data->wgt[i]);
  }
  FREE(cycle_data->source);
  FREE(cycle_data->vis_size);
  FREE(cycle_data->vis);
  FREE(cycle_data->wgt);

  FREE(cycle_data->cal_ifs);
  FREE(cycle_data->cal_ants);
  for (i = 0; i < cycle_data->num_cal_ifs; i++) {
    for (j = 0; j < cycle_data->num_cal_ants; j++) {
      FREE(cycle_data->tsys[i][j]);
      FREE(cycle_data->tsys_applied[i][j]);
    }
    FREE(cycle_data->tsys[i]);
    FREE(cycle_data->tsys_applied[i]);
    FREE(cycle_data->xyphase[i]);
    FREE(cycle_data->xyamp[i]);
    FREE(cycle_data->parangle[i]);
    FREE(cycle_data->tracking_error_max[i]);
    FREE(cycle_data->tracking_error_rms[i]);
    FREE(cycle_data->flagging[i]);
  }
  FREE(cycle_data->tsys);
  FREE(cycle_data->tsys_applied);
  FREE(cycle_data->xyphase);
  FREE(cycle_data->xyamp);
  FREE(cycle_data->parangle);
  FREE(cycle_data->tracking_error_max);
  FREE(cycle_data->tracking_error_rms);
  FREE(cycle_data->flagging);
  
  cycle_data = NULL;
}

/**
 * Free memory from a scan.
 */
void free_scan_data(struct scan_data *scan_data) {
  int i = 0;

  for (i = 0; i < scan_data->num_cycles; i++) {
    free_cycle_data(scan_data->cycles[i]);
    FREE(scan_data->cycles[i]);
  }
  FREE(scan_data->cycles);
  free_scan_header_data(&(scan_data->header_data));
  FREE(scan_data);
}

/**
 * This routine reads in a full cycle's worth of data.
 */
int read_cycle_data(struct scan_header_data *scan_header_data,
                    struct cycle_data *cycle_data) {
  int this_jstat = JSTAT_READDATA, read_data = 1, rpfits_result = 0;
  int flag, bin, if_no, sourceno, vis_size = 0, rv = READER_HEADER_AVAILABLE;
  int baseline, ant1, ant2, i, bidx = 1, sif;
  float *vis = NULL, *wgt = NULL, ut, last_ut = -1, u, v, w;
  float complex *cvis = NULL;

  // This is only here to prevent a compiler warning.
  if (scan_header_data == NULL) {
    return(-1);
  }
  
  /* printf("reading data\n"); */
  while (read_data) {
    // Allocate some memory.
    vis_size = max_size_of_vis();
    MALLOC(vis, 2 * vis_size);
    CALLOC(wgt, vis_size);

    // Read in the data.
    this_jstat = JSTAT_READDATA;
    rpfits_result = rpfitsin_(&this_jstat, vis, wgt, &baseline, &ut,
			      &u, &v, &w, &flag, &bin, &if_no, &sourceno);
    /* printf("got read result %d for ut = %.6f\n", rpfits_result, ut); */
    if (last_ut == -1) {
      // Set it here.
      last_ut = ut;
      cycle_data->ut_seconds = ut;
    }
    
    // Check for success.
    if (this_jstat != JSTAT_SUCCESSFUL) {
      /* printf("this isn't right... %d\n", this_jstat); */
      // Ignore illegal data.
      FREE(vis);
      FREE(wgt);
      if (this_jstat == JSTAT_ILLEGALDATA) {
        /* printf("got illegal data, %d\n", fg_.n_fg); */
        continue;
      }
      // Stop reading.
      read_data = 0;
      if (this_jstat == JSTAT_ENDOFFILE) {
        /* printf("reached end of file\n"); */
        rv = READER_EXHAUSTED;
      } else if (this_jstat == JSTAT_FGTABLE) {
        // We've hit the end of this data.
        /* printf("reached a new header\n"); */
        /* printf("READER: while reading data, hit a FG table with %d entries\n", */
        /*        fg_.n_fg); */
        rv = READER_HEADER_AVAILABLE;
      } else if (this_jstat == JSTAT_HEADERNOTDATA) {
        /* printf("READER: while reading data, hit a header with %d entries\n", */
        /*        fg_.n_fg); */
        
      }
    } else if (rpfits_result == 0) {
      fprintf(stderr, "While reading data, rpfitsin encountered an error\n");
    } else {
      /* seconds_to_hourlabel(ut, utstring); */
      /* seconds_to_hourlabel(last_ut, lastutstring); */
      /* printf("jstat = %d ut = %s last = %s baseline = %d\n", this_jstat, */
      /* 	     utstring, lastutstring, baseline); */
      /* printf("if_no = %d sourceno = %d flag = %d bin = %d\n", */
      /* 	     if_no, sourceno, flag, bin); */
      // Check for a time change. The time has to change by at least 0.5
      // otherwise it's not really a cycle change.
      /* if ((baseline == -1) && (ut > (last_ut + 0.5))) { */
      if (baseline == -1) {
        // This is the syscal data group.
        // The number of IFs is determined by how many times we see this table.
        cycle_data->num_cal_ifs = cycle_data->num_cal_ifs + 1;
        sif = cycle_data->num_cal_ifs - 1;
        REALLOC(cycle_data->cal_ifs, cycle_data->num_cal_ifs);
        cycle_data->cal_ifs[sif] = SYSCAL_IF(0, 0);
        if (cycle_data->num_cal_ifs == 1) {
          // We do a bunch of stuff only once.
          // The number of antennas is given by the header.
          // It will always be 1 less than the specified number since when it's
          // 0 it's actually presenting the additional syscal record.
          cycle_data->num_cal_ants = SYSCAL_NUM_ANTS - 1;
          MALLOC(cycle_data->cal_ants, (SYSCAL_NUM_ANTS - 1));
          // Let's deal with the additional syscal record now, which is
          // just meteorological data.
          if (SYSCAL_ADDITIONAL_CHECK == 0) {
            // The syscal data is formatted as we expect.
            cycle_data->temperature = SYSCAL_ADDITIONAL_TEMP;
            cycle_data->air_pressure = SYSCAL_ADDITIONAL_AIRPRESS;
            cycle_data->humidity = SYSCAL_ADDITIONAL_HUMI;
            cycle_data->wind_speed = SYSCAL_ADDITIONAL_WINDSPEED;
            cycle_data->wind_direction = SYSCAL_ADDITIONAL_WINDDIR;
            cycle_data->rain_gauge = SYSCAL_ADDITIONAL_RAIN;
            cycle_data->weather_valid = SYSCAL_ADDITIONAL_WEATHERFLAG;
            cycle_data->seemon_phase = SYSCAL_ADDITIONAL_SEEMON_PHASE;
            cycle_data->seemon_rms = SYSCAL_ADDITIONAL_SEEMON_RMS;
            cycle_data->seemon_valid = SYSCAL_ADDITIONAL_SEEMON_FLAG;
            /* printf("temp = %.1f pressure = %.1f seemon rms = %.1f\n", */
            /*        cycle_data->temperature, cycle_data->air_pressure, cycle_data->seemon_rms); */
          }
        }
        // Do some array allocation.
        REALLOC(cycle_data->tsys, cycle_data->num_cal_ifs);
        REALLOC(cycle_data->tsys_applied, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->xyphase, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->xyamp, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->parangle, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->tracking_error_max, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->tracking_error_rms, cycle_data->num_cal_ifs);
	REALLOC(cycle_data->flagging, cycle_data->num_cal_ifs);
	
        MALLOC(cycle_data->tsys[sif], cycle_data->num_cal_ants);
        MALLOC(cycle_data->tsys_applied[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->xyphase[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->xyamp[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->parangle[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->tracking_error_max[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->tracking_error_rms[sif], cycle_data->num_cal_ants);
	MALLOC(cycle_data->flagging[sif], cycle_data->num_cal_ants);
        for (i = 0; i < cycle_data->num_cal_ants; i++) {
	  cycle_data->cal_ants[i] = SYSCAL_ANT(i, 0);
          MALLOC(cycle_data->tsys[sif][i], 2);
          MALLOC(cycle_data->tsys_applied[sif][i], 2);
          cycle_data->tsys[sif][i][CAL_XX] = SYSCAL_TSYS_X(i, 0);
          cycle_data->tsys[sif][i][CAL_YY] = SYSCAL_TSYS_Y(i, 0);
          cycle_data->tsys_applied[sif][i][CAL_XX] = SYSCAL_TSYS_X_APPLIED(i, 0);
          cycle_data->tsys_applied[sif][i][CAL_YY] = SYSCAL_TSYS_Y_APPLIED(i, 0);

	  cycle_data->xyphase[sif][i] = SYSCAL_XYPHASE(i, 0);
	  cycle_data->xyamp[sif][i] = SYSCAL_XYAMP(i, 0);
	  cycle_data->parangle[sif][i] = SYSCAL_PARANGLE(i, 0);
	  cycle_data->tracking_error_max[sif][i] = SYSCAL_TRACKERR_MAX(i, 0);
	  cycle_data->tracking_error_rms[sif][i] = SYSCAL_TRACKERR_RMS(i, 0);
	  cycle_data->flagging[sif][i] = SYSCAL_FLAG_BAD(i, 0);
        }
        /* printf("Baseline is -1\n"); */
        /* fprintf(stdout, "found SYSCAL group with %d ants %d IFs %d quantities for source %d\n", */
        /*         sc_.sc_ant, sc_.sc_if, sc_.sc_q, sc_.sc_srcno); */
        /* for (i = 0; i < (sc_.sc_ant * sc_.sc_if * sc_.sc_q); i++) { */
        /*   fprintf(stdout, "cal param %d: %.5f\n", i, sc_.sc_cal[i]); */
        /* } */
        /* for (i = 0; i < sc_.sc_ant; i++) { */
        /*   for (j = 0; j < sc_.sc_if; j++) { */
        /*     printf("index %d ANT %d IF %d XYphase = %.4f TSys X = %.4f Y = %.4f\n", */
        /*            SYSCAL_GRP(i, j), */
        /*            SYSCAL_ANT(i, j), SYSCAL_IF(i, j), SYSCAL_XYPHASE(i, j), */
        /*            SYSCAL_TSYS_X(i, j), SYSCAL_TSYS_Y(i, j)); */
        /*   } */
        /* } */
        if (ut > (last_ut + 0.5)) {
          // We've gone to a new cycle.
          /* printf("stopping because new cycle!\n"); */
          rv = READER_HEADER_AVAILABLE | READER_DATA_AVAILABLE;
          read_data = 0;
        }
	FREE(wgt);
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
        ARRAY_APPEND(cycle_data->vis_size, cycle_data->num_points, vis_size);
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
        // Weight is not used for CABB RPFITS, but we keep this here.
        ARRAY_APPEND(cycle_data->wgt, cycle_data->num_points, wgt);
      }
      FREE(vis);
    }
  }

  cycle_data->n_baselines = bidx - 1;
  
  return(rv);
  
}

/* int generate_rpfits_index(struct rpfits_index *rpfits_index) { */
/*   // Make an index of the currently opened RPFITS file. */
/*   long int entrypos; */

/*   // Keep a record of where the file was up to when we entered. */
/*   //entrypos = ftell(); */

/*   // Go back to the start. */
/*   //rewind(); */

/*   // We're finished, go back to the entry point. */
  
/* } */
