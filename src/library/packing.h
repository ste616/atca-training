/**
 * ATCA Training Library: packing.h
 * (C) Jamie Stevens CSIRO 2020
 *
 * This library contains routines necessary to pack and unpack
 * data.
 */

#pragma once
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
void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a);
void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a);
void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a);

// Some shortcut definitions for writing and reading to the
// packing object.
// Boolean write.
#define CMPW_BOOL(c, v) if (!cmp_write_bool(c, (bool)v)) error_and_exit(cmp_strerror(c))
// Signed integer write.
#define CMPW_SINT(c, v) if (!cmp_write_sint(c, (int)v)) error_and_exit(cmp_strerror(c))
// Unsigned integer write.
#define CMPW_UINT(c, v) if (!cmp_write_uint(c, (unsigned int)v)) error_and_exit(cmp_strerror(c))
// Float write.
#define CMPW_FLOAT(c, v) if (!cmp_write_float(c, (float)v)) error_and_exit(cmp_strerror(c))
// String write. We write the length of the string first, then the string.
#define CMPW_STRING(c, v, m)                                        \
    do {                                                            \
        CMPW_UINT(c, (strlen(v) < m) ? strlen(v) : m);              \
        if (!cmp_write_str(c, v, (strlen(v) < m) ? strlen(v) : m))  \
            error_and_exit(cmp_strerror(c));                        \
    } while(0)
// Array size initialiser.
#define CMPW_ARRAYINIT(c, l) if (!cmp_write_array(c, l)) error_and_exit(cmp_strerror(c))
// Array writers.
#define CMPW_ARRAYFLOAT(c, l, a)                \
    do {                                        \
        int ii;                                 \
        CMPW_ARRAYINIT(c, l);                   \
        for (ii = 0; ii < l; ii++) {            \
            CMPW_FLOAT(c, a[ii]);               \
        }                                       \
    } while (0)
#define CMPW_ARRAYFLOATCOMPLEX(c, l, a)         \
    do {                                        \
        int ii;                                 \
        CMPW_ARRAYINIT(c, 2 * l);               \
        for (ii = 0; ii < l; ii++) {            \
            CMPW_FLOAT(c, creal(a[ii]));        \
            CMPW_FLOAT(c, cimag(a[ii]));        \
        }                                       \
    } while (0)
#define CMPW_ARRAYSINT(c, l, a)                 \
    do {                                        \
        int ii;                                 \
        CMPW_ARRAYINIT(c, l);                   \
        for (ii = 0; ii < l; ii++) {            \
            CMPW_SINT(c, a[ii]);                \
        }                                       \
    } while (0)
