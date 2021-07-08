/** \file compute.c
 *  \brief Routines to handle computing quantities from the raw data
 *
 * ATCA Training Library
 * (C) Jamie Stevens CSIRO 2020
 *
 * This module handles computing quantities like amplitude, phase, delay etc.
 * using the vis data, and system temperatures.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <complex.h>
#include <stdbool.h>
#include <stdarg.h>
#include "atrpfits.h"
#include "memory.h"
#include "compute.h"
#include "common.h"

/*!
 *  \brief Get the median value of a float array
 *  \param a the array of values, which must be sorted before calling
 *  \param n the number of values in the array
 *  \return the median value of the first \a n entries in the array \a a, or 0
 *          if \a n <= 0
 */
float fmedianf(float *a, int n) {
  if (n <= 0) {
    return 0;
  }
  if (n % 2) {
    // Odd number of points.
    return (a[((n + 1) / 2) - 1]);
  } else {
    return ((a[(n / 2) - 1] + a[((n / 2) + 1) - 1]) / 2.0);
  }
}

/*!
 *  \brief Get the median value of a float complex array
 *  \param a the array of values, which must be sorted before calling
 *  \param n the number of values in the array
 *  \return the median value of the first \a n entries in the array \a a, or 0
 *          if \a n <= 0
 */
float complex fcmedianfc(float complex *a, int n) {
  if (n <= 0) {
    return (0 + 0 * I);
  }
  if (n % 2) {
    // Odd number of points.
    return (a[((n + 1) / 2) - 1]);
  } else {
    return ((a[(n / 2) - 1] + a[((n / 2) + 1) - 1]) / 2.0);
  }
}

/*!
 *  \brief Get the total sum of a float array
 *  \param a the array of values
 *  \param n the number of values in the array
 *  \return the total sum of the first \a n entries in the array \a a
 */
float fsumf(float *a, int n) {
  float r = 0;
  int i;
  for (i = 0; i < n; i++) {
    r += a[i];
  }

  return (r);
}

/*!
 *  \brief Get the total sum of a float complex array
 *  \param a the array of values
 *  \param n the number of values in the array
 *  \return the total sum of the first \a n entries in the array \a a
 */
float complex fcsumfc(float complex *a, int n) {
  float complex r = 0 + 0 * I;
  int i;
  for (i = 0; i < n; i++) {
    r += a[i];
  }

  return (r);
}

/*!
 *  \brief Get the mean value of a float array
 *  \param a the array of values
 *  \param n the number of values in the array
 *  \return the mean value of the first \a n entries in the array \a a, or 0
 *          if \a n <= 0
 */
float fmeanf(float *a, int n) {
  if (n <= 0) {
    return 0;
  }
  return (fsumf(a, n) / (float)n);
}

/*!
 *  \brief Get the mean value of a float complex array
 *  \param a the array of values
 *  \param n the number of values in the array
 *  \return the mean value of the first \a n entries in the array \a a, or 0
 *          if \a n <= 0
 */
float complex fcmeanfc(float complex *a, int n) {
  if (n <= 0) {
    return (0 + 0 * I);
  }
  return (fcsumfc(a, n) / (float)n);
}

/*!
 *  \brief Return the smallest value from a variadic list of arguments
 *  \param n the number of values to follow
 *  \param ... the variadic list of arguments
 *  \returns the smallest of the arguments
 */
double smallest(int n, ...) {
  va_list ap;
  double s = INFINITY, t;
  int i;

  va_start(ap, n);
  for (i = 0; i < n; i++) {
    t = va_arg(ap, double);
    s = (t < s) ? t : s;
  }
  va_end(ap);
  
  return s;
}

/*!
 *  \brief Return the smallest value from a variadic list of arguments,
 *         whose value is smallest when considered as a positive number
 *  \param n the number of values to follow
 *  \param ... the variadic list of arguments
 *  \returns the smallest of the arguments
 *
 * For example, if we called this function like so:
 * ```
 * a = smallest_abs(3, 1, -2, 4); // a will return as 1
 * ```
 * This is because abs(1) < abs(-2).
 */
double smallest_abs(int n, ...) {
  va_list ap;
  float s, t, c = INFINITY;
  int i;

  va_start(ap, n);
  for (i = 0; i < n; i++) {
    t = va_arg(ap, double);
    if (fabsf(t) < c) {
      c = fabsf(t);
      s = t;
    }
  }
  va_end(ap);

  return s;
}

/*!
 *  \brief Initialise and return an ampphase structure
 *  \return a pointer to a properly initialised ampphase structure
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
  ampphase->source_no = -1;
  
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
  ampphase->min_real = NULL;
  ampphase->max_real = NULL;
  ampphase->min_imag = NULL;
  ampphase->max_imag = NULL;

  ampphase->min_amplitude_global = INFINITY;
  ampphase->max_amplitude_global = -INFINITY;
  ampphase->min_phase_global = INFINITY;
  ampphase->max_phase_global = -INFINITY;
  ampphase->options = NULL;
  ampphase->syscal_data = NULL;
  
  return(ampphase);
}

/*!
 *  \brief Initialise and return a vis_quantities structure
 *  \return a pointer to a properly initialised vis_quantities structure
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
  vis_quantities->ntriangles = 0;
  vis_quantities->nbins_cross = 0;
  vis_quantities->triangles = NULL;
  
  vis_quantities->amplitude = NULL;
  vis_quantities->phase = NULL;
  vis_quantities->delay = NULL;
  vis_quantities->closure_phase = NULL;
  
  vis_quantities->min_amplitude = INFINITY;
  vis_quantities->max_amplitude = -INFINITY;
  vis_quantities->min_phase = INFINITY;
  vis_quantities->max_phase = -INFINITY;
  vis_quantities->min_delay = INFINITY;
  vis_quantities->max_delay = -INFINITY;
  vis_quantities->min_closure_phase = INFINITY;
  vis_quantities->max_closure_phase = -INFINITY;
  
  return(vis_quantities);
}

/*!
 *  \brief Free an ampphase structure's memory
 *  \param ampphase a pointer the ampphase structure pointer
 *
 * This routine frees all the memory that would have been allocated within
 * the ampphase structure, and also frees the structure itself.
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
  FREE((*ampphase)->min_real);
  FREE((*ampphase)->max_real);
  FREE((*ampphase)->min_imag);
  FREE((*ampphase)->max_imag);
  
  free_ampphase_options((*ampphase)->options);
  FREE((*ampphase)->options);
  free_syscal_data((*ampphase)->syscal_data);
  FREE((*ampphase)->syscal_data);
  
  FREE(*ampphase);
}

/*!
 *  \brief Free a vis_quantities structure's memory
 *  \param vis_quantities a pointer to the vis_quantities structure pointer
 *
 * This routine frees all the memory that would have been allocated within
 * the vis_quantities structure, and also frees the structure itself.
 */
void free_vis_quantities(struct vis_quantities **vis_quantities) {
  int i;
  free_ampphase_options((*vis_quantities)->options);
  FREE((*vis_quantities)->options);
  for (i = 0; i < (*vis_quantities)->ntriangles; i++) {
    FREE((*vis_quantities)->triangles[i]);
    FREE((*vis_quantities)->closure_phase[i]);
  }
  FREE((*vis_quantities)->triangles);
  FREE((*vis_quantities)->closure_phase);
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

/*!
 *  \brief This routine looks at a string representation of a polarisation
 *         specification and returns our magic number for it
 *  \param polstring the string from the RPFITS file indicating a polarisation
 *  \return an internal magic number representation of the polarisation, one of the
 *          POL_* numbers defined in our header file
 *
 * This routine is here to convert the RPFITS representation of polarisation parameters
 * (which are strings) into an integer flag for easier processing.
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

/*!
 *  \brief Set default options in an ampphase_options structure
 *  \return a properly initialised ampphase_options structure
 *
 * The default parameters set in this routine are:
 * - compute phase in degrees
 * - do not include flagged data in computations
 */
struct ampphase_options ampphase_options_default(void) {
  struct ampphase_options options;

  options.phase_in_degrees = true;
  options.include_flagged_data = 0;
  options.num_ifs = 0;
  options.if_centre_freq = NULL;
  options.if_bandwidth = NULL;
  options.if_nchannels = NULL;
  options.min_tvchannel = NULL;
  options.max_tvchannel = NULL;
  options.delay_averaging = NULL;
  options.averaging_method = NULL;
  options.systemp_reverse_online = false;
  options.systemp_apply_computed = false;
  options.reference_antenna = 0;
  options.num_modifiers = NULL;
  options.modifiers = NULL;
  
  return options;
}

/*!
 *  \brief Set default options in an already existing ampphase_options structure
 *  \param options the ampphase_options structure to reset to default
 *
 * The default parameters set in this routine are:
 * - compute phase in degrees
 * - do not include flagged data in computations
 */
void set_default_ampphase_options(struct ampphase_options *options) {
  options->phase_in_degrees = true;
  options->include_flagged_data = 0;
  options->num_ifs = 0;
  options->if_centre_freq = NULL;
  options->if_bandwidth = NULL;
  options->if_nchannels = NULL;
  options->min_tvchannel = NULL;
  options->max_tvchannel = NULL;
  options->delay_averaging = NULL;
  options->averaging_method = NULL;
  options->systemp_reverse_online = false;
  options->systemp_apply_computed = false;
  options->reference_antenna = 0;
  options->num_modifiers = NULL;
  options->modifiers = NULL;
}

/*! \brief Set default options in an already existing ampphase_modifiers structure
 *  \param modifiers the ampphase_modifiers structure to reset to default
 */
void set_default_ampphase_modifiers(struct ampphase_modifiers *modifiers) {
  modifiers->add_delay = false;
  modifiers->delay_num_antennas = 0;
  modifiers->delay_num_pols = 0;
  modifiers->delay_start_mjd = -1;
  modifiers->delay_end_mjd = -1;
  modifiers->delay = NULL;
  modifiers->add_phase = false;
  modifiers->phase_num_antennas = 0;
  modifiers->phase_num_pols = 0;
  modifiers->phase_start_mjd = -1;
  modifiers->phase_end_mjd = -1;
  modifiers->phase = NULL;
}

/*! \brief Add a new modifier to the list of modifiers in an options structure
 *  \param options the ampphase_options structure to add the modifier to
 *  \param idx the index in modifiers to add to, which is the IF index
 *  \param modifier_to_add an already created modifier that you may wish to add to the
 *                         list; specify this as NULL to make this routine add a default
 *                         modifier instead
 *  \returns a pointer to the modifier structure added
 */
struct ampphase_modifiers* add_modifier(struct ampphase_options *options,
					int idx, struct ampphase_modifiers *modifier_to_add) {
  struct ampphase_modifiers *new_modifier = NULL;
  options->num_modifiers[idx] += 1;
  REALLOC(options->modifiers[idx], options->num_modifiers[idx]);
  CALLOC(new_modifier, 1);
  if (modifier_to_add == NULL) {
    set_default_ampphase_modifiers(new_modifier);
  } else {
    copy_ampphase_modifiers(new_modifier, modifier_to_add);
  }
  options->modifiers[idx][options->num_modifiers[idx] - 1] = new_modifier;
  return(new_modifier);
}

/*! \brief Remove some modifiers from the list of modifiers in an options
 *         structure
 *  \param options the ampphase_options structure to remove the modifiers from
 *  \param idx the index in modifiers to remove from, which is the IF index
 *  \param n_modifiers the number of modifiers in \a modidx, or -1 if all the
 *                     modifiers should be removed
 *  \param modidx a 1-D list of the indices of the modifiers in the list that you want
 *                removed, with length \a n_modifiers, indexed starting at 0
 */
void remove_modifiers(struct ampphase_options *options, int idx, int n_modifiers,
		      int *modidx) {
  int *local_modidx = NULL, i, j, len_local = 0;

  // Make a local copy of the modidx.
  if ((n_modifiers > 0) && (modidx != NULL)) {
    CALLOC(local_modidx, n_modifiers);
    for (i = 0; i < n_modifiers; i++) {
      local_modidx[i] = modidx[i];
    }
    // And sort it into ascending order.
    len_local = n_modifiers;
    qsort(local_modidx, len_local, sizeof(int), cmpfunc_integer);
  } else if (n_modifiers == -1) {
    // Make the local array contain all the indices.
    len_local = options->num_modifiers[idx];
    CALLOC(local_modidx, len_local);
    for (i = 0; i < len_local; i++) {
      local_modidx[i] = i;
    }
  }

  // Go through the modifiers backwards.
  for (i = (len_local - 1); i >= 0; i--) {
    // Free the modifier memory.
    free_ampphase_modifiers(options->modifiers[idx][local_modidx[i]]);
    FREE(options->modifiers[idx][local_modidx[i]]);
    // Copy all those later in the list backwards.
    for (j = local_modidx[i]; j < (options->num_modifiers[idx] - 1); j++) {
      options->modifiers[idx][j] = options->modifiers[idx][j + 1];
    }
    options->num_modifiers[idx] -= 1;
  }
  // Reallocate the memory.
  if (options->num_modifiers[idx] > 0) {
    REALLOC(options->modifiers[idx], options->num_modifiers[idx]);
  } else {
    FREE(options->modifiers[idx]);
  }

  // Free our local memory.
  FREE(local_modidx);
}

/*!
 *  \brief Copy one ampphase_modifiers structure into another
 *  \param dest the destinations structure which will be over-written
 *  \param src the source structure from which all values will be copied
 */
void copy_ampphase_modifiers(struct ampphase_modifiers *dest,
			     struct ampphase_modifiers *src) {
  int i, j;
  STRUCTCOPY(src, dest, add_delay);
  STRUCTCOPY(src, dest, delay_num_antennas);
  STRUCTCOPY(src, dest, delay_num_pols);
  STRUCTCOPY(src, dest, delay_start_mjd);
  STRUCTCOPY(src, dest, delay_end_mjd);
  STRUCTCOPY(src, dest, add_phase);
  STRUCTCOPY(src, dest, phase_num_antennas);
  STRUCTCOPY(src, dest, phase_num_pols);
  STRUCTCOPY(src, dest, phase_start_mjd);
  STRUCTCOPY(src, dest, phase_end_mjd);
  STRUCTCOPY(src, dest, set_noise_diode_amplitude);
  STRUCTCOPY(src, dest, noise_diode_num_antennas);
  STRUCTCOPY(src, dest, noise_diode_num_pols);
  STRUCTCOPY(src, dest, noise_diode_start_mjd);
  STRUCTCOPY(src, dest, noise_diode_end_mjd);
  if (dest->delay_num_antennas > 0) {
    CALLOC(dest->delay, dest->delay_num_antennas);
    for (i = 0; i < dest->delay_num_antennas; i++) {
      if (dest->delay_num_pols > 0) {
	CALLOC(dest->delay[i], dest->delay_num_pols);
	for (j = 0; j < dest->delay_num_pols; j++) {
	  STRUCTCOPY(src, dest, delay[i][j]);
	}
      }
    }
  } else {
    dest->delay = NULL;
  }
  if (dest->phase_num_antennas > 0) {
    CALLOC(dest->phase, dest->phase_num_antennas);
    for (i = 0; i < dest->phase_num_antennas; i++) {
      if (dest->phase_num_pols > 0) {
	CALLOC(dest->phase[i], dest->phase_num_pols);
	for (j = 0; j < dest->phase_num_pols; j++) {
	  STRUCTCOPY(src, dest, phase[i][j]);
	}
      }
    }
  } else {
    dest->phase = NULL;
  }
  if (dest->noise_diode_num_antennas > 0) {
    CALLOC(dest->noise_diode_amplitude, dest->noise_diode_num_antennas);
    for (i = 0; i < dest->noise_diode_num_antennas; i++) {
      if (dest->noise_diode_num_pols > 0) {
	CALLOC(dest->noise_diode_amplitude[i], dest->noise_diode_num_pols);
	for (j = 0; j < dest->noise_diode_num_pols; j++) {
	  STRUCTCOPY(src, dest, noise_diode_amplitude[i][j]);
	}
      }
    }
  } else {
    dest->noise_diode_amplitude = NULL;
  }
}

/*!
 *  \brief Copy one ampphase structure into another
 *  \param dest the destination structure which will be over-written
 *  \param src the source structure from which all values will be copied
 */
void copy_ampphase_options(struct ampphase_options *dest,
                           struct ampphase_options *src) {
  int i, j;
  STRUCTCOPY(src, dest, phase_in_degrees);
  STRUCTCOPY(src, dest, include_flagged_data);
  STRUCTCOPY(src, dest, num_ifs);
  REALLOC(dest->if_centre_freq, dest->num_ifs);
  REALLOC(dest->if_bandwidth, dest->num_ifs);
  REALLOC(dest->if_nchannels, dest->num_ifs);
  REALLOC(dest->min_tvchannel, dest->num_ifs);
  REALLOC(dest->max_tvchannel, dest->num_ifs);
  REALLOC(dest->delay_averaging, dest->num_ifs);
  REALLOC(dest->averaging_method, dest->num_ifs);
  REALLOC(dest->num_modifiers, dest->num_ifs);
  REALLOC(dest->modifiers, dest->num_ifs);
  for (i = 0; i < dest->num_ifs; i++) {
    STRUCTCOPY(src, dest, if_centre_freq[i]);
    STRUCTCOPY(src, dest, if_bandwidth[i]);
    STRUCTCOPY(src, dest, if_nchannels[i]);
    STRUCTCOPY(src, dest, min_tvchannel[i]);
    STRUCTCOPY(src, dest, max_tvchannel[i]);
    STRUCTCOPY(src, dest, delay_averaging[i]);
    STRUCTCOPY(src, dest, averaging_method[i]);
    STRUCTCOPY(src, dest, num_modifiers[i]);
    CALLOC(dest->modifiers[i], dest->num_modifiers[i]);
    for (j = 0; j < dest->num_modifiers[i]; j++) {
      CALLOC(dest->modifiers[i][j], 1);
      copy_ampphase_modifiers(dest->modifiers[i][j], src->modifiers[i][j]);
    }
  }
  STRUCTCOPY(src, dest, systemp_reverse_online);
  STRUCTCOPY(src, dest, systemp_apply_computed);
  STRUCTCOPY(src, dest, reference_antenna);

}

/*!
 *  \brief Free an ampphase_modifiers structure
 *  \param modifiers a pointer to the ampphase_modifiers structure
 *
 * This routine will free all the memory that would have been allocated within
 * the structure, but will not free the structure memory.
 */
void free_ampphase_modifiers(struct ampphase_modifiers *modifiers) {
  int i;
  for (i = 0; i < modifiers->delay_num_antennas; i++) {
    FREE(modifiers->delay[i]);
  }
  FREE(modifiers->delay);
  for (i = 0; i < modifiers->phase_num_antennas; i++) {
    FREE(modifiers->phase[i]);
  }
  FREE(modifiers->phase);
  for (i = 0; i < modifiers->noise_diode_num_antennas; i++) {
    FREE(modifiers->noise_diode_amplitude[i]);
  }
  FREE(modifiers->noise_diode_amplitude);
}

/*!
 *  \brief Free an ampphase_options structure
 *  \param options a pointer to the ampphase_options structure
 *
 * This routine will free all the memory that would have been allocated within
 * the structure, but will not free the structure memory.
 */
void free_ampphase_options(struct ampphase_options *options) {
  int i, j;
  
  FREE(options->if_centre_freq);
  FREE(options->if_bandwidth);
  FREE(options->if_nchannels);
  FREE(options->min_tvchannel);
  FREE(options->max_tvchannel);
  FREE(options->delay_averaging);
  FREE(options->averaging_method);
  for (i = 0; i < options->num_ifs; i++) {
    for (j = 0; j < options->num_modifiers[i]; j++) {
      free_ampphase_modifiers(options->modifiers[i][j]);
      FREE(options->modifiers[i][j]);
    }
    FREE(options->modifiers[i]);
  }
  FREE(options->modifiers);
  FREE(options->num_modifiers);
}

/*!
 *  \brief Find ampphase_options structure from an array that matches
 *         the band configuration in the scan header
 *  \param num_options the number of options structures in the array \a options
 *  \param options an array of ampphase_options structures
 *  \param scan_header_data the scan header data describing the band configuration
 *                          you're seeeking
 *  \return a pointer to the ampphase_options structure, or NULL if no match
 *          was found
 */
struct ampphase_options* find_ampphase_options(int num_options,
					       struct ampphase_options **options,
					       struct scan_header_data *scan_header_data) {
  int i, j;
  bool all_bands_match = false;
  struct ampphase_options *match = NULL;

  /* printf("[find_ampphase_options] searching set of %d options\n", num_options); */
  
  for (i = 0; i < num_options; i++) {
    /* printf(" options %d has %d windows, scan header has %d windows\n", i, */
    /* 	   options[i]->num_ifs, scan_header_data->num_ifs); */
    if ((options[i]->num_ifs - 1) == scan_header_data->num_ifs) {
      all_bands_match = true;
      for (j = 1; j < options[i]->num_ifs; j++) {
	/* printf("  MATCHING CF %.1f to %.1f\n", options[i]->if_centre_freq[j], */
	/*        scan_header_data->if_centre_freq[j - 1]); */
	if ((options[i]->if_centre_freq[j] != scan_header_data->if_centre_freq[j - 1]) ||
	    (options[i]->if_bandwidth[j] != scan_header_data->if_bandwidth[j - 1]) ||
	    (options[i]->if_nchannels[j] != scan_header_data->if_num_channels[j - 1])) {
	  all_bands_match = false;
	  break;
	}
      }
      if (all_bands_match == true) {
	match = options[i];
	break;
      }
    }
  }

  return match;
}

/*!
 *  \brief Copy one metinfo structure into another
 *  \param dest the destination structure which will be over-written
 *  \param src the source structure from which all values will be copied
 */
void copy_metinfo(struct metinfo *dest,
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

/*!
 *  \brief Copy one syscal_data structure into another
 *  \param dest the destination structure which will be over-written
 *  \param src the source structure from which all values will be copied
 *
 * The memory in the destination structure is reallocated as required to match
 * the source; the pointers in the destination should therefore be NULL or
 * properly pre-allocated otherwise a segfault will occur.
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
  REALLOC(dest->gtp, dest->num_ants);
  REALLOC(dest->sdo, dest->num_ants);
  REALLOC(dest->caljy, dest->num_ants);
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
    MALLOC(dest->gtp[i], dest->num_ifs);
    MALLOC(dest->sdo[i], dest->num_ifs);
    MALLOC(dest->caljy[i], dest->num_ifs);
    for (j = 0; j < dest->num_ifs; j++) {
      STRUCTCOPY(src, dest, xyphase[i][j]);
      STRUCTCOPY(src, dest, xyamp[i][j]);
      MALLOC(dest->online_tsys[i][j], dest->num_pols);
      MALLOC(dest->online_tsys_applied[i][j], dest->num_pols);
      MALLOC(dest->computed_tsys[i][j], dest->num_pols);
      MALLOC(dest->computed_tsys_applied[i][j], dest->num_pols);
      MALLOC(dest->gtp[i][j], dest->num_pols);
      MALLOC(dest->sdo[i][j], dest->num_pols);
      MALLOC(dest->caljy[i][j], dest->num_pols);
      for (k = 0; k < dest->num_pols; k++) {
        STRUCTCOPY(src, dest, online_tsys[i][j][k]);
        STRUCTCOPY(src, dest, online_tsys_applied[i][j][k]);
        STRUCTCOPY(src, dest, computed_tsys[i][j][k]);
        STRUCTCOPY(src, dest, computed_tsys_applied[i][j][k]);
        STRUCTCOPY(src, dest, gtp[i][j][k]);
        STRUCTCOPY(src, dest, sdo[i][j][k]);
        STRUCTCOPY(src, dest, caljy[i][j][k]);
      }
    }
  }
}

/*!
 *  \brief Free a syscal_data structure's memory
 *  \param syscal_data a pointer to the syscal_data structure pointer
 *
 * This routine frees all the memory that would have been allocated within
 * the free_syscal_data structure, but does not free the structure pointer.
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
  if (syscal_data->xyphase != NULL) {
    for (i = 0; i < syscal_data->num_ants; i++) {
      FREE(syscal_data->xyphase[i]);
      FREE(syscal_data->xyamp[i]);
    }
    FREE(syscal_data->xyphase);
    FREE(syscal_data->xyamp);
  }
  if (syscal_data->online_tsys != NULL) {
    for (i = 0; i < syscal_data->num_ants; i++) {
      for (j = 0; j < syscal_data->num_ifs; j++) {
        FREE(syscal_data->online_tsys[i][j]);
        FREE(syscal_data->online_tsys_applied[i][j]);
        FREE(syscal_data->computed_tsys[i][j]);
        FREE(syscal_data->computed_tsys_applied[i][j]);
        FREE(syscal_data->gtp[i][j]);
        FREE(syscal_data->sdo[i][j]);
        FREE(syscal_data->caljy[i][j]);
      }
      FREE(syscal_data->online_tsys[i]);
      FREE(syscal_data->online_tsys_applied[i]);
      FREE(syscal_data->computed_tsys[i]);
      FREE(syscal_data->computed_tsys_applied[i]);
      FREE(syscal_data->gtp[i]);
      FREE(syscal_data->sdo[i]);
      FREE(syscal_data->caljy[i]);
    }
    FREE(syscal_data->online_tsys);
    FREE(syscal_data->online_tsys_applied);
    FREE(syscal_data->computed_tsys);
    FREE(syscal_data->computed_tsys_applied);
    FREE(syscal_data->gtp);
    FREE(syscal_data->sdo);
    FREE(syscal_data->caljy);
  }
}

/*!
 *  \brief Compute default tvchannel ranges for an IF
 *  \param num_chan the number of channels in the IF
 *  \param chan_width the width of each channel, in MHz
 *  \param centre_freq the frequency of the centre channel of the IF, in MHz
 *  \param min_tvchannel a pointer to a variable that upon exit will contain the
 *                       default minimum tvchannel
 *  \param max_tvchannel a pointer to a variable that upon exit will contain the
 *                       default maximum tvchannel
 *
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

/*!
 *  \brief Add a tvchannel specification to an ampphase_options structure
 *  \param ampphase_options a pointer to a structure which will upon exit contain at
 *                          least as many windows required to accommodate the IF window
 *                          number specified as \a window
 *  \param window the IF number to add tvchannels for (starts at 1)
 *  \param window_centre_freq the central sky frequency of the window, in MHz
 *  \param window_bandwidth the bandwidth of the window, in MHz
 *  \param window_nchannels the number of channels in the window
 *  \param min_tvchannel the minimum channel to use in the tvchannel range
 *  \param max_tvchannel the maximum channel to use in the tvchannel range
 *
 * Routine to add a tvchannel specification for a specified window
 * into the ampphase_options structure. It takes care of putting in
 * defaults which get recognised as not being set if windows are missing.
 */
void add_tvchannels_to_options(struct ampphase_options *ampphase_options, int window,
			       float window_centre_freq, float window_bandwidth,
			       int window_nchannels,
			       int min_tvchannel, int max_tvchannel) {
  int i, nwindow = window + 1;
  if (nwindow > ampphase_options->num_ifs) {
    // We have to reallocate the memory.
    REALLOC(ampphase_options->if_centre_freq, nwindow);
    REALLOC(ampphase_options->if_bandwidth, nwindow);
    REALLOC(ampphase_options->if_nchannels, nwindow);
    REALLOC(ampphase_options->min_tvchannel, nwindow);
    REALLOC(ampphase_options->max_tvchannel, nwindow);
    for (i = ampphase_options->num_ifs; i < nwindow; i++) {
      ampphase_options->if_centre_freq[i] = -1;
      ampphase_options->if_bandwidth[i] = 0;
      ampphase_options->if_nchannels[i] = 0;
      ampphase_options->min_tvchannel[i] = -1;
      ampphase_options->max_tvchannel[i] = -1;
    }
    // Copy the other IF-specific options.
    REALLOC(ampphase_options->delay_averaging, nwindow);
    REALLOC(ampphase_options->averaging_method, nwindow);
    REALLOC(ampphase_options->num_modifiers, nwindow);
    REALLOC(ampphase_options->modifiers, nwindow);
    for (i = ampphase_options->num_ifs; i < nwindow; i++) {
      if (i == 0) {
        // First go!
        ampphase_options->delay_averaging[i] = 1;
        ampphase_options->averaging_method[i] = AVERAGETYPE_MEAN | AVERAGETYPE_VECTOR;
	ampphase_options->num_modifiers[i] = 0;
	ampphase_options->modifiers[i] = NULL;
      } else {
        ampphase_options->delay_averaging[i] =
          ampphase_options->delay_averaging[i - 1];
        ampphase_options->averaging_method[i] =
          ampphase_options->averaging_method[i - 1];
	// We don't make modifiers out of thin air.
	ampphase_options->num_modifiers[i] = 0;
	ampphase_options->modifiers[i] = NULL;
      }
    }
    ampphase_options->num_ifs = nwindow;
  }

  // Set the band identifiers.
  ampphase_options->if_centre_freq[window] = window_centre_freq;
  ampphase_options->if_bandwidth[window] = window_bandwidth;
  ampphase_options->if_nchannels[window] = window_nchannels;
  
  // Set the tvchannels.
  ampphase_options->min_tvchannel[window] = min_tvchannel;
  ampphase_options->max_tvchannel[window] = max_tvchannel;
}


/*!
 *  \brief Compute amplitude and phase from raw data for a single cycle, pol
 *         and window
 *  \param scan_header_data the header information for the scan
 *  \param cycle_data the raw data and metadata for a cycle within the scan
 *  \param ampphase a pointer to the output ampphase structure that will be filled
 *                  with the computed data; if this dereferenced pointer is NULL,
 *                  this routine will allocate the memory for the output structure
 *  \param pol the polarisation to compute the data for; one of the magic numbers POL_*
 *  \param ifnum the window number to compute the data for; the first window is 1
 *  \param num_options the number of ampphase_options structures in the following
 *                     array
 *  \param options the array of options which can be used when doing the computations;
 *                 this array will be searched for the options matching the band
 *                 configuration present in the scan_header_data, and if no match is
 *                 found, a new set of options will be added to this array
 *  \return an indication of whether the computations have worked:
 *          - -1 means that the specified `ifnum` or `pol` aren't in the raw data
 *          - 0 means the computations were successful
 *
 * This routine takes some raw complex data from a scan's cycle, and computes several
 * parameters, which would normally be thought of as "spectra" and viewed in SPD.
 * The data in the output `ampphase` is formatted in a way to allow for easy plotting,
 * and this routine also computes the maximum and minimum values along the plotting
 * axes. All raw data is available in the output, but a "clean" version is also available
 * which omits any channels that have been flagged bad.
 *
 * The metadata regarding calibration is also collated for the requested window and
 * polarisation.
 */
int vis_ampphase(struct scan_header_data *scan_header_data,
                 struct cycle_data *cycle_data,
                 struct ampphase **ampphase,
                 int pol, int ifnum, int *num_options,
                 struct ampphase_options ***options) {
  int ap_created = 0, reqpol = -1, i = 0, polnum = -1, bl = -1, bidx = -1;
  int j = 0, k = 0, jflag = 0, vidx = -1, cidx = -1, ifno, syscal_if_idx = -1;
  int syscal_pol_idx = -1, pidx1, pidx2;
  float rcheck = 0, chanwidth, firstfreq, nhalfchan, total_delay = 0;
  float phase_correction_angle, delay_angle;
  double cmjd;
  bool needs_new_options = false, correct_delay = false, correct_phase = false;
  float complex phase_correction;
  struct ampphase_options *band_options = NULL;
  
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

  // Try to find the correct options for this band.
  band_options = find_ampphase_options(*num_options, *options,
				       scan_header_data);
  needs_new_options = (band_options == NULL);

  /* printf("[vis_ampphase] after finding options\n"); */
  /* print_options_set(*num_options, *options); */
  /* printf("[vis_ampphase] band options are\n"); */
  /* print_options_set(1, &band_options); */

  if (needs_new_options) {
    CALLOC(band_options, 1);
    set_default_ampphase_options(band_options);
    *num_options += 1;
    REALLOC(*options, *num_options);
    CALLOC((*options)[*num_options - 1], 1);
    (*options)[*num_options - 1] = band_options;
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

  // Later, if we need to do delay correction, we will need to know which
  // polarisations to deal with on each antenna, so we determine that here.
  switch (pol) {
  case POL_XX:
    pidx1 = pidx2 = POL_X;
    break;
  case POL_YY:
    pidx1 = pidx2 = POL_Y;
    break;
  case POL_XY:
    pidx1 = POL_X;
    pidx2 = POL_Y;
    break;
  case POL_YX:
    pidx1 = POL_Y;
    pidx2 = POL_X;
    break;
  }
  
  // Prepare the structure if required.
  if (*ampphase == NULL) {
    ap_created = 1;
    *ampphase = prepare_ampphase();
  }
  (*ampphase)->window = ifnum;
  (void)strncpy((*ampphase)->window_name, scan_header_data->if_name[ifno][1], 8);
  CALLOC((*ampphase)->options, 1);
  copy_ampphase_options((*ampphase)->options, band_options);
  // Get the number of channels in this window.
  (*ampphase)->nchannels = scan_header_data->if_num_channels[ifno];
  
  (*ampphase)->pol = pol;
  strncpy((*ampphase)->obsdate, scan_header_data->obsdate, OBSDATE_LENGTH);
  (*ampphase)->ut_seconds = cycle_data->ut_seconds;
  cmjd = date2mjd((*ampphase)->obsdate, (*ampphase)->ut_seconds);
  strncpy((*ampphase)->scantype, scan_header_data->obstype, OBSTYPE_LENGTH);
  if (cycle_data->source_no != NULL) {
    (*ampphase)->source_no = cycle_data->source_no[0];
  }
  
  // Deal with the syscal_data structure if necessary. We only copy over the
  // information for the selected pol and IF, and we don't copy the Tsys unless
  // we've been given XX or YY as the requested pol.
  CALLOC((*ampphase)->syscal_data, 1);
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
    (*ampphase)->syscal_data->tracking_error_max[i] =
      cycle_data->tracking_error_max[0][i];
    (*ampphase)->syscal_data->tracking_error_rms[i] =
      cycle_data->tracking_error_rms[0][i];
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
      CALLOC((*ampphase)->syscal_data->online_tsys,
             (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->online_tsys_applied,
             (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->computed_tsys,
             (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->computed_tsys_applied,
             (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->gtp,
	     (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->sdo,
	     (*ampphase)->syscal_data->num_ants);
      CALLOC((*ampphase)->syscal_data->caljy,
	     (*ampphase)->syscal_data->num_ants);
      for (i = 0; i < (*ampphase)->syscal_data->num_ants; i++) {
        CALLOC((*ampphase)->syscal_data->online_tsys[i], 1);
        CALLOC((*ampphase)->syscal_data->online_tsys[i][0], 1);
        (*ampphase)->syscal_data->online_tsys[i][0][0] =
          cycle_data->tsys[syscal_if_idx][i][syscal_pol_idx];
        CALLOC((*ampphase)->syscal_data->online_tsys_applied[i], 1);
        CALLOC((*ampphase)->syscal_data->online_tsys_applied[i][0], 1);
        (*ampphase)->syscal_data->online_tsys_applied[i][0][0] =
          cycle_data->tsys[syscal_if_idx][i][syscal_pol_idx];
        CALLOC((*ampphase)->syscal_data->computed_tsys[i], 1);
        CALLOC((*ampphase)->syscal_data->computed_tsys[i][0], 1);
	(*ampphase)->syscal_data->computed_tsys[i][0][0] =
	  cycle_data->computed_tsys[syscal_if_idx][i][syscal_pol_idx];
        CALLOC((*ampphase)->syscal_data->computed_tsys_applied[i], 1);
        CALLOC((*ampphase)->syscal_data->computed_tsys_applied[i][0], 1);
	(*ampphase)->syscal_data->computed_tsys_applied[i][0][0] =
	  cycle_data->computed_tsys_applied[syscal_if_idx][i][syscal_pol_idx];
        CALLOC((*ampphase)->syscal_data->gtp[i], 1);
        CALLOC((*ampphase)->syscal_data->gtp[i][0], 1);
        if (syscal_pol_idx == CAL_XX) {
          (*ampphase)->syscal_data->gtp[i][0][0] =
            cycle_data->gtp_x[syscal_if_idx][i];
        } else if (syscal_pol_idx == CAL_YY) {
          (*ampphase)->syscal_data->gtp[i][0][0] =
            cycle_data->gtp_y[syscal_if_idx][i];
        }
        CALLOC((*ampphase)->syscal_data->sdo[i], 1);
        CALLOC((*ampphase)->syscal_data->sdo[i][0], 1);
        if (syscal_pol_idx == CAL_XX) {
          (*ampphase)->syscal_data->sdo[i][0][0] =
            cycle_data->gtp_x[syscal_if_idx][i];
        } else if (syscal_pol_idx == CAL_YY) {
          (*ampphase)->syscal_data->sdo[i][0][0] =
            cycle_data->gtp_y[syscal_if_idx][i];
        }
        CALLOC((*ampphase)->syscal_data->caljy[i], 1);
        CALLOC((*ampphase)->syscal_data->caljy[i][0], 1);
        if (syscal_pol_idx == CAL_XX) {
          (*ampphase)->syscal_data->caljy[i][0][0] =
            cycle_data->caljy_x[syscal_if_idx][i];
        } else if (syscal_pol_idx == CAL_YY) {
          (*ampphase)->syscal_data->caljy[i][0][0] =
            cycle_data->caljy_y[syscal_if_idx][i];
        }
      }
    } else {
      (*ampphase)->syscal_data->online_tsys = NULL;
      (*ampphase)->syscal_data->online_tsys_applied = NULL;
      (*ampphase)->syscal_data->computed_tsys = NULL;
      (*ampphase)->syscal_data->computed_tsys_applied = NULL;
    }
  } else {
    (*ampphase)->syscal_data->xyphase = NULL;
    (*ampphase)->syscal_data->xyamp = NULL;
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
  MALLOC((*ampphase)->min_real, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->max_real, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->min_imag, (*ampphase)->nbaselines);
  MALLOC((*ampphase)->max_imag, (*ampphase)->nbaselines);
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
    (*ampphase)->min_real[i] = INFINITY;
    (*ampphase)->max_real[i] = -INFINITY;
    (*ampphase)->min_imag[i] = INFINITY;
    (*ampphase)->max_imag[i] = -INFINITY;
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
    // Check for a modifier which might need us to add some delay.
    total_delay = 0;
    phase_correction_angle = 0;
    correct_delay = false;
    correct_phase = false;
    for (k = 0; k < band_options->num_modifiers[ifnum]; k++) {
      /* printf(" modifier found MJD %.6f - %.6f\n", */
      /* 	     band_options->modifiers[ifnum][k]->delay_start_mjd, */
      /* 	     band_options->modifiers[ifnum][k]->delay_end_mjd); */
      /* printf(" modifier found for:\n"); */
      /* if (band_options->modifiers[ifnum][k]->add_delay) { */
      /* 	printf("   delay (MJD %.6f - %.6f)\n", */
      /* 	       band_options->modifiers[ifnum][k]->delay_start_mjd, */
      /* 	       band_options->modifiers[ifnum][k]->delay_end_mjd); */
      /* } */
      /* if (band_options->modifiers[ifnum][k]->add_phase) { */
      /* 	printf("   phase (MJD %.6f - %.6f)\n", */
      /* 	       band_options->modifiers[ifnum][k]->phase_start_mjd, */
      /* 	       band_options->modifiers[ifnum][k]->phase_end_mjd); */
      /* } */
      if ((band_options->modifiers[ifnum][k]->add_delay) &&
	  (cmjd >= band_options->modifiers[ifnum][k]->delay_start_mjd) &&
	  (cmjd <= band_options->modifiers[ifnum][k]->delay_end_mjd)) {
	if (cycle_data->ant1[i] != cycle_data->ant2[i]) {
	  total_delay += (band_options->modifiers[ifnum][k]->delay[cycle_data->ant2[i]][pidx2] -
			  band_options->modifiers[ifnum][k]->delay[cycle_data->ant1[i]][pidx1]);
	} else if (pol == POL_XY) {
	  total_delay += band_options->modifiers[ifnum][k]->delay[cycle_data->ant1[i]][POL_XY];
	} else if (pol == POL_YX) {
	  total_delay -= band_options->modifiers[ifnum][k]->delay[cycle_data->ant1[i]][POL_XY];
	}
	/* printf(" baseline %d-%d pol %d, delay ant %d %.3f ns, delay ant %d %.3f ns, delay %.3f ns\n", */
	/*        cycle_data->ant1[i], cycle_data->ant2[i], pol, */
	/*        cycle_data->ant1[i], */
	/*        band_options->modifiers[ifnum][k]->delay[cycle_data->ant1[i]][pidx1], */
	/*        cycle_data->ant2[i], */
	/*        band_options->modifiers[ifnum][k]->delay[cycle_data->ant2[i]][pidx2], */
	/*        total_delay); */
	correct_delay = true;
      }
      if ((band_options->modifiers[ifnum][k]->add_phase) &&
	  (cmjd >= band_options->modifiers[ifnum][k]->phase_start_mjd) &&
	  (cmjd <= band_options->modifiers[ifnum][k]->phase_end_mjd)) {
	if (cycle_data->ant1[i] != cycle_data->ant2[i]) {
	  phase_correction_angle +=
	    (band_options->modifiers[ifnum][k]->phase[cycle_data->ant2[i]][pidx2] -
	     band_options->modifiers[ifnum][k]->phase[cycle_data->ant1[i]][pidx1]);
	} else if (pol == POL_XY) {
	  phase_correction_angle +=
	    band_options->modifiers[ifnum][k]->phase[cycle_data->ant1[i]][POL_XY];
	} else if (pol == POL_YX) {
	  phase_correction_angle -=
	    band_options->modifiers[ifnum][k]->phase[cycle_data->ant1[i]][POL_XY];
	}
	correct_phase = true;
      }
    }
    for (j = 0, jflag = 0; j < (*ampphase)->nchannels; j++) {
      vidx = reqpol + j * scan_header_data->if_num_stokes[ifno];
      (*ampphase)->weight[bidx][cidx][j] = cycle_data->wgt[i][vidx];
      if (correct_delay) {
	delay_angle = -2.0 * M_PI * total_delay * (*ampphase)->frequency[j] / 1000.0;
	/* printf(" delay %.3f frequency %.3f angle = %.6f\n", total_delay, (*ampphase)->frequency[j], */
	/*        delay_angle); */
      } else {
	delay_angle = 0;
      }
      if (correct_delay || correct_phase) {
	phase_correction = cos(delay_angle - phase_correction_angle) +
	  I * sin(delay_angle - phase_correction_angle);
	(*ampphase)->raw[bidx][cidx][j] = cycle_data->vis[i][vidx] * phase_correction;
      } else {
	(*ampphase)->raw[bidx][cidx][j] = cycle_data->vis[i][vidx];
      }
      (*ampphase)->amplitude[bidx][cidx][j] = cabsf((*ampphase)->raw[bidx][cidx][j]);
      (*ampphase)->phase[bidx][cidx][j] = cargf((*ampphase)->raw[bidx][cidx][j]);
      if (band_options->phase_in_degrees == true) {
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
        if (crealf((*ampphase)->raw[bidx][cidx][j]) < (*ampphase)->min_real[bidx]) {
          (*ampphase)->min_real[bidx] = crealf((*ampphase)->raw[bidx][cidx][j]);
        }
        if (crealf((*ampphase)->raw[bidx][cidx][j]) > (*ampphase)->max_real[bidx]) {
          (*ampphase)->max_real[bidx] = crealf((*ampphase)->raw[bidx][cidx][j]);
        }
        if (cimagf((*ampphase)->raw[bidx][cidx][j]) < (*ampphase)->min_imag[bidx]) {
          (*ampphase)->min_imag[bidx] = cimagf((*ampphase)->raw[bidx][cidx][j]);
        }
        if (cimagf((*ampphase)->raw[bidx][cidx][j]) > (*ampphase)->max_imag[bidx]) {
          (*ampphase)->max_imag[bidx] = cimagf((*ampphase)->raw[bidx][cidx][j]);
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

/*!
 *  \brief A qsort comparator function for float real type numbers
 *  \param a a pointer to a number
 *  \param b a pointer to another number
 *  \return - 0 if the two dereferenced numbers are equal
 *          - -ive if the dereferenced \a b number is larger
 *          - +ive if the dereferenced \a a number is larger
 */
int cmpfunc_real(const void *a, const void *b) {
  const float va = *(float *)a;
  const float vb = *(float *)b;
  return ( (va > vb) - (va < vb) );
}

/*!
 *  \brief A qsort comparator function for float complex type numbers
 *  \param a a pointer to a number
 *  \param b a pointer to another number
 *  \return - 0 if the two dereferenced numbers are equal
 *          - -ive if the dereferenced \a b number is larger
 *          - +ive if the dereferenced \a a number is larger
 */
int cmpfunc_complex(const void *a, const void *b) {
  const float va = cabsf(*(float complex *)a);
  const float vb = cabsf(*(float complex *)b);
  return ( (va > vb) - (va < vb) );
}

/*!
 *  \brief A qsort comparator function for integer real type numbers
 *  \param a a pointer to a number
 *  \param b a pointer to another number
 *  \return - 0 if the two dereferenced numbers are equal
 *          - -1 if the dereferenced \a b number is larger
 *          - +1 if the dereferenced \a a number is larger
 */
int cmpfunc_integer(const void *a, const void *b) {
  const int va = *(int *)a;
  const int vb = *(int *)b;
  return ( (va > vb) - (va < vb) );
}

/*!
 *  \brief Calculate average amplitude, phase and delays from data in an
 *         ampphase structure
 *  \param scan_header_data the header information for the scan the cycle comes from
 *  \param ampphase the structure holding the raw data, along with data computed
 *                  with vis_ampphase, which will be averaged in this routine
 *  \param vis_quantities a pointer to the structure which will be filled with the
 *                        average values computed in this routine; if the dereferenced
 *                        value of this pointer is NULL at entry, a vis_quantities
 *                        structure will be allocated and properly initialised
 *  \param num_options the number of ampphase_options structures in the following array
 *  \param options the array of options that control how this routine will do the averaging;
 *                 if this is NULL at entry, the same options structure from
 *                 \a ampphase will be used, and if those options do not have
 *                 already have their tvchannel ranges set, this routine will set
 *                 them to sensible defaults
 *  \return an indication of whether this routine was able to successfully complete or
 *          not (0 means success)
 *
 * This routine takes some amplitude and phase spectra (which must have been pre-computed
 * from the raw data), and takes averages of these within definable tvchannel ranges;
 * the averaging method can also be selected (mean or median). From the phase spectra,
 * the delays are computed within the same tvchannel ranges, and with a specified delavg
 * parameter.
 *
 * The minimum and maximum values of the amplitudes, phases and delays over the baselines
 * and bins is kept here for easy plotting later.
 *
 * The polarisation and window present in the \a ampphase input will of course be the
 * same in the \a vis_quantities output.
 */
int ampphase_average(struct scan_header_data *scan_header_data,
		     struct ampphase *ampphase,
                     struct vis_quantities **vis_quantities,
		     int *num_options,
                     struct ampphase_options ***options) {
  int n_points = 0, i, j, k, n_expected = 0, n_delavg_expected = 0;
  int *delavg_n = NULL, delavg_idx = 0, n_delay_points = 0;
  int min_tvchannel, max_tvchannel, a1, a2, *n_delavg_median = NULL;
  float total_amplitude = 0, total_phase = 0, total_delay = 0;
  float delta_phase, delta_frequency, dp, p1, p2, p3;
  float *median_array_amplitude = NULL, *median_array_phase = NULL;
  float *median_array_delay = NULL;
  float *array_frequency = NULL, *delavg_frequency = NULL;
  float *delavg_phase = NULL, **median_delavg_frequency = NULL;
  float **median_delavg_phase = NULL;
  float complex total_complex, *median_complex = NULL, average_complex;
  float complex *delavg_raw = NULL, **median_delavg_raw = NULL;
  bool needs_new_options = false;
  struct ampphase_options *band_options = NULL;
  /* FILE *debug = NULL; */
  /* char debug_fname[1024]; */
  
  // Prepare the structure if required.
  if (*vis_quantities == NULL) {
    *vis_quantities = prepare_vis_quantities();
  }

  // Copy the data from the ampphase to the vis_quantities.
  (*vis_quantities)->nbaselines = ampphase->nbaselines;
  (*vis_quantities)->pol = ampphase->pol;
  (*vis_quantities)->window = ampphase->window;
  (*vis_quantities)->source_no = ampphase->source_no;
  strncpy((*vis_quantities)->obsdate, ampphase->obsdate, OBSDATE_LENGTH);
  (*vis_quantities)->ut_seconds = ampphase->ut_seconds;
  strncpy((*vis_quantities)->scantype, ampphase->scantype, OBSTYPE_LENGTH);

  /* sprintf(debug_fname, "debugging/delay_debug_window-%d_pol-%d_%s_%d.txt", */
  /* 	  ampphase->window, ampphase->pol, ampphase->obsdate, */
  /* 	  (int)floorf(ampphase->ut_seconds)); */
  
  // Check for options.
  if (options == NULL) {
    // Use the options from the ampphase.
    band_options = ampphase->options;
  } else {
    band_options = find_ampphase_options(*num_options, *options,
					 scan_header_data);
  }
  needs_new_options = (band_options == NULL);

  if (needs_new_options) {
    CALLOC(band_options, 1);
    set_default_ampphase_options(band_options);
    *num_options += 1;
    REALLOC(*options, *num_options);
    CALLOC((*options)[*num_options - 1], 1);
    (*options)[*num_options - 1] = band_options;
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
  
  if ((ampphase->window < band_options->num_ifs) &&
      (band_options->min_tvchannel[ampphase->window] > 0) &&
      (band_options->max_tvchannel[ampphase->window] > 0)) {
    // Use the set tvchannels if possible.
    min_tvchannel = band_options->min_tvchannel[ampphase->window];
    max_tvchannel = band_options->max_tvchannel[ampphase->window];
  } else {
    // Get the default values for this type of IF.
    default_tvchannels(ampphase->nchannels,
                       (ampphase->frequency[1] - ampphase->frequency[0]) * 1000,
                       (1000 * ampphase->frequency[(ampphase->nchannels + 1) / 2]),
                       &min_tvchannel, &max_tvchannel);
    add_tvchannels_to_options(band_options, ampphase->window,
			      scan_header_data->if_centre_freq[ampphase->window - 1],
			      scan_header_data->if_bandwidth[ampphase->window - 1],
			      ampphase->nchannels, min_tvchannel, max_tvchannel);
  }
  CALLOC((*vis_quantities)->options, 1);
  copy_ampphase_options((*vis_quantities)->options, band_options);

  // Always set the non-transported stuff to 0.
  (*vis_quantities)->ntriangles = 0;
  (*vis_quantities)->triangles = NULL;
  (*vis_quantities)->closure_phase = NULL;
  
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
				 band_options->delay_averaging[ampphase->window]);
  if (n_delavg_expected < 1) n_delavg_expected = 1;
  /* fprintf(stderr, "[ampphase_average] allocating memory for %d delay-averaged samples\n", */
  /*         n_delavg_expected); */
  // Safety.
  n_delavg_expected += 1;
  CALLOC(delavg_frequency, n_delavg_expected);
  CALLOC(delavg_phase, n_delavg_expected);
  CALLOC(delavg_raw, n_delavg_expected);
  CALLOC(delavg_n, n_delavg_expected);
  CALLOC(median_array_delay, (n_delavg_expected - 1));
  CALLOC(median_delavg_phase, n_delavg_expected);
  CALLOC(median_delavg_raw, n_delavg_expected);
  CALLOC(median_delavg_frequency, n_delavg_expected);
  CALLOC(n_delavg_median, n_delavg_expected);
  for (i = 0; i < n_delavg_expected; i++) {
    CALLOC(median_delavg_phase[i], band_options->delay_averaging[ampphase->window]);
    CALLOC(median_delavg_raw[i], band_options->delay_averaging[ampphase->window]);
    CALLOC(median_delavg_frequency[i], band_options->delay_averaging[ampphase->window]);
  }
  // Do the averaging loop.
  /* fprintf(stderr, "[ampphase_averaging] starting averaging loop\n"); */
  /* debug = fopen(debug_fname, "w"); */
  for (i = 0; i < (*vis_quantities)->nbaselines; i++) {
    base_to_ants((*vis_quantities)->baseline[i], &a1, &a2);
    /* fprintf(debug, " baseline %d-%d\n", a1, a2); */
    for (k = 0; k < (*vis_quantities)->nbins[i]; k++) {
      // Check if this quantity is flagged.
      if ((band_options->include_flagged_data == 0) &&
          (ampphase->flagged_bad[i][k] == 1)) {
        (*vis_quantities)->flagged_bad[i] += 1;
      }
      // Reset our averaging counters.
      total_amplitude = 0;
      total_phase = 0;
      total_delay = 0;
      total_complex = 0 + 0 * I;
      n_points = 0;
      // Reset the delay averaging quantities.
      for (j = 0; j < n_delavg_expected; j++) {
	delavg_frequency[j] = 0;
	delavg_phase[j] = 0;
	delavg_raw[j] = 0;
	delavg_n[j] = 0;
	n_delavg_median[j] = 0;
      }
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
                  band_options->delay_averaging[ampphase->window]);
          delavg_frequency[delavg_idx] += ampphase->f_frequency[i][k][j];
          delavg_raw[delavg_idx] += ampphase->f_raw[i][k][j];
	  // Phase accounting should probably be optimised out at some point since
	  // for delavg > 1 we recompute it.
	  delavg_phase[delavg_idx] += ampphase->f_phase[i][k][j];
          delavg_n[delavg_idx] += 1;
	  median_delavg_frequency[delavg_idx][n_delavg_median[delavg_idx]] =
	    ampphase->f_frequency[i][k][j];
	  // Same comment as above.
	  median_delavg_phase[delavg_idx][n_delavg_median[delavg_idx]] =
	    ampphase->f_phase[i][k][j];
	  median_delavg_raw[delavg_idx][n_delavg_median[delavg_idx]] =
	    ampphase->f_raw[i][k][j];
	  n_delavg_median[delavg_idx] += 1;
        }
      }
      if (n_points > 0) {
        // Calculate the delay. Begin by averaging and calculating
        // the phase in each averaging bin.
        for (j = 0; j < n_delavg_expected; j++) {
	  if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_MEAN) {
	    if (delavg_n[j] > 0) {
	      delavg_raw[j] /= (float)delavg_n[j];
	      /* delavg_phase[j] = cargf(delavg_raw[j]); */
	      /* if (band_options->phase_in_degrees) { */
	      /* 	delavg_phase[j] *= 180.0 / M_PI; */
	      /* } */
	      delavg_frequency[j] /= (float)delavg_n[j];
	    }
	  } else if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
	    if (n_delavg_median[j] > 0) {
	      qsort(median_delavg_raw[j], n_delavg_median[j], sizeof(float complex),
		    cmpfunc_complex);
	      /* qsort(median_delavg_phase[j], n_delavg_median[j], sizeof(float), cmpfunc_real); */
	      qsort(median_delavg_frequency[j], n_delavg_median[j], sizeof(float), cmpfunc_real);
	      delavg_raw[j] = fcmedianfc(median_delavg_raw[j], n_delavg_median[j]);
	      /* delavg_phase[j] = fmedianf(median_delavg_phase[j], n_delavg_median[j]); */
	      delavg_frequency[j] = fmedianf(median_delavg_frequency[j], n_delavg_median[j]);
	      delavg_n[j] = n_delavg_median[j];
	    }
	  }
	  // Phase needs to be recomputed since wrapping is basically impossible
	  // to deal with while averaging.
	  delavg_phase[j] = cargf(delavg_raw[j]);
	  if (band_options->phase_in_degrees) {
	    delavg_phase[j] *= 180.0 / M_PI;
	  }
        }
        // Now work out the delays calculated between each bin.
        for (j = 1, n_delay_points = 0; j < n_delavg_expected; j++) {
          if ((delavg_n[j - 1] > 0) &&
              (delavg_n[j] > 0)) {
	    dp = (band_options->phase_in_degrees) ? 360.0 : (2 * M_PI);
	    p1 = delavg_phase[j] - delavg_phase[j - 1];
	    p2 = (delavg_phase[j] + dp) - delavg_phase[j - 1];
	    p3 = (delavg_phase[j] - dp) - delavg_phase[j - 1];
	    
            delta_phase = (float)smallest_abs(3, p1, p2, p3);

	    if (band_options->phase_in_degrees) {
              // Change to radians.
              delta_phase *= (M_PI / 180);
            }
            delta_frequency = delavg_frequency[j] - delavg_frequency[j - 1];
            // This frequency is in MHz, change to Hz.
            //delta_frequency *= 1E6;
            total_delay += delta_phase / (2 * M_PI * delta_frequency);
	    /* fprintf(debug, " chans %d %d, phase = %.1f %.1f, diffs = %.1f %.1f %.1f, " */
	    /* 	    "chosen = %.6f, delta_f = %.6f, delay = %.6f\n", */
	    /* 	    (j - 1), j, delavg_phase[j - 1], delavg_phase[j], */
	    /* 	    p1, p2, p3, delta_phase, delta_frequency, (delta_phase / delta_frequency)); */
            median_array_delay[n_delay_points] = (delta_phase / (2 * M_PI * delta_frequency));
            n_delay_points++;
          }
        }
        
        if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_MEAN) {
          if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_SCALAR) {
            (*vis_quantities)->amplitude[i][k] = total_amplitude / (float)n_points;
            (*vis_quantities)->phase[i][k] = total_phase / (float)n_points;
          } else if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_VECTOR) {
            average_complex = total_complex / (float)n_points;
            (*vis_quantities)->amplitude[i][k] = cabsf(average_complex);
            (*vis_quantities)->phase[i][k] = cargf(average_complex);
	    // We have to change to degrees here because that was only done for phase
	    // in the computer.
	    if (band_options->phase_in_degrees) {
	      (*vis_quantities)->phase[i][k] *= (180 / M_PI);
	    }
          }
          // Calculate the final average delay, return in ns.
          (*vis_quantities)->delay[i][k] = (n_delay_points > 0) ?
            (1E3 * total_delay / (float)n_delay_points) : 0;
        } else if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
          if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_SCALAR) {
            qsort(median_array_amplitude, n_points, sizeof(float), cmpfunc_real);
            qsort(median_array_phase, n_points, sizeof(float), cmpfunc_real);
	    (*vis_quantities)->amplitude[i][k] = fmedianf(median_array_amplitude, n_points);
	    (*vis_quantities)->phase[i][k] = fmedianf(median_array_phase, n_points);
          } else if (band_options->averaging_method[ampphase->window] & AVERAGETYPE_VECTOR) {
            qsort(median_complex, n_points, sizeof(float complex), cmpfunc_complex);
	    average_complex = fcmedianfc(median_complex, n_points);
            (*vis_quantities)->amplitude[i][k] = cabsf(average_complex);
            (*vis_quantities)->phase[i][k] = cargf(average_complex);
	    // We have to change to degrees here because that was only done for phase
	    // in the computer.
	    if (band_options->phase_in_degrees) {
	      (*vis_quantities)->phase[i][k] *= (180 / M_PI);
	    }
          }
          // Calculate the final median delay, return in ns.
          if (n_delay_points == 0) {
            (*vis_quantities)->delay[i][k] = 0;
          } else {
            qsort(median_array_delay, n_delay_points, sizeof(float),
                  cmpfunc_real);
	    (*vis_quantities)->delay[i][k] = 1E3 * fmedianf(median_array_delay,
	    						    n_delay_points);
	    /* (*vis_quantities)->delay[i][k] = (n_delay_points > 0) ? */
	    /*   (1E3 * total_delay / (float)n_delay_points) : 0; */
          }
        }
      }
    }
  }
  /* fclose(debug); */
  
  FREE(median_array_amplitude);
  FREE(median_array_phase);
  FREE(median_complex);
  FREE(array_frequency);
  FREE(delavg_phase);
  FREE(delavg_raw);
  FREE(delavg_n);
  FREE(delavg_frequency);
  FREE(median_array_delay);
  for (i = 0; i < n_delavg_expected; i++) {
    FREE(median_delavg_phase[i]);
    FREE(median_delavg_raw[i]);
    FREE(median_delavg_frequency[i]);
  }
  FREE(median_delavg_phase);
  FREE(median_delavg_raw);
  FREE(median_delavg_frequency);
  FREE(n_delavg_median);
  
  return 0;
}

/*!
 *  \brief Compare to ampphase_modifiers structres to determine if they match
 *  \param a a pointer to an ampphase_modifiers structure
 *  \param b a pointer to another ampphase_modifiers structure
 *  \return true if \a a and \a b have exactly the same values for all their
 *          parameters, or false otherwise
 */
bool ampphase_modifiers_match(struct ampphase_modifiers *a,
			      struct ampphase_modifiers *b) {
  bool match = false;
  int i, j;
  if ((a == NULL) && (b == NULL)) {
    return true;
  } else if ((a == NULL) || (b == NULL)) {
    return false;
  }
  if ((a->add_delay == b->add_delay) &&
      (a->delay_num_antennas == b->delay_num_antennas) &&
      (a->delay_num_pols == b->delay_num_pols) &&
      (a->delay_start_mjd == b->delay_start_mjd) &&
      (a->delay_end_mjd == b->delay_end_mjd) &&
      (a->add_phase == b->add_phase) &&
      (a->phase_num_antennas == b->phase_num_antennas) &&
      (a->phase_num_pols == b->phase_num_pols) &&
      (a->phase_start_mjd == b->phase_start_mjd) &&
      (a->phase_end_mjd == b->phase_end_mjd)) {
    // Looks good so far, now check the delay settings.
    match = true;
    for (i = 0; i < a->delay_num_antennas; i++) {
      for (j = 0; j < b->delay_num_pols; j++) {
	if (a->delay[i][j] != b->delay[i][j]) {
	  match = false;
	  break;
	}
      }
      if (!match) {
	break;
      }
    }
    // And check the phase settings.
    for (i = 0; i < a->phase_num_antennas; i++) {
      for (j = 0; j < b->phase_num_pols; j++) {
	if (a->phase[i][j] != b->phase[i][j]) {
	  match = false;
	  break;
	}
      }
      if (!match) {
	break;
      }
    }
  }
  return match;
}

/*!
 *  \brief Compare two ampphase_options structures to determine if they match
 *  \param a a pointer to an ampphase_options structure
 *  \param b a pointer to another ampphase_options structure
 *  \return true if \a a and \a b have exactly the same values for all their
 *          parameters, or false otherwise
 */
bool ampphase_options_match(struct ampphase_options *a,
                            struct ampphase_options *b) {
  // Check if two ampphase_options structures match.
  bool match = false;
  int i, j;

  if ((a->phase_in_degrees == b->phase_in_degrees) &&
      (a->include_flagged_data == b->include_flagged_data) &&
      (a->num_ifs == b->num_ifs) &&
      (a->systemp_reverse_online == b->systemp_reverse_online) &&
      (a->systemp_apply_computed == b->systemp_apply_computed) &&
      (a->reference_antenna == b->reference_antenna)) {
    // Looks good so far, now check the tvchannels and the modifiers.
    match = true;
    for (i = 0; i < a->num_ifs; i++) {
      if ((a->min_tvchannel[i] != b->min_tvchannel[i]) ||
          (a->max_tvchannel[i] != b->max_tvchannel[i]) ||
          (a->delay_averaging[i] != b->delay_averaging[i]) ||
          (a->averaging_method[i] != b->averaging_method[i]) ||
	  (a->num_modifiers[i] != b->num_modifiers[i])) {
        match = false;
        break;
      }
      for (j = 0; j < a->num_modifiers[i]; j++) {
	if (!ampphase_modifiers_match(a->modifiers[i][j],
				      b->modifiers[i][j])) {
	  match = false;
	  break;
	}
      }
      if (!match) {
	break;
      }
    }
  }
  return match;
}

/*!
 *  \brief Compute system temperatures from raw data for a whole cycle
 *  \param cycle_data the raw data and metadata for a cycle
 *  \param scan_header_data the header information for the scan that the cycle is part of
 *  \param num_options the number of ampphase_options structures in the following array
 *  \param options the array of options which can be used when doing the computations;
 *                 this array will be searched for the options matching the band
 *                 configuration present in the \a scan_header_data, and if no match
 *                 is found, a new set of options will be added to this array
 */
void calculate_system_temperatures_cycle_data(struct cycle_data *cycle_data,
                                              struct scan_header_data *scan_header_data,
					      int *num_options,
                                              struct ampphase_options ***options) {
  int i, j, k, bl, bidx, ***n_tp_on_array = NULL, ***n_tp_off_array = NULL;
  int aidx, iidx, nchannels, ifno, chan_low = -1, chan_high = -1, ifsidx;
  int reqpol[2], polnum, vidx;
  float ****tp_on_array = NULL, ****tp_off_array = NULL;
  float nhalfchan, chanwidth, rcheck, med_tp_on, med_tp_off, tp_on, tp_off;
  float fs, fd, dx;
  bool computed_tsys_applied = false, needs_new_options = false;
  struct ampphase_options *band_options = NULL;
  // Recalculate the system temperature from the data within the
  // specified tvchannel range, and with different options.

  // This routine is designed to run straight after the data is read into
  // the cycle_data structure.

  // Make some initial allocations.
  CALLOC(tp_on_array, cycle_data->num_cal_ants);
  CALLOC(tp_off_array, cycle_data->num_cal_ants);
  CALLOC(n_tp_on_array, cycle_data->num_cal_ants);
  CALLOC(n_tp_off_array, cycle_data->num_cal_ants);
  for (i = 0; i < cycle_data->num_cal_ants; i++) {
    CALLOC(tp_on_array[i], scan_header_data->num_ifs);
    CALLOC(tp_off_array[i], scan_header_data->num_ifs);
    CALLOC(n_tp_on_array[i], scan_header_data->num_ifs);
    CALLOC(n_tp_off_array[i], scan_header_data->num_ifs);
    for (j = 0; j < scan_header_data->num_ifs; j++) {
      // Pols, we only care about XX and YY.
      CALLOC(tp_on_array[i][j], 2);
      CALLOC(tp_off_array[i][j], 2);
      CALLOC(n_tp_on_array[i][j], 2);
      CALLOC(n_tp_off_array[i][j], 2);
    }
  }

  // Try to find the correct options for this band.
  band_options = find_ampphase_options(*num_options, *options, scan_header_data);
  needs_new_options = (band_options == NULL);

  /* printf("[calculate_system_temperatures_cycle_data] after finding options\n"); */
  /* print_options_set(*num_options, *options); */
  /* printf("[calculate_system_temperatures_cycle_data] band options are\n"); */
  /* print_options_set(1, &band_options); */
  
  if (needs_new_options) {
    CALLOC(band_options, 1);
    set_default_ampphase_options(band_options);
    *num_options += 1;
    REALLOC(*options, *num_options);
    (*options)[*num_options - 1] = band_options;
    band_options = (*options)[*num_options - 1];
  }

  /* printf("[calculate_system_temperatures_cycle_data] after creating new options\n"); */
  /* print_options_set(*num_options, *options); */
  
  for (i = 0; i < cycle_data->num_points; i++) {
    bl = ants_to_base(cycle_data->ant1[i], cycle_data->ant2[i]);
    if (bl < 0) {
      continue;
    }
    // We only do system temperature calculation if the baseline
    // is an autocorrelation.
    bidx = cycle_data->all_baselines[bl] - 1;
    if (bidx < 0) {
      return;
    }
    if (cycle_data->ant1[i] == cycle_data->ant2[i]) {
      aidx = cycle_data->ant1[i] - 1;
      iidx = cycle_data->if_no[i] - 1;
      /* if (cycle_data->bin[i] == 1) { */
      /*   // The noise diode is off. */
      /*   *tp_array = tp_off_array; */
      /*   *n_tp_array = n_tp_off_array; */
      /* } else if (cycle_data->bin[i] == 2) { */
      /*   // The noise diode is on. */
      /*   *tp_array = tp_on_array; */
      /*   *n_tp_array = n_tp_on_array; */
      /* } */
      ifno = cycle_data->if_no[i];
      nchannels = scan_header_data->if_num_channels[iidx];
      // Loop over the tvchannels.
      // But if our options don't know about the tvchannels, set them up now.
      if ((ifno < band_options->num_ifs) &&
	  (band_options->min_tvchannel[ifno] > 0) &&
	  (band_options->max_tvchannel[ifno] > 0)) {
	// We already have valid tvchannels.
	chan_low = band_options->min_tvchannel[ifno];
	chan_high = band_options->max_tvchannel[ifno];
      } else {
	// Get default values for this type of IF.
	nhalfchan = (nchannels % 2) == 1 ?
	  (float)((nchannels - 1) / 2) : (float)(nchannels / 2);
	chanwidth = scan_header_data->if_sideband[iidx] *
	  scan_header_data->if_bandwidth[iidx] / (nhalfchan * 2);
	default_tvchannels(nchannels, chanwidth * 1000,
			   scan_header_data->if_centre_freq[iidx] * 1000,
			   &chan_low, &chan_high);
	add_tvchannels_to_options(band_options, ifno,
				  scan_header_data->if_centre_freq[iidx],
				  scan_header_data->if_bandwidth[iidx],
				  nchannels, chan_low, chan_high);
      }

      // Find the polarisations we need in the data.
      for (j = 0; j < scan_header_data->if_num_stokes[iidx]; j++) {
	polnum = polarisation_number(scan_header_data->if_stokes_names[iidx][j]);
	if (polnum == POL_XX) {
	  reqpol[CAL_XX] = j;
	} else if (polnum == POL_YY) {
	  reqpol[CAL_YY] = j;
	}
      }
  
      for (j = chan_low; j <= chan_high; j++) {
	for (k = CAL_XX; k <= CAL_YY; k++) {
	  // Work out where our data is.
	  vidx = reqpol[k] + j * scan_header_data->if_num_stokes[iidx];
	  rcheck = crealf(cycle_data->vis[i][vidx]);
	  if (rcheck != rcheck) {
	    // Flagged channel.
	    continue;
	  }
	  if (cycle_data->bin[i] == 1) {
	    // The noise diode is off.
	    n_tp_off_array[aidx][iidx][k] += 1;
	    ARRAY_APPEND(tp_off_array[aidx][iidx][k], n_tp_off_array[aidx][iidx][k], rcheck);
	  } else if (cycle_data->bin[i] == 2) {
	    // The noise diode is on.
	    n_tp_on_array[aidx][iidx][k] += 1;
	    ARRAY_APPEND(tp_on_array[aidx][iidx][k], n_tp_on_array[aidx][iidx][k], rcheck);
	  }
	}
      }
    }
  }

  /* printf("[calculate_system_temperatures_cycle_data] after checking for all IFs in all points\n"); */
  /* print_options_set(*num_options, *options); */
  
  // We've compiled all the information from the auto-correlations at this point.
  // We can compute the system temperatures now, but before we do, we need to check
  // if previously computed Tsys has been applied to the data. Since we'll be
  // overwriting the computed Tsys numbers, we have to first reverse the application
  // otherwise we'll have no way to reverse them later.
  // We make only a simple check for this.
  if ((cycle_data->computed_tsys_applied != NULL) &&
      (cycle_data->computed_tsys_applied[0][0][0] == SYSCAL_TSYS_APPLIED)) {
    computed_tsys_applied = true;
    system_temperature_modifier(STM_REMOVE, cycle_data, scan_header_data);
  }

  // Now we're free to actually do the system temperature computations.
  for (i = 0; i < cycle_data->num_cal_ifs; i++) {
    ifno = cycle_data->cal_ifs[i];
    // The cal_ifs array is not necessarily in ascending IF order.
    ifsidx = ifno - 1;
    for (j = 0; j < cycle_data->num_cal_ants; j++) {
      for (k = CAL_XX; k <= CAL_YY; k++) {
	if (band_options->averaging_method[ifno] & AVERAGETYPE_MEDIAN) {
	  qsort(tp_on_array[j][ifsidx][k], n_tp_on_array[j][ifsidx][k], sizeof(float),
		cmpfunc_real);
	  qsort(tp_off_array[j][ifsidx][k], n_tp_off_array[j][ifsidx][k], sizeof(float),
		cmpfunc_real);
	  med_tp_on = fmedianf(tp_on_array[j][ifsidx][k], n_tp_on_array[j][ifsidx][k]);
	  med_tp_off = fmedianf(tp_off_array[j][ifsidx][k], n_tp_off_array[j][ifsidx][k]);
	  fs = 0.5 * (med_tp_on + med_tp_off);
	  fd = med_tp_on - med_tp_off;
	} else if (band_options->averaging_method[ifno] & AVERAGETYPE_MEAN) {
	  tp_on = fsumf(tp_on_array[j][ifsidx][k], n_tp_on_array[j][ifsidx][k]);
	  tp_off = fsumf(tp_off_array[j][ifsidx][k], n_tp_off_array[j][ifsidx][k]);
	  fs = 0.5 * (tp_on + tp_off);
	  fd = tp_on - tp_off;
	}
	dx = 99.995 * 99.995;
	if (fd > (0.01 * fs)) {
	  if ((k == CAL_XX) && (cycle_data->caljy_x[ifsidx][j] > 0)) {
	    dx = (fs / fd) * cycle_data->caljy_x[ifsidx][j];
	  } else if ((k == CAL_YY) && (cycle_data->caljy_y[ifsidx][j] > 0)) {
	    dx = (fs / fd) * cycle_data->caljy_y[ifsidx][j];
	  }
	}
	// Fill in the Tsys.
	cycle_data->computed_tsys[i][j][k] = sqrtf(dx);
      }
    }
  }

  // If upon entry the computed Tsys was applied to the data, we now reapply the newly
  // computed Tsys.
  if (computed_tsys_applied == true) {
    system_temperature_modifier(STM_APPLY_COMPUTED, cycle_data, scan_header_data);
  }

  // Work out from the options what to do about correcting the visibilities.
  if (band_options->systemp_reverse_online) {
    if (band_options->systemp_apply_computed) {
      // Apply the computed Tsys.
      system_temperature_modifier(STM_APPLY_COMPUTED, cycle_data, scan_header_data);
    } else {
      // We are being asked to remove all calibration.
      system_temperature_modifier(STM_REMOVE, cycle_data, scan_header_data);
    }
  } else {
    // The user wants to apply the online Tsys.
    system_temperature_modifier(STM_APPLY_CORRELATOR, cycle_data, scan_header_data);
  }
  
  // Free all the memory we've used.
  for (i = 0; i < cycle_data->num_cal_ants; i++) {
    for (j = 0; j < cycle_data->num_cal_ifs; j++) {
      for (k = CAL_XX; k <= CAL_YY; k++) {
	FREE(tp_on_array[i][j][k]);
	FREE(tp_off_array[i][j][k]);
      }
      FREE(n_tp_on_array[i][j]);
      FREE(n_tp_off_array[i][j]);
      FREE(tp_on_array[i][j]);
      FREE(tp_off_array[i][j]);
    }
    FREE(n_tp_on_array[i]);
    FREE(n_tp_off_array[i]);
    FREE(tp_on_array[i]);
    FREE(tp_off_array[i]);
  }
  FREE(tp_on_array);
  FREE(tp_off_array);
  FREE(n_tp_on_array);
  FREE(n_tp_off_array);
}

void calculate_system_temperatures(struct ampphase *ampphase,
                                   struct ampphase_options *options) {
  int i, j, k, window_idx = -1, pol_idx = -1, a1, a2, n_expected, n_actual = 0;
  float tp_on = 0, tp_off = 0, *median_array_tpon = NULL;
  float *median_array_tpoff = NULL, fs, fd, dx;
  // Recalculate the system temperature from the data in the new
  // tvchannel range, and with different options.
  
  // First, we need to calculate the total power while the noise diode
  // is on and off.

  // Find the matching window in the syscal_data structure to the
  // current data.
  for (i = 0; i < ampphase->syscal_data->num_ifs; i++) {
    if (ampphase->syscal_data->if_num[i] == ampphase->window) {
      window_idx = i;
      break;
    }
  }
  // Find the matching pol in the same way.
  for (i = 0; i < ampphase->syscal_data->num_pols; i++) {
    if (ampphase->syscal_data->pol[i] == ampphase->pol) {
      pol_idx = i;
      break;
    }
  }

  // Fail if we don't find matches.
  if ((window_idx < 0) || (pol_idx < 0)) {
    return;
  }

  // Fail if the ampphase options aren't suitable.
  if (options->num_ifs <= ampphase->window) {
    /* fprintf(stderr, "Options has %d windows, window number is %d\n", */
    /*         options->num_ifs, ampphase->window); */
    return;
  }

  // Allocate the median arrays if necessary.
  if (options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
    n_expected = 1 + options->max_tvchannel[ampphase->window] -
      options->min_tvchannel[ampphase->window];
    if (n_expected > 0) {
      CALLOC(median_array_tpon, n_expected);
      CALLOC(median_array_tpoff, n_expected);
    }
  }
  
  // We need to have the syscal_data structure filled with the data
  // from the telescope first, which shouldn't be too much of an
  // assumption.
  for (i = 0; i < ampphase->syscal_data->num_ants; i++) {
    // Find the autocorrelation for this antenna.
    for (j = 0; j < ampphase->nbaselines; j++) {
      base_to_ants(ampphase->baseline[j], &a1, &a2);
      if ((a1 == a2) && (a1 == ampphase->syscal_data->ant_num[i])) {
        // Make our sums.
        for (k = 0, n_actual = 0; k < ampphase->f_nchannels[j][0]; k++) {
          if ((ampphase->f_channel[j][0][k] >=
               options->min_tvchannel[ampphase->window]) &&
              (ampphase->f_channel[j][0][k] <=
               options->max_tvchannel[ampphase->window]) &&
              (ampphase->nbins[j] > 1)) {
            if (options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
              median_array_tpon[n_actual] = crealf(ampphase->f_raw[j][1][k]);
              median_array_tpoff[n_actual] = crealf(ampphase->f_raw[j][0][k]);
            } else {
              tp_on += crealf(ampphase->f_raw[j][1][k]);
              tp_off += crealf(ampphase->f_raw[j][0][k]);
            }
            n_actual++;
          }
        }
        if (options->averaging_method[ampphase->window] & AVERAGETYPE_MEDIAN) {
          qsort(median_array_tpon, n_actual, sizeof(float), cmpfunc_real);
          qsort(median_array_tpoff, n_actual, sizeof(float), cmpfunc_real);
          fs = 0.5 * (fmedianf(median_array_tpon, n_actual) +
                      fmedianf(median_array_tpoff, n_actual));
          fd = fmedianf(median_array_tpon, n_actual) -
            fmedianf(median_array_tpoff, n_actual);
        } else {
          fs = 0.5 * (tp_on + tp_off);
          fd = tp_on - tp_off;
        }
        if ((ampphase->syscal_data->caljy[i][window_idx][pol_idx] > 0) &&
            (fd > (0.01 * fs))) {
          dx = (fs / fd) * ampphase->syscal_data->caljy[i][window_idx][pol_idx];
        } else {
          dx = 99.995 * 99.995;
        }
        // The memory for the computed_tsys is allocated at the same
        // time as for the online_tsys.
        ampphase->syscal_data->computed_tsys[i][window_idx][pol_idx] =
          sqrtf(dx);
        // We leave it to another routine to apply any Tsys.
        ampphase->syscal_data->computed_tsys_applied[i][window_idx][pol_idx] = 0;
      }
    }
  }

}

void spectrum_data_compile_system_temperatures(struct spectrum_data *spectrum_data,
                                               struct syscal_data **syscal_data) {
  // Go through a spectrum_data structure and find all the system temperatures.
  int i, j, k, l, m, n, kk, ll, mm, high_if;
  struct syscal_data *tsys_data = NULL;

  if (spectrum_data == NULL) {
    return;
  }

  if (spectrum_data->spectrum == NULL) {
    return;
  }

  CALLOC(*syscal_data, 1);
  // We always assume 6 antennas.
  (*syscal_data)->num_ants = 6;
  CALLOC((*syscal_data)->ant_num, (*syscal_data)->num_ants);
  // Fill with the antennas in order.
  for (i = 1; i <= (*syscal_data)->num_ants; i++) {
    (*syscal_data)->ant_num[i - 1] = i;
  }
  // We only ever measure Tsys in the normal pols.
  (*syscal_data)->num_pols = 2;
  CALLOC((*syscal_data)->pol, (*syscal_data)->num_pols);
  (*syscal_data)->pol[CAL_XX] = POL_XX;
  (*syscal_data)->pol[CAL_YY] = POL_YY;
  // We set a maximum number of IFs (for future needs, for CABB it's only 2).
  (*syscal_data)->num_ifs = 34;
  CALLOC((*syscal_data)->if_num, (*syscal_data)->num_ifs);
  for (i = 0; i < (*syscal_data)->num_ifs; i++) {
    (*syscal_data)->if_num[i] = i + 1;
  }
  // Initialise the Tsys arrays with a non-specified indicator.
  CALLOC((*syscal_data)->online_tsys, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->online_tsys_applied, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->computed_tsys, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->computed_tsys_applied, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->gtp, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->sdo, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->caljy, (*syscal_data)->num_ants);
  // Do the same for the non-Tsys parameters.
  CALLOC((*syscal_data)->xyphase, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->xyamp, (*syscal_data)->num_ants);
  // And the antenna-based parameters.
  CALLOC((*syscal_data)->parangle, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->tracking_error_max, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->tracking_error_rms, (*syscal_data)->num_ants);
  CALLOC((*syscal_data)->flagging, (*syscal_data)->num_ants);
  for (i = 0; i < (*syscal_data)->num_ants; i++) {
    CALLOC((*syscal_data)->online_tsys[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->online_tsys_applied[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->computed_tsys[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->computed_tsys_applied[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->gtp[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->sdo[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->caljy[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->xyphase[i], (*syscal_data)->num_ifs);
    CALLOC((*syscal_data)->xyamp[i], (*syscal_data)->num_ifs);
    for (j = 0; j < (*syscal_data)->num_ifs; j++) {
      CALLOC((*syscal_data)->online_tsys[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->online_tsys_applied[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->computed_tsys[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->computed_tsys_applied[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->gtp[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->sdo[i][j], (*syscal_data)->num_pols);
      CALLOC((*syscal_data)->caljy[i][j], (*syscal_data)->num_pols);
      for (k = 0; k < (*syscal_data)->num_pols; k++) {
        (*syscal_data)->online_tsys[i][j][k] = -1;
        (*syscal_data)->online_tsys_applied[i][j][k] = -1;
        (*syscal_data)->computed_tsys[i][j][k] = -1;
        (*syscal_data)->computed_tsys_applied[i][j][k] = -1;
        (*syscal_data)->gtp[i][j][k] = -1;
        (*syscal_data)->sdo[i][j][k] = -1;
        (*syscal_data)->caljy[i][j][k] = -1;
      }
      (*syscal_data)->xyphase[i][j] = -1; // Makes no sense...
      (*syscal_data)->xyamp[i][j] = -1;
    }
    (*syscal_data)->parangle[i] = -1; // This default makes no sense...
    (*syscal_data)->tracking_error_max[i] = -1;
    (*syscal_data)->tracking_error_rms[i] = -1;
    (*syscal_data)->flagging[i] = -1;
  }

  high_if = 0;
  for (i = 0; i < spectrum_data->num_ifs; i++) {
    if (spectrum_data->spectrum[i] == NULL) {
      continue;
    }
    for (j = 0; j < spectrum_data->num_pols; j++) {
      if (spectrum_data->spectrum[i][j] == NULL) {
        continue;
      }
      // Check if this spectrum has all the required cal data.
      if ((spectrum_data->spectrum[i][j]->syscal_data != NULL) &&
          (spectrum_data->spectrum[i][j]->syscal_data->num_ifs > 0) &&
          (spectrum_data->spectrum[i][j]->syscal_data->num_ants > 0) &&
          (spectrum_data->spectrum[i][j]->syscal_data->num_pols > 0) &&
          (spectrum_data->spectrum[i][j]->syscal_data->online_tsys != NULL)) {
        tsys_data = spectrum_data->spectrum[i][j]->syscal_data;
        for (k = 0; k < tsys_data->num_ants; k++) {
          kk = tsys_data->ant_num[k] - 1;
          for (l = 0; l < tsys_data->num_ifs; l++) {
            ll = tsys_data->if_num[l] - 1;
            for (m = 0; m < tsys_data->num_pols; m++) {
              mm = -1;
              for (n = 0; n < (*syscal_data)->num_pols; n++) {
                if ((*syscal_data)->pol[n] == tsys_data->pol[m]) {
                  mm = n; //tsys_data->pol[m];
                  break;
                }
              }
              if (mm < 0) {
                continue;
              }
              if ((*syscal_data)->online_tsys[kk][ll][mm] == -1) {
                (*syscal_data)->online_tsys[kk][ll][mm] =
                  tsys_data->online_tsys[k][l][m];
                (*syscal_data)->online_tsys_applied[kk][ll][mm] =
                  tsys_data->online_tsys_applied[k][l][m];
                if (ll > high_if) {
                  high_if = ll;
                }
              }
              if ((*syscal_data)->computed_tsys[kk][ll][mm] == -1) {
                (*syscal_data)->computed_tsys[kk][ll][mm] =
                  tsys_data->computed_tsys[k][l][m];
                (*syscal_data)->computed_tsys_applied[kk][ll][mm] =
                  tsys_data->computed_tsys_applied[k][l][m];
              }
              if ((*syscal_data)->gtp[kk][ll][mm] == -1) {
                (*syscal_data)->gtp[kk][ll][mm] =
                  tsys_data->gtp[k][l][m];
                (*syscal_data)->sdo[kk][ll][mm] =
                  tsys_data->sdo[k][l][m];
                (*syscal_data)->caljy[kk][ll][mm] =
                  tsys_data->caljy[k][l][m];
              }
              if ((*syscal_data)->tracking_error_max[kk] == -1) {
                (*syscal_data)->parangle[kk] = tsys_data->parangle[k];
                (*syscal_data)->tracking_error_max[kk] =
                  tsys_data->tracking_error_max[k];
                (*syscal_data)->tracking_error_rms[kk] =
                  tsys_data->tracking_error_rms[k];
                (*syscal_data)->flagging[kk] = tsys_data->flagging[k];
              }
              if ((*syscal_data)->xyamp[kk][ll] == -1) {
                (*syscal_data)->xyphase[kk][ll] = tsys_data->xyphase[k][l];
                (*syscal_data)->xyamp[kk][ll] = tsys_data->xyamp[k][l];
              }
            }
          }
        }
      }
    }
  }

  // Now we know how many IFs actually had useful data, we free the unnecessary memory.
  for (i = 0; i < (*syscal_data)->num_ants; i++) {
    for (j = (high_if + 1); j < (*syscal_data)->num_ifs; j++) {
      FREE((*syscal_data)->online_tsys[i][j]);
      FREE((*syscal_data)->online_tsys_applied[i][j]);
      FREE((*syscal_data)->computed_tsys[i][j]);
      FREE((*syscal_data)->computed_tsys_applied[i][j]);
      FREE((*syscal_data)->gtp[i][j]);
      FREE((*syscal_data)->sdo[i][j]);
      FREE((*syscal_data)->caljy[i][j]);
    }
    REALLOC((*syscal_data)->online_tsys[i], (high_if + 1));
    REALLOC((*syscal_data)->online_tsys_applied[i], (high_if + 1));
    REALLOC((*syscal_data)->computed_tsys[i], (high_if + 1));
    REALLOC((*syscal_data)->computed_tsys_applied[i], (high_if + 1));
    REALLOC((*syscal_data)->gtp[i], (high_if + 1));
    REALLOC((*syscal_data)->sdo[i], (high_if + 1));
    REALLOC((*syscal_data)->caljy[i], (high_if + 1));
    REALLOC((*syscal_data)->xyphase[i], (high_if + 1));
    REALLOC((*syscal_data)->xyamp[i], (high_if + 1));
  }
  REALLOC((*syscal_data)->if_num, (high_if + 1));
  (*syscal_data)->num_ifs = (high_if + 1);

}

/*!
 *  \brief Routine to modify the raw visibilities of a cycle to account for
 *         system temperature
 *  \param action the magic number for what type of action this routine should do:
 *                - STM_APPLY_CORRELATOR: apply the correlator Tsys
 *                - STM_APPLY_COMPUTED: apply the computed Tsys
 *                - STM_REMOVE: remove any applied Tsys
 *  \param cycle_data the cycle data to be modified
 *  \param scan_header_data the header of the scan this cycle_data comes from
 */
void system_temperature_modifier(int action,
				 struct cycle_data *cycle_data,
				 struct scan_header_data *scan_header_data) {
  int i, j, k, bl, ant1_idx, ant2_idx, iidx, ifno, *ant1_polidx = NULL, *ant2_polidx = NULL;
  int vidx, ***applied_target = NULL, applied_value = -1, polnum;
  float rcheck, mfac = 1.0, ***tsys_target = NULL;
  bool already_applied = false, needs_removal = false, modified = false;
  
  if ((action == STM_APPLY_CORRELATOR) || (action == STM_APPLY_COMPUTED)) {
    // Check first if something has already been applied.
    for (i = 0; i < cycle_data->num_cal_ifs; i++) {
      for (j = 0; j < cycle_data->num_cal_ants; j++) {
	for (k = 0; k < 2; k++) {
	  if (((cycle_data->tsys_applied[i][j][k] == SYSCAL_TSYS_APPLIED) &&
	       (action == STM_APPLY_CORRELATOR)) ||
	      ((cycle_data->computed_tsys_applied[i][j][k] == SYSCAL_TSYS_APPLIED) &&
	       (action == STM_APPLY_COMPUTED))) {
	    // The action doesn't need to be done.
	    already_applied = true;
	  } else if (((cycle_data->tsys_applied[i][j][k] == SYSCAL_TSYS_APPLIED) &&
		      (action == STM_APPLY_COMPUTED)) ||
		     ((cycle_data->computed_tsys_applied[i][j][k] == SYSCAL_TSYS_APPLIED) &&
		      (action == STM_APPLY_CORRELATOR))) {
	    // The opposite system temperature needs to be removed.
	    needs_removal = true;
	  }
	}
      }
    }

    // Check for actions.
    if ((already_applied == true) && (needs_removal == true)) {
      // What??
      fprintf(stderr, "[system_temperature_modifier] unusable system temperature arrays detected\n");
      return;
    }
    if (already_applied == true) {
      // Don't need to do anything.
      return;
    }
    if (needs_removal == true) {
      // Call ourselves to remove what's currently there.
      system_temperature_modifier(STM_REMOVE, cycle_data, scan_header_data);
    }
  }

  // Go through all the visibilities and make the appropriate computation.
  for (i = 0; i < cycle_data->num_points; i++) {
    bl = ants_to_base(cycle_data->ant1[i], cycle_data->ant2[i]);
    if (bl < 0) {
      continue;
    }
    // Work out where the system temperatures will be in our array.
    ant1_idx = cycle_data->ant1[i] - 1;
    ant2_idx = cycle_data->ant2[i] - 1;
    ifno = cycle_data->if_no[i];
    iidx = ifno - 1;
    // Each polarisation value will need a different combination of antenna
    // polarisations; we work these out now.
    MALLOC(ant1_polidx, scan_header_data->if_num_stokes[iidx]);
    MALLOC(ant2_polidx, scan_header_data->if_num_stokes[iidx]);
    for (j = 0; j < scan_header_data->if_num_stokes[iidx]; j++) {
      polnum = polarisation_number(scan_header_data->if_stokes_names[iidx][j]);
      if (polnum == POL_XX) {
	ant1_polidx[j] = CAL_XX;
	ant2_polidx[j] = CAL_XX;
      } else if (polnum == POL_YY) {
	ant1_polidx[j] = CAL_YY;
	ant2_polidx[j] = CAL_YY;
      } else if (polnum == POL_XY) {
	ant1_polidx[j] = CAL_XX;
	ant2_polidx[j] = CAL_YY;
      } else if (polnum == POL_YX) {
	ant1_polidx[j] = CAL_YY;
	ant2_polidx[j] = CAL_XX;
      } else {
	// We don't modify weird single polarisation stuff.
	ant1_polidx[j] = -1;
	ant2_polidx[j] = -1;
      }
    }

    // Now make the modifications.
    for (j = 0; j < scan_header_data->if_num_stokes[iidx]; j++) {
      if ((ant1_polidx[j] == -1) || (ant2_polidx[j] == -1)) {
	// Don't modify.
	continue;
      }
      
      if (action == STM_APPLY_CORRELATOR) {
	mfac = sqrtf(cycle_data->tsys[iidx][ant1_idx][ant1_polidx[j]] *
		     cycle_data->tsys[iidx][ant2_idx][ant2_polidx[j]]);
      } else if (action == STM_APPLY_COMPUTED) {
	mfac = sqrtf(cycle_data->computed_tsys[iidx][ant1_idx][ant1_polidx[j]] *
		     cycle_data->computed_tsys[iidx][ant2_idx][ant2_polidx[j]]);
      } else if (action == STM_REMOVE) {
	// Work out which one has been applied.
	if ((cycle_data->tsys_applied[iidx][ant1_idx][ant1_polidx[j]] == SYSCAL_TSYS_APPLIED) &&
	    (cycle_data->tsys_applied[iidx][ant2_idx][ant2_polidx[j]] == SYSCAL_TSYS_APPLIED)) {
	  tsys_target = cycle_data->tsys;
	  applied_target = cycle_data->tsys_applied;
	} else if ((cycle_data->computed_tsys_applied[iidx][ant1_idx][ant1_polidx[j]] ==
		    SYSCAL_TSYS_APPLIED) &&
		   (cycle_data->computed_tsys_applied[iidx][ant2_idx][ant2_polidx[j]] ==
		    SYSCAL_TSYS_APPLIED)) {
	  tsys_target = cycle_data->computed_tsys;
	  applied_target = cycle_data->computed_tsys_applied;
	}
	mfac = 1.0 / sqrtf(tsys_target[iidx][ant1_idx][ant1_polidx[j]] *
			   tsys_target[iidx][ant1_idx][ant1_polidx[j]]);
      }
      if ((mfac != mfac) || (mfac < 0)) {
	continue;
      }
      for (k = 0; k < scan_header_data->if_num_channels[iidx]; k++) {
	// Work out where our data is.
	vidx = j + k * scan_header_data->if_num_stokes[iidx];
	rcheck = crealf(cycle_data->vis[i][vidx]);
	if (rcheck != rcheck) {
	  // This is NaN, so don't do anything to it.
	  continue;
	}
	cycle_data->vis[i][vidx] *= mfac;
	modified = true;
      }
    }

    FREE(ant1_polidx);
    FREE(ant2_polidx);
  }

  if (modified == true) {
    if (action == STM_APPLY_CORRELATOR) {
      applied_target = cycle_data->tsys_applied;
      applied_value = SYSCAL_TSYS_APPLIED;
    } else if (action == STM_APPLY_COMPUTED) {
      applied_target = cycle_data->computed_tsys_applied;
      applied_value = SYSCAL_TSYS_APPLIED;
    } else if (action == STM_REMOVE) {
      applied_value = SYSCAL_TSYS_NOT_APPLIED;
    }
    if (applied_target != NULL) {
      for (i = 0; i < cycle_data->num_cal_ifs; i++) {
	for (j = 0; j < cycle_data->num_cal_ants; j++) {
	  for (k = 0; k < 2; k++) {
	    applied_target[i][j][k] = applied_value;
	  }
	}
      }
    }
  }

}

/*!
 *  \brief Produce a text description of an averaging type magic number
 *  \param averaging_type a bitwise-OR combination of the AVERAGETYPE_*
 *                        magic numbers
 *  \param output the variable to store the string output
 *  \param output_length the maximum length of the string that can fit in
 *                       \a output
 */
void averaging_type_string(int averaging_type, char *output,
			   int output_length) {
  int ol;
  char scavec[7], meanmed[7];
  
  // Initialise the string.
  output[0] = 0;
  ol = 0;

  if (averaging_type & AVERAGETYPE_VECTOR) {
    strcpy(scavec, "VECTOR");
    ol += 6;
  } else if (averaging_type & AVERAGETYPE_SCALAR) {
    strcpy(scavec, "SCALAR");
    ol += 6;
  } else {
    strcpy(scavec, "ERROR");
    ol += 5;
  }

  if (averaging_type & AVERAGETYPE_MEAN) {
    strcpy(meanmed, "MEAN");
    ol += 4;
  } else if (averaging_type & AVERAGETYPE_MEDIAN) {
    strcpy(meanmed, "MEDIAN");
    ol += 6;
  } else {
    strcpy(meanmed, "ERROR");
    ol += 5;
  }

  ol += 2; // For the space and the end character.
  ol = (ol < output_length) ? ol : output_length;
  snprintf(output, ol, "%s %s", scavec, meanmed);
}

/*!
 *  \brief Produce a description of the supplied set of options
 *  \param num_options the number of structures in the set
 *  \param options the set of options structures
 *  \param output if not set to NULL, put the output in this string instead of
 *                printing it to screen
 *  \param output_length the maximum length of the string that can fit in \a output
 */
void print_options_set(int num_options,
		       struct ampphase_options **options, char *output,
		       int output_length) {
  int i, j;
  char avtype[20];

  // Initialise the strings.
  if (output != NULL) {
    output[0] = 0;
  }
  
  if ((options == NULL) || (*options == NULL)) {
    info_print(output, output_length, "[print_options_set] NULL passed as options set\n");
    return;
  }

  info_print(output, output_length, "Options set has %d elements:\n", num_options);
  for (i = 0; i < num_options; i++) {
    info_print(output, output_length, "  SET %d:\n", i);
    info_print(output, output_length, "     PHASE IN DEGREES: %s\n",
	       ((options[i]->phase_in_degrees) ? "YES": "NO"));
    info_print(output, output_length, "     INCLUDE FLAGGED: %s\n",
	       ((options[i]->include_flagged_data) ? "YES" : "NO"));
    info_print(output, output_length, "     TSYS CORRECTION:");
    if ((options[i]->systemp_reverse_online == true) &&
	(options[i]->systemp_apply_computed == false)) {
      info_print(output, output_length, " NONE\n");
    } else {
      info_print(output, output_length, " %s\n",
		 ((options[i]->systemp_reverse_online == false) ? "CORRELATOR" : "COMPUTED"));
    }
    info_print(output, output_length,   "     # WINDOWS: %d\n", (options[i]->num_ifs - 1));
    for (j = 1; j < options[i]->num_ifs; j++) {
      info_print(output, output_length, "     --WINDOW %d:\n", j);
      info_print(output, output_length, "        CENTRE FREQ: %.1f MHz\n",
		 options[i]->if_centre_freq[j]);
      info_print(output, output_length, "        BANDWIDTH: %.1f MHz\n",
		 options[i]->if_bandwidth[j]);
      info_print(output, output_length, "        # CHANNELS: %d\n",
		 options[i]->if_nchannels[j]);
      info_print(output, output_length, "        TVCHAN RANGE: %d - %d\n",
	     options[i]->min_tvchannel[j], options[i]->max_tvchannel[j]);
      info_print(output, output_length, "        DELAY AVERAGING: %d\n",
	     options[i]->delay_averaging[j]);
      averaging_type_string(options[i]->averaging_method[j], avtype, 20);
      info_print(output, output_length, "        AVERAGING METHOD: %s\n",
		 avtype);
    }
  }
}


/*!
 *  \brief Produce a description of the supplied header
 *  \param header_data the scan header data structure to summarise
 *  \param output if not set to NULL, put the output in this string
 *                instead of printing it to screen
 *  \param output_length the maximum length of the string that can fit in \a output
 */
void print_information_scan_header(struct scan_header_data *header_data,
				   char *output, int output_length) {
  int i;

  // Initialise the strings.
  if (output != NULL) {
    output[0] = 0;
  }
  
  // Print out the number of IFs.
  info_print(output, output_length, " NUMBER OF WINDOWS: %d\n", header_data->num_ifs);

  // Each IF now.
  for (i = 0; i < header_data->num_ifs; i++) {
    info_print(output, output_length, " IF %d: CF %.1f MHz BW %.1f MHz NCHAN %d\n",
	       (i + 1), header_data->if_centre_freq[i],
	       header_data->if_bandwidth[i], header_data->if_num_channels[i]);
  }
}

/*!
 *  \brief Perform averaging over some ampphase data
 *  \param ampphase the data before averaging
 *  \param avg_ampphase upon exit this will contain the averaged data
 *  \param averaging the number of channels to average together
 *  \param averaging_type bitwise-OR combination of AVERAGETYPE_* magic numbers
 *                        specifying how the average should be computed
 *  \param phase_in_degrees whether to output phase in degrees (true) or not
 */
void chanaverage_ampphase(struct ampphase *ampphase, struct ampphase *avg_ampphase,
			  int averaging, int averaging_type, bool phase_in_degrees) {
  int n_delavg_expected, i, j, k, l, n_points;
  int n_unflagged_points, chan_index, unflagged_index;
  float *median_array_amplitude = NULL, *median_array_phase = NULL;
  float *median_array_channel = NULL, *median_array_frequency = NULL;
  float *median_unflagged_frequency = NULL, *median_unflagged_channel = NULL;
  float *median_unflagged_amplitude = NULL, *median_unflagged_phase = NULL;
  float checkval;
  float complex *median_array_raw = NULL, *median_unflagged_raw = NULL;
  
  // Make the arrays required.
  if (averaging < 1) {
    averaging = 1;
  }
  n_delavg_expected = (int)ceilf((float)ampphase->nchannels / (float)averaging);
  if (n_delavg_expected < 1) {
    n_delavg_expected = 1;
  }
  CALLOC(median_array_amplitude, averaging);
  CALLOC(median_array_phase, averaging);
  CALLOC(median_array_raw, averaging);
  CALLOC(median_array_channel, averaging);
  CALLOC(median_array_frequency, averaging);
  CALLOC(median_unflagged_amplitude, averaging);
  CALLOC(median_unflagged_phase, averaging);
  CALLOC(median_unflagged_raw, averaging);
  CALLOC(median_unflagged_channel, averaging);
  CALLOC(median_unflagged_frequency, averaging);
  
  // Set the quantities in the output.
  avg_ampphase->nchannels = n_delavg_expected;
  STRUCTCOPY(ampphase, avg_ampphase, nbaselines);
  CALLOC(avg_ampphase->baseline, ampphase->nbaselines);
  CALLOC(avg_ampphase->nbins, ampphase->nbaselines);
  CALLOC(avg_ampphase->flagged_bad, ampphase->nbaselines);
  for (i = 0; i < ampphase->nbaselines; i++) {
    STRUCTCOPY(ampphase, avg_ampphase, baseline[i]);
    STRUCTCOPY(ampphase, avg_ampphase, nbins[i]);
    CALLOC(avg_ampphase->flagged_bad[i], ampphase->nbins[i]);
    for (j = 0; j < ampphase->nbins[i]; j++) {
      STRUCTCOPY(ampphase, avg_ampphase, flagged_bad[i][j]);
    }
  }
  STRUCTCOPY(ampphase, avg_ampphase, pol);
  STRUCTCOPY(ampphase, avg_ampphase, window);
  strncpy(avg_ampphase->window_name, ampphase->window_name, 8);
  strncpy(avg_ampphase->obsdate, ampphase->obsdate, OBSDATE_LENGTH);
  STRUCTCOPY(ampphase, avg_ampphase, ut_seconds);
  strncpy(avg_ampphase->scantype, ampphase->scantype, OBSTYPE_LENGTH);

  // Copy the metadata.
  CALLOC(avg_ampphase->options, 1);
  copy_ampphase_options(avg_ampphase->options, ampphase->options);
  copy_metinfo(&(avg_ampphase->metinfo), &(ampphase->metinfo));
  CALLOC(avg_ampphase->syscal_data, 1);
  copy_syscal_data(avg_ampphase->syscal_data, ampphase->syscal_data);

  // Set initial defaults for the global maxima and minima.
  avg_ampphase->min_amplitude_global = INFINITY;
  avg_ampphase->max_amplitude_global = -INFINITY;
  avg_ampphase->min_phase_global = INFINITY;
  avg_ampphase->max_phase_global = -INFINITY;
  
  // Allocate some memory.
  CALLOC(avg_ampphase->weight, ampphase->nbaselines);
  CALLOC(avg_ampphase->amplitude, ampphase->nbaselines);
  CALLOC(avg_ampphase->phase, ampphase->nbaselines);
  CALLOC(avg_ampphase->raw, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_nchannels, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_channel, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_frequency, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_weight, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_amplitude, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_phase, ampphase->nbaselines);
  CALLOC(avg_ampphase->f_raw, ampphase->nbaselines);
  CALLOC(avg_ampphase->min_amplitude, ampphase->nbaselines);
  CALLOC(avg_ampphase->max_amplitude, ampphase->nbaselines);
  CALLOC(avg_ampphase->min_phase, ampphase->nbaselines);
  CALLOC(avg_ampphase->max_phase, ampphase->nbaselines);
  CALLOC(avg_ampphase->min_real, ampphase->nbaselines);
  CALLOC(avg_ampphase->max_real, ampphase->nbaselines);
  CALLOC(avg_ampphase->min_imag, ampphase->nbaselines);
  CALLOC(avg_ampphase->max_imag, ampphase->nbaselines);
  CALLOC(avg_ampphase->channel, n_delavg_expected);
  CALLOC(avg_ampphase->frequency, n_delavg_expected);
  
  // The averaging loop.
  for (i = 0; i < ampphase->nbaselines; i++) {

    // Set initial defaults for the baseline maxima and minima.
    avg_ampphase->min_amplitude[i] = INFINITY;
    avg_ampphase->max_amplitude[i] = -INFINITY;
    avg_ampphase->min_phase[i] = INFINITY;
    avg_ampphase->max_phase[i] = -INFINITY;
    avg_ampphase->min_real[i] = INFINITY;
    avg_ampphase->max_real[i] = -INFINITY;
    avg_ampphase->min_imag[i] = INFINITY;
    avg_ampphase->max_imag[i] = -INFINITY;
    
    // Allocate required memory.
    CALLOC(avg_ampphase->weight[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->amplitude[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->phase[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->raw[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_nchannels[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_channel[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_frequency[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_weight[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_amplitude[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_phase[i], ampphase->nbins[i]);
    CALLOC(avg_ampphase->f_raw[i], ampphase->nbins[i]);
    
    for (j = 0; j < ampphase->nbins[i]; j++) {

      // Allocate required memory for the normal arrays.
      CALLOC(avg_ampphase->weight[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->amplitude[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->phase[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->raw[i][j], avg_ampphase->nchannels);
      // We allocate the unflagged arrays large to begin with, we can always
      // lower their size later.
      CALLOC(avg_ampphase->f_channel[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->f_frequency[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->f_weight[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->f_amplitude[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->f_phase[i][j], avg_ampphase->nchannels);
      CALLOC(avg_ampphase->f_raw[i][j], avg_ampphase->nchannels);
      
      // We traverse all the channels in the original structure, with a stride
      // of the averaging parameter.
      for (k = 0, chan_index = 0, unflagged_index = 0; k < ampphase->nchannels;
	   k += averaging, chan_index++) {
	n_points = 0;
	n_unflagged_points = 0;
	if (averaging == 1) {
	  // Special case where we can just copy everything over.
	  STRUCTCOPY(ampphase, avg_ampphase, weight[i][j][k]);
	  STRUCTCOPY(ampphase, avg_ampphase, amplitude[i][j][k]);
	  STRUCTCOPY(ampphase, avg_ampphase, phase[i][j][k]);
	  STRUCTCOPY(ampphase, avg_ampphase, raw[i][j][k]);
	  if (k == 0) {
	    STRUCTCOPY(ampphase, avg_ampphase, f_nchannels[i][j]);
	    if (j == 0) {
	      STRUCTCOPY(ampphase, avg_ampphase, min_amplitude[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, max_amplitude[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, min_phase[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, max_phase[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, min_real[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, max_real[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, min_imag[i]);
	      STRUCTCOPY(ampphase, avg_ampphase, max_imag[i]);
	    }
	  }
	  if (k < ampphase->f_nchannels[i][j]) {
	    STRUCTCOPY(ampphase, avg_ampphase, f_channel[i][j][k]);
	    STRUCTCOPY(ampphase, avg_ampphase, f_frequency[i][j][k]);
	    STRUCTCOPY(ampphase, avg_ampphase, f_weight[i][j][k]);
	    STRUCTCOPY(ampphase, avg_ampphase, f_amplitude[i][j][k]);
	    STRUCTCOPY(ampphase, avg_ampphase, f_phase[i][j][k]);
	    STRUCTCOPY(ampphase, avg_ampphase, f_raw[i][j][k]);
	  }
	} else {
	  for (l = 0; l < averaging; l++) {
	    if ((k + l) < ampphase->nchannels) {
	      // Easy to use all the channels regardless of the flagging.
	      median_array_amplitude[n_points] = ampphase->amplitude[i][j][k + l];
	      median_array_phase[n_points] = ampphase->phase[i][j][k + l];
	      median_array_raw[n_points] = ampphase->raw[i][j][k + l];
	      median_array_channel[n_points] = ampphase->channel[k + l];
	      median_array_frequency[n_points] = ampphase->frequency[k + l];
	      n_points++;
	    } else {
	      break;
	    }
	  }
	  // Find the unflagged channels required.
	  while ((unflagged_index < ampphase->f_nchannels[i][j]) &&
		 (ampphase->f_channel[i][j][unflagged_index] >= k) &&
		 (ampphase->f_channel[i][j][unflagged_index] < (k + averaging))) {
	    median_unflagged_amplitude[n_unflagged_points] =
	      ampphase->f_amplitude[i][j][unflagged_index];
	    median_unflagged_phase[n_unflagged_points] =
	      ampphase->f_phase[i][j][unflagged_index];
	    median_unflagged_raw[n_unflagged_points] =
	      ampphase->f_raw[i][j][unflagged_index];
	    median_unflagged_channel[n_unflagged_points] =
	      ampphase->f_channel[i][j][unflagged_index];
	    median_unflagged_frequency[n_unflagged_points] =
	      ampphase->f_frequency[i][j][unflagged_index];
	    n_unflagged_points++;
	    unflagged_index++;
	  }
	  // Set the averaged values.
	  if (n_points > 0) {
	    if (averaging_type & AVERAGETYPE_MEAN) {
	      avg_ampphase->raw[i][j][chan_index] = fcmeanfc(median_array_raw, n_points);
	      avg_ampphase->channel[chan_index] = (int)fmeanf(median_array_channel, n_points);
	      avg_ampphase->frequency[chan_index] = fmeanf(median_array_frequency, n_points);
	      if (averaging_type & AVERAGETYPE_SCALAR) {
		avg_ampphase->amplitude[i][j][chan_index] =
		  fmeanf(median_array_amplitude, n_points);
		avg_ampphase->phase[i][j][chan_index] =
		  fmeanf(median_array_phase, n_points);
	      } else if (averaging_type & AVERAGETYPE_VECTOR) {
		avg_ampphase->amplitude[i][j][chan_index] =
		  cabsf(avg_ampphase->raw[i][j][chan_index]);
		avg_ampphase->phase[i][j][chan_index] =
		  cargf(avg_ampphase->raw[i][j][chan_index]);
		if (phase_in_degrees) {
		  avg_ampphase->phase[i][j][chan_index] *= 180.0 / M_PI;
		}
	      }
	    } else if (averaging_type & AVERAGETYPE_MEDIAN) {
	      // Work out the channel and frequency first.
	      qsort(median_array_channel, n_points, sizeof(float), cmpfunc_real);
	      qsort(median_array_frequency, n_points, sizeof(float), cmpfunc_real);
	      avg_ampphase->channel[chan_index] =
		(int)fmedianf(median_array_channel, n_points);
	      avg_ampphase->frequency[chan_index] =
		fmedianf(median_array_frequency, n_points);

	      qsort(median_array_raw, n_points, sizeof(float complex), cmpfunc_complex);
	      avg_ampphase->raw[i][j][chan_index] =
		fcmedianfc(median_array_raw, n_points);
	      if (averaging_type & AVERAGETYPE_SCALAR) {
		qsort(median_array_amplitude, n_points, sizeof(float), cmpfunc_real);
		qsort(median_array_phase, n_points, sizeof(float), cmpfunc_real);
		avg_ampphase->amplitude[i][j][chan_index] =
		  fmedianf(median_array_amplitude, n_points);
		avg_ampphase->phase[i][j][chan_index] =
		  fmedianf(median_array_phase, n_points);
	      } else if (averaging_type & AVERAGETYPE_VECTOR) {
		avg_ampphase->amplitude[i][j][chan_index] =
		  cabsf(avg_ampphase->raw[i][j][chan_index]);
		avg_ampphase->phase[i][j][chan_index] =
		  cargf(avg_ampphase->raw[i][j][chan_index]);
		if (phase_in_degrees) {
		  avg_ampphase->phase[i][j][chan_index] *= 180.0 / M_PI;
		}
	      }
	    }
	  }
	  if (n_unflagged_points > 0) {
	    if (averaging_type & AVERAGETYPE_MEAN) {
	      avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]] =
		fcmeanfc(median_unflagged_raw, n_unflagged_points);
	      avg_ampphase->f_channel[i][j][avg_ampphase->f_nchannels[i][j]] =
		(int)fmeanf(median_unflagged_channel, n_unflagged_points);
	      avg_ampphase->f_frequency[i][j][avg_ampphase->f_nchannels[i][j]] =
		fmeanf(median_unflagged_frequency, n_unflagged_points);
	      if (averaging_type & AVERAGETYPE_SCALAR) {
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] =
		  fmeanf(median_unflagged_amplitude, n_unflagged_points);
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] =
		  fmeanf(median_unflagged_phase, n_unflagged_points);
	      } else if (averaging_type & AVERAGETYPE_VECTOR) {
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] =
		  cabsf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] =
		  cargf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
		if (phase_in_degrees) {
		  avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] *= 180.0 / M_PI;
		}
	      }
	    } else if (averaging_type & AVERAGETYPE_MEDIAN) {
	      qsort(median_unflagged_channel, n_unflagged_points, sizeof(float), cmpfunc_real);
	      qsort(median_unflagged_frequency, n_unflagged_points, sizeof(float),
		    cmpfunc_real);
	      avg_ampphase->f_channel[i][j][avg_ampphase->f_nchannels[i][j]] =
		(int)fmedianf(median_unflagged_channel, n_unflagged_points);
	      avg_ampphase->f_frequency[i][j][avg_ampphase->f_nchannels[i][j]] =
		fmedianf(median_unflagged_frequency, n_unflagged_points);

	      qsort(median_unflagged_raw, n_unflagged_points, sizeof(float complex),
		    cmpfunc_complex);
	      avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]] =
		fcmedianfc(median_unflagged_raw, n_unflagged_points);
	      if (averaging_type & AVERAGETYPE_SCALAR) {
		qsort(median_unflagged_amplitude, n_unflagged_points, sizeof(float),
		      cmpfunc_real);
		qsort(median_unflagged_phase, n_unflagged_points, sizeof(float),
		      cmpfunc_real);
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] =
		  fmedianf(median_unflagged_amplitude, n_unflagged_points);
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] =
		  fmedianf(median_unflagged_phase, n_unflagged_points);
	      } else if (averaging_type & AVERAGETYPE_VECTOR) {
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] =
		  cabsf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] =
		  cargf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
		if (phase_in_degrees) {
		  avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] *= 180.0 / M_PI;
		}
	      }
	    }
	    // That's a successful unflagged channel.
	    // Update the maxima and minima.
	    checkval = crealf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
	    if (checkval < avg_ampphase->min_real[i]) {
	      avg_ampphase->min_real[i] = checkval;
	    }
	    if (checkval > avg_ampphase->max_real[i]) {
	      avg_ampphase->max_real[i] = checkval;
	    }
	    checkval = cimagf(avg_ampphase->f_raw[i][j][avg_ampphase->f_nchannels[i][j]]);
	    if (checkval < avg_ampphase->min_imag[i]) {
	      avg_ampphase->min_imag[i] = checkval;
	    }
	    if (checkval > avg_ampphase->max_imag[i]) {
	      avg_ampphase->max_imag[i] = checkval;
	    }
	    if (avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] <
		avg_ampphase->min_amplitude_global) {
	      avg_ampphase->min_amplitude_global =
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] >
		avg_ampphase->max_amplitude_global) {
	      avg_ampphase->max_amplitude_global =
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] <
		avg_ampphase->min_amplitude[i]) {
	      avg_ampphase->min_amplitude[i] =
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]] >
		avg_ampphase->max_amplitude[i]) {
	      avg_ampphase->max_amplitude[i] =
		avg_ampphase->f_amplitude[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] <
		avg_ampphase->min_phase_global) {
	      avg_ampphase->min_phase_global =
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] >
		avg_ampphase->max_phase_global) {
	      avg_ampphase->max_phase_global =
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] <
		avg_ampphase->min_phase[i]) {
	      avg_ampphase->min_phase[i] =
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    if (avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]] >
		avg_ampphase->max_phase[i]) {
	      avg_ampphase->max_phase[i] =
		avg_ampphase->f_phase[i][j][avg_ampphase->f_nchannels[i][j]];
	    }
	    // Go to the next channel.
	    avg_ampphase->f_nchannels[i][j] += 1;
	  }
	} // CALLOC makes everything 0 by default, for when n_points <= 0.
      }
    }
  }

  // Free the temporary arrays we made for median averaging.
  FREE(median_array_amplitude);
  FREE(median_array_phase);
  FREE(median_array_raw);
  FREE(median_array_channel);
  FREE(median_array_frequency);
  FREE(median_unflagged_amplitude);
  FREE(median_unflagged_phase);
  FREE(median_unflagged_raw);
  FREE(median_unflagged_channel);
  FREE(median_unflagged_frequency);
}

/*!
 *  \brief Return the phase on a specified baseline
 *  \param vis_quantities pre-computed averaged data
 *  \param ant1 the first antenna in the baseline pair, should be a real number not an index
 *  \param ant2 the second antenna in the baseline pair, should be a real number not an index
 *  \param bin the bin number for the phase; if this is larger than the number of bins
 *             available, the phase from the last bin is returned
 *  \return the phase on the baseline formed between \a ant1 and \a ant2,
 *          properly signed considering the relationship between \a ant1 and \a ant2
 *          and the pre-computed phase in \a vis_quantities
 *
 * This is primarily a helper function to make it easier to compute closure phase when
 * at least one of the phase terms will need to have its sign swapped.
 */
float baseline_phase(struct vis_quantities *vis_quantities,
		     int ant1, int ant2, int bin) {
  int basenum, i, rbin;

  // The following routine automatically swaps the antennas to the correct order
  // to give us the baseline number.
  basenum = ants_to_base(ant1, ant2);

  // Find this baseline number in the data.
  for (i = 0; i < vis_quantities->nbaselines; i++) {
    if (vis_quantities->baseline[i] == basenum) {
      rbin = (bin < vis_quantities->nbins[i]) ? bin : (vis_quantities->nbins[i] - 1);
      if (ant1 < ant2) {
	return (vis_quantities->phase[i][rbin]);
      } else {
	return (-1 * vis_quantities->phase[i][rbin]);
      }
    }
  }

  // If we get here we've failed.
  return (0.0);
}

/*!
 *  \brief Compute the closure phase from the precomputed averaged data
 *  \param scan_header_data the header data relating to the scan
 *  \param vis_quantities the precomputed data from that scan
 *  \param reference_antenna the antenna which is included in all the closure
 *                           phase computations, to ensure independence; this should
 *                           be an index, not a real number
 */
void compute_closure_phase(struct scan_header_data *scan_header_data,
			   struct vis_quantities *vis_quantities,
			   int reference_antenna) {
  int i, j, k, n_triangles_set = 0, a1, a2;

  for (i = 0; i < vis_quantities->nbaselines; i++) {
    // Store the number of bins for later if this is a cross-correlation.
    base_to_ants(vis_quantities->baseline[i], &a1, &a2);
    if (a1 != a2) {
      vis_quantities->nbins_cross = vis_quantities->nbins[i];
    }
  }

  // Clean up any previously computed closure phases (for memory management).
  for (i = 0; i < vis_quantities->ntriangles; i++) {
    FREE(vis_quantities->triangles[i]);
    FREE(vis_quantities->closure_phase[i]);
  }
  FREE(vis_quantities->triangles);
  FREE(vis_quantities->closure_phase);
  
  // Compute the closure phase triangles.
  vis_quantities->ntriangles = ((scan_header_data->num_ants - 1) *
				(scan_header_data->num_ants - 2)) / 2;
  MALLOC(vis_quantities->triangles, vis_quantities->ntriangles);
  MALLOC(vis_quantities->closure_phase, vis_quantities->ntriangles);
  for (i = 0; i < vis_quantities->ntriangles; i++) {
    CALLOC(vis_quantities->triangles[i], 3);
    CALLOC(vis_quantities->closure_phase[i], vis_quantities->nbins_cross);
  }
  n_triangles_set = 0;
  for (i = 0; i < scan_header_data->num_ants - 2; i++) {
    for (j = (i + 1); j < scan_header_data->num_ants - 1; j++) {
      for (k = (j + 1); k < scan_header_data->num_ants; k++) {
	if ((n_triangles_set < vis_quantities->ntriangles) &&
	    ((i == reference_antenna) ||
	     (j == reference_antenna) ||
	     (k == reference_antenna))) {
	  vis_quantities->triangles[n_triangles_set][0] =
	    scan_header_data->ant_label[i];
	  vis_quantities->triangles[n_triangles_set][1] =
	    scan_header_data->ant_label[j];
	  vis_quantities->triangles[n_triangles_set][2] =
	    scan_header_data->ant_label[k];
	  n_triangles_set++;
	}
      }
    }
  }

  // Compute the closure phases now.
  vis_quantities->min_closure_phase = INFINITY;
  vis_quantities->max_closure_phase = -INFINITY;
  for (i = 0; i < n_triangles_set; i++) {
    for (j = 0; j < vis_quantities->nbins_cross; j++) {
      vis_quantities->closure_phase[i][j] =
	baseline_phase(vis_quantities, vis_quantities->triangles[i][0],
		       vis_quantities->triangles[i][1], j) +
	baseline_phase(vis_quantities, vis_quantities->triangles[i][1],
		       vis_quantities->triangles[i][2], j) +
	baseline_phase(vis_quantities, vis_quantities->triangles[i][2],
		       vis_quantities->triangles[i][0], j);
      if (vis_quantities->closure_phase[i][j] < vis_quantities->min_closure_phase) {
	vis_quantities->min_closure_phase = vis_quantities->closure_phase[i][j];
      }
      if (vis_quantities->closure_phase[i][j] > vis_quantities->max_closure_phase) {
	vis_quantities->max_closure_phase = vis_quantities->closure_phase[i][j];
      }
    }
  }
}

/*!
 *  \brief Compute all the adjacent channel delays and return them
 *  \param ampphase the raw data from which to compute the delays
 *  \param phase_in_degrees indicates if phase is measured in degrees (true) or radians (false)
 *  \param min_chan the lowest channel to include in the computation
 *  \param max_chan the highest channel to include in the computation
 *  \param delays a pointer to a variable which will hold the array of the delay
 *                values; enough memory will be allocated here to hold the array.
 *                index 0 = baseline, index 1 = bins, index 2 = channels
 *  \param n_baselines on exit, this will be the length of index 0 of \a delays
 *  \param n_bins on exit, this will be the length of index 1 of \a delays, and this
 *                array has length \a n_baselines
 *  \param n_delays on exit, this will be the length of index 2 of \a delays, and this
 *                  array has length \a n_baselines on index 0, and \a n_bins on index 1
 *  \param mean_delay on exit, this will be filled with the mean value of the \a delays
 *                    array, for the same index 0 and index 1
 *  \param median_delay on exit, this will be filled with the median value of the
 *                      \a delays array, for the same index 0 and index 1
 */
void compute_delays(struct ampphase *ampphase, bool phase_in_degrees, int min_chan, int max_chan,
		    float ****delays, int *n_baselines, int **n_bins, int ***n_delays,
		    float ***mean_delay, float ***median_delay) {
  int i, j, k, nchans;
  float dp, p1, p2, p3, delta_phase, delta_frequency, total_delay;
  
  // Allocate the first index for all these arrays.
  *n_baselines = ampphase->nbaselines;
  CALLOC(*delays, *n_baselines);
  CALLOC(*n_bins, *n_baselines);
  CALLOC(*n_delays, *n_baselines);
  CALLOC(*mean_delay, *n_baselines);
  CALLOC(*median_delay, *n_baselines);
  
  for (i = 0; i < *n_baselines; i++) {
    // How many bins on this baseline?
    (*n_bins)[i] = ampphase->nbins[i];
    CALLOC((*delays)[i], (*n_bins)[i]);
    CALLOC((*n_delays)[i], (*n_bins)[i]);
    CALLOC((*mean_delay)[i], (*n_bins)[i]);
    CALLOC((*median_delay)[i], (*n_bins)[i]);
    for (j = 0; j < (*n_bins)[i]; j++) {
      // How many channels?
      nchans = 0;
      for (k = 0; k < ampphase->f_nchannels[i][j]; k++) {
	if ((ampphase->f_channel[i][j][k] >= min_chan) &&
	    (ampphase->f_channel[i][j][k] < max_chan)) {
	  nchans++;
	}
      }
      (*n_delays)[i][j] = nchans;
      CALLOC((*delays)[i][j], (*n_delays)[i][j]);
      for (k = 1, nchans = 0, total_delay = 0; k < ampphase->f_nchannels[i][j]; k++) {
	if ((nchans < (*n_delays)[i][j]) &&
	    (ampphase->f_channel[i][j][k - 1] >= min_chan) &&
	    (ampphase->f_channel[i][j][k] <= max_chan)) {
	  dp = (phase_in_degrees) ? 360.0 : (2 * M_PI);
	  p1 = ampphase->f_phase[i][j][k] - ampphase->f_phase[i][j][k - 1];
	  p2 = (ampphase->f_phase[i][j][k] + dp) - ampphase->f_phase[i][j][k - 1];
	  p3 = (ampphase->f_phase[i][j][k] - dp) - ampphase->f_phase[i][j][k - 1];

	  delta_phase = (float)smallest_abs(3, p1, p2, p3);
	  if (phase_in_degrees) {
	    delta_phase *= (M_PI / 180);
	  }
	  delta_frequency = ampphase->f_frequency[i][j][k] - ampphase->f_frequency[i][j][k - 1];
	  (*delays)[i][j][nchans] = 1E3 * delta_phase / (2 * M_PI * delta_frequency);
	  total_delay += (*delays)[i][j][nchans];
	  nchans++;
	}
      }
      if (nchans > 0) {
	(*mean_delay)[i][j] = total_delay / (float)nchans;
	qsort((*delays)[i][j], nchans, sizeof(float), cmpfunc_real);
	(*median_delay)[i][j] = fmedianf((*delays)[i][j], nchans);
      } else {
	(*mean_delay)[i][j] = 0;
	(*median_delay)[i][j] = 0;
      }
      
    }
  }
}

/*!
 *  \brief Free memory in a fluxdensity_specification structure, but don't
 *         free the pointer memory itself
 *  \param a the pointer to the structure to free
 */
void free_fluxdensity_specification(struct fluxdensity_specification *a) {
  int i;

  for (i = 0; i < a->num_models; i++) {
    FREE(a->model_terms[i]);
  }
  FREE(a->model_terms);
  FREE(a->model_frequency);
  FREE(a->model_frequency_tolerance);

  a->num_models = 0;
}

/*!
 *  \brief Take some ampphase cycle data and calculate the noise diode amplitudes
 *         given a supplied flux density
 *  \param fluxdensity_models the flux density models for the source
 *  \param num_cycles the number of cycles in the \a cycle_ampphase array
 *  \param window the IF to look at
 *  \param options the options structure to direct which channels to use
 *  \param cycle_spectra the array of cycle data, with length \a num_cycles
 *  \param noise_diode_modifier a pointer to a variable which upon exit will be
 *         a modifier structure filled with the derived noise diode amplitudes
 */
void compute_noise_diode_amplitudes(struct fluxdensity_specification *fluxdensity_models,
				    int num_cycles, int window, struct ampphase_options *options,
				    struct spectrum_data **cycle_spectra,
				    struct ampphase_modifiers **noise_diode_modifier) {
  int i, j, k, a, b, c, ant_label, polidx[POL_YY + 1], winidx = -1;
  int bnt_label, cnt_label, x1, y1, x2, y2, x3, y3, a1, a2, nbins1, nbins2, nbins3;
  int *gains_nbins = NULL, nbinsa, nfreqmatch = 0, maxfreqmatch = 0, freqmatchidx;
  int ***gainsum_numtriplets = NULL, nbins_gainsum = 0;
  bool aons, bons, cons;
  float *rsum1, *rsum2, *rsum3, *isum1, *isum2, *isum3, **gains = NULL, *asuma;
  float nd_amp, src_fd, ***gainsum = NULL;
  double cmjd;

  // Figure out the IF index.
  for (i = 0; i < cycle_spectra[0]->num_ifs; i++) {
    if (cycle_spectra[0]->spectrum[i][0]->window == window) {
      winidx = i;
      break;
    }
  }
  if (winidx == -1) {
    // We don't know about this window.
    return;
  }
  
  // Check if we're supposed to create a modifier structure.
  if (*noise_diode_modifier == NULL) {
    CALLOC(*noise_diode_modifier, 1);
    (*noise_diode_modifier)->set_noise_diode_amplitude = true;
    (*noise_diode_modifier)->noise_diode_num_antennas = 7;
    (*noise_diode_modifier)->noise_diode_num_pols = POL_Y + 1;
    CALLOC((*noise_diode_modifier)->noise_diode_amplitude,
	   (*noise_diode_modifier)->noise_diode_num_antennas);
    for (i = 0; i < (*noise_diode_modifier)->noise_diode_num_antennas; i++) {
      CALLOC((*noise_diode_modifier)->noise_diode_amplitude[i],
	     (*noise_diode_modifier)->noise_diode_num_pols);
    }
    // Take the dates from the cycles.
    (*noise_diode_modifier)->noise_diode_start_mjd =
      date2mjd(cycle_spectra[0]->spectrum[winidx][0]->obsdate,
	       cycle_spectra[0]->spectrum[winidx][0]->ut_seconds);
    (*noise_diode_modifier)->noise_diode_end_mjd =
      (*noise_diode_modifier)->noise_diode_start_mjd;
    for (i = 1; i < num_cycles; i++) {
      cmjd = date2mjd(cycle_spectra[i]->spectrum[winidx][0]->obsdate,
		      cycle_spectra[i]->spectrum[winidx][0]->ut_seconds);
      MINASSIGN((*noise_diode_modifier)->noise_diode_start_mjd, cmjd);
      MAXASSIGN((*noise_diode_modifier)->noise_diode_end_mjd, cmjd);
    }
  }

  // Find the polarisations.
  for (i = POL_XX; i <= POL_YY; i++) {
    polidx[i] = -1;
    for (j = 0; j < cycle_spectra[0]->num_pols; j++) {
      if (cycle_spectra[0]->spectrum[winidx][j]->pol == i) {
	polidx[i] = j;
      }
    }
  }

  if ((polidx[POL_XX] < 0) || (polidx[POL_YY] < 0)) {
    // Weren't able to find this pol, we're in trouble!
    (*noise_diode_modifier)->set_noise_diode_amplitude = false;
    return;
  }
  
  // Work out which flux density model we use.
  maxfreqmatch = 0;
  freqmatchidx = -1;
  for (i = 0; i < fluxdensity_models->num_models; i++) {
    nfreqmatch = 0;
    for (j = 0; j < cycle_spectra[0]->spectrum[winidx][polidx[POL_XX]]->nchannels; j++) {
      if ((cycle_spectra[0]->spectrum[winidx][polidx[POL_XX]]->frequency[j] >=
	   fluxdensity_models->model_frequency[i] -
	   fluxdensity_models->model_frequency_tolerance[i]) &&
	  (cycle_spectra[0]->spectrum[winidx][polidx[POL_XX]]->frequency[j] <=
	   fluxdensity_models->model_frequency[i] +
	   fluxdensity_models->model_frequency_tolerance[i])) {
	nfreqmatch++;
      }
    }
    if (nfreqmatch > maxfreqmatch) {
      maxfreqmatch = nfreqmatch;
      freqmatchidx = i;
    }
  }

  if ((freqmatchidx < 0) ||
      (fluxdensity_models->model_num_terms[freqmatchidx] <= 0)) {
    // No usable frequency model.
    (*noise_diode_modifier)->set_noise_diode_amplitude = false;
    return;
  }
  
  // Loop over the polarisations.
  CALLOC(gainsum, num_cycles);
  CALLOC(gainsum_numtriplets, num_cycles);
  for (i = POL_XX; i <= POL_YY; i++) {
    // We have to find the noise diode amplitude per antenna, so it is
    // most sensible to loop over the antennas here, and do the same
    // procedure for each.
    CALLOC(gains, cycle_spectra[0]->header_data->num_ants);
    for (j = 0; j < num_cycles; j++) {
      CALLOC(gainsum[j], cycle_spectra[0]->header_data->num_ants);
      CALLOC(gainsum_numtriplets[j], cycle_spectra[0]->header_data->num_ants);
    }
    CALLOC(gains_nbins, cycle_spectra[0]->header_data->num_ants);
    for (a = 0; a < cycle_spectra[0]->header_data->num_ants; a++) {
      ant_label = cycle_spectra[0]->header_data->ant_label[a];
      // This antenna has to be on source for all the cycles to count.
      aons = true;
      for (j = 0; j < num_cycles; j++) {
	aons &=
	  (!(cycle_spectra[j]->spectrum[winidx][polidx[i]]->syscal_data->flagging[ant_label - 1] & 1));
      }
      if (!aons) {
	// Mark this antenna as off-source in the modifier.
	(*noise_diode_modifier)->noise_diode_amplitude[ant_label][POL_X] =
	  (*noise_diode_modifier)->noise_diode_amplitude[ant_label][POL_Y] = -1.0;
	continue;
      }
      // Now determine all the gain triplets that can be made with this
      // antenna.
      for (b = 0; b < cycle_spectra[0]->header_data->num_ants; b++) {
	bnt_label = cycle_spectra[0]->header_data->ant_label[b];
	if (bnt_label != ant_label) {
	  bons = true;
	  for (j = 0; j < num_cycles; j++) {
	    bons &=
	      (!(cycle_spectra[j]->spectrum[winidx][polidx[i]]->syscal_data->flagging[bnt_label - 1] & 1));
	  }
	  if (bons) {
	    for (c = b + 1; c < cycle_spectra[0]->header_data->num_ants; c++) {
	      cnt_label = cycle_spectra[0]->header_data->ant_label[c];
	      if ((cnt_label != bnt_label) && (cnt_label != ant_label)) {
		cons = true;
		for (j = 0; j < num_cycles; j++) {
		  cons &=
		    (!(cycle_spectra[j]->spectrum[winidx][polidx[i]]->syscal_data->flagging[cnt_label - 1] & 1));
		}
		if (cons) {
		  // All three antennas are unique and on-source.
		  if (ant_label < bnt_label) {
		    x1 = ant_label;
		    y1 = bnt_label;
		  } else {
		    x1 = bnt_label;
		    y1 = ant_label;
		  }
		  if (ant_label < cnt_label) {
		    x2 = ant_label;
		    y2 = cnt_label;
		  } else {
		    x2 = cnt_label;
		    y2 = ant_label;
		  }
		  if (bnt_label < cnt_label) {
		    x3 = bnt_label;
		    y3 = cnt_label;
		  } else {
		    x3 = cnt_label;
		    y3 = bnt_label;
		  }
		  // Now we collect data over the baselines and cycles and put them in memory
		  // to do calculations later.
		  for (j = 0; j < num_cycles; j++) {
		    for (k = 0; k < cycle_spectra[j]->spectrum[winidx][polidx[i]]->nbaselines; k++) {
		      base_to_ants(cycle_spectra[j]->spectrum[winidx][polidx[i]]->baseline[k],
				   &a1, &a2);
		      if ((a1 == x1) && (a2 == y1)) {
			rsum1 = isum1 = NULL;
			nbins1 = 0;
			sum_vis(cycle_spectra[j]->spectrum[winidx][polidx[i]], k, options,
				&nbins1, &rsum1, &isum1, NULL);
		      } else if ((a1 == x2) && (a2 == y2)) {
			rsum2 = isum2 = NULL;
			nbins2 = 0;
			sum_vis(cycle_spectra[j]->spectrum[winidx][polidx[i]], k, options,
				&nbins2, &rsum2, &isum2, NULL);
		      } else if ((a1 == x3) && (a2 == y3)) {
			rsum3 = isum3 = NULL;
			nbins3 = 0;
			sum_vis(cycle_spectra[j]->spectrum[winidx][polidx[i]], k, options,
				&nbins3, &rsum3, &isum3, NULL);
		      } else if ((a1 == ant_label) && (a2 == ant_label)) {
			// This is the autocorrelation, and we want to use this to
			// get the noise diode amplitude.
			asuma = NULL;
			nbinsa = 0;
			sum_vis(cycle_spectra[j]->spectrum[winidx][polidx[i]], k, options,
				&nbinsa, NULL, NULL, &asuma);
		      }
		    }
		    if ((nbins1 == nbins2) && (nbins1 == nbins3)) {
		      nbins_gainsum = nbins1;
		      CALLOC(gains[ant_label - 1], nbins1);
		      if (gainsum[j][ant_label - 1] == NULL) {
			CALLOC(gainsum[j][ant_label - 1], nbins1);
			CALLOC(gainsum_numtriplets[j][ant_label - 1], nbins1);
		      }
		      for (k = 0; k < nbins1; k++) {
			gains[ant_label - 1][k] = sqrtf((rsum1[k] * rsum1[k] + isum1[k] * isum1[k]) *
							(rsum2[k] * rsum2[k] + isum2[k] * isum2[k]) /
							(rsum3[k] * rsum3[k] + isum3[k] * isum3[k]));
		      }
		      if (nbinsa == 2) {
			// We need the autos to have the niose diode on-off bins.
			nd_amp = asuma[1] - asuma[0];
		      } else {
			nd_amp = 0;
		      }
		      // In the CABB method, the flux density of the source is just the
		      // first term of the model.
		      src_fd = fluxdensity_models->model_terms[freqmatchidx][0];
		      // Now calculate the noise diode strength in Jy.
		      for (k = 0; k < nbins1; k++) {
			gainsum[j][ant_label - 1][k] += (src_fd * nd_amp) / gains[ant_label - 1][k];
			gainsum_numtriplets[j][ant_label - 1][k] += 1;
		      }
		      FREE(gains[ant_label - 1]);
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
    // At this point, we have an array with all the antennas and all the cycles
    // filled in with the noise diode strengths. Calculate the average noise
    // diode strength.
    for (j = 0; j < num_cycles; j++) {
      for (a = 0; a < cycle_spectra[0]->header_data->num_ants; a++) {
	for (k = 0; k < nbins_gainsum; k++) {
	  (*noise_diode_modifier)->noise_diode_amplitude[a + 1][polidx[i]] +=
	    gainsum[j][a][k] / (gainsum_numtriplets[j][a][k] * nbins_gainsum);
	}
      }
    }
    
    FREE(gains);
    FREE(gains_nbins);
  }

  for (j = 0; j < num_cycles; j++) {
    for (a = 0; a < cycle_spectra[0]->header_data->num_ants; a++) {
      FREE(gainsum[j][a]);
      FREE(gainsum_numtriplets[j][a]);
    }
    FREE(gainsum[j]);
    FREE(gainsum_numtriplets[j]);
  }
  FREE(gainsum);
  FREE(gainsum_numtriplets);

  FREE(rsum1);
  FREE(rsum2);
  FREE(rsum3);
  FREE(isum1);
  FREE(isum2);
  FREE(isum3);
}


/*!
 *  \brief Create sums of imaginary and real variables over tvchannels
 *  \param ampphase the ampphase structure with the data to sum
 *  \param baseline_idx the index of the baseline to sum
 *  \param options the specification of the tvchannels
 *  \param nbins a pointer which will be filled with the length of \a rsum and \a isum
 *  \param rsum a pointer that will be filled with an array of length \a nbins, with each
 *              element being the sum of the real components of all the tvchannels in each bin
 *  \param isum a pointer that will be filled with an array of length \a nbins, with each
 *              element being the sum of the imaginary components of all the tvchannels in each bin
 *  \param asum a pointer that will be filled with an array of length \a nbins, with each
 *              element being the sum of the amplitude of all the tvchannels in each bin
 *
 * Each of \a rsum, \a isum and \a asum can be sent as NULL to tell this routine not to
 * return that parameter.
 */
void sum_vis(struct ampphase *ampphase, int baseline_idx, struct ampphase_options *options,
	     int *nbins, float **rsum, float **isum, float **asum) {
  int oidx, i, j;
  
  // Allocate the memory.
  *nbins = ampphase->nbins[baseline_idx];
  if (rsum != NULL) {
    REALLOC(*rsum, *nbins);
  }
  if (isum != NULL) {
    REALLOC(*isum, *nbins);
  }
  if (asum != NULL) {
    REALLOC(*asum, *nbins);
  }
  for (i = 0; i < *nbins; i++) {
    if (rsum != NULL) {
      (*rsum)[i] = 0;
    }
    if (isum != NULL) {
      (*isum)[i] = 0;
    }
    if (asum != NULL) {
      (*asum)[i] = 0;
    }
  }
  
  // For now we assume the window index works for the options.
  oidx = ampphase->window;
  if (oidx > options->num_ifs) {
    // Bad.
    return;
  }

  for (i = 0; i < *nbins; i++) {
    for (j = 0; j < ampphase->f_nchannels[baseline_idx][i]; j++) {
      if ((ampphase->f_channel[baseline_idx][i][j] >= options->min_tvchannel[oidx]) &&
	  (ampphase->f_channel[baseline_idx][i][j] <= options->max_tvchannel[oidx])) {
	if (rsum != NULL) {
	  (*rsum)[i] += crealf(ampphase->f_raw[baseline_idx][i][j]);
	}
	if (isum != NULL) {
	  (*isum)[i] += cimagf(ampphase->f_raw[baseline_idx][i][j]);
	}
	if (asum != NULL) {
	  (*asum)[i] += ampphase->f_amplitude[baseline_idx][i][j];
	}
      }
    }
  }

}

/*!
 *  \brief Take a flux density model, evaluate it at a particular frequency, and
 *         return the flux density
 *  \param num_terms the number of terms in the model, and length of \a terms array
 *  \param log_terms flag to indicate if the model is in log S log nu space (true), or
 *                   in linear S nu (false)
 *  \param terms an array of the model terms
 *  \param eval_freq the frequency (in MHz) at which to evaluate the model
 *  \returns the flux density (in Jy) the model specifies at the \a eval_freq
 */
float fluxdensity_model_evaluate(int num_terms, bool log_terms, float *terms,
				 float eval_freq) {
  int i;
  float fd = 0, freq;

  if (log_terms) {
    freq = log10f(eval_freq);
  } else {
    freq = eval_freq;
  }
  if (num_terms > 0) {
    fd = terms[0];
    for (i = 1; i < num_terms; i++) {
      fd += terms[i] * powf(freq, i);
    }
  }

  if (log_terms) {
    fd = powf(10, fd);
  }
  
  return (fd);
}

/*!
 *  \brief Return a flux density model for a source
 *  \param source_name the name of the source
 *  \param eval_freq the frequency (in MHz) at which the model must be valid for (which
 *                   is required for several models which are frequency-dependent)
 *  \param num_terms a pointer to a variable which will be filled with the number of
 *                   terms in the returned model
 *  \param log_terms a pointer to a variable which will be filled with a flag that indicates
 *                   if the model is in log S log nu space (true), or in linear S nu (false)
 *  \param terms a pointer to a variable which will be allocated into an array of length
 *               \a num_terms with the model terms
 *  \returns a flag indicating if a model is returned for the specified source (true),
 *           or if no model matched (false)
 */
bool source_model(char *source_name, float eval_freq, int *num_terms, bool *log_terms,
		  float **terms) {
  int i;
  float model_1934_low[4] = { -30.7667, 26.4908, -7.0977, 0.605334 };
  float model_1934_high[2] = { 5.887, -1.3763 };
  float model_0823[4] = { -51.0361, 41.4101, -10.7771, 0.90468 };
  float *model_ptr = NULL;
  
  // Branch based on source name (a primitive way to do this I know, I'm sorry).
  if (strcmp(source_name, "1934-638") == 0) {
    // Our best flux density calibrator, has a frequency-dependent model.
    *log_terms = true;
    if (eval_freq < 11150) {
      *num_terms = 4;
      model_ptr = model_1934_low;
    } else {
      *num_terms = 2;
      model_ptr = model_1934_high;
    }
  } else if (strcmp(source_name, "0823-500") == 0) {
    // This is not a very good model, since 0823-500 varies over time.
    *log_terms = true;
    *num_terms = 4;
    model_ptr = model_0823;
  }

  if (model_ptr == NULL) {
    // No match.
    return (false);
  }

  // Copy the model over.
  CALLOC(*terms, *num_terms);
  for (i = 0; i < *num_terms; i++) {
    (*terms)[i] = model_ptr[i];
  }
  return (true);
}
