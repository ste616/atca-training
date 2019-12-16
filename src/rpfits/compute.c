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
  ampphase->bin = -1;
  
  ampphase->weight = NULL;
  ampphase->amplitude = NULL;
  ampphase->phase = NULL;

  ampphase->min_amplitude = NULL;
  ampphase->max_amplitude = NULL;
  ampphase->min_phase = NULL;
  ampphase->max_phase = NULL;

  ampphase->min_amplitude_global = INFINITY;
  ampphase->max_amplitude_global = -INFINITY;
  ampphase->min_phase_global = INFINITY;
  ampphase->max_phase_global = -INFINITY;
  
  return(ampphase);
}

/**
 * Free an ampphase structure's memory.
 */
void free_ampphase(struct ampphase **ampphase) {
  int i = 0;

  for (i = 0; i < (*ampphase)->nbaselines; i++) {
    FREE((*ampphase)->weight[i]);
    FREE((*ampphase)->amplitude[i]);
    FREE((*ampphase)->phase[i]);
  }
  FREE((*ampphase)->weight);
  FREE((*ampphase)->amplitude);
  FREE((*ampphase)->phase);

  FREE((*ampphase)->channel);
  FREE((*ampphase)->frequency);
  FREE((*ampphase)->baseline);

  FREE((*ampphase)->min_amplitude);
  FREE((*ampphase)->max_amplitude);
  FREE((*ampphase)->min_phase);
  FREE((*ampphase)->max_phase);
  
  FREE(*ampphase);
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
}

/**
 * Routine to compute amplitude and phase from a vis array.
 * The user needs to specify which pol quantity they want,
 * and which IF and bin.
 */
int vis_ampphase(struct scan_header_data *scan_header_data,
		  struct cycle_data *cycle_data,
		  struct ampphase **ampphase,
		 int pol, int ifno, int bin,
		 struct ampphase_options *options) {
  int ap_created = 0, reqpol = -1, i = 0, polnum = -1, bl = -1, bidx = -1;
  int j = 0, jflag = 0, vidx = -1;
  float rcheck = 0;
  struct ampphase_options default_options;

  // Prepare the structure if required.
  if (*ampphase == NULL) {
    ap_created = 1;
    *ampphase = prepare_ampphase();
  }

  // Check we know about the window number we were given.
  if ((ifno < 0) || (ifno >= scan_header_data->num_ifs)) {
    if (ap_created) {
      free_ampphase(ampphase);
    }
    return -1;
  }
  (*ampphase)->window = ifno;

  // Check for options.
  default_options = ampphase_options_default();
  if (options == NULL) {
    options = &default_options;
  }
  
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

  // Allocate the necessary arrays.
  MALLOC((*ampphase)->channel, (*ampphase)->nchannels);
  MALLOC((*ampphase)->frequency, (*ampphase)->nchannels);

  (*ampphase)->nbaselines = cycle_data->n_baselines;
  MALLOC((*ampphase)->weight, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->amplitude, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->phase, (*ampphase)->nbaselines);
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
  for (i = 0; i < (*ampphase)->nbaselines; i++) {
    MALLOC((*ampphase)->weight[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->amplitude[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->phase[i], (*ampphase)->nchannels);
    (*ampphase)->min_amplitude[i] = INFINITY;
    (*ampphase)->max_amplitude[i] = -INFINITY;
    (*ampphase)->min_phase[i] = INFINITY;
    (*ampphase)->max_phase[i] = -INFINITY;
    (*ampphase)->baseline[i] = 0;

    (*ampphase)->f_nchannels[i] = (*ampphase)->nchannels;
    MALLOC((*ampphase)->f_channel[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->f_frequency[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->f_weight[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->f_amplitude[i], (*ampphase)->nchannels);
    MALLOC((*ampphase)->f_phase[i], (*ampphase)->nchannels);
  }
  
  // Fill the arrays.
  for (i = 0; i < (*ampphase)->nchannels; i++) {
    (*ampphase)->channel[i] = i;
    
  }

  (*ampphase)->bin = bin;
  /* printf("bin is %d\n", (*ampphase)->bin); */
  for (i = 0; i < cycle_data->num_points; i++) {
    // Check this is the bin we are after.
    if (cycle_data->bin[i] != bin) {
      continue;
    }
    /*printf("found the bin with point %d, baseline %d-%d!\n", i,
      cycle_data->ant1[i], cycle_data->ant2[i]); */
    bl = ants_to_base(cycle_data->ant1[i], cycle_data->ant2[i]);
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
    /*printf("baseline number is %d, index is %d, confirmation %d\n",
	   bl, bidx, (*ampphase)->baseline[bidx]);
	   printf("this baseline has %d channels\n", (*ampphase)->nchannels);*/
    for (j = 0, jflag = 0; j < (*ampphase)->nchannels; j++) {
      vidx = reqpol + j * scan_header_data->if_num_stokes[ifno];
      (*ampphase)->weight[bidx][j] = cycle_data->wgt[i][vidx];
      (*ampphase)->amplitude[bidx][j] = cabsf(cycle_data->vis[i][vidx]);
      (*ampphase)->phase[bidx][j] = cargf(cycle_data->vis[i][vidx]);
      if (options->phase_in_degrees == true) {
	(*ampphase)->phase[bidx][j] *= (180 / M_PI);
      }
      // Now assign the data to the arrays considering flagging.
      rcheck = crealf(cycle_data->vis[i][vidx]);
      if (rcheck != rcheck) {
	// A bad channel.
	(*ampphase)->f_nchannels[bidx] -= 1;
      } else {
	(*ampphase)->f_channel[bidx][jflag] = (*ampphase)->channel[j];
	(*ampphase)->f_frequency[bidx][jflag] = (*ampphase)->frequency[j];
	(*ampphase)->f_weight[bidx][jflag] = (*ampphase)->weight[bidx][j];
	(*ampphase)->f_amplitude[bidx][jflag] = (*ampphase)->amplitude[bidx][j];
	(*ampphase)->f_phase[bidx][jflag] = (*ampphase)->phase[bidx][j];
	jflag++;
      }
      /* printf("weight is %.2f real %.3f image %.3f\n", */
      /* 	     (*ampphase)->weight[bidx][j], crealf(cycle_data->vis[i][vidx]), */
      /* 	     cimagf(cycle_data->vis[i][vidx])); */
      // Continually assess limits.
      //if ((*ampphase)->weight[bidx][i] > 0) {
      if ((*ampphase)->amplitude[bidx][j] == (*ampphase)->amplitude[bidx][j]) {
	if ((*ampphase)->amplitude[bidx][j] < (*ampphase)->min_amplitude[bidx]) {
	  (*ampphase)->min_amplitude[bidx] = (*ampphase)->amplitude[bidx][j];
	  if ((*ampphase)->amplitude[bidx][j] < (*ampphase)->min_amplitude_global) {
	    (*ampphase)->min_amplitude_global = (*ampphase)->amplitude[bidx][j];
	  }
	}
	if ((*ampphase)->amplitude[bidx][j] > (*ampphase)->max_amplitude[bidx]) {
	  (*ampphase)->max_amplitude[bidx] = (*ampphase)->amplitude[bidx][j];
	  if ((*ampphase)->amplitude[bidx][j] > (*ampphase)->max_amplitude_global) {
	    (*ampphase)->max_amplitude_global = (*ampphase)->amplitude[bidx][j];
	  }
	}
	if ((*ampphase)->phase[bidx][j] < (*ampphase)->min_phase[bidx]) {
	  (*ampphase)->min_phase[bidx] = (*ampphase)->phase[bidx][j];
	  if ((*ampphase)->phase[bidx][j] < (*ampphase)->min_phase_global) {
	    (*ampphase)->min_phase_global = (*ampphase)->phase[bidx][j];
	  }
	}
	if ((*ampphase)->phase[bidx][j] > (*ampphase)->max_phase[bidx]) {
	  (*ampphase)->max_phase[bidx] = (*ampphase)->phase[bidx][j];
	  if ((*ampphase)->phase[bidx][j] > (*ampphase)->max_phase_global) {
	    (*ampphase)->max_phase_global = (*ampphase)->phase[bidx][j];
	  }
	}
      }
    }
    
  }

  return 0;
}
