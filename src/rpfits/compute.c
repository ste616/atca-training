/**
 * ATCA Training Library: compute.c
 * (C) Jamie Stevens CSIRO 2019
 *
 * This library is designed to read an RPFITS file and for each cycle make available
 * the products which would be available online: spectra, amp, phase, delay,
 * u, v, w, etc.
 * This is so we can make tools which will let observers muck around with what they 
 * would see online in all sorts of real situations, as represented by real data 
 * files.
 * 
 * This module handles computing quantities like amplitude, phase, delay etc.
 * using the vis data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdbool.h>
#include "atrpfits.h"
#include "memory.h"
#include "compute.h"

/**
 * Initialise and return an ampphase structure.
 */
struct ampphase* prepare_ampphase(void) {
  struct ampphase *ampphase = NULL;

  MALLOC(ampphase, 1);

  ampphase->nchannels = 0;
  ampphase->channel = NULL;
  ampphase->frequency = NULL;

  ampphase->nbaselines = 0;
  ampphase->baseline = NULL;

  ampphase->pol = -1;
  ampphase->window = -1;

  ampphase->nbins = NULL;
  
  ampphase->flagged_bad = NULL;
  
  ampphase->weight = NULL;
  ampphase->amplitude = NULL;
  ampphase->phase = NULL;
  ampphase->raw = NULL;
  
  ampphase->f_nchannels = NULL;
  ampphase->f_channel = NULL;
  ampphase->f_frequency = NULL;
  ampphase->f_weight = NULL;
  ampphase->f_amplitude = NULL;
  ampphase->f_phase = NULL;
  ampphase->f_raw = NULL;
  
  ampphase->min_amplitude = NULL;
  ampphase->max_amplitude = NULL;
  ampphase->min_phase = NULL;
  ampphase->max_phase = NULL;

  ampphase->min_amplitude_global = INFINITY;
  ampphase->max_amplitude_global = -INFINITY;
  ampphase->min_phase_global = INFINITY;
  ampphase->max_phase_global = -INFINITY;
  ampphase->options = NULL;
  
  return(ampphase);
}

/**
 * Initialise and return a vis_quantities structure.
 */
struct vis_quantities* prepare_vis_quantities(void) {
  struct vis_quantities *vis_quantities = NULL;

  MALLOC(vis_quantities, 1);

  vis_quantities->options = NULL;
  vis_quantities->nbaselines = 0;

  vis_quantities->pol = -1;
  vis_quantities->window = -1;
  vis_quantities->nbins = NULL;
  vis_quantities->baseline = NULL;
  vis_quantities->flagged_bad = NULL;
  vis_quantities->scantype[0] = 0;
  
  vis_quantities->amplitude = NULL;
  vis_quantities->phase = NULL;
  vis_quantities->delay = NULL;

  vis_quantities->min_amplitude = INFINITY;
  vis_quantities->max_amplitude = -INFINITY;
  vis_quantities->min_phase = INFINITY;
  vis_quantities->max_phase = -INFINITY;
  vis_quantities->min_delay = INFINITY;
  vis_quantities->max_delay = -INFINITY;

  return(vis_quantities);
}

/**
 * Free an ampphase structure's memory.
 */
void free_ampphase(struct ampphase **ampphase) {
  int i = 0, j = 0;

  for (i = 0; i < (*ampphase)->nbaselines; i++) {
    for (j = 0; j < (*ampphase)->nbins[i]; j++) {
      FREE((*ampphase)->weight[i][j]);
      FREE((*ampphase)->amplitude[i][j]);
      FREE((*ampphase)->phase[i][j]);
      FREE((*ampphase)->raw[i][j]);
      FREE((*ampphase)->f_channel[i][j]);
      FREE((*ampphase)->f_frequency[i][j]);
      FREE((*ampphase)->f_weight[i][j]);
      FREE((*ampphase)->f_amplitude[i][j]);
      FREE((*ampphase)->f_phase[i][j]);
      FREE((*ampphase)->f_raw[i][j]);
    }
    FREE((*ampphase)->flagged_bad[i]);
    FREE((*ampphase)->weight[i]);
    FREE((*ampphase)->amplitude[i]);
    FREE((*ampphase)->phase[i]);
    FREE((*ampphase)->raw[i]);
    FREE((*ampphase)->f_nchannels[i]);
    FREE((*ampphase)->f_channel[i]);
    FREE((*ampphase)->f_frequency[i]);
    FREE((*ampphase)->f_weight[i]);
    FREE((*ampphase)->f_amplitude[i]);
    FREE((*ampphase)->f_phase[i]);
    FREE((*ampphase)->f_raw[i]);
  }
  FREE((*ampphase)->flagged_bad);
  FREE((*ampphase)->weight);
  FREE((*ampphase)->amplitude);
  FREE((*ampphase)->phase);
  FREE((*ampphase)->raw);
  FREE((*ampphase)->f_nchannels);
  FREE((*ampphase)->nbins);

  FREE((*ampphase)->channel);
  FREE((*ampphase)->frequency);
  FREE((*ampphase)->baseline);

  FREE((*ampphase)->f_channel);
  FREE((*ampphase)->f_frequency);
  FREE((*ampphase)->f_weight);
  FREE((*ampphase)->f_amplitude);
  FREE((*ampphase)->f_phase);
  FREE((*ampphase)->f_raw);

  FREE((*ampphase)->min_amplitude);
  FREE((*ampphase)->max_amplitude);
  FREE((*ampphase)->min_phase);
  FREE((*ampphase)->max_phase);

  free_ampphase_options((*ampphase)->options);
  FREE((*ampphase)->options);
  
  FREE(*ampphase);
}

/**
 * Free a vis_quantities structure's memory.
 */
void free_vis_quantities(struct vis_quantities **vis_quantities) {
  int i;
  /* FREE((*vis_quantities)->options->min_tvchannel); */
  /* FREE((*vis_quantities)->options->max_tvchannel); */
  free_ampphase_options((*vis_quantities)->options);
  FREE((*vis_quantities)->options);
  
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    FREE((*vis_quantities)->amplitude[i]);
    FREE((*vis_quantities)->phase[i]);
    FREE((*vis_quantities)->delay[i]);
  }
  FREE((*vis_quantities)->amplitude);
  FREE((*vis_quantities)->phase);
  FREE((*vis_quantities)->delay);
  FREE((*vis_quantities)->nbins);
  FREE((*vis_quantities)->baseline);
  FREE((*vis_quantities)->flagged_bad);
  FREE(*vis_quantities);
}

/**
 * This routine looks at a string representation of a polarisation
 * specification and returns our magic number for it.
 */
int polarisation_number(char *polstring) {
  int ncmp = 2;
  if (strncmp(polstring, STRPOL_X, ncmp) == 0) {
    return POL_X;
  }
  if (strncmp(polstring, STRPOL_Y, ncmp) == 0) {
    return POL_Y;
  }
  if (strncmp(polstring, STRPOL_XX, ncmp) == 0) {
    return POL_XX;
  }
  if (strncmp(polstring, STRPOL_YY, ncmp) == 0) {
    return POL_YY;
  }
  if (strncmp(polstring, STRPOL_XY, ncmp) == 0) {
    return POL_XY;
  }
  if (strncmp(polstring, STRPOL_YX, ncmp) == 0) {
    return POL_YX;
  }
  return -1;
}

/**
 * Set default options in an ampphase_options structure.
 */
struct ampphase_options ampphase_options_default(void) {
  struct ampphase_options options;

  options.phase_in_degrees = true;
  options.include_flagged_data = 0;
  options.num_ifs = 0;
  options.min_tvchannel = NULL;
  options.max_tvchannel = NULL;
  //options.delay_averaging = 1;
  options.delay_averaging = NULL;
  //options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_VECTOR;
  options.averaging_method = NULL;

  return options;
}

/**
 * Set default options in an already existing ampphase_options
 * structure.
 */
void set_default_ampphase_options(struct ampphase_options *options) {
  options->phase_in_degrees = true;
  options->include_flagged_data = 0;
  options->num_ifs = 0;
  options->min_tvchannel = NULL;
  options->max_tvchannel = NULL;
  options->delay_averaging = NULL;
  options->averaging_method = NULL;
}

/**
 * Copy one ampphase structure into another.
 */
void copy_ampphase_options(struct ampphase_options *dest,
                           struct ampphase_options *src) {
  int i;
  STRUCTCOPY(src, dest, phase_in_degrees);
  STRUCTCOPY(src, dest, include_flagged_data);
  STRUCTCOPY(src, dest, num_ifs);
  REALLOC(dest->min_tvchannel, dest->num_ifs);
  REALLOC(dest->max_tvchannel, dest->num_ifs);
  REALLOC(dest->delay_averaging, dest->num_ifs);
  REALLOC(dest->averaging_method, dest->num_ifs);
  for (i = 0; i < dest->num_ifs; i++) {
    STRUCTCOPY(src, dest, min_tvchannel[i]);
    STRUCTCOPY(src, dest, max_tvchannel[i]);
    STRUCTCOPY(src, dest, delay_averaging[i]);
    STRUCTCOPY(src, dest, averaging_method[i]);
  }
}

/**
 * Free an ampphase_options structure.
 */
void free_ampphase_options(struct ampphase_options *options) {
  FREE(options->min_tvchannel);
  FREE(options->max_tvchannel);
  FREE(options->delay_averaging);
  FREE(options->averaging_method);
}

/**
 * Copy one metinfo structure into another.
 */
void copy_metinfo_data(struct metinfo *dest,
		       struct metinfo *src) {
  strncpy(dest->obsdate, src->obsdate, OBSDATE_LENGTH);
  STRUCTCOPY(src, dest, ut_seconds);
  STRUCTCOPY(src, dest, temperature);
  STRUCTCOPY(src, dest, air_pressure);
  STRUCTCOPY(src, dest, humidity);
  STRUCTCOPY(src, dest, wind_speed);
  STRUCTCOPY(src, dest, wind_direction);
  STRUCTCOPY(src, dest, rain_gauge);
  STRUCTCOPY(src, dest, weather_valid);
  STRUCTCOPY(src, dest, seemon_phase);
  STRUCTCOPY(src, dest, seemon_rms);
  STRUCTCOPY(src, dest, seemon_valid);
}

/**
 * Copy one syscal_data structure into another.
 */
void copy_syscal_data(struct syscal_data *dest,
		      struct syscal_data *src) {
  int i, j, k;
  strncpy(dest->obsdate, src->obsdate, OBSDATE_LENGTH);
  STRUCTCOPY(src, dest, utseconds);
  STRUCTCOPY(src, dest, num_ifs);
  STRUCTCOPY(src, dest, num_ants);
  STRUCTCOPY(src, dest, num_pols);
  REALLOC(dest->if_num, dest->num_ifs);
  for (i = 0; i < dest->num_ifs; i++) {
    STRUCTCOPY(src, dest, if_num[i]);
  }
  REALLOC(dest->ant_num, dest->num_ants);
  for (i = 0; i < dest->num_ants; i++) {
    STRUCTCOPY(src, dest, ant_num[i]);
  }
  REALLOC(dest->pol, dest->num_pols);
  for (i = 0; i < dest->num_pols; i++) {
    STRUCTCOPY(src, dest, pol[i]);
  }
  REALLOC(dest->parangle, dest->num_ants);
  REALLOC(dest->tracking_error_max, dest->num_ants);
  REALLOC(dest->tracking_error_rms, dest->num_ants);
  REALLOC(dest->flagging, dest->num_ants);
  REALLOC(dest->xyphase, dest->num_ants);
  REALLOC(dest->xyamp, dest->num_ants);
  REALLOC(dest->online_tsys, dest->num_ants);
  REALLOC(dest->online_tsys_applied, dest->num_ants);
  REALLOC(dest->computed_tsys, dest->num_ants);
  REALLOC(dest->computed_tsys_applied, dest->num_ants);
  for (i = 0; i < dest->num_ants; i++) {
    STRUCTCOPY(src, dest, parangle[i]);
    STRUCTCOPY(src, dest, tracking_error_max[i]);
    STRUCTCOPY(src, dest, tracking_error_rms[i]);
    STRUCTCOPY(src, dest, flagging[i]);
    MALLOC(dest->xyphase[i], dest->num_ifs);
    MALLOC(dest->xyamp[i], dest->num_ifs);
    MALLOC(dest->online_tsys[i], dest->num_ifs);
    MALLOC(dest->online_tsys_applied[i], dest->num_ifs);
    MALLOC(dest->computed_tsys[i], dest->num_ifs);
    MALLOC(dest->computed_tsys_applied[i], dest->num_ifs);
    for (j = 0; j < dest->num_ifs; j++) {
      STRUCTCOPY(src, dest, xyphase[i][j]);
      STRUCTCOPY(src, dest, xyamp[i][j]);
      MALLOC(dest->online_tsys[i][j], dest->num_pols);
      MALLOC(dest->online_tsys_applied[i][j], dest->num_pols);
      MALLOC(dest->computed_tsys[i][j], dest->num_pols);
      MALLOC(dest->computed_tsys_applied[i][j], dest->num_pols);
      for (k = 0; k < dest->num_pols; k++) {
	STRUCTCOPY(src, dest, online_tsys[i][j][k]);
	STRUCTCOPY(src, dest, online_tsys_applied[i][j][k]);
	STRUCTCOPY(src, dest, computed_tsys[i][j][k]);
	STRUCTCOPY(src, dest, computed_tsys_applied[i][j][k]);
      }
    }
  }
}

/**
 * Free a syscal_data structure.
 */
void free_syscal_data(struct syscal_data *syscal_data) {
  int i, j;
  FREE(syscal_data->if_num);
  FREE(syscal_data->ant_num);
  FREE(syscal_data->pol);
  FREE(syscal_data->parangle);
  FREE(syscal_data->tracking_error_max);
  FREE(syscal_data->tracking_error_rms);
  FREE(syscal_data->flagging);
  for (i = 0; i < syscal_data->num_ants; i++) {
    FREE(syscal_data->xyphase[i]);
    FREE(syscal_data->xyamp[i]);
  }
  FREE(syscal_data->xyphase);
  FREE(syscal_data->xyamp);
  for (i = 0; i < syscal_data->num_ants; i++) {
    for (j = 0; j < syscal_data->num_ifs; j++) {
      FREE(syscal_data->online_tsys[i][j]);
      FREE(syscal_data->online_tsys_applied[i][j]);
      FREE(syscal_data->computed_tsys[i][j]);
      FREE(syscal_data->computed_tsys_applied[i][j]);
    }
    FREE(syscal_data->online_tsys[i]);
    FREE(syscal_data->online_tsys_applied[i]);
    FREE(syscal_data->computed_tsys[i]);
    FREE(syscal_data->computed_tsys_applied[i]);
  }
  FREE(syscal_data->online_tsys);
  FREE(syscal_data->online_tsys_applied);
  FREE(syscal_data->computed_tsys);
  FREE(syscal_data->computed_tsys_applied);
}

/**
 * Routine that computes the "default" tv channel range given
 * the number of channels present in an IF, the channel width,
 * and the centre frequency in MHz.
 */
void default_tvchannels(int num_chan, float chan_width,
			float centre_freq, int *min_tvchannel,
			int *max_tvchannel) {
  if (num_chan <= 33) {
    // We're likely in 64 MHz mode.
    *min_tvchannel = 9;
    *max_tvchannel = 17;
    return;
  } else {
    if (chan_width < 1) {
      // A zoom band.
      *min_tvchannel = 256;
      *max_tvchannel = 1792;
      return;
    } else {
      // 1 Mhz continuum band.
      if (centre_freq == 2100) {
        *min_tvchannel = 200;
        *max_tvchannel = 900;
        return;
      } else {
        *min_tvchannel = 513;
        *max_tvchannel = 1537;
        return;
      }
    }
  }

  // For safety:
  *min_tvchannel = 2;
  *max_tvchannel = num_chan - 2;
}

/**
 * Routine to add a tvchannel specification for a specified window
 * into the ampphase_options structure. It takes care of putting in
 * defaults which get recognised as not being set if windows are missing.
 */
void add_tvchannels_to_options(struct ampphase_options *ampphase_options,
                               int window, int min_tvchannel, int max_tvchannel) {
  int i, nwindow = window + 1;
  if (nwindow > ampphase_options->num_ifs) {
    // We have to reallocate the memory.
    REALLOC(ampphase_options->min_tvchannel, nwindow);
    REALLOC(ampphase_options->max_tvchannel, nwindow);
    for (i = ampphase_options->num_ifs; i < nwindow; i++) {
      ampphase_options->min_tvchannel[i] = -1;
      ampphase_options->max_tvchannel[i] = -1;
    }
    // Copy the other IF-specific options.
    REALLOC(ampphase_options->delay_averaging, nwindow);
    REALLOC(ampphase_options->averaging_method, nwindow);
    for (i = ampphase_options->num_ifs; i < nwindow; i++) {
      if (i == 0) {
	// First go!
	ampphase_options->delay_averaging[i] = 1;
	ampphase_options->averaging_method[i] = AVERAGETYPE_MEAN | AVERAGETYPE_SCALAR;
      } else {
	ampphase_options->delay_averaging[i] =
	  ampphase_options->delay_averaging[i - 1];
	ampphase_options->averaging_method[i] =
	  ampphase_options->averaging_method[i - 1];
      }
    }
    ampphase_options->num_ifs = nwindow;
  }

  // Set the tvchannels.
  ampphase_options->min_tvchannel[window] = min_tvchannel;
  ampphase_options->max_tvchannel[window] = max_tvchannel;
}


/**
 * Routine to compute amplitude and phase from a vis array.
 * The user needs to specify which pol quantity they want,
 * and which IF and bin.
 */
int vis_ampphase(struct scan_header_data *scan_header_data,
                 struct cycle_data *cycle_data,
                 struct ampphase **ampphase,
                 int pol, int ifnum,
                 struct ampphase_options *options) {
  int ap_created = 0, reqpol = -1, i = 0, polnum = -1, bl = -1, bidx = -1;
  int j = 0, jflag = 0, vidx = -1, cidx = -1, ifno, syscal_if_idx = -1;
  int syscal_pol_idx = -1;
  float rcheck = 0, chanwidth, firstfreq, nhalfchan;
  struct ampphase_options default_options;

  // Check we know about the window number we were given.
  if ((ifnum < 1) || (ifnum > scan_header_data->num_ifs)) {
    return -1;
  }
  // Search for the index of the requested IF.
  for (i = 0, ifno = -1; i < scan_header_data->num_ifs; i++) {
    if (ifnum == scan_header_data->if_label[i]) {
      ifno = i;
      break;
    }
  }
  if (ifno < 0) {
    return -1;
  }

  // Check for options.
  default_options = ampphase_options_default();
  if (options == NULL) {
    options = &default_options;
  }
  
  // Determine which of the polarisations is the one requested.
  for (i = 0; i < scan_header_data->if_num_stokes[ifno]; i++) {
    polnum = polarisation_number(scan_header_data->if_stokes_names[ifno][i]);
    if (polnum == pol) {
      reqpol = i;
      break;
    }
  }
  if (reqpol == -1) {
    // Didn't find the requested polarisation.
    return -1;
  }

  // Prepare the structure if required.
  if (*ampphase == NULL) {
    ap_created = 1;
    *ampphase = prepare_ampphase();
  }
  (*ampphase)->window = ifnum;
  (void)strncpy((*ampphase)->window_name, scan_header_data->if_name[ifno][1], 8);
  (*ampphase)->options = options;
  // Get the number of channels in this window.
  (*ampphase)->nchannels = scan_header_data->if_num_channels[ifno];
  
  (*ampphase)->pol = pol;
  strncpy((*ampphase)->obsdate, scan_header_data->obsdate, OBSDATE_LENGTH);
  (*ampphase)->ut_seconds = cycle_data->ut_seconds;
  strncpy((*ampphase)->scantype, scan_header_data->obstype, OBSTYPE_LENGTH);

  // Deal with the syscal_data structure if necessary. We only copy over the
  // information for the selected pol and IF, and we don't copy the Tsys unless
  // we've been given XX or YY as the requested pol.
  MALLOC((*ampphase)->syscal_data, 1);
  strncpy((*ampphase)->syscal_data->obsdate, (*ampphase)->obsdate, OBSDATE_LENGTH);
  (*ampphase)->syscal_data->utseconds = (*ampphase)->ut_seconds;
  // Locate this IF number in the data.
  for (i = 0; i < cycle_data->num_cal_ifs; i++) {
    if (cycle_data->cal_ifs[i] == ifnum) {
      syscal_if_idx = i;
      break;
    }
  }
  if (syscal_if_idx >= 0) {
    (*ampphase)->syscal_data->num_ifs = 1;
    MALLOC((*ampphase)->syscal_data->if_num, 1);
    (*ampphase)->syscal_data->if_num[0] = ifnum;
  } else {
    (*ampphase)->syscal_data->num_ifs = 0;
    (*ampphase)->syscal_data->if_num = NULL;
  }
  // Check for a usable pol.
  if ((polarisation_number(scan_header_data->if_stokes_names[ifno][reqpol]) == POL_XX) ||
      (polarisation_number(scan_header_data->if_stokes_names[ifno][reqpol]) == POL_YY)) {
    (*ampphase)->syscal_data->num_pols = 1;
    MALLOC((*ampphase)->syscal_data->pol, 1);
    (*ampphase)->syscal_data->pol[0] = pol;
    if (pol == POL_XX) {
      syscal_pol_idx = CAL_XX;
    } else if (pol == POL_YY) {
      syscal_pol_idx = CAL_YY;
    }
  } else {
    (*ampphase)->syscal_data->num_pols = 0;
    (*ampphase)->syscal_data->pol = NULL;
  }
  // Allocate all the antennas in the original table.
  (*ampphase)->syscal_data->num_ants = cycle_data->num_cal_ants;
  MALLOC((*ampphase)->syscal_data->ant_num, (*ampphase)->syscal_data->num_ants);
  MALLOC((*ampphase)->syscal_data->parangle, (*ampphase)->syscal_data->num_ants);
  MALLOC((*ampphase)->syscal_data->tracking_error_max,
	 (*ampphase)->syscal_data->num_ants);
  MALLOC((*ampphase)->syscal_data->tracking_error_rms,
	 (*ampphase)->syscal_data->num_ants);
  MALLOC((*ampphase)->syscal_data->flagging,
	 (*ampphase)->syscal_data->num_ants);
  for (i = 0; i < (*ampphase)->syscal_data->num_ants; i++) {
    // Copy the antenna-based parameters now. These should have been stored
    // identically for all IFs in the SYSCAL table so we just take it from the
    // first IF.
    (*ampphase)->syscal_data->ant_num[i] = cycle_data->cal_ants[i];
    (*ampphase)->syscal_data->parangle[i] = cycle_data->parangle[0][i];
    (*ampphase)->syscal_data->tracking_error_max[i] = cycle_data->tracking_error_max[0][i];
    (*ampphase)->syscal_data->tracking_error_rms[i] = cycle_data->tracking_error_rms[0][i];
    (*ampphase)->syscal_data->flagging[i] = cycle_data->flagging[0][i];
  }
  if (syscal_if_idx >= 0) {
    MALLOC((*ampphase)->syscal_data->xyphase,
	   (*ampphase)->syscal_data->num_ants);
    MALLOC((*ampphase)->syscal_data->xyamp,
	   (*ampphase)->syscal_data->num_ants);
    for (i = 0; i < (*ampphase)->syscal_data->num_ants; i++) {
      MALLOC((*ampphase)->syscal_data->xyphase[i], 1);
      (*ampphase)->syscal_data->xyphase[i][0] =
	cycle_data->xyphase[syscal_if_idx][i];
      MALLOC((*ampphase)->syscal_data->xyamp[i], 1);
      (*ampphase)->syscal_data->xyamp[i][0] =
	cycle_data->xyamp[syscal_if_idx][i];
    }
    if (syscal_pol_idx >= 0) {
      MALLOC((*ampphase)->syscal_data->online_tsys,
	     (*ampphase)->syscal_data->num_ants);
      for (i = 0; i < (*ampphase)->syscal_data->num_ants; i++) {
	MALLOC((*ampphase)->syscal_data->online_tsys[i], 1);
	MALLOC((*ampphase)->syscal_data->online_tsys[i][0], 1);
	(*ampphase)->syscal_data->online_tsys[i][0][0] =
	  cycle_data->tsys[syscal_if_idx][i][syscal_pol_idx];
      }
    }
  }
  
  
  // Allocate the necessary arrays.
  MALLOC((*ampphase)->channel, (*ampphase)->nchannels);
  MALLOC((*ampphase)->frequency, (*ampphase)->nchannels);

  (*ampphase)->nbaselines = cycle_data->n_baselines;
  MALLOC((*ampphase)->flagged_bad, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->weight, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->amplitude, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->phase, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->raw, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->baseline, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->min_amplitude, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->max_amplitude, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->min_phase, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->max_phase, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_nchannels, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_channel, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_frequency, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_weight, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_amplitude, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_phase, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->f_raw, (*ampphase)->nbaselines);

  CALLOC((*ampphase)->nbins, (*ampphase)->nbaselines);
  for (i = 0; i < (*ampphase)->nbaselines; i++) {
    (*ampphase)->flagged_bad[i] = NULL;
    (*ampphase)->weight[i] = NULL;
    (*ampphase)->amplitude[i] = NULL;
    (*ampphase)->phase[i] = NULL;
    (*ampphase)->raw[i] = NULL;
    (*ampphase)->min_amplitude[i] = INFINITY;
    (*ampphase)->max_amplitude[i] = -INFINITY;
    (*ampphase)->min_phase[i] = INFINITY;
    (*ampphase)->max_phase[i] = -INFINITY;
    (*ampphase)->baseline[i] = 0;
    (*ampphase)->f_nchannels[i] = NULL;
    (*ampphase)->f_channel[i] = NULL;
    (*ampphase)->f_frequency[i] = NULL;
    (*ampphase)->f_weight[i] = NULL;
    (*ampphase)->f_amplitude[i] = NULL;
    (*ampphase)->f_phase[i] = NULL;
    (*ampphase)->f_raw[i] = NULL;
  }
  
  // Fill the arrays.
  // Work out the channel width (GHz).
  nhalfchan = (scan_header_data->if_num_channels[ifno] % 2 == 1) ?
    (float)((scan_header_data->if_num_channels[ifno] - 1) / 2) :
    (float)(scan_header_data->if_num_channels[ifno] / 2);
  chanwidth = scan_header_data->if_sideband[ifno] *
    scan_header_data->if_bandwidth[ifno] / (nhalfchan * 2);
  // And the first channel's frequency.
  firstfreq = scan_header_data->if_centre_freq[ifno] -
     nhalfchan * chanwidth;
  /* printf("with nchan = %d, chanwidth = %.6f, first = %.6f\n", */
  /* 	  scan_header_data->if_num_channels[ifno], chanwidth, */
  /* 	  firstfreq); */
  for (i = 0; i < (*ampphase)->nchannels; i++) {
    (*ampphase)->channel[i] = i;
    // Compute the channel frequency.
    (*ampphase)->frequency[i] = firstfreq + (float)i * chanwidth;
  }

  for (i = 0; i < cycle_data->num_points; i++) {
    // Check this is the IF we are after.
    if (cycle_data->if_no[i] != (ifno + 1)) {
      continue;
    }
    /*printf("found the bin with point %d, baseline %d-%d!\n", i,
      cycle_data->ant1[i], cycle_data->ant2[i]); */
    bl = ants_to_base(cycle_data->ant1[i], cycle_data->ant2[i]);
    if (bl < 0) {
      continue;
    }
    bidx = cycle_data->all_baselines[bl] - 1;
    if (bidx < 0) {
      // This is wrong.
      if (ap_created) {
        free_ampphase(ampphase);
      }
      return -1;
    }
    if ((*ampphase)->baseline[bidx] == 0) {
      (*ampphase)->baseline[bidx] = bl;
    }
    if ((*ampphase)->nbins[bidx] < cycle_data->bin[i]) {
      // Found another bin, add it to the list.
      REALLOC((*ampphase)->flagged_bad[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->weight[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->amplitude[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->phase[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->raw[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_nchannels[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_channel[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_frequency[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_weight[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_amplitude[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_phase[bidx], cycle_data->bin[i]);
      REALLOC((*ampphase)->f_raw[bidx], cycle_data->bin[i]);
      for (j = (*ampphase)->nbins[bidx]; j < cycle_data->bin[i]; j++) {
        (*ampphase)->flagged_bad[bidx][j] = cycle_data->flag[i];
        MALLOC((*ampphase)->weight[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->amplitude[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->phase[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->raw[bidx][j], (*ampphase)->nchannels);
        (*ampphase)->f_nchannels[bidx][j] = (*ampphase)->nchannels;
        MALLOC((*ampphase)->f_channel[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->f_frequency[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->f_weight[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->f_amplitude[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->f_phase[bidx][j], (*ampphase)->nchannels);
        MALLOC((*ampphase)->f_raw[bidx][j], (*ampphase)->nchannels);
      }
      (*ampphase)->nbins[bidx] = cycle_data->bin[i];
    }
    /*printf("baseline number is %d, index is %d, confirmation %d\n",
	   bl, bidx, (*ampphase)->baseline[bidx]);
	   printf("this baseline has %d channels\n", (*ampphase)->nchannels);*/
    cidx = cycle_data->bin[i] - 1;
    for (j = 0, jflag = 0; j < (*ampphase)->nchannels; j++) {
      vidx = reqpol + j * scan_header_data->if_num_stokes[ifno];
      (*ampphase)->weight[bidx][cidx][j] = cycle_data->wgt[i][vidx];
      (*ampphase)->amplitude[bidx][cidx][j] = cabsf(cycle_data->vis[i][vidx]);
      (*ampphase)->phase[bidx][cidx][j] = cargf(cycle_data->vis[i][vidx]);
      (*ampphase)->raw[bidx][cidx][j] = cycle_data->vis[i][vidx];
      if (options->phase_in_degrees == true) {
        (*ampphase)->phase[bidx][cidx][j] *= (180 / M_PI);
      }
      // Now assign the data to the arrays considering flagging.
      rcheck = crealf(cycle_data->vis[i][vidx]);
      if (rcheck != rcheck) {
        // A bad channel.
        (*ampphase)->f_nchannels[bidx][cidx] -= 1;
      } else {
        (*ampphase)->f_channel[bidx][cidx][jflag] = (*ampphase)->channel[j];
        (*ampphase)->f_frequency[bidx][cidx][jflag] = (*ampphase)->frequency[j];
        (*ampphase)->f_weight[bidx][cidx][jflag] =
          (*ampphase)->weight[bidx][cidx][j];
        (*ampphase)->f_amplitude[bidx][cidx][jflag] =
          (*ampphase)->amplitude[bidx][cidx][j];
        (*ampphase)->f_phase[bidx][cidx][jflag] =
          (*ampphase)->phase[bidx][cidx][j];
        (*ampphase)->f_raw[bidx][cidx][jflag] =
          (*ampphase)->raw[bidx][cidx][j];
        jflag++;
      }
      
      // Continually assess limits.
      //if ((*ampphase)->weight[bidx][i] > 0) {
      if ((*ampphase)->amplitude[bidx][cidx][j] ==
          (*ampphase)->amplitude[bidx][cidx][j]) {
        if ((*ampphase)->amplitude[bidx][cidx][j] <
            (*ampphase)->min_amplitude[bidx]) {
          (*ampphase)->min_amplitude[bidx] =
            (*ampphase)->amplitude[bidx][cidx][j];
          if ((*ampphase)->amplitude[bidx][cidx][j] <
              (*ampphase)->min_amplitude_global) {
            (*ampphase)->min_amplitude_global =
              (*ampphase)->amplitude[bidx][cidx][j];
          }
        }
        if ((*ampphase)->amplitude[bidx][cidx][j] >
            (*ampphase)->max_amplitude[bidx]) {
          (*ampphase)->max_amplitude[bidx] =
            (*ampphase)->amplitude[bidx][cidx][j];
          if ((*ampphase)->amplitude[bidx][cidx][j] >
              (*ampphase)->max_amplitude_global) {
            (*ampphase)->max_amplitude_global =
              (*ampphase)->amplitude[bidx][cidx][j];
          }
        }
        if ((*ampphase)->phase[bidx][cidx][j] <
            (*ampphase)->min_phase[bidx]) {
          (*ampphase)->min_phase[bidx] = (*ampphase)->phase[bidx][cidx][j];
          if ((*ampphase)->phase[bidx][cidx][j] < (*ampphase)->min_phase_global) {
            (*ampphase)->min_phase_global = (*ampphase)->phase[bidx][cidx][j];
          }
        }
        if ((*ampphase)->phase[bidx][cidx][j] > (*ampphase)->max_phase[bidx]) {
          (*ampphase)->max_phase[bidx] = (*ampphase)->phase[bidx][cidx][j];
          if ((*ampphase)->phase[bidx][cidx][j] > (*ampphase)->max_phase_global) {
            (*ampphase)->max_phase_global = (*ampphase)->phase[bidx][cidx][j];
          }
        }
      }
    }
    
  }

  // Fill the metinfo structure.
  strncpy((*ampphase)->metinfo.obsdate, (*ampphase)->obsdate, OBSDATE_LENGTH);
  (*ampphase)->metinfo.ut_seconds = (*ampphase)->ut_seconds;
  (*ampphase)->metinfo.temperature = cycle_data->temperature;
  (*ampphase)->metinfo.air_pressure = cycle_data->air_pressure;
  (*ampphase)->metinfo.humidity = cycle_data->humidity;
  (*ampphase)->metinfo.wind_speed = cycle_data->wind_speed;
  (*ampphase)->metinfo.wind_direction = cycle_data->wind_direction;
  (*ampphase)->metinfo.rain_gauge = cycle_data->rain_gauge;
  (*ampphase)->metinfo.weather_valid = (cycle_data->weather_valid == SYSCAL_VALID);
  (*ampphase)->metinfo.seemon_phase = cycle_data->seemon_phase;
  (*ampphase)->metinfo.seemon_rms = cycle_data->seemon_rms;
  (*ampphase)->metinfo.seemon_valid = (cycle_data->seemon_valid == SYSCAL_VALID);

  
  
  return 0;
}

/**
 * Sorting comparator functions for the median calculation.
 */
int cmpfunc_real(const void *a, const void *b) {
  return ( *(float*)a - *(float*)b );
}

int cmpfunc_complex(const void *a, const void *b) {
  return ( *(float complex*)a - *(float complex*)b );
}

/**
 * Calculate average amplitude, phase and delays from data in an
 * ampphase structure. The ampphase_options structure control how the
 * calculations are made.
 */
int ampphase_average(struct ampphase *ampphase,
                     struct vis_quantities **vis_quantities,
                     struct ampphase_options *options) {
  int n_points = 0, i, j, k, n_expected = 0, n_delavg_expected = 0;
  int *delavg_n = NULL, delavg_idx = 0, n_delay_points = 0;
  int min_tvchannel, max_tvchannel;
  float total_amplitude = 0, total_phase = 0, total_delay = 0;
  float delta_phase, delta_frequency;
  float *median_array_amplitude = NULL, *median_array_phase = NULL;
  float *median_array_delay = NULL;
  float *array_frequency = NULL, *delavg_frequency = NULL;
  float *delavg_phase = NULL;
  float complex total_complex, *median_complex = NULL, average_complex;
  float complex *delavg_raw = NULL;

  // Prepare the structure if required.
  if (*vis_quantities == NULL) {
    *vis_quantities = prepare_vis_quantities();
  }

  // Copy the data from the ampphase to the vis_quantities.
  (*vis_quantities)->nbaselines = ampphase->nbaselines;
  (*vis_quantities)->pol = ampphase->pol;
  (*vis_quantities)->window = ampphase->window;
  strncpy((*vis_quantities)->obsdate, ampphase->obsdate, OBSDATE_LENGTH);
  (*vis_quantities)->ut_seconds = ampphase->ut_seconds;
  strncpy((*vis_quantities)->scantype, ampphase->scantype, OBSTYPE_LENGTH);

  // Check for options.
  if (options == NULL) {
    // Use the options from the ampphase.
    options = ampphase->options;
  }
  
  // Allocate the necessary arrays.
  /* fprintf(stderr, "[ampphase_average] allocating memory for %d baselines\n", */
  /*         (*vis_quantities)->nbaselines); */
  
  MALLOC((*vis_quantities)->amplitude, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->phase, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->delay, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->nbins, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->baseline, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->flagged_bad, (*vis_quantities)->nbaselines);
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    (*vis_quantities)->nbins[i] = ampphase->nbins[i];
    (*vis_quantities)->baseline[i] = ampphase->baseline[i];
    (*vis_quantities)->flagged_bad[i] = 0;
    /* fprintf(stderr, "[ampphase_average] allocating memory for %d bins\n", */
    /*         (*vis_quantities)->nbins[i]); */
    MALLOC((*vis_quantities)->amplitude[i], (*vis_quantities)->nbins[i]);
    MALLOC((*vis_quantities)->phase[i], (*vis_quantities)->nbins[i]);
    MALLOC((*vis_quantities)->delay[i], (*vis_quantities)->nbins[i]);
  }
  
  if ((ampphase->window < options->num_ifs) &&
      (options->min_tvchannel[ampphase->window] > 0) &&
      (options->max_tvchannel[ampphase->window] > 0)) {
    // Use the set tvchannels if possible.
    min_tvchannel = options->min_tvchannel[ampphase->window];
    max_tvchannel = options->max_tvchannel[ampphase->window];
  } else {
    // Get the default values for this type of IF.
    default_tvchannels(ampphase->nchannels,
                       (ampphase->frequency[1] - ampphase->frequency[0]) * 1000,
                       (1000 * ampphase->frequency[(ampphase->nchannels + 1) / 2]),
                       &min_tvchannel, &max_tvchannel);
    add_tvchannels_to_options(options, ampphase->window,
                              min_tvchannel, max_tvchannel);
  }
  (*vis_quantities)->options = options;
  //n_expected = (options->max_tvchannel - options->min_tvchannel) + 1;
  n_expected = (max_tvchannel - min_tvchannel) + 1;
  /* fprintf(stderr, "[ampphase_average] allocating memory for %d samples\n", */
  /*         n_expected); */
  MALLOC(median_array_amplitude, n_expected);
  MALLOC(median_array_phase, n_expected);
  MALLOC(median_complex, n_expected);
  MALLOC(array_frequency, n_expected);
  // Make some arrays for the delay-averaged phases and frequencies.
  n_delavg_expected = (int)ceilf(n_expected /
				 options->delay_averaging[ampphase->window]);
  if (n_delavg_expected < 1) n_delavg_expected = 1;
  /* fprintf(stderr, "[ampphase_average] allocating memory for %d delay-averaged samples\n", */
  /*         n_delavg_expected); */
  CALLOC(delavg_frequency, n_delavg_expected);
  CALLOC(delavg_phase, n_delavg_expected);
  CALLOC(delavg_raw, n_delavg_expected);
  CALLOC(delavg_n, n_delavg_expected);
  CALLOC(median_array_delay, (n_delavg_expected - 1));
  // Do the averaging loop.
  /* fprintf(stderr, "[ampphase_averaging] starting averaging loop\n"); */
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    for (k = 0; k < (*vis_quantities)->nbins[i]; k++) {
      // Check if this quantity is flagged.
      if ((options->include_flagged_data == 0) &&
          (ampphase->flagged_bad[i][k] == 1)) {
        (*vis_quantities)->flagged_bad[i] += 1;
      }
      // Reset our averaging counters.
      total_amplitude = 0;
      total_phase = 0;
      total_complex = 0 + 0 * I;
      n_points = 0;
      for (j = 0; j < ampphase->f_nchannels[i][k]; j++) {
        // Check for in range.
        if ((ampphase->f_channel[i][k][j] >= min_tvchannel) &&
            (ampphase->f_channel[i][k][j] < max_tvchannel)) {
          total_amplitude += ampphase->f_amplitude[i][k][j];
          total_phase += ampphase->f_phase[i][k][j];
          total_complex += ampphase->f_raw[i][k][j];
          median_array_amplitude[n_points] = ampphase->f_amplitude[i][k][j];
          median_array_phase[n_points] = ampphase->f_phase[i][k][j];
          median_complex[n_points] = ampphase->f_raw[i][k][j];
          array_frequency[n_points] = ampphase->f_frequency[i][k][j];
          n_points++;
          delavg_idx =
            (int)(floorf(ampphase->f_channel[i][k][j] - min_tvchannel) /
                  options->delay_averaging[ampphase->window]);
          delavg_frequency[delavg_idx] += ampphase->f_frequency[i][k][j];
          delavg_raw[delavg_idx] += ampphase->f_raw[i][k][j];
          delavg_n[delavg_idx] += 1;
        }
      }
      if (n_points > 0) {
        // Calculate the delay. Begin by averaging and calculating
        // the phase in each averaging bin.
        for (j = 0; j < n_delavg_expected; j++) {
          if (delavg_n[j] > 0) {
            delavg_raw[j] /= (float)delavg_n[j];
            delavg_phase[j] = cargf(delavg_raw[j]);
            delavg_frequency[j] /= (float)delavg_n[j];
          }
        }
        // Now work out the delays calculated between each bin.
        for (j = 1, n_delay_points = 0; j < n_delavg_expected; j++) {
          if ((delavg_n[j - 1] > 0) &&
              (delavg_n[j] > 0)) {
            delta_phase = delavg_phase[j] - delavg_phase[j - 1];
            if (options->phase_in_degrees) {
              // Change to radians.
              delta_phase *= (M_PI / 180);
            }
            delta_frequency = delavg_frequency[j] - delavg_frequency[j - 1];
            // This frequency is in MHz, change to Hz.
            delta_frequency *= 1E6;
            total_delay += delta_phase / delta_frequency;
            median_array_delay[n_delay_points] = delta_phase / delta_frequency;
            n_delay_points++;
          }
        }
        
        if (options->averaging_method[ampphase->window] & AVERAGETYPE_MEAN) {
          if (options->averaging_method[ampphase->window] & AVERAGETYPE_SCALAR) {
            (*vis_quantities)->amplitude[i][k] = total_amplitude / (float)n_points;
            (*vis_quantities)->phase[i][k] = total_phase / (float)n_points;
          } else if (options->averaging_method[ampphase->window] & AVERAGETYPE_VECTOR) {
            average_complex = total_complex / (float)n_points;
            (*vis_quantities)->amplitude[i][k] = cabsf(average_complex);
            (*vis_quantities)->phase[i][k] = cargf(average_complex);
          }
          // Calculate the final average delay, return in ns.
          (*vis_quantities)->delay[i][k] = (n_delay_points > 0) ?
            (1E9 * total_delay / (float)n_delay_points) : 0;
        } else if (options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
          if (options->averaging_method[ampphase->window] & AVERAGETYPE_SCALAR) {
            qsort(median_array_amplitude, n_points, sizeof(float), cmpfunc_real);
            qsort(median_array_phase, n_points, sizeof(float), cmpfunc_real);
            if (n_points % 2) {
              // Odd number of points.
              (*vis_quantities)->amplitude[i][k] =
                median_array_amplitude[(n_points + 1) / 2];
              (*vis_quantities)->phase[i][k] =
                median_array_phase[(n_points + 1) / 2];
            } else {
              (*vis_quantities)->amplitude[i][k] =
                (median_array_amplitude[n_points / 2] +
                 median_array_amplitude[n_points / 2 + 1]) / 2;
              (*vis_quantities)->phase[i][k] =
                (median_array_amplitude[n_points / 2] +
                 median_array_amplitude[n_points / 2 + 1]) / 2;
            }
          } else if (options->averaging_method[ampphase->window] & AVERAGETYPE_VECTOR) {
            qsort(median_complex, n_points, sizeof(float complex), cmpfunc_complex);
            if (n_points % 2) {
              average_complex = median_complex[(n_points + 1) / 2];
            } else {
              average_complex =
                (median_complex[n_points / 2] + median_complex[n_points / 2 + 1]) / 2;
            }
            (*vis_quantities)->amplitude[i][k] = cabsf(average_complex);
            (*vis_quantities)->phase[i][k] = cargf(average_complex);
          }
          // Calculate the final median delay, return in ns.
          if (n_delay_points == 0) {
            (*vis_quantities)->delay[i][k] = 0;
          } else {
            qsort(median_array_delay, n_delay_points, sizeof(float),
                  cmpfunc_real);
            if (n_delay_points % 2) {
              (*vis_quantities)->delay[i][k] = 1E9 *
                median_array_delay[(n_delay_points + 1) / 2];
            } else {
              (*vis_quantities)->delay[i][k] = 1E9 *
                (median_array_delay[n_delay_points / 2] +
                 median_array_delay[n_delay_points / 2 + 1]) / 2;
            }
          }
        }
      }
    }
  }

  FREE(median_array_amplitude);
  FREE(median_array_phase);
  FREE(median_complex);
  FREE(array_frequency);
  FREE(delavg_phase);
  FREE(delavg_raw);
  FREE(delavg_n);
  FREE(delavg_frequency);
  FREE(median_array_delay);
  
  return 0;
}

bool ampphase_options_match(struct ampphase_options *a,
                            struct ampphase_options *b) {
  // Check if two ampphase_options structures match.
  bool match = false;
  int i;

  if ((a->phase_in_degrees == b->phase_in_degrees) &&
      (a->include_flagged_data == b->include_flagged_data) &&
      (a->num_ifs == b->num_ifs)) {
    // Looks good so far, now check the tvchannels.
    match = true;
    for (i = 0; i < a->num_ifs; i++) {
      if ((a->min_tvchannel[i] != b->min_tvchannel[i]) ||
          (a->max_tvchannel[i] != b->max_tvchannel[i]) ||
	  (a->delay_averaging[i] != b->delay_averaging[i]) ||
	  (a->averaging_method[i] != b->averaging_method[i])) {
        match = false;
        break;
      }
    }
  }
  return match;
}

