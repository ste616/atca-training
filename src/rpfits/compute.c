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
  
  FREE(*ampphase);
}

/**
 * Free a vis_quantities structure's memory.
 */
void free_vis_quantities(struct vis_quantities **vis_quantities) {
  int i;
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    FREE((*vis_quantities)->amplitude[i]);
    FREE((*vis_quantities)->phase[i]);
    FREE((*vis_quantities)->delay[i]);
  }
  FREE((*vis_quantities)->amplitude);
  FREE((*vis_quantities)->phase);
  FREE((*vis_quantities)->delay);
  FREE((*vis_quantities)->nbins);
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

  options.phase_in_degrees = false;
  options.delay_averaging = 1;
  options.min_tvchannel = 513;
  options.max_tvchannel = 1537;
  options.averaging_method = AVERAGETYPE_MEAN | AVERAGETYPE_VECTOR;
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
  int j = 0, jflag = 0, vidx = -1, cidx = -1, ifno;
  float rcheck = 0, chanwidth, firstfreq, nhalfchan;
  struct ampphase_options default_options;

  // Prepare the structure if required.
  if (*ampphase == NULL) {
    ap_created = 1;
    *ampphase = prepare_ampphase();
  }

  // Check we know about the window number we were given.
  if ((ifnum < 1) || (ifnum > scan_header_data->num_ifs)) {
    if (ap_created) {
      free_ampphase(ampphase);
    }
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
    // Didn't find the IF number.
    if (ap_created) {
      free_ampphase(ampphase);
    }
    return -1;
  }
  (*ampphase)->window = ifno;
  (void)strncpy((*ampphase)->window_name, scan_header_data->if_name[ifno][1], 8);

  // Check for options.
  default_options = ampphase_options_default();
  if (options == NULL) {
    options = &default_options;
  }
  (*ampphase)->options = options;
  
  // Get the number of channels in this window.
  (*ampphase)->nchannels = scan_header_data->if_num_channels[ifno];
  
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
    if (ap_created) {
      free_ampphase(ampphase);
    }
    return -1;
  }

  (*ampphase)->pol = pol;
  strncpy((*ampphase)->obsdate, scan_header_data->obsdate, OBSDATE_LENGTH);
  (*ampphase)->ut_seconds = cycle_data->ut_seconds;
  
  // Allocate the necessary arrays.
  MALLOC((*ampphase)->channel, (*ampphase)->nchannels);
  MALLOC((*ampphase)->frequency, (*ampphase)->nchannels);

  (*ampphase)->nbaselines = cycle_data->n_baselines;
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
      /* printf("weight is %.2f real %.3f image %.3f\n", */
      /* 	     (*ampphase)->weight[bidx][j], crealf(cycle_data->vis[i][vidx]), */
      /* 	     cimagf(cycle_data->vis[i][vidx])); */
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
  int vq_created = 0, n_points = 0, i, j, k, n_expected = 0;
  float total_amplitude = 0, total_phase = 0;
  float *median_array_amplitude = NULL, *median_array_phase = NULL;
  float complex total_complex, *median_complex = NULL, average_complex;

  // Prepare the structure if required.
  if (*vis_quantities == NULL) {
    vq_created = 1;
    *vis_quantities = prepare_vis_quantities();
  }

  // Copy the data from the ampphase to the vis_quantities.
  (*vis_quantities)->nbaselines = ampphase->nbaselines;
  (*vis_quantities)->pol = ampphase->pol;
  (*vis_quantities)->window = ampphase->window;
  strncpy((*vis_quantities)->obsdate, ampphase->obsdate, OBSDATE_LENGTH);
  (*vis_quantities)->ut_seconds = ampphase->ut_seconds;

  // Check for options.
  if (options == NULL) {
    // Use the options from the ampphase.
    options = ampphase->options;
  }
  (*vis_quantities)->options = options;
  
  // Allocate the necessary arrays.
  MALLOC((*vis_quantities)->amplitude, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->phase, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->delay, (*vis_quantities)->nbaselines);
  MALLOC((*vis_quantities)->nbins, (*vis_quantities)->nbaselines);
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    (*vis_quantities)->nbins[i] = ampphase->nbins[i];
    MALLOC((*vis_quantities)->amplitude[i], (*vis_quantities)->nbins[i]);
    MALLOC((*vis_quantities)->phase[i], (*vis_quantities)->nbins[i]);
    MALLOC((*vis_quantities)->delay[i], (*vis_quantities)->nbins[i]);
  }

  n_expected = (options->max_tvchannel - options->min_tvchannel) + 1;
  MALLOC(median_array_amplitude, n_expected);
  MALLOC(median_array_phase, n_expected);
  MALLOC(median_complex, n_expected);

  // Do the averaging loop.
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    for (k = 0; k < (*vis_quantities)->nbins[i]; k++) {
      // Reset our averaging counters.
      total_amplitude = 0;
      total_phase = 0;
      total_complex = 0 + 0 * I;
      n_points = 0;
      for (j = 0; j < ampphase->f_nchannels[i][k]; j++) {
	// Check for in range.
	if ((ampphase->f_channel[i][k][j] >= options->min_tvchannel) &&
	    (ampphase->f_channel[i][k][j] < options->max_tvchannel)) {
	  total_amplitude += ampphase->f_amplitude[i][k][j];
	  total_phase += ampphase->f_phase[i][k][j];
	  total_complex += ampphase->f_raw[i][k][j];
	  median_array_amplitude[n_points] = ampphase->f_amplitude[i][k][j];
	  median_array_phase[n_points] = ampphase->f_phase[i][k][j];
	  median_complex[n_points] = ampphase->f_raw[i][k][j];
	  n_points++;
	}
      }
      if (n_points > 0) {
	if (options->averaging_method & AVERAGETYPE_MEAN) {
	  if (options->averaging_method & AVERAGETYPE_SCALAR) {
	    (*vis_quantities)->amplitude[i][k] = total_amplitude / (float)n_points;
	    (*vis_quantities)->phase[i][k] = total_phase / (float)n_points;
	  } else if (options->averaging_method & AVERAGETYPE_VECTOR) {
	    average_complex = total_complex / (float)n_points;
	    (*vis_quantities)->amplitude[i][k] = cabsf(average_complex);
	    (*vis_quantities)->phase[i][k] = cargf(average_complex);
	  }
	} else if (options->averaging_method & AVERAGETYPE_MEDIAN) {
	  if (options->averaging_method & AVERAGETYPE_SCALAR) {
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
	  } else if (options->averaging_method & AVERAGETYPE_VECTOR) {
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
	}
      }
    }
  }

  FREE(median_array_amplitude);
  FREE(median_array_phase);
  FREE(median_complex);
  
  return 0;
}
