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
  
  
  return(ampphase);
}

/**
 * Free an ampphase structure's memory.
 */
void free_ampphase(struct ampphase *ampphase) {
  int i = 0;

  for (i = 0; i < ampphase->nbaselines; i++) {
    FREE(ampphase->weight[i]);
    FREE(ampphase->amplitude[i]);
    FREE(ampphase->phase[i]);
  }
  FREE(ampphase->weight);
  FREE(ampphase->amplitude);
  FREE(ampphase->phase);

  FREE(ampphase->channel);
  FREE(ampphase->frequency);
  FREE(ampphase->baseline);

  FREE(ampphase);
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
 * Routine to compute amplitude and phase from a vis array.
 * The user needs to specify which pol quantity they want,
 * and which IF and bin.
 */
int vis_ampphase(struct scan_header_data *scan_header_data,
		  struct cycle_data *cycle_data,
		  struct ampphase *ampphase,
		  int pol, int ifno, int bin) {
  int ap_created = 0, reqpol = -1, i = 0, polnum = -1, bl = -1, bidx = -1;
  int j = 0, vidx = -1;

  // Prepare the structure if required.
  if (ampphase == NULL) {
    ap_created = 1;
    ampphase = prepare_ampphase();
  }

  // Check we know about the window number we were given.
  if ((ifno < 0) || (ifno >= scan_header_data->num_ifs)) {
    if (ap_created) {
      free_ampphase(ampphase);
    }
    return -1;
  }
  ampphase->window = ifno;
  
  // Get the number of channels in this window.
  ampphase->nchannels = scan_header_data->if_num_channels[ifno];

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

  ampphase->pol = pol;

  // Allocate the necessary arrays.
  ampphase->nbaselines = cycle_data->n_baselines;
  MALLOC(ampphase->weight, ampphase->nbaselines);
  MALLOC(ampphase->amplitude, ampphase->nbaselines);
  MALLOC(ampphase->phase, ampphase->nbaselines);
  for (i = 0; i < ampphase->nbaselines; i++) {
    MALLOC(ampphase->weight[i], ampphase->nchannels);
    MALLOC(ampphase->amplitude[i], ampphase->nchannels);
    MALLOC(ampphase->phase[i], ampphase->nchannels);
  }
  
  // Fill the arrays.
  ampphase->bin = bin;
  for (i = 0; i < cycle_data->num_points; i++) {
    // Check this is the bin we are after.
    if (cycle_data->bin[i] != bin) {
      continue;
    }
    bl = ants_to_base(cycle_data->ant1[i], cycle_data->ant2[i]);
    bidx = cycle_data->all_baselines[bl] - 1;
    if (bidx < 0) {
      // This is wrong.
      if (ap_created) {
	free_ampphase(ampphase);
      }
      return -1;
    }
    for (j = 0; j < ampphase->nchannels; j++) {
      vidx = reqpol + j * scan_header_data->if_num_stokes[ifno];
      ampphase->weight[bidx][j] = cycle_data->weight[i][vidx];
      ampphase->amplitude[bidx][j] = cabsf(cycle_data->vis[i][vidx]);
      ampphase->phase[bidx][j] = cargf(cycle_data->vis[i][vidx]);
    }
    
  }

  return 0;
}
