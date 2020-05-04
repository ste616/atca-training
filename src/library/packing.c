/**
 * ATCA Training Library: packing.c
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains routines necessary to pack and unpack
 * data.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "packing.h"
#include "memory.h"
#include "atnetworking.h"

// Some definitions for the packing routines.
void error_and_exit(const char *msg) {
  fprintf(stderr, "PACKING ERROR %s\n\n", msg);
  exit(EXIT_FAILURE);
}

bool read_bytes(void *data, size_t sz, FILE *fh) {
  return fread(data, sizeof(uint8_t), sz, fh) == (sz * sizeof(uint8_t));
}

bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  return read_bytes(data, limit, (FILE *)ctx->buf);
}

bool file_skipper(cmp_ctx_t *ctx, size_t count) {
  return fseek((FILE *)ctx->buf, count, SEEK_CUR);
}

size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  return fwrite(data, sizeof(uint8_t), count, (FILE *)ctx->buf);
}

// Routines that wrap around the fundamental serializer routines, to
// make them more useful for our purposes, and to encapsulate error
// handling.

// Boolean.
// Reader.
void pack_read_bool(cmp_ctx_t *cmp, bool *value) {
  if (!cmp_read_bool(cmp, value)) CMPERROR(cmp);
}
// Writer.
void pack_write_bool(cmp_ctx_t *cmp, bool value) {
  if (!cmp_write_bool(cmp, value)) CMPERROR(cmp);
}

// Signed integer.
// Reader.
void pack_read_sint(cmp_ctx_t *cmp, int *value) {
  int64_t v;
  if (!cmp_read_sinteger(cmp, &v)) CMPERROR(cmp);
  *value = (int)v;
}
// Writer.
void pack_write_sint(cmp_ctx_t *cmp, int value) {
  if (!cmp_write_sint(cmp, value)) CMPERROR(cmp);
}

// Unsigned integer.
// Reader.
void pack_read_uint(cmp_ctx_t *cmp, unsigned int *value) {
  uint64_t v;
  if (!cmp_read_uinteger(cmp, &v)) CMPERROR(cmp);
  *value = (unsigned int)v;
}
// Writer.
void pack_write_uint(cmp_ctx_t *cmp, unsigned int value) {
  if (!cmp_write_uint(cmp, value)) CMPERROR(cmp);
}

// Float.
// Reader.
void pack_read_float(cmp_ctx_t *cmp, float *value) {
  if (!cmp_read_float(cmp, value)) CMPERROR(cmp);
}
// Writer.
void pack_write_float(cmp_ctx_t *cmp, float value) {
  if (!cmp_write_float(cmp, value)) CMPERROR(cmp);
}

// Double.
// Reader.
void pack_read_double(cmp_ctx_t *cmp, double *value) {
  if (!cmp_read_double(cmp, value)) CMPERROR(cmp);
}
// Writer.
void pack_write_double(cmp_ctx_t *cmp, double value) {
  if (!cmp_write_double(cmp, value)) CMPERROR(cmp);
}

// String.
// Reader.
void pack_read_string(cmp_ctx_t *cmp, char *value, int maxlength) {
  uint32_t string_size = (uint32_t)maxlength;
  if (!cmp_read_str(cmp, value, &string_size)) CMPERROR(cmp);
}
// Writer.
void pack_write_string(cmp_ctx_t *cmp, char *value, long unsigned int maxlength) {
  int write_length = (strlen(value) < maxlength) ? strlen(value) : maxlength;
  if (!cmp_write_str(cmp, value, write_length)) CMPERROR(cmp);
}

// Arrays.
// Support checker.
unsigned int pack_readarray_checksize(cmp_ctx_t *cmp, unsigned int expected_length) {
  unsigned int size_read;
  char emsg[1024];
  // Get the array size.
  CMPR_ARRAYSIZE(cmp, size_read);

  if (expected_length != size_read) {
    sprintf(emsg, "Read length of %d is different to expected %d",
            size_read, expected_length);
    error_and_exit(emsg);
  }
  return size_read;
}
// Float.
// Reader.
void pack_readarray_float(cmp_ctx_t *cmp, unsigned int expected_length,
                          float *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);
  
  for (i = 0; i < expected_length; i++) {
    pack_read_float(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_float(cmp_ctx_t *cmp, unsigned int length,
                           float *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);
  for (i = 0; i < length; i++) {
    pack_write_float(cmp, array[i]);
  }
}

// Double.
// Reader.
void pack_readarray_double(cmp_ctx_t *cmp, unsigned int expected_length,
                           double *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);
  for (i = 0; i < expected_length; i++) {
    pack_read_double(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_double(cmp_ctx_t *cmp, unsigned int length,
                            double *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);
  for (i = 0; i < length; i++) {
    pack_write_double(cmp, array[i]);
  }
}

// Float complex.
// Reader.
void pack_readarray_floatcomplex(cmp_ctx_t *cmp, unsigned int expected_length,
                                 float complex *array) {
  unsigned int i;
  float fr, fi;
  pack_readarray_checksize(cmp, 2 * expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_float(cmp, &fr);
    pack_read_float(cmp, &fi);
    array[i] = fr + fi * I;
  }
}
// Writer.
void pack_writearray_floatcomplex(cmp_ctx_t *cmp, unsigned int length,
                                  float complex *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, 2 * length);

  for (i = 0; i < length; i++) {
    pack_write_float(cmp, creal(array[i]));
    pack_write_float(cmp, cimag(array[i]));
  }
}

// Signed integer.
// Reader.
void pack_readarray_sint(cmp_ctx_t *cmp, unsigned int expected_length,
                         int *array) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_sint(cmp, &(array[i]));
  }
}
// Writer.
void pack_writearray_sint(cmp_ctx_t *cmp, unsigned int length,
                          int *array) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);

  for (i = 0; i < length; i++) {
    pack_write_sint(cmp, array[i]);
  }
}

// String.
// Reader.
void pack_readarray_string(cmp_ctx_t *cmp, unsigned int expected_length,
                           char **array, long unsigned int maxlength) {
  unsigned int i;
  pack_readarray_checksize(cmp, expected_length);

  for (i = 0; i < expected_length; i++) {
    pack_read_string(cmp, array[i], maxlength);
  }
}
// Writer.
void pack_writearray_string(cmp_ctx_t *cmp, unsigned int length,
                            char **array, long unsigned int maxlength) {
  unsigned int i;
  CMPW_ARRAYINIT(cmp, length);

  for (i = 0; i < length; i++) {
    pack_write_string(cmp, array[i], maxlength);
  }
}

void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a) {
  // This routine takes an ampphase_options structure and packs it for
  // transport.
  pack_write_bool(cmp, a->phase_in_degrees);
  pack_write_sint(cmp, a->delay_averaging);
  pack_write_sint(cmp, a->min_tvchannel);
  pack_write_sint(cmp, a->max_tvchannel);
  pack_write_sint(cmp, a->averaging_method);
  pack_write_sint(cmp, a->include_flagged_data);
}

void unpack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a) {
  // This routine unpacks an ampphase_options structure from the
  // serializer, and stores it in the passed structure.
  pack_read_bool(cmp, &(a->phase_in_degrees));
  pack_read_sint(cmp, &(a->delay_averaging));
  pack_read_sint(cmp, &(a->min_tvchannel));
  pack_read_sint(cmp, &(a->max_tvchannel));
  pack_read_sint(cmp, &(a->averaging_method));
  pack_read_sint(cmp, &(a->include_flagged_data));
}

void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a) {
  // This routine takes an ampphase structure and packs it for transport.
  int i, j;
  // The number of quantities in each array.
  pack_write_sint(cmp, a->nchannels);
  pack_write_sint(cmp, a->nbaselines);

  // Now the arrays storing the labels for each quantity.
  pack_writearray_float(cmp, a->nchannels, a->channel);
  pack_writearray_float(cmp, a->nchannels, a->frequency);
  pack_writearray_sint(cmp, a->nbaselines, a->baseline);

  // The static quantities.
  pack_write_sint(cmp, a->pol);
  pack_write_sint(cmp, a->window);
  pack_write_string(cmp, a->window_name, 8);
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);
  pack_write_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The bin arrays have one element per baseline.
  pack_writearray_sint(cmp, a->nbaselines, a->nbins);

  // The flag array has the indexing:
  // array[baseline][bin]
  for (i = 0; i < a->nbaselines; i++) {
    pack_writearray_sint(cmp, a->nbins[i], a->flagged_bad[i]);
  }

  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      pack_writearray_float(cmp, a->nchannels, a->weight[i][j]);
      pack_writearray_float(cmp, a->nchannels, a->amplitude[i][j]);
      pack_writearray_float(cmp, a->nchannels, a->phase[i][j]);
      pack_writearray_floatcomplex(cmp, a->nchannels, a->raw[i][j]);
    }
  }

  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      pack_write_sint(cmp, a->f_nchannels[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_channel[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_frequency[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_weight[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_amplitude[i][j]);
      pack_writearray_float(cmp, a->f_nchannels[i][j], a->f_phase[i][j]);
      pack_writearray_floatcomplex(cmp, a->f_nchannels[i][j], a->f_raw[i][j]);
    }
  }

  // Some metadata.
  pack_write_float(cmp, a->min_amplitude_global);
  pack_write_float(cmp, a->max_amplitude_global);
  pack_write_float(cmp, a->min_phase_global);
  pack_write_float(cmp, a->max_phase_global);
  pack_writearray_float(cmp, a->nbaselines, a->min_amplitude);
  pack_writearray_float(cmp, a->nbaselines, a->max_amplitude);
  pack_writearray_float(cmp, a->nbaselines, a->min_phase);
  pack_writearray_float(cmp, a->nbaselines, a->max_phase);

  pack_ampphase_options(cmp, a->options);
}

void unpack_ampphase(cmp_ctx_t *cmp, struct ampphase *a) {
  // This routine unpacks an ampphase structure from the
  // serializer and stores it in the passed structure.
  int i, j;
  // The number of quantities in each array.
  pack_read_sint(cmp, &(a->nchannels));
  pack_read_sint(cmp, &(a->nbaselines));

  // Now the arrays storing the labels for each quantity.
  MALLOC(a->channel, a->nchannels);
  pack_readarray_float(cmp, a->nchannels, a->channel);
  MALLOC(a->frequency, a->nchannels);
  pack_readarray_float(cmp, a->nchannels, a->frequency);
  MALLOC(a->baseline, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->baseline);

  // The static quantities.
  pack_read_sint(cmp, &(a->pol));
  pack_read_sint(cmp, &(a->window));
  pack_read_string(cmp, a->window_name, 8);
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));
  pack_read_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The bin arrays have one element per baseline.
  MALLOC(a->nbins, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->nbins);

  // The flag array has the indexing:
  // array[baseline][bin]
  MALLOC(a->flagged_bad, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->flagged_bad[i], a->nbins[i]);
    pack_readarray_sint(cmp, a->nbins[i], a->flagged_bad[i]);
  }

  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  MALLOC(a->weight, a->nbaselines);
  MALLOC(a->amplitude, a->nbaselines);
  MALLOC(a->phase, a->nbaselines);
  MALLOC(a->raw, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->weight[i], a->nbins[i]);
    MALLOC(a->amplitude[i], a->nbins[i]);
    MALLOC(a->phase[i], a->nbins[i]);
    MALLOC(a->raw[i], a->nbins[i]);
    for (j = 0; j < a->nbins[i]; j++) {
      MALLOC(a->weight[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->weight[i][j]);
      MALLOC(a->amplitude[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->amplitude[i][j]);
      MALLOC(a->phase[i][j], a->nchannels);
      pack_readarray_float(cmp, a->nchannels, a->phase[i][j]);
      MALLOC(a->raw[i][j], a->nchannels);
      pack_readarray_floatcomplex(cmp, a->nchannels, a->raw[i][j]);
    }
  }

  
  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  MALLOC(a->f_nchannels, a->nbaselines);
  MALLOC(a->f_channel, a->nbaselines);
  MALLOC(a->f_frequency, a->nbaselines);
  MALLOC(a->f_weight, a->nbaselines);
  MALLOC(a->f_amplitude, a->nbaselines);
  MALLOC(a->f_phase, a->nbaselines);
  MALLOC(a->f_raw, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->f_nchannels[i], a->nbins[i]);
    MALLOC(a->f_channel[i], a->nbins[i]);
    MALLOC(a->f_frequency[i], a->nbins[i]);
    MALLOC(a->f_weight[i], a->nbins[i]);
    MALLOC(a->f_amplitude[i], a->nbins[i]);
    MALLOC(a->f_phase[i], a->nbins[i]);
    MALLOC(a->f_raw[i], a->nbins[i]);
    for (j = 0; j < a->nbins[i]; j++) {
      pack_read_sint(cmp, &(a->f_nchannels[i][j]));
      MALLOC(a->f_channel[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_channel[i][j]);
      MALLOC(a->f_frequency[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_frequency[i][j]);
      MALLOC(a->f_weight[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_weight[i][j]);
      MALLOC(a->f_amplitude[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_amplitude[i][j]);
      MALLOC(a->f_phase[i][j], a->f_nchannels[i][j]);
      pack_readarray_float(cmp, a->f_nchannels[i][j], a->f_phase[i][j]);
      MALLOC(a->f_raw[i][j], a->f_nchannels[i][j]);
      pack_readarray_floatcomplex(cmp, a->f_nchannels[i][j], a->f_raw[i][j]);
    }
  }

  // Some metadata.
  pack_read_float(cmp, &(a->min_amplitude_global));
  pack_read_float(cmp, &(a->max_amplitude_global));
  pack_read_float(cmp, &(a->min_phase_global));
  pack_read_float(cmp, &(a->max_phase_global));
  MALLOC(a->min_amplitude, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_amplitude);
  MALLOC(a->max_amplitude, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_amplitude);
  MALLOC(a->min_phase, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->min_phase);
  MALLOC(a->max_phase, a->nbaselines);
  pack_readarray_float(cmp, a->nbaselines, a->max_phase);

  MALLOC(a->options, 1);
  unpack_ampphase_options(cmp, a->options);
  
}

void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a) {
  int i, j;

  // The spectrum header.
  pack_scan_header_data(cmp, a->header_data);

  // The number of IFs in this spectra set.
  pack_write_sint(cmp, a->num_ifs);
  // The number of polarisations.
  pack_write_sint(cmp, a->num_pols);

  // The ampphase structures.
  for (i = 0; i < a->num_ifs; i++) {
    for (j = 0; j < a->num_pols; j++) {
      pack_ampphase(cmp, a->spectrum[i][j]);
    }
  }
}

void unpack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a) {
  int i, j;
  // The spectrum header.
  MALLOC(a->header_data, 1);
  unpack_scan_header_data(cmp, a->header_data);
  
  // The number of IFs in this spectra set.
  pack_read_sint(cmp, &(a->num_ifs));
  // The number of polarisations.
  pack_read_sint(cmp, &(a->num_pols));

  // Allocate some memory.
  MALLOC(a->spectrum, a->num_ifs);
  for (i = 0; i < a->num_ifs; i++) {
    MALLOC(a->spectrum[i], a->num_pols);
    for (j = 0; j < a->num_pols; j++) {
      MALLOC(a->spectrum[i][j], 1);
      unpack_ampphase(cmp, a->spectrum[i][j]);
    }
  }
}

void pack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a) {
  int i;
  
  // The options that were used.
  pack_ampphase_options(cmp, a->options);

  // Number of quantities in the array.
  pack_write_sint(cmp, a->nbaselines);

  // The time.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);

  // Labels.
  pack_write_sint(cmp, a->pol);
  pack_write_sint(cmp, a->window);
  pack_writearray_sint(cmp, a->nbaselines, a->nbins);
  pack_writearray_sint(cmp, a->nbaselines, a->baseline);
  pack_writearray_sint(cmp, a->nbaselines, a->flagged_bad);
  pack_write_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The arrays.
  for (i = 0; i < a->nbaselines; i++) {
    pack_writearray_float(cmp, a->nbins[i], a->amplitude[i]);
    pack_writearray_float(cmp, a->nbins[i], a->phase[i]);
    pack_writearray_float(cmp, a->nbins[i], a->delay[i]);
  }

  // Metadata.
  pack_write_float(cmp, a->min_amplitude);
  pack_write_float(cmp, a->max_amplitude);
  pack_write_float(cmp, a->min_phase);
  pack_write_float(cmp, a->max_phase);
  pack_write_float(cmp, a->min_delay);
  pack_write_float(cmp, a->max_delay);
}

void unpack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a) {
  int i;

  // The options that were used.
  MALLOC(a->options, 1);
  unpack_ampphase_options(cmp, a->options);

  // Number of quantities in the array.
  pack_read_sint(cmp, &(a->nbaselines));

  // The time.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));
  
  // Labels.
  pack_read_sint(cmp, &(a->pol));
  pack_read_sint(cmp, &(a->window));
  MALLOC(a->nbins, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->nbins);
  MALLOC(a->baseline, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->baseline);
  MALLOC(a->flagged_bad, a->nbaselines);
  pack_readarray_sint(cmp, a->nbaselines, a->flagged_bad);
  pack_read_string(cmp, a->scantype, OBSTYPE_LENGTH);

  // The arrays.
  MALLOC(a->amplitude, a->nbaselines);
  MALLOC(a->phase, a->nbaselines);
  MALLOC(a->delay, a->nbaselines);
  for (i = 0; i < a->nbaselines; i++) {
    MALLOC(a->amplitude[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->amplitude[i]);
    MALLOC(a->phase[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->phase[i]);
    MALLOC(a->delay[i], a->nbins[i]);
    pack_readarray_float(cmp, a->nbins[i], a->delay[i]);
  }

  // Metadata.
  pack_read_float(cmp, &(a->min_amplitude));
  pack_read_float(cmp, &(a->max_amplitude));
  pack_read_float(cmp, &(a->min_phase));
  pack_read_float(cmp, &(a->max_phase));
  pack_read_float(cmp, &(a->min_delay));
  pack_read_float(cmp, &(a->max_delay));
  
}

void pack_vis_data(cmp_ctx_t *cmp, struct vis_data *a) {
  int i, j, k;
  // The number of cycles contained here.
  pack_write_sint(cmp, a->nviscycles);

  // The number of IFs per cycle.
  pack_writearray_sint(cmp, a->nviscycles, a->num_ifs);

  // The number of pols per cycle per IF.
  for (i = 0; i < a->nviscycles; i++) {
    pack_writearray_sint(cmp, a->num_ifs[i], a->num_pols[i]);
  }

  // The vis_quantities structures.
  for (i = 0; i < a->nviscycles; i++) {
    for (j = 0; j < a->num_ifs[i]; j++) {
      for (k = 0; k < a->num_pols[i][j]; k++) {
	pack_vis_quantities(cmp, a->vis_quantities[i][j][k]);
      }
    }
  }
}

void unpack_vis_data(cmp_ctx_t *cmp, struct vis_data *a) {
  int i, j, k;

  // The number of cycles contained here.
  pack_read_sint(cmp, &(a->nviscycles));

  // The number of IFs per cycle.
  MALLOC(a->num_ifs, a->nviscycles);
  pack_readarray_sint(cmp, a->nviscycles, a->num_ifs);

  // The number of pols per cycle per IF.
  MALLOC(a->num_pols, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->num_pols[i], a->num_ifs[i]);
    pack_readarray_sint(cmp, a->num_ifs[i], a->num_pols[i]);
  }

  // The vis_quantities structures.
  MALLOC(a->vis_quantities, a->nviscycles);
  for (i = 0; i < a->nviscycles; i++) {
    MALLOC(a->vis_quantities[i], a->num_ifs[i]);
    for (j = 0; j < a->num_ifs[i]; j++) {
      MALLOC(a->vis_quantities[i][j], a->num_pols[i][j]);
      for (k = 0; k < a->num_pols[i][j]; k++) {
        MALLOC(a->vis_quantities[i][j][k], 1);
        unpack_vis_quantities(cmp, a->vis_quantities[i][j][k]);
      }
    }
  }
}

void pack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a) {
  int i;
  // Time variables.
  pack_write_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_write_float(cmp, a->ut_seconds);

  // Details about the observation.
  pack_write_string(cmp, a->obstype, OBSTYPE_LENGTH);
  pack_write_string(cmp, a->calcode, CALCODE_LENGTH);
  pack_write_sint(cmp, a->cycle_time);

  // Name of the source.
  pack_write_string(cmp, a->source_name, SOURCE_LENGTH);

  // Source coordinates.
  pack_write_float(cmp, a->rightascension_hours);
  pack_write_float(cmp, a->declination_degrees);

  // Frequency configuration.
  pack_write_sint(cmp, a->num_ifs);
  pack_writearray_float(cmp, a->num_ifs, a->if_centre_freq);
  pack_writearray_float(cmp, a->num_ifs, a->if_bandwidth);
  pack_writearray_sint(cmp, a->num_ifs, a->if_num_channels);
  pack_writearray_sint(cmp, a->num_ifs, a->if_num_stokes);
  pack_writearray_sint(cmp, a->num_ifs, a->if_sideband);
  pack_writearray_sint(cmp, a->num_ifs, a->if_chain);
  pack_writearray_sint(cmp, a->num_ifs, a->if_label);
  for (i = 0; i < a->num_ifs; i++) {
    pack_writearray_string(cmp, 3, a->if_name[i], 8);
    pack_writearray_string(cmp, a->if_num_stokes[i], a->if_stokes_names[i], 3);
  }
  pack_write_sint(cmp, a->num_ants);
  pack_writearray_sint(cmp, a->num_ants, a->ant_label);
  pack_writearray_string(cmp, a->num_ants, a->ant_name, 9);
  for (i = 0; i < a->num_ants; i++) {
    pack_writearray_double(cmp, 3, a->ant_cartesian[i]);
  }

}

void unpack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a) {
  int i, j;
  // Time variables.
  pack_read_string(cmp, a->obsdate, OBSDATE_LENGTH);
  pack_read_float(cmp, &(a->ut_seconds));

  // Details about the observation.
  pack_read_string(cmp, a->obstype, OBSTYPE_LENGTH);
  pack_read_string(cmp, a->calcode, CALCODE_LENGTH);
  pack_read_sint(cmp, &(a->cycle_time));

  // Name of the source.
  pack_read_string(cmp, a->source_name, SOURCE_LENGTH);
  
  // Source coordinates.
  pack_read_float(cmp, &(a->rightascension_hours));
  pack_read_float(cmp, &(a->declination_degrees));
  
  // Frequency configuration.
  pack_read_sint(cmp, &(a->num_ifs));
  MALLOC(a->if_centre_freq, a->num_ifs);
  pack_readarray_float(cmp, a->num_ifs, a->if_centre_freq);
  MALLOC(a->if_bandwidth, a->num_ifs);
  pack_readarray_float(cmp, a->num_ifs, a->if_bandwidth);
  MALLOC(a->if_num_channels, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_num_channels);
  MALLOC(a->if_num_stokes, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_num_stokes);
  MALLOC(a->if_sideband, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_sideband);
  MALLOC(a->if_chain, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_chain);
  MALLOC(a->if_label, a->num_ifs);
  pack_readarray_sint(cmp, a->num_ifs, a->if_label);
  MALLOC(a->if_name, a->num_ifs);
  MALLOC(a->if_stokes_names, a->num_ifs);
  for (i = 0; i < a->num_ifs; i++) {
    MALLOC(a->if_name[i], 3);
    for (j = 0; j < 3; j++) {
      MALLOC(a->if_name[i][j], 8);
    }
    MALLOC(a->if_stokes_names[i], a->if_num_stokes[i]);
    for (j = 0; j < a->if_num_stokes[i]; j++) {
      MALLOC(a->if_stokes_names[i][j], 3);
    }
    pack_readarray_string(cmp, 3, a->if_name[i], 8);
    pack_readarray_string(cmp, a->if_num_stokes[i], a->if_stokes_names[i], 3);
  }
  pack_read_sint(cmp, &(a->num_ants));
  MALLOC(a->ant_label, a->num_ants);
  pack_readarray_sint(cmp, a->num_ants, a->ant_label);
  MALLOC(a->ant_name, a->num_ants);
  for (i = 0; i < a->num_ants; i++) {
    MALLOC(a->ant_name[i], 9);
  }
  pack_readarray_string(cmp, a->num_ants, a->ant_name, 9);
  MALLOC(a->ant_cartesian, a->num_ants);
  for (i = 0; i < a->num_ants; i++) {
    MALLOC(a->ant_cartesian[i], 3);
    pack_readarray_double(cmp, 3, a->ant_cartesian[i]);
  }
  
}

void pack_requests(cmp_ctx_t *cmp, struct requests *a) {
  // The request type.
  pack_write_sint(cmp, a->request_type);
}

void unpack_requests(cmp_ctx_t *cmp, struct requests *a) {
  // The request type
  pack_read_sint(cmp, &(a->request_type));
}

void pack_responses(cmp_ctx_t *cmp, struct responses *a) {
  // The response type.
  pack_write_sint(cmp, a->response_type);
}

void unpack_responses(cmp_ctx_t *cmp, struct responses *a) {
  // The response type.
  pack_read_sint(cmp, &(a->response_type));
}

void init_cmp_memory_buffer(cmp_ctx_t *cmp, cmp_mem_access_t *mem, void *buffer,
                            size_t buffer_len) {
  cmp_mem_access_init(cmp, mem, buffer, buffer_len);
}
