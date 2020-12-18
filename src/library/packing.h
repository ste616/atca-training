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
#include "cmp_mem_access.h"
#include "atnetworking.h"
#include "compute.h"

// Our routine definitions.
void error_and_exit(const char *msg);
bool buffer_read_bytes(void *data, size_t sz, char *buffer);
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
void pack_read_double(cmp_ctx_t *cmp, double *value);
void pack_write_double(cmp_ctx_t *cmp, double value);
void pack_read_string(cmp_ctx_t *cmp, char *value, int maxlength);
void pack_write_string(cmp_ctx_t *cmp, char *value, long unsigned int maxlength);
unsigned int pack_readarray_checksize(cmp_ctx_t *cmp, unsigned int expected_length);
void pack_readarray_float(cmp_ctx_t *cmp, unsigned int expected_length, float *array);
void pack_writearray_float(cmp_ctx_t *cmp, unsigned int length, float *array);
void pack_readarray_double(cmp_ctx_t *cmp, unsigned int expected_length, double *array);
void pack_writearray_double(cmp_ctx_t *cmp, unsigned int length, double *array);
void pack_readarray_floatcomplex(cmp_ctx_t *cmp, unsigned int expected_length,
                                 float complex *array);
void pack_writearray_floatcomplex(cmp_ctx_t *cmp, unsigned int length,
                                  float complex *array);
void pack_readarray_sint(cmp_ctx_t *cmp, unsigned int expected_length, int *array);
void pack_writearray_sint(cmp_ctx_t *cmp, unsigned int length, int *array);
void pack_readarray_string(cmp_ctx_t *cmp, unsigned int expected_length, char **array,
                           long unsigned int maxlength);
void pack_writearray_string(cmp_ctx_t *cmp, unsigned int length, char **array,
                            long unsigned int maxlength);
void pack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a);
void unpack_ampphase_options(cmp_ctx_t *cmp, struct ampphase_options *a);
void pack_ampphase(cmp_ctx_t *cmp, struct ampphase *a);
void unpack_ampphase(cmp_ctx_t *cmp, struct ampphase *a);
void pack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a);
void unpack_spectrum_data(cmp_ctx_t *cmp, struct spectrum_data *a);
void free_spectrum_data(struct spectrum_data *spectrum_data);
void pack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a);
void unpack_vis_quantities(cmp_ctx_t *cmp, struct vis_quantities *a);
void copy_spectrum_data(struct spectrum_data *dest, struct spectrum_data *src);
void copy_vis_data(struct vis_data *dest, struct vis_data *src);
void pack_vis_data(cmp_ctx_t *cmp, struct vis_data *a);
void unpack_vis_data(cmp_ctx_t *cmp, struct vis_data *a);
void free_vis_data(struct vis_data *vis_data);
void pack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a);
void unpack_scan_header_data(cmp_ctx_t *cmp, struct scan_header_data *a);
void pack_requests(cmp_ctx_t *cmp, struct requests *a);
void unpack_requests(cmp_ctx_t *cmp, struct requests *a);
void pack_responses(cmp_ctx_t *cmp, struct responses *a);
void unpack_responses(cmp_ctx_t *cmp, struct responses *a);
void pack_metinfo(cmp_ctx_t *cmp, struct metinfo *a);
void unpack_metinfo(cmp_ctx_t *cmp, struct metinfo *a);
void pack_syscal_data(cmp_ctx_t *cmp, struct syscal_data *a);
void unpack_syscal_data(cmp_ctx_t *cmp, struct syscal_data *a);
void init_cmp_memory_buffer(cmp_ctx_t *cmp, cmp_mem_access_t *mem, void *buffer,
                            size_t buffer_len);

/*! \def CMPERROR
 *  \brief Output an error message relevant to a problem encountered while
 *         working with the CMP stream, and stop execution
 *  \param c the CMP stream
 */
#define CMPERROR(c) error_and_exit(cmp_strerror(c))

// Some shortcut definitions for writing and reading to the
// packing object.
// Array size initialiser.
/*! \def CMPW_ARRAYINIT
 *  \brief Initialise an array in the stream by marking that an array is
 *         starting and specifying the number of elements that will be in
 *         the array
 *  \param c the CMP stream
 *  \param l the number of elements that will be written to the array
 */
#define CMPW_ARRAYINIT(c, l) if (!cmp_write_array(c, l)) CMPERROR(c)
// Array size getter.
/*! \def CMPR_ARRAYSIZE
 *  \brief Find the array in the stream, and read how many elements to
 *         expect to be read
 *  \param c the CMP stream
 *  \param l the variable which will be used to store the number of elements
 *           the stream says will be coming from the array
 */
#define CMPR_ARRAYSIZE(c, l) if (!cmp_read_array(c, &l)) CMPERROR(c)
