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


void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a) {
  // This routine takes an ampphase_options structure and packs it for
  // transport.
  CMPW_BOOL(cmp, a->phase_in_degrees);
  CMPW_SINT(cmp, a->delay_averaging);
  CMPW_SINT(cmp, a->min_tvchannel);
  CMPW_SINT(cmp, a->max_tvchannel);
  CMPW_SINT(cmp, a->averaging_method);
  CMPW_SINT(cmp, a->include_flagged_data);
}


void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a) {
  // This routine takes an ampphase structure and packs it for transport.
  int i, j;
  // The number of quantities in each array.
  CMPW_SINT(cmp, a->nchannels);
  CMPW_SINT(cmp, a->nbaselines);

  // Now the arrays storing the labels for each quantity.
  CMPW_ARRAYFLOAT(cmp, a->nchannels, a->channel);
  CMPW_ARRAYFLOAT(cmp, a->nchannels, a->frequency);
  CMPW_ARRAYSINT(cmp, a->nbaselines, a->baseline);

  // The static quantities.
  CMPW_SINT(cmp, a->pol);
  CMPW_SINT(cmp, a->window);
  CMPW_STRING(cmp, a->window_name, 8);
  CMPW_STRING(cmp, a->obsdate, OBSDATE_LENGTH);
  CMPW_FLOAT(cmp, a->ut_seconds);
  CMPW_STRING(cmp, a->scantype, OBSTYPE_LENGTH);

  // The bin arrays have one element per baseline.
  CMPW_ARRAYSINT(cmp, a->nbaselines, a->nbins);

  // The flag array has the indexing:
  // array[baseline][bin]
  for (i = 0; i < a->nbaselines; i++) {
    CMPW_ARRAYSINT(cmp, a->nbins[i], a->flagged_bad[i]);
  }

  // The arrays here have the following indexing.
  // array[baseline][bin][channel]
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      CMPW_ARRAYFLOAT(cmp, a->nchannels, a->weight[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->nchannels, a->amplitude[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->nchannels, a->phase[i][j]);
      CMPW_ARRAYFLOATCOMPLEX(cmp, a->nchannels, a->raw[i][j]);
    }
  }

  // These next arrays contain the same data as above, but
  // do not include the flagged channels.
  for (i = 0; i < a->nbaselines; i++) {
    for (j = 0; j < a->nbins[i]; j++) {
      CMPW_SINT(cmp, a->f_nchannels[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->f_nchannels[i][j], a->f_channel[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->f_nchannels[i][j], a->f_frequency[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->f_nchannels[i][j], a->f_weight[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->f_nchannels[i][j], a->f_amplitude[i][j]);
      CMPW_ARRAYFLOAT(cmp, a->f_nchannels[i][j], a->f_phase[i][j]);
      CMPW_ARRAYFLOATCOMPLEX(cmp, a->f_nchannels[i][j], a->f_raw[i][j]);
    }
  }

  // Some metadata.
  CMPW_FLOAT(cmp, a->min_amplitude_global);
  CMPW_FLOAT(cmp, a->max_amplitude_global);
  CMPW_FLOAT(cmp, a->min_phase_global);
  CMPW_FLOAT(cmp, a->max_phase_global);
  CMPW_ARRAYFLOAT(cmp, a->nbaselines, a->min_amplitude);
  CMPW_ARRAYFLOAT(cmp, a->nbaselines, a->max_amplitude);
  CMPW_ARRAYFLOAT(cmp, a->nbaselines, a->min_phase);
  CMPW_ARRAYFLOAT(cmp, a->nbaselines, a->max_phase);

  pack_ampphase_options(cmp, a->options);
}

void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a) {
  int i, j;

  // The number of IFs in this spectra set.
  CMPW_SINT(cmp, a->num_ifs);
  // The number of polarisations.
  CMPW_SINT(cmp, a->num_pols);

  // The ampphase structures.
  for (i = 0; i < a->num_ifs; i++) {
    for (j = 0; j < a->num_pols; j++) {
      pack_ampphase(cmp, a->spectrum[i][j]);
    }
  }
}
