/**
 * ATCA Training Library: packing.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains routines necessary to pack and unpack
 * data.
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "atrpfits.h"
#include "cmp.h"

// The ways in which we can read data.
#define READ_SCAN_METADATA   1<<1
#define COMPUTE_VIS_PRODUCTS 1<<2
#define GRAB_SPECTRUM        1<<3

// This structure wraps around all the ampphase structures and
// will allow for easy transport.
struct spectrum_data {
  // The number of IFs in this spectra set.
  int num_ifs;
  // The number of polarisations.
  int num_pols;
  // The ampphase structures.
  struct ampphase ***spectrum;
};


// Our routine definitions.
void error_and_exit(const char *msg);
bool read_bytes(void *data, size_t sz, FILE *fh);
bool file_reader(cmp_ctx_t *ctx, void *data, size_t limit);
bool file_skipper(cmp_ctx_t *ctx, size_t count);
size_t file_writer(cmp_ctx_t *ctx, const void *data, size_t count);
void pack_read_bool(cmp_ctx_t *cmp, bool *value);
void pack_write_bool(cmp_ctx_t *cmp, bool value);
void pack_read_sint(cmp_ctx_t *cmp, int *value);
void pack_write_sint(cmp_ctx_t *cmp, int value);
void pack_read_uint(cmp_ctx_t *cmp, unsigned int *value);
void pack_write_uint(cmp_ctx_t *cmp, unsigned int value);
void pack_read_float(cmp_ctx_t *cmp, float *value);
void pack_write_float(cmp_ctx_t *cmp, float value);
void pack_read_string(cmp_ctx_t *cmp, char *value, int maxlength);
void pack_write_string(cmp_ctx_t *cmp, char *value, long unsigned int maxlength);
unsigned int pack_readarray_checksize(cmp_ctx_t *cmp, unsigned int expected_length);
void pack_readarray_float(cmp_ctx_t *cmp, unsigned int expected_length, float *array);
void pack_writearray_float(cmp_ctx_t *cmp, unsigned int length, float *array);
void pack_readarray_floatcomplex(cmp_ctx_t *cmp, unsigned int expected_length,
                                 float complex *array);
void pack_writearray_floatcomplex(cmp_ctx_t *cmp, unsigned int length,
                                  float complex *array);
void pack_readarray_sint(cmp_ctx_t *cmp, unsigned int expected_length, int *array);
void pack_writearray_sint(cmp_ctx_t *cmp, unsigned int length, int *array);
void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a);
void unpack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a);
void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a);
void unpack_ampphase(cmp_ctx_t *cmp, struct ampphase *a);
void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a);
void unpack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a);

#define CMPERROR(c) error_and_exit(cmp_strerror(c))

// Some shortcut definitions for writing and reading to the
// packing object.
// Array size initialiser.
#define CMPW_ARRAYINIT(c, l) if (!cmp_write_array(c, l)) CMPERROR(c)
// Array size getter.
#define CMPR_ARRAYSIZE(c, l) if (!cmp_read_array(c, &l)) CMPERROR(c)
